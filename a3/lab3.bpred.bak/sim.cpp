#include "common.h"
#include "sim.h"
#include "trace.h" 
#include "cache.h"  /**** NEW-LAB2*/ 
#include "memory.h" // NEW-LAB2 
#include <stdlib.h>
#include <ctype.h> /* Library for useful character operations */

#include "bpred.h"
#include "vmem.h"

/*******************************************************************/
/* Simulator frame */ 
/*******************************************************************/

bool run_a_cycle(memory_c *m ); // please modify run_a_cycle function argument  /** NEW-LAB2 */ 
void init_structures(memory_c *m); // please modify init_structures function argument  /** NEW-LAB2 */ 



/* uop_pool related variables */ 

uint32_t free_op_num;
uint32_t active_op_num; 
Op *op_pool; 
Op *op_pool_free_head = NULL; 

/* simulator related local functions */ 

bool icache_access(ADDRINT addr); /** please change uint32_t to ADDRINT NEW-LAB2 */  
bool dcache_access(ADDRINT addr); /** please change uint32_t to ADDRINT NEW-LAB2 */   
void  init_latches(void);

#include "knob.h"
#include "all_knobs.h"

// knob variables
KnobsContainer *g_knobsContainer; /* < knob container > */
all_knobs_c    *g_knobs; /* < all knob variables > */

gzFile g_stream;

void init_knobs(int argc, char** argv)
{
  // Create the knob managing class
  g_knobsContainer = new KnobsContainer();

  // Get a reference to the actual knobs for this component instance
  g_knobs = g_knobsContainer->getAllKnobs();

  // apply the supplied command line switches
  char* pInvalidArgument = NULL;
  g_knobsContainer->applyComandLineArguments(argc, argv, &pInvalidArgument);

  g_knobs->display();
}

void read_trace_file(void)
{
  g_stream = gzopen((KNOB(KNOB_TRACE_FILE)->getValue()).c_str(), "r");
}

// simulator main function is called from outside of this file 

void simulator_main(int argc, char** argv) 
{
  init_knobs(argc, argv);

  // trace driven simulation 
  read_trace_file();

  /** NEW-LAB2 */ /* just note: passing main memory pointers is hack to mix up c++ objects and c-style code */  /* Not recommended at all */ 
  memory_c *main_memory = new memory_c();  // /** NEW-LAB2 */ 



  init_structures(main_memory);  // please modify run_a_cycle function argument  /** NEW-LAB2 */ 
  run_a_cycle(main_memory); // please modify run_a_cycle function argument  /** NEW-LAB2 */ 


}
int op_latency[NUM_OP_TYPE]; 

void init_op_latency(void)	/* DO NOT MODIFY */
{
  op_latency[OP_INV]   = 1; 
  op_latency[OP_NOP]   = 1; 
  op_latency[OP_CF]    = 1; 
  op_latency[OP_CMOV]  = 1; 
  op_latency[OP_LDA]   = 1;
  op_latency[OP_LD]    = 1; 
  op_latency[OP_ST]    = 1; 
  op_latency[OP_IADD]  = 1; 
  op_latency[OP_IMUL]  = 2; 
  op_latency[OP_IDIV]  = 4; 
  op_latency[OP_ICMP]  = 2; 
  op_latency[OP_LOGIC] = 1; 
  op_latency[OP_SHIFT] = 2; 
  op_latency[OP_BYTE]  = 1; 
  op_latency[OP_MM]    = 2; 
  op_latency[OP_FMEM]  = 2; 
  op_latency[OP_FCF]   = 1; 
  op_latency[OP_FCVT]  = 4; 
  op_latency[OP_FADD]  = 2; 
  op_latency[OP_FMUL]  = 4; 
  op_latency[OP_FDIV]  = 16; 
  op_latency[OP_FCMP]  = 2; 
  op_latency[OP_FBIT]  = 2; 
  op_latency[OP_FCMP]  = 2; 
}

