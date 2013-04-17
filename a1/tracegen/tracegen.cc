#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <zlib.h>
#include "string.h"
#include <iostream>
#include <stdint.h>

using namespace std;

#define MAX_NAME_SIZE 81
#define MAX_LINE_LENGTH 255
#define MAX_ARG_NUM 50
#define MAX_CF_OPCODE_NAME 10
#define MAX_MEM_TYPE_NAME 3 
   
/*
typedef unsigned short int uint8_t;
typedef unsigned int uint32_t;
*/
enum
{
	DONE, OK, EMPTY_LINE
};


#define ADDRINT uint64_t
typedef enum Cf_Type_enum {
  NOT_CF,      // not a control flow instruction
  CF_BR,       // an unconditional branch
  CF_CBR,      // a conditional branch
  CF_CALL,     // a call
  CF_IBR,      // an indirect branch
  CF_ICALL,    // an indirect call
  CF_RET,      // a return
  CF_ICO,      // a system call 
  NUM_CF_TYPES,
}Cf_Type;


typedef enum Mem_Type_enum{
  NOT_MEM,       // not a memory instruction
  MEM_LD,        // a load instruction
  MEM_ST,        // a store instruction
  NUM_MEM_TYPES,
}Mem_Type;


/* Data structure for trace in the lab assignment*/ 
typedef struct Trace_op_struct{
  uint8_t num_src;   /* number or sources */
  int8_t  src[2];    /* source register id */ 
  int8_t  dst;       /* destiation register id */ 

  uint8_t opcode;    /* opcode */ 

  bool is_fp;        /* floating point instruction */ 
  Cf_Type cf_type;   /* control flow type */ 
  Mem_Type mem_type; /* memory operation type */ 
  bool write_flag;    /* Does instruction wrige flag (conditional code)? */

  uint8_t inst_size;   /* instruction size  (B) */ 

  ADDRINT/*uint32_t*/ ld_vaddr;           /* load virtual address */ 
  ADDRINT/*uint32_t*/ st_vaddr;           /* store virtual address */ 
  ADDRINT/*uint32_t*/ instruction_addr;   /* instruction address */ 
  ADDRINT/*uint32_t*/ branch_target;      /* branch target address */ 
  bool actually_taken;                    /* branch direction */ 

  uint8_t mem_read_size;   /* read memory size */ 
  uint8_t mem_write_size;  /* write memory size */ 

} Trace_op;


int toNum(char *pStr);
int readAndParse(FILE *pInfile, char *pLine, char **pLabel, char **pArgs);

int isOpcode(char *strPtr);
int regNum(char *strPtr);
int cfNum(char *strPtr);
int memTypeNum(char *strPtr);

int allocTraceInfo(char **pArgs, Trace_op *trace_op);


void dprint_trace(Trace_op *trace_op);
void MakeCompatibleTrace();
void read_trace(); 

int op_count; 
gzFile stream, stream2; 

int main(int argc, char *argv[])
{
	FILE *ifp;
	FILE *ofp;

	char out_file_name[MAX_NAME_SIZE];
	char lLine[MAX_LINE_LENGTH + 1];
	char *lLabel;
	//char *lOpcode;
	
	char **lArgs;

	int pc_addr;

	int lRet;

	int temp;
	int i;

	

	Trace_op trace_op; 

	/* open the source file */
	if (argc>2){ 
	ifp = fopen(argv[1], "r");
	stream = gzopen(argv[2], "w"); 
	}
	else {
	    ifp = fopen("input.txt", "r");
	    stream = gzopen("out.pzip", "w"); 
	    printf("please provide input and output file names\n"); 
	    printf("default input file is input.txt, default output file is out.pzip\n"); 

	    printf("/*********************************\n"); 
	    printf("* input format order              \n"); 
            printf("*       pc addr                 : int:  pc addr, addr should be multiple of 4  \n");
	    printf("*	opcode			: string : g_tr_opcode_names[] in string.h\n");
	    printf("*	dst			: string: r0 ~ r31  or -1 (if there is no destimation)\n"); 
	    printf("*	num_read_regs	        : int :  0<= * <=2 \n");
	    printf("*	src[]			: string: r0 ~ r31  or -1 (if there is no source) \n"); 
	    printf("*	mem_vaddr		: int : LD/ST addr  (int) \n");
	    printf("*	mem_size		: int :  data size (int)\n"); 
	    printf("*	cf_type			: string:  (NOT_CF, CF_BR) \n");
	    printf("&       branch_target           : int : target addr \n");
	    printf("*	actually_taken 	        : int : 0 or 1 \n"); 
	    printf("*	mem_type		: string (NOT_MEM, MEM_LD, MEM_ST) \n"); 
	    printf("***********************************/ \n"); 

	}
	
	if(stream == NULL)
	{
		printf("Error: Cannot open write file %s\n", argv[2]);
		exit(-1);
	}
	

	if(ifp == NULL)
	{
		printf("ERROR: Cannot open input file %s\n", argv[1]);
		exit(-1);
	}


	lArgs = (char **) malloc(MAX_ARG_NUM * sizeof(char *));

	pc_addr = 444; 
	int k = 0;	
	do
	{
		// parsing input file
		lRet = readAndParse(ifp, lLine, &lLabel, lArgs);
		if(lRet != DONE && lRet!= EMPTY_LINE)
		{
		
		    // allocate parsed data into corresponding fields in inst
		    allocTraceInfo(lArgs, &trace_op);
		    
		    dprint_trace(&trace_op);
		    
		    
		    pc_addr = pc_addr + 4;
		}
		
	}while(lRet != DONE);

	free(lArgs);	
	fclose(ifp);
	gzclose(stream);

	/* for debugging 
	stream2 = gzopen("out.pzip", "r"); 
	read_trace(); 
	gzclose(stream2);
	*/ 


}