void init_op(Op *op)
{
  op->num_src               = 0; 
  op->src[0]                = -1; 
  op->src[1]                = -1;
  op->dst                   = -1; 
  op->opcode                = 0; 
  op->is_fp                 = false;
  op->cf_type               = NOT_CF;
  op->mem_type              = NOT_MEM;
  op->write_flag            = 0;
  op->inst_size             = 0;
  op->ld_vaddr              = 0;
  op->st_vaddr              = 0;
  op->instruction_addr      = 0;
  op->branch_target         = 0;
  op->actually_taken        = 0;
  op->mem_read_size         = 0;
  op->mem_write_size        = 0;
  op->valid                 = FALSE; 
  /* you might add more features here */ 
  op->is_last_instruction   = false;
  op->wait_till_cycle	    = -1;
  op->is_waiting	    = false;
}


void init_op_pool(void)
{
  /* initialize op pool */ 
  op_pool = new Op [1024];
  free_op_num = 1024; 
  active_op_num = 0; 
  uint32_t op_pool_entries = 0; 
  int ii;
  for (ii = 0; ii < 1023; ii++) {

    op_pool[ii].op_pool_next = &op_pool[ii+1]; 
    op_pool[ii].op_pool_id   = op_pool_entries++; 
    init_op(&op_pool[ii]); 
  }
  op_pool[ii].op_pool_next = op_pool_free_head; 
  op_pool[ii].op_pool_id   = op_pool_entries++;
  init_op(&op_pool[ii]); 
  op_pool_free_head = &op_pool[0]; 
}


Op *get_free_op(void)
{
  /* return a free op from op pool */ 

  if (op_pool_free_head == NULL || (free_op_num == 1)) {
    std::cout <<"ERROR! OP_POOL SIZE is too small!! " << endl; 
    std::cout <<"please check free_op function " << endl; 
    assert(1); 
    exit(1);
  }

  free_op_num--;
  assert(free_op_num); 

  Op *new_op = op_pool_free_head; 
  op_pool_free_head = new_op->op_pool_next; 
  assert(!new_op->valid); 
  init_op(new_op);
  active_op_num++; 
  return new_op; 
}

Op *copy_op(Op *copyop)
{
	/* Get a free op from pool and return copied op */
	Op * op = get_free_op();

	op->num_src               = copyop->num_src; 
	op->src[0]                = copyop->src[0]; 
	op->src[1]                = copyop->src[1];
	op->dst                   = copyop->dst; 
	op->opcode                = copyop->opcode; 
	op->is_fp                 = copyop->is_fp;
	op->cf_type               = copyop->cf_type;
	op->mem_type              = copyop->mem_type;
	op->write_flag            = copyop->write_flag;
	op->inst_size             = copyop->inst_size;
	op->ld_vaddr              = copyop->ld_vaddr;
	op->st_vaddr              = copyop->st_vaddr;
	op->instruction_addr      = copyop->instruction_addr;
	op->branch_target         = copyop->branch_target;
	op->actually_taken        = copyop->actually_taken;
	op->mem_read_size         = copyop->mem_read_size;
	op->mem_write_size        = copyop->mem_write_size;
	op->valid                 = copyop->valid; 
	/* you might add more features here */ 
	op->is_last_instruction   = copyop->is_last_instruction;
	op->wait_till_cycle	    = copyop->wait_till_cycle;
	op->is_waiting	    	  = copyop->is_waiting;

}

void free_op(Op *op)
{
  free_op_num++;
  active_op_num--; 
  op->valid = FALSE; 
  op->op_pool_next = op_pool_free_head;
  op_pool_free_head = op; 
}



/*******************************************************************/
/*  Data structure */
/*******************************************************************/

typedef struct pipeline_latch_struct {
  Op *op; /* you must update this data structure. */
  bool op_valid; 
   /* you might add more data structures. But you should complete the above data elements */ 
  bool pipeline_stall_enabled;		/* To tell the previous cycle to stall the pipeline */
  list<Op *> oplist;
}pipeline_latch; 


typedef struct Reg_element_struct{
  bool valid;
  // data is not needed 
  /* you might add more data structures. But you should complete the above data elements */ 
  bool busy;
}REG_element; 

REG_element register_file[NUM_REG];


/*******************************************************************/
/* These are the functions you'll have to write.  */ 
/*******************************************************************/

void FE_stage();
void ID_stage();
void EX_stage(); 
void MEM_stage(memory_c *main_memory); // please modify MEM_stage function argument  /** NEW-LAB2 */ 
void WB_stage(memory_c *main_memory); 

/*******************************************************************/
/*  These are the variables you'll have to write.  */ 
/*******************************************************************/

bool sim_end_condition = FALSE;      /* please complete the condition. */ 
bool last_instruction_recvd = false;
int num_of_inst_with_last_inst = 0;
int EX_cycles_completed = 0;	     /* Number of execution cycles completed for current op */
bool stop_fetching = false;	     /* end of trace. stop fetching */
bool returning_on_mshr_full = false; /* to indicate that whether the instruction is new or if its just returning due to earlier mshr full condition */
list<Op *> WB_pending;
list<UINT64> WB_pending_at_cycle;

UINT64 retired_instruction = 0;    /* number of retired instruction. (only correct instructions) */ 
UINT64 cycle_count = 0;            /* total number of cycles */ 
UINT64 data_hazard_count = 0;  
UINT64 control_hazard_count = 0; 
UINT64 icache_miss_count = 0;      /* total number of icache misses. for Lab #2 and Lab #3 */ 
UINT64 dcache_hit_count = 0;      /* total number of dcache  hits. for Lab #2 and Lab #3 */ 
UINT64 dcache_miss_count = 0;      /* total number of dcache  misses. for Lab #2 and Lab #3 */ 
UINT64 l2_cache_miss_count = 0;    /* total number of L2 cache  misses. for Lab #2 and Lab #3 */  
UINT64 dram_row_buffer_hit_count; /* total number of dram row buffer hit. for Lab #2 and Lab #3 */   // NEW-LAB2
UINT64 dram_row_buffer_miss_count; /* total number of dram row buffer hit. for Lab #2 and Lab #3 */   // NEW-LAB2
UINT64 store_load_forwarding_count = 0;  /* total number of store load forwarding for Lab #2 and Lab #3 */  // NEW-LAB2
uint64_t bpred_mispred_count = 0;  /* total number of branch mispredictions */  // NEW-LAB3
uint64_t bpred_okpred_count = 0;   /* total number of correct branch predictions */  // NEW-LAB3
uint64_t dtlb_hit_count = 0;       /* total number of DTLB hits */ // NEW-LAB3
uint64_t dtlb_miss_count = 0;      /* total number of DTLB miss */ // NEW-LAB3


pipeline_latch *MEM_latch;  
pipeline_latch *EX_latch;
pipeline_latch *ID_latch;
pipeline_latch *FE_latch;
UINT64 ld_st_buffer[LD_ST_BUFFER_SIZE];   /* this structure is deprecated. do not use */ 
UINT64 next_pc; 

Cache *data_cache;  // NEW-LAB2 
bpred *branchpred; // NEW-LAB3 (student need to initialize)
tlb *dtlb;        // NEW-LAB3 (student need to intialize)


/*******************************************************************/
/*  Print messages  */
/*******************************************************************/

void print_stats() {			/* DO NOT MODIFY */
  std::ofstream out((KNOB(KNOB_OUTPUT_FILE)->getValue()).c_str());
  /* Do not modify this function. This messages will be used for grading */ 
  out << "Total instruction: " << retired_instruction << endl; 
  out << "Total cycles: " << cycle_count << endl; 
  float ipc = (cycle_count ? ((float)retired_instruction/(float)cycle_count): 0 );
  out << "IPC: " << ipc << endl; 
  out << "Total I-cache miss: " << icache_miss_count << endl; 
  out << "Total D-cache miss: " << dcache_miss_count << endl; 
  out << "Total D-cache hit: " << dcache_hit_count << endl; 
//  out << "Total L2-cache miss: " << l2_cache_miss_count << endl; 
  out << "Total data hazard: " << data_hazard_count << endl;
  out << "Total control hazard : " << control_hazard_count << endl; 
  out << "Total DRAM ROW BUFFER Hit: " << dram_row_buffer_hit_count << endl;  // NEW-LAB2
  out << "Total DRAM ROW BUFFER Miss: "<< dram_row_buffer_miss_count << endl;  // NEW-LAB2 
  out <<" Total Store-load forwarding: " << store_load_forwarding_count << endl;  // NEW-LAB2 

  // new for LAB3
  out << "Total Branch Predictor Mispredictions: " << bpred_mispred_count << endl;   
  out << "Total Branch Predictor OK predictions: " << bpred_okpred_count << endl;   
  out << "Total DTLB Hit: " << dtlb_hit_count << endl; 
  out << "Total DTLB Miss: " << dtlb_miss_count << endl; 

  out.close();
}