/**********************
* Function: allocTraceInfo
* Description: allocate Trace info from pArgs to correspondig fields 
***************************/

int allocTraceInfo(char **pArgs, Trace_op *trace_op)
{
	int i;
	int num_regs  = 0; 
	int num_dest_regs = 1; 
	ADDRINT vaddr; 
	int Opcode= -1; 
	uint8_t  mem_size; 
	//	std::cout <<"starting " << endl; 
	trace_op->instruction_addr =   toNum(pArgs[0]); 
	//	std::cout <<"inst_addr:" << hex << trace_op->instruction_addr << endl;
	Opcode = isOpcode(pArgs[1]);


	if(Opcode == -1)
	{
		printf("Error: invalid opcode, %s\n", pArgs[1]);
		exit(-1);
	}
	else { 
	    trace_op->opcode = (uint8_t) Opcode; 
	}
	
	//	std::cout <<"opcode: " << (int) trace_op->opcode << endl;
	trace_op->dst = regNum(pArgs[2]);
	if( (trace_op->dst< 0) || (trace_op->dst > 31))
	{
	    trace_op->dst = -1; 
	}
	trace_op->num_src = toNum(pArgs[3]);

	for(i=0; i< 2; i++)
	{
		trace_op->src[i] = regNum(pArgs[i+4]); 
		if( (trace_op->src[i] < 0) || (trace_op->src[i] > 31))
		{
		    trace_op->src[i] = -1; 
		}
		
	}
	
	//	std::cout <<"num_src:" << (int) trace_op->num_src << endl;
	//	std::cout <<"src[0]:"<< (int) trace_op->src[0] << endl;
	//	std::cout <<"src[1]:"<< (int) trace_op->src[1] << endl;
	//	std::cout <<"dst: " <<(int) trace_op->dst << endl;

	num_regs = 3; 
	
	vaddr = (ADDRINT)toNum(pArgs[num_regs + 3]);
	
	// 	std::cout << "vaddr: " << dec  <<  vaddr << endl;
	
	mem_size =  toNum(pArgs[num_regs + 4]);
	// 	std::cout <<"mem_size: " << (int)mem_size << endl;

	trace_op->cf_type = (Cf_Type) cfNum(pArgs[num_regs + 5]);
	// 	std::cout <<"cf_type: " << (int) trace_op->cf_type << endl;

	trace_op->branch_target = (ADDRINT)toNum(pArgs[num_regs +6]); 
	// 	std::cout <<"branch_traget: " << (int) trace_op->branch_target << endl;

	trace_op->actually_taken = (bool)toNum(pArgs[num_regs + 7]);
	// 	std::cout <<"actually_taken: " << (int) trace_op->actually_taken << endl;
	
	trace_op->mem_type = (Mem_Type)memTypeNum(pArgs[num_regs + 8]);	
	// 	std::cout <<"mem_type: " << (int) trace_op->mem_type << endl;	
	
	if (trace_op->mem_type  == MEM_LD) {
	    trace_op->ld_vaddr = vaddr; 
	    trace_op->mem_read_size =(uint8_t) mem_size; 
	    trace_op->st_vaddr = 0; 
	    trace_op->mem_write_size = 0; 
	}
	else if (trace_op->mem_type == MEM_ST) {

	    trace_op->st_vaddr = vaddr; 
	    trace_op->mem_write_size =(uint8_t)  mem_size; 
	    trace_op->ld_vaddr = 0; 
	    trace_op->mem_read_size = 0; 
	}
	else {
	    trace_op->ld_vaddr = 0; 
	    trace_op->mem_read_size = 0; 
	    trace_op->st_vaddr = 0; 
	    trace_op->mem_write_size = 0; 
	}
	
	// 	std::cout <<"ld_vaddr: " << hex << trace_op->ld_vaddr << endl;
	// 	std::cout <<"st_vaddr: " << hex <<  trace_op->st_vaddr << endl;
	// 	std::cout <<"mem_read_size: " << (int)trace_op->mem_read_size << endl;
	// 	std::cout <<"mem_write_size: " << (int) trace_op->mem_write_size << endl;	  
	/* default values */ 
	trace_op->inst_size = 4; 
	trace_op->write_flag = 0; 
	if (trace_op->opcode >= 15 && trace_op->opcode <= 23) 
	{
	    trace_op->is_fp = 1; 
	}
	else {
	    trace_op->is_fp = 0; 
	}
	op_count++;
	if (gzwrite(stream, trace_op,sizeof(Trace_op)) != sizeof(Trace_op))
	    cout << "Error when writting instruction " << op_count << endl;
	
	
	return 1; 
}