/*******************************************************************/
/*  Support Functions  */ 
/*******************************************************************/

bool get_op(Op *op)
{
  static UINT64 unique_count = 1; 
  Trace_op trace_op; 
  bool success = FALSE; 
  // read trace 
  // fill out op info 
  // return FALSE if the end of trace 
  success = (gzread(g_stream, &trace_op, sizeof(Trace_op)) >0 );
  if (KNOB(KNOB_PRINT_INST)->getValue()) dprint_trace(&trace_op); 

  /* copy trace structure to op */ 
  if (success) { 
    copy_trace_op(&trace_op, op); 

    op->inst_id  = unique_count++;
    op->valid    = TRUE; 
  }
  return success; 
}
/* return op execution cycle latency */ 

int get_op_latency (Op *op) 
{
  assert (op->opcode < NUM_OP_TYPE); 
  return op_latency[op->opcode];
}

/* Print out all the register values */ 
void dump_reg() {
  for (int ii = 0; ii < NUM_REG; ii++) {
    std::cout << cycle_count << ":register[" << ii  << "]: V:" << register_file[ii].valid << endl; 
  }
}

void print_pipeline()
{		/* DO NOT MODIFY */
	std::cout << "--------------------------------------------" << endl; 
	std::cout <<"cycle count : " << dec << cycle_count << " retired_instruction : " << retired_instruction << endl; 
	std::cout << (int)cycle_count << " FE: " ;
	if (FE_latch->op_valid)
	{
		Op *op = FE_latch->op; 
		cout << (int)op->inst_id ;
	}
	else
	{
		cout <<"####";
	}
	std::cout << " ID: " ;
	if (ID_latch->op_valid)
	{
		Op *op = ID_latch->op; 
		cout << (int)op->inst_id ;
	}
	else 
	{
		cout <<"####";
	}
	std::cout << " EX: " ;
	if (EX_latch->op_valid) 
	{
		Op *op = EX_latch->op; 
		cout << (int)op->inst_id ;
	}
	else 
	{
		cout <<"####";
	}


	std::cout << " MEM: " ;
	if (MEM_latch->op_valid) 
	{
		for(list<Op *>::const_iterator cii = MEM_latch->oplist.begin() ; cii != MEM_latch->oplist.end() ; cii++)
		{
			Op *op = (*cii); 
			cout << (int)op->inst_id << " ";
		}
	}
	else 
	{
		cout <<"####";
	}
	cout << endl; 
	//  dump_reg();   
	std::cout << "--------------------------------------------" << endl; 
}

void print_heartbeat()
{
  static uint64_t last_cycle ;
  static uint64_t last_inst_count; 
  float temp_ipc = float(retired_instruction - last_inst_count) /(float)(cycle_count-last_cycle) ;
  float ipc = float(retired_instruction) /(float)(cycle_count) ;
  /* Do not modify this function. This messages will be used for grading */ 
  cout <<"**Heartbeat** cycle_count: " << cycle_count << " inst:" << retired_instruction << " IPC: " << temp_ipc << " Overall IPC: " << ipc << endl; 
  last_cycle = cycle_count;
  last_inst_count = retired_instruction; 
}
/*******************************************************************/
/*                                                                 */
/*******************************************************************/