/****************************************************
* Function: toNum 
* Description: Convert a string to a number
* Parameters:
*	[in] pStr
* Return value:
*   	
*****************************************************/
int toNum(char *pStr)
{
	char *t_ptr;
	char *orig_pStr;
	int t_length;
	int k;
	int lNum = 0;
	int lNeg = 0;
	long int lNumLong;

	orig_pStr = pStr;
	if(*pStr == 'x')	// hex 
	{
		pStr++;
		if(*pStr == '-')	// hex is negative
		{
			lNeg = 1;
			pStr++;
		}

		t_ptr = pStr;
		t_length = strlen(t_ptr);
		for(k=0; k<t_length; k++)
		{
			if(!isxdigit(*t_ptr))
			{
				printf("Error: invalid hex operand, %s\n", orig_pStr);
				exit(-1);
			}
			t_ptr++;
		}
		lNumLong = strtol(pStr, NULL, 16);	// convert hex string into integer
		lNum = (lNumLong > INT_MAX)? INT_MAX : lNumLong;
		if(lNeg)
		{
			lNum = -lNum;
			return lNum;
		}
	}
	else 		// decimal 
	{	
		//pStr++;		// do not need this because we assume that decimal number string do not start with '#'
		if(*pStr == '-')	// dec is negative
		{
			lNeg = 1;
			pStr++;
		}
		// need to check the range? ex) checking the 16 bit

		t_ptr = pStr;
		t_length = strlen(t_ptr);
		for(k=0; k<t_length; k++)
		{
			if(!isdigit(*t_ptr))
			{
				printf("Error: invalid decimal operand, %s\n", orig_pStr);
				exit(-1);
			}
			t_ptr++;
		}
		lNum = atoi(pStr);
		if(lNeg)
		{
			lNum = -lNum;
		}

		return lNum;
	}
	/*
	else
	{
		printf("Error: invalid operand, %s\n", orig_pStr);	// or invalid conatant?
		exit(-1);
	}
	*/
}

/*********************************
* Function: readAndParse
* Description: Take a line of the input file and parse it into pArgs 
*************************************/
int readAndParse(FILE *pInfile, char *pLine, char **pLabel, char **pArgs)
{
	char *lRet;
	char *lPtr;
	int i;

	if(!fgets(pLine, MAX_LINE_LENGTH, pInfile))
		return (DONE);

	/*
	for(i=0; i<strlen(pLine); i++)	// convert uppercase letter to lowercase
		pLine[i] = tolower(pLine[i]);
	*/

	for(i=0; i<MAX_ARG_NUM; i++)
	{
		pArgs[i] = pLine + strlen(pLine);	
	}
	*pLabel = pLine + strlen(pLine);

	/* ignore the comments */
	lPtr = pLine;
	while(*lPtr != ';' && *lPtr != '\0' && *lPtr != '\n')
	{
		lPtr++;
	}
	
	*lPtr = '\0';
	if(!(lPtr = strtok(pLine, "\t\n ,")))
	{
		return(EMPTY_LINE);
	}
	

	for(i=0; i<MAX_ARG_NUM; i++ )
	{
		pArgs[i] = lPtr;
		if(!(lPtr = strtok(NULL, "\t\n ,")))
			return(OK);
		
		//printf("lPtr %d = %s\n", i, pArgs[i]);
	}
	pArgs[i] = lPtr;
}