bool run_a_cycle(memory_c *main_memory){   // please modify run_a_cycle function argument  /** NEW-LAB2 */ 
  int i = 0;
  for (;;) { 
    if (((KNOB(KNOB_MAX_SIM_COUNT)->getValue() && (cycle_count >= KNOB(KNOB_MAX_SIM_COUNT)->getValue())) || 
      (KNOB(KNOB_MAX_INST_COUNT)->getValue() && (retired_instruction >= KNOB(KNOB_MAX_INST_COUNT)->getValue())) ||  (sim_end_condition))) { 
        // please complete sim_end_condition 
        // finish the simulation 
        print_heartbeat(); 
        print_stats();
        return TRUE; 
    }
    cycle_count++; 
    if (!(cycle_count%5000)) 
    {
      print_heartbeat(); 
    }

    
    main_memory->run_a_cycle();          // *NEW-LAB2 

    WB_stage(main_memory); 
    MEM_stage(main_memory);  // please modify MEM_stage function argument  /** NEW-LAB2 */ 
    EX_stage();
    ID_stage();
    FE_stage(); 
    if (KNOB(KNOB_PRINT_PIPE_FREQ)->getValue() && !(cycle_count%KNOB(KNOB_PRINT_PIPE_FREQ)->getValue())) print_pipeline();
  }
  return TRUE; 
}


/*******************************************************************/
/* Complete the following fuctions.  */
/* You can add new data structures and also new elements to Op, Pipeline_latch data structure */ 
/*******************************************************************/
void init_reg_file()
{
	int i;
	for(i = 0 ; i < NUM_REG ; i++)
	{
		register_file[i].valid = false;
		register_file[i].busy = false;
	}
}

void init_structures(memory_c *main_memory) // please modify init_structures function argument  /** NEW-LAB2 */ 
{
  init_op_pool(); 
  init_op_latency();
  init_reg_file();
  /* please initialize other data stucturs */ 
  /* you must complete the function */
  init_latches();
  cache_size = KNOB(KNOB_DCACHE_SIZE)->getValue();
  block_size = KNOB(KNOB_BLOCK_SIZE)->getValue();
  assoc = KNOB(KNOB_DCACHE_WAY)->getValue();
  dcache_latency = KNOB(KNOB_DCACHE_LATENCY)->getValue();
  mshr_size = KNOB(KNOB_MSHR_SIZE)->getValue();
  use_bpred = KNOB(KNOB_USE_BPRED)->getValue();
  type_bpred = (bpred_type) KNOB(KNOB_BPRED_TYPE)->getValue();
  hist_len = KNOB(KNOB_BPRED_HIST_LEN)->getValue();

  data_cache = (Cache *) malloc(sizeof(Cache));
  cache_init(data_cache,cache_size,block_size,assoc,"dcache");
  main_memory->init_mem();

  if(use_bpred)
	  branchpred = bpred_new(type_bpred,hist_len);
  else
	  branchpred = NULL;
}

void WB_stage(memory_c *main_memory)
{
  /* You MUST call free_op function here after an op is retired */
//  if(last_instruction_recvd && (main_memory->m_mshr.empty()) && (!MEM_latch->op_valid))

  if(!MEM_latch->op_valid)
	  return;

  if(MEM_latch->oplist.empty())
	  return;
  for(list<Op *>::iterator it = MEM_latch->oplist.begin() ; it != MEM_latch->oplist.end() ; it++)
  {
	  Op *op = *it;
	  if((op->dst >= 0) && (op->dst < NUM_REG))
	  {
		/* Mark the destination register as not busy */
		register_file[op->dst].busy = false;
	  }
	  /* If it is a branch instruction, the address of branch is now available.
	     Tell the FE_stage to Fetch next instruction */
	  if((op->cf_type >= CF_BR) && (op->cf_type < NUM_CF_TYPES))
	  {
		FE_latch->pipeline_stall_enabled = false;
	  }

	  if(op->is_last_instruction)
	  {
		  last_instruction_recvd = true;
	  	  retired_instruction--;
		  num_of_inst_with_last_inst = MEM_latch->oplist.size();
	  }
  	  free_op(op);
   }
  
  retired_instruction += MEM_latch->oplist.size();
  MEM_latch->op_valid = false;
  MEM_latch->oplist.clear();
  if(last_instruction_recvd && (main_memory->m_mshr.empty()) && (!MEM_latch->op_valid) && (WB_pending.empty()))
  {
	  sim_end_condition = true;
	  if(num_of_inst_with_last_inst > 1)
		  cycle_count--;
	  return;
  }

}

void MEM_stage(memory_c *main_memory)  // please modify MEM_stage function argument  /** NEW-LAB2 */ 
{
	if((MEM_latch->op_valid) || (!EX_latch->op_valid))
		return;

	Op *op = EX_latch->op;

	if(!((op->mem_type > NOT_MEM) && (op->mem_type < NUM_MEM_TYPES)))	// Not a memory op
	{
			MEM_latch->op = op;		// deprecated
			MEM_latch->oplist.push_back(op);
			MEM_latch->op_valid = true;
			EX_latch->op_valid = false;
			return;
	}

	/* If it is a memory instruction, wait for dcache latency cycles */
	if((op->mem_type > NOT_MEM) && (op->mem_type < NUM_MEM_TYPES))
	{
		if(op->is_waiting)	
		{
			if(cycle_count < op->wait_till_cycle)	// op should remain blocked for dcache access latency
				return;

			op->is_waiting = false;			// op completed wait for dcache access latency
		}
		else
		{
			if(!returning_on_mshr_full)
			{
				op->wait_till_cycle = cycle_count + dcache_latency - 1;	// new op - set a deadline
				op->is_waiting = true;					// order it to wait
				return;
			}
		}
	}

	/* Op has completed its wait for dcache latency amount of cycles */
	if(returning_on_mshr_full)
	{
		UINT64 ac_addr = (op->mem_type == MEM_ST) ? op->st_vaddr : op->ld_vaddr;
		if(dcache_access(ac_addr))
		{
			/* we got a cache hit - pass op to WB stage*/
			dcache_hit_count++;
			cache_update(data_cache,ac_addr);
			MEM_latch->op = op;		// deprecated
			MEM_latch->oplist.push_back(op);
			MEM_latch->op_valid = true;
			EX_latch->op_valid = false;	/* will help in handling Case #2 hit under miss */
			return;
		}

		if(main_memory->insert_mshr(op))
		{
			/* added successfully into mshr */
			EX_latch->op_valid = false;
			returning_on_mshr_full = false;
			return;
		}
		else
		{
			returning_on_mshr_full = true;
			return;		// MSHR is full - wait for next cycle
		}
	}

	/* Check if we get the hit */
	UINT64 ac_addr = (op->mem_type == MEM_ST) ? op->st_vaddr : op->ld_vaddr;
	if(dcache_access(ac_addr))
	{
		/* we got a cache hit - pass op to WB stage*/
		dcache_hit_count++;
		cache_update(data_cache,ac_addr);
		MEM_latch->op = op;		// deprecated
		MEM_latch->oplist.push_back(op);
		MEM_latch->op_valid = true;
		EX_latch->op_valid = false;	/* will help in handling Case #2 hit under miss */
		return;
	}
	
	/* We got a cache miss */
	dcache_miss_count++;
	/* Store Load Forwarding */
	if(main_memory->store_load_forwarding(op))
	{
		/* We got MSHR Hit in store load forwarding */
		store_load_forwarding_count++;
		MEM_latch->op = op;		// deprecated
		MEM_latch->oplist.push_back(op);
		MEM_latch->op_valid = true;
		EX_latch->op_valid = false;
		return;

	}
	/* Check if there is block hit for inst already present in MSHR - Case #4 MSHR HIT*/
	else if(main_memory->check_piggyback(op))
	{
		/* instruction piggybacked - allow EX to send next instruction */
		EX_latch->op_valid = false;
		return;
	}
	else
	{
		/* cache & MSHR miss - add into mshr */
		if(main_memory->insert_mshr(op))
		{
			/* added successfully into mshr */
			EX_latch->op_valid = false;
			returning_on_mshr_full = false;
			return;
		}
		else
		{
			returning_on_mshr_full = true;
			return;		// MSHR is full - wait for next cycle
		}
	}
	return;
}


void EX_stage() 
{
	if((EX_latch->op_valid) || (!ID_latch->op_valid))
	{
		/* Either MEM has not completed its work ==> pipeline stall
		   Or ID has not yet given any new op ==> No work to do */
		return;
	}
	Op *op = ID_latch->op;
	EX_cycles_completed++;

	if((op->opcode > 0) && (op->opcode < NUM_OP_TYPE))
	{
		// This is a valid opcode
		int oplatency = get_op_latency(op);
		if(EX_cycles_completed != oplatency)
		{
			// Operation not completed. Still more work to do.
			return;
		}
	}

	EX_latch->op = op;
	EX_latch->op_valid = true;
	ID_latch->op_valid = false;
	EX_cycles_completed = 0;
	return;
}