/****************************************************
* Function:memTypeNum 
* Description: find the memory type index in the g_mem_type_names in string.h
* Parameters:
*	[in] strPtr
* Return value:
*   	
*****************************************************/
int memTypeNum(char *strPtr)
{
	int i;
	int ret = -1;
	
	for(i=0; i<MAX_MEM_TYPE_NAME; i++)
	{
		if(!(strcmp(strPtr, g_mem_type_names[i])))
		{
			ret = i;
		}
	}
	return ret;
}


/****************************************************
* Function:isOpcode 
* Description: find the opcode index in the g_tr_opcode_names in string.h
* Parameters:
*	[in] strPtr
* Return value:
*   	
*****************************************************/
int isOpcode(char *strPtr)
{
	int i;
	int ret = -1;

	//for(i=0; i<12; i++)
	for(i=0; i<MAX_TR_OPCODE_NAME; i++)
	{
		if(!(strcmp(strPtr, g_tr_opcode_names[i])))
		{
			ret = i;
		}
	}
	return ret;
}

/**************************************
* Function: regNum
* Description: find the index in g_tr_reg_names in string.h
**************************************/
int regNum(char *strPtr)
{
	int i;
	int ret = -1;

	// convert uppercase letter to lowercase
	for(i=0; i<strlen(strPtr); i++)	
	{
		strPtr[i] = tolower(strPtr[i]);
	}
	
	for(i=0; i<MAX_TR_REG; i++)
	{	
		if(!(strcmp(strPtr, g_tr_reg_names[i])))
		{
			ret = i;
		}
	}
	
	return ret;
	
}



/****************************************************
* Function:cfNum
* Description: find the opcode index in the g_tr_opcode_names in string.h
* Parameters:
*	[in] strPtr
* Return value:
*   	
*****************************************************/
int cfNum(char *strPtr)
{
	int i;
	int ret = -1;

	for(i=0; i<MAX_CF_OPCODE_NAME; i++)
	{
		if(!(strcmp(strPtr, g_tr_cf_names[i])))
		{
			ret = i;
		}
	}
	return ret;
}


void dprint_trace(Trace_op *trace_op)
{
  std::cout << "*****Trace Data **** " << endl;
  std::cout <<"inst_addr:" << dec << trace_op->instruction_addr << endl;
  std::cout <<"inst_size:" << (int)  trace_op->inst_size << endl;
  std::cout <<"num_src:" << (int) trace_op->num_src << endl;
  std::cout <<"src[0]:"<< (int) trace_op->src[0] << endl;
  std::cout <<"src[1]:"<< (int) trace_op->src[1] << endl;
  std::cout <<"dst: " <<(int) trace_op->dst << endl;
  std::cout <<"opcode: " << (int) trace_op->opcode << endl;
  std::cout <<"is_fp: " << (int) trace_op->is_fp << endl;
  std::cout <<"cf_type: " << (int) trace_op->cf_type << endl;
  std::cout <<"mem_type: " << (int) trace_op->mem_type << endl;
  std::cout <<"write_flag: " << (int) trace_op->write_flag << endl;
  std::cout <<"ld_vaddr: " << dec << trace_op->ld_vaddr << endl;
  std::cout <<"st_vaddr: " << dec <<  trace_op->st_vaddr << endl;
  std::cout <<"instruction_addr: " << (int) trace_op->instruction_addr << endl;
  std::cout <<"branch_traget: " << dec << (int) trace_op->branch_target << endl;
  std::cout <<"actually_taken: " << (int) trace_op->actually_taken << endl;
  std::cout <<"mem_read_size: " << (int)trace_op->mem_read_size << endl;
  std::cout <<"mem_write_size: " << (int) trace_op->mem_write_size << endl;
}



/*****************************************************/
/* Trace read test file  */ 
/*****************************************************/

void read_trace() 
{
    
    Trace_op trace_op; 
    bool success = true; 

    cout <<"****************************read*******************" << endl; 

    while(success){
	success = (gzread(stream2, &trace_op, sizeof(Trace_op)) > 0); 
	dprint_trace(&trace_op); 
    }

}