void ID_stage()
{
	if((!FE_latch->op_valid) || (ID_latch->op_valid))
	{
		/* If FE_latch has not yet written the data
		   or if the EX is still busy,
		   then, do not perform any work.
		*/
		return;
	}

	Op* op = FE_latch->op;		// get data from the latch

	/* Now, we'll check for data dependency - Data Hazard MUST be checked before control hazard.*/
	int r;
	for(r = 0 ; r < 2 ; r++)
	{
		if((op->src[r] >= 0) && (op->src[r] < NUM_REG))
		{
			if(register_file[(op->src[r])].busy)
			{
				/* Data Hazard detected. Do not perform any work */
				data_hazard_count++;
				return;
			}

		}
	}


	if((!use_bpred) && (op->cf_type >= CF_BR) && (op->cf_type < NUM_CF_TYPES))
	{
		/* A branch instruction is identified. 
		   If we are using branch predictor, then nothing to do here.
		   Processor stall has been taken cae of in FE_stage
		   Otherwise, tell the FE stage to stall. Control Hazard.*/

		FE_latch->pipeline_stall_enabled = true;
		control_hazard_count++;
	}
	/* Pass on the instruction to the EX stage now */

	if((op->dst >= 0) && (op->dst < NUM_REG))
	{
		/* Mark the destination register as busy */
		register_file[op->dst].busy = true;
	}

	ID_latch->op = op;
	ID_latch->op_valid = true;
	FE_latch->op_valid = false;
}


void FE_stage()
{
  /* only part of FE_stage function is implemented */ 
  /* please complete the rest of FE_stage function */ 

  if(stop_fetching)
  	return;

  if(FE_latch->op_valid || FE_latch->pipeline_stall_enabled)
  {
	  /* Data inside the latch is valid and next stage is still using it.
	     Or ID stage has enabled pipeline stalling because of a branch instruction.
	     Do not fetch */
	  return;
  }

  Op *op = get_free_op();
  bool next_op_exists = get_op(op);
  if(!next_op_exists)			/* Indicate the end_of_trace by the FE cycle */
  {
	free_op(op);
	op = get_free_op();
	op->is_last_instruction = true;
	stop_fetching = true;
  }

  /* If the op is an branch other than conditional branch, assume that it is predicted correctly,
     if the branch predictor is used */
  if(use_bpred && (op->cf_type >= CF_BR) && (op->cf_type < NUM_CF_TYPES) && (op->cf_type != CF_CBR))
	  bpred_okpred_count++;

  /* If we are using branch predictor and type of opcode is conditional branch,
     get a prediction and update GHR and PHT */
  if(use_bpred && (op->cf_type == CF_CBR))
  {
	int prediction = bpred_access(branchpred, op->instruction_addr);
	if(prediction == op->actually_taken)
		bpred_okpred_count++;
	else
	{
		bpred_mispred_count++;
		/* stall the pipeline if we mispredict */
		FE_latch->pipeline_stall_enabled = true;;
	}
	bpred_update(branchpred,op->instruction_addr,prediction,op->actually_taken);
  }

  /* hwsim : get the instruction and pass to ID phase */
  FE_latch->op = op;				/* pass the op to ID stage */
  FE_latch->op_valid = true;			/* Mark it as valid */
//FE_latch->pipeline_stall_enabled = false;	/* Disable pipeline stall, if any */

  //   next_pc = pc + op->inst_size;  // you need this code for building a branch predictor 

}


void  init_latches()
{
  MEM_latch = new pipeline_latch();
  EX_latch = new pipeline_latch();
  ID_latch = new pipeline_latch();
  FE_latch = new pipeline_latch();

  MEM_latch->op = NULL;
  EX_latch->op = NULL;
  ID_latch->op = NULL;
  FE_latch->op = NULL;

  /* you must set valid value correctly  */ 
  MEM_latch->op_valid = false;
  EX_latch->op_valid = false;
  ID_latch->op_valid = false;
  FE_latch->op_valid = false;

  MEM_latch->pipeline_stall_enabled = false;
  EX_latch->pipeline_stall_enabled = false;
  ID_latch->pipeline_stall_enabled = false;
  FE_latch->pipeline_stall_enabled = false;

  MEM_latch->oplist.clear();
  EX_latch->oplist.clear();
  ID_latch->oplist.clear();
  FE_latch->oplist.clear();

}

bool icache_access(ADDRINT addr) {   /** please change uint32_t to ADDRINT NEW-LAB2 */ 

  /* For Lab #1, you assume that all I-cache hit */     
  bool hit = FALSE; 
  if (KNOB(KNOB_PERFECT_ICACHE)->getValue()) hit = TRUE; 
  return hit; 
}


bool dcache_access(ADDRINT addr) 
{ 
	/** please change uint32_t to ADDRINT NEW-LAB2 */ 
	/* For Lab #1, you assume that all D-cache hit */     
	/* For Lab #2, you need to connect cache here */   // NEW-LAB2 
	bool hit = FALSE;
	if (KNOB(KNOB_PERFECT_DCACHE)->getValue())	  /* always get a hit for perfect dcache */
	{
		hit = TRUE; 
		return hit; 
	}

	if(cache_read(data_cache,addr))
	{
		/* cache hit */
		return true;
	}
	else
		return false;				/* cache miss */
}



// NEW-LAB2 
void dcache_insert(ADDRINT addr)  // NEW-LAB2 
{                                 // NEW-LAB2 
  /* dcache insert function */   // NEW-LAB2 
  cache_insert(data_cache, addr) ;   // NEW-LAB2 
 
}                                       // NEW-LAB2 

void broadcast_rdy_op(Op* op)             // NEW-LAB2 
{                                          // NEW-LAB2 
	/* you must complete the function */     // NEW-LAB2 
	// mem ops are done.  move the op into WB stage   // NEW-LAB2 
	WB_pending.push_back(op);
	WB_pending_at_cycle.push_back(cycle_count);
/*	MEM_latch->oplist.push_back(op);
	MEM_latch->op_valid = true;
*/
}      // NEW-LAB2 

/* utility functions that you might want to implement */     // NEW-LAB2 
int64_t get_dram_row_id(ADDRINT addr)    // NEW-LAB2 
{  // NEW-LAB2 
 // NEW-LAB2 
/* utility functions that you might want to implement */     // NEW-LAB2 
/* if you want to use it, you should find the right math! */     // NEW-LAB2 
/* pleaes carefull with that DRAM_PAGE_SIZE UNIT !!! */     // NEW-LAB2 
  // addr >> 6;   // NEW-LAB2 

  return ((addr)/((KNOB(KNOB_DRAM_PAGE_SIZE)->getValue())*1024));
//  return 2;   // NEW-LAB2 
}  // NEW-LAB2 

int get_dram_bank_id(ADDRINT addr)  // NEW-LAB2 
{  
	// NEW-LAB2 
	// NEW-LAB2 
	/* utility functions that you might want to implement */     // NEW-LAB2 
	/* if you want to use it, you should find the right math! */     // NEW-LAB2 
	return (((addr)/((KNOB(KNOB_DRAM_PAGE_SIZE)->getValue()) * 1024)) % (KNOB(KNOB_DRAM_BANK_NUM)->getValue()));

	// (addr >> 6);   // NEW-LAB2 
	//  return 1;   // NEW-LAB2 
}  // NEW-LAB2 

void move_to_mem_latch()
{
	UINT64 prev_cycle = cycle_count - 1;
	while(!WB_pending.empty())
	{
		if(WB_pending_at_cycle.front() == prev_cycle)
		{
			WB_pending_at_cycle.pop_front();
			Op * op = WB_pending.front();
			WB_pending.pop_front();
//			cache_insert(data_cache,(op->mem_type == MEM_ST ? op->st_vaddr : op->ld_vaddr));
			MEM_latch->oplist.push_back(op);
			MEM_latch->op_valid = true;
		}
		else
			break;
	}
	return;
}
