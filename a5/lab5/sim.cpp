#include "common.h"
#include "sim.h"
#include "trace.h" 
#include "cache.h"  /**** NEW-LAB2*/ 
#include "memory.h" // NEW-LAB2 
#include <stdlib.h>
#include <ctype.h> /* Library for useful character operations */

#include "bpred.h"
#include "vmem.h"

#include "thread_support.h"

//#define HW_MAX_THREAD 4   // add this line --> added to thread_support.h

/*******************************************************************/
/* Simulator frame */ 
/*******************************************************************/
void generate_mcpat_xml();
void generate_power_counters();

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

gzFile g_stream[HW_MAX_THREAD];    // replace extern gzFile stream; 

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
void read_trace_file(void) {
  g_stream[0] = gzopen((KNOB(KNOB_TRACE_FILE)->getValue()).c_str(), "r");

  if ((KNOB(KNOB_RUN_THREAD_NUM)->getValue())<2) return;    /** NEW-LAB4 **/
   g_stream[1] = gzopen((KNOB(KNOB_TRACE_FILE2)->getValue()).c_str(), "r"); /** NEW-LAB4 **/
   if ((KNOB(KNOB_RUN_THREAD_NUM)->getValue())<3) return;  /** NEW-LAB4 **/
   g_stream[2] = gzopen((KNOB(KNOB_TRACE_FILE3)->getValue()).c_str(), "r"); /** NEW-LAB4 **/
   if ((KNOB(KNOB_RUN_THREAD_NUM)->getValue())<4) return;  /** NEW-LAB4 **/
   g_stream[3] = gzopen((KNOB(KNOB_TRACE_FILE4)->getValue()).c_str(), "r"); /** NEW-LAB4 **/
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
  op->is_pteop		    = false;
  op->vpn		    = 0;
  op->thread_id		    = 0;
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
  uint64_t stall_enforcer;		/* Only the op that enforces stall can remove it. Kepp its record here */
  list<Op *> oplist;
  Op *op_queue[HW_MAX_THREAD];
  bool op_valid_thread[HW_MAX_THREAD];
  bool pipeline_stall_enabled_thread[HW_MAX_THREAD];
  uint64_t stall_enforcer_thread[HW_MAX_THREAD];
}pipeline_latch; 


typedef struct Reg_element_struct{
  bool valid;
  // data is not needed 
  /* you might add more data structures. But you should complete the above data elements */ 
  bool busy;
}REG_element; 

REG_element register_file[HW_MAX_THREAD][NUM_REG];


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
bool pteop_returning_on_mshr_full = false; /* to indicate whether pteop is trying again in mshr because it was full last time */
bool pteaddr_broadcasted = false;
list<Op *> WB_pending;
list<UINT64> WB_pending_at_cycle;
bool end_of_stream[HW_MAX_THREAD];
bool have_to_send_EOS = false;

int thread_count = 0;

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

uint64_t fp_ops = 0;
uint64_t int_ops = 0;
uint64_t ld_ops = 0;
uint64_t st_ops = 0;
uint64_t br_ops = 0;
uint64_t int_reg_reads = 0;
uint64_t fp_reg_reads = 0;
uint64_t int_reg_writes = 0;
uint64_t fp_reg_writes = 0;
uint64_t mul_ops = 0;
uint64_t dcache_reads = 0;
uint64_t dcache_writes = 0;
uint64_t sched_access = 0;
uint64_t mem_access = 0;
uint64_t pred_entries = 0;
uint64_t dcache_read_misses = 0;
uint64_t dcache_write_misses = 0;


bool br_stall[HW_MAX_THREAD] = {0};  
uint64_t retired_instruction_thread[HW_MAX_THREAD] = {0};       // NEW for LAB4
uint64_t dcache_miss_count_thread[HW_MAX_THREAD] = {0};         // NEW for LAB4
uint64_t dcache_hit_count_thread[HW_MAX_THREAD] = {0};  // NEW for LAB4
uint64_t data_hazard_count_thread[HW_MAX_THREAD] = {0};         // NEW for LAB4
uint64_t control_hazard_count_thread[HW_MAX_THREAD] = {0};      // NEW for LAB4
uint64_t store_load_forwarding_count_thread[HW_MAX_THREAD] = {0};       // NEW for LAB4
uint64_t bpred_mispred_count_thread[HW_MAX_THREAD] = {0};       // NEW for LAB4
uint64_t bpred_okpred_count_thread[HW_MAX_THREAD] = {0};        // NEW for LAB4
uint64_t dtlb_hit_count_thread[HW_MAX_THREAD] = {0};    // NEW for LAB4
uint64_t dtlb_miss_count_thread[HW_MAX_THREAD] = {0};   // NEW for LAB4

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
void print_stats() {
  std::ofstream out((KNOB(KNOB_OUTPUT_FILE)->getValue()).c_str());
  /* Do not modify this function. This messages will be used for grading */

  out << "Total instruction: " << retired_instruction << endl;
  out << "Total cycles: " << cycle_count << endl;
  float ipc = (cycle_count ? ((float)retired_instruction/(float)cycle_count): 0 );
  out << "Total IPC: " << ipc << endl;
  out << "Total D-cache miss: " << dcache_miss_count << endl;
  out << "Total D-cache hit: " << dcache_hit_count << endl;
  out << "Total data hazard: " << data_hazard_count << endl;
  out << "Total control hazard : " << control_hazard_count << endl;
  out << "Total DRAM ROW BUFFER Hit: " << dram_row_buffer_hit_count << endl;
  out << "Total DRAM ROW BUFFER Miss: "<< dram_row_buffer_miss_count << endl;
  out << "Total Store-load forwarding: " << store_load_forwarding_count << endl;
  out << "Total Branch Predictor Mispredictions: " << bpred_mispred_count << endl;
  out << "Total Branch Predictor OK predictions: " << bpred_okpred_count << endl;
  out << "Total DTLB Hit: " << dtlb_hit_count << endl;
  out << "Total DTLB Miss: " << dtlb_miss_count << endl;

  out << endl << endl << endl;

  for (int ii = 0; ii < (KNOB(KNOB_RUN_THREAD_NUM)->getValue()); ii++ ) {
    out << "THREAD instruction: " << retired_instruction_thread[ii] << " Thread id: " << ii << endl;
    float thread_ipc = (cycle_count ? ((float)retired_instruction_thread[ii]/(float)cycle_count): 0 );
    out << "THREAD IPC: " << thread_ipc << endl;
    out << "THREAD D-cache miss: " << dcache_miss_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD D-cache hit: " << dcache_hit_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD data hazard: " << data_hazard_count_thread[ii] << " Thread id: " << ii <<   endl;
    out << "THREAD control hazard : " << control_hazard_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD Store-load forwarding: " << store_load_forwarding_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD Branch Predictor Mispredictions: " << bpred_mispred_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD Branch Predictor OK predictions: " << bpred_okpred_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD DTLB Hit: " << dtlb_hit_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD DTLB Miss: " << dtlb_miss_count_thread[ii] << " Thread id: " << ii << endl;
  }
  out.close();
}



/*******************************************************************/
/*  Support Functions  */ 
/*******************************************************************/
/* getOp */
bool get_op(Op *op, int fetch_id)
{
	static UINT64 unique_count = 0; 

	if(end_of_stream[fetch_id])
		return false;

	Trace_op trace_op; 
	bool success = false; 
	// read trace 
	// fill out op info 
	// return FALSE if the end of trace
	int read_size;

	read_size = gzread(g_stream[fetch_id], &trace_op, sizeof(Trace_op));
	success = read_size>0;
	if(read_size!=sizeof(Trace_op) && read_size>0) 
	{
		printf( "ERROR!! gzread reads corrupted op! @cycle:%llu\n", cycle_count);
		success = false;
	}

	if (KNOB(KNOB_PRINT_INST)->getValue()) 
		dprint_trace(&trace_op);

	/* copy trace structure to op */ 
	if (success) 
	{ 
		copy_trace_op(&trace_op, op); 

		op->inst_id  = unique_count++;
		op->valid    = TRUE; 
		op->thread_id = fetch_id; 
		return success;  // get op so return 
	}
	else
		end_of_stream[fetch_id] = true;

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
	for (int jj = 0 ; jj < HW_MAX_THREAD ; jj++)
		for (int ii = 0; ii < NUM_REG; ii++)
			std::cout << cycle_count << ":register[" << ii  << "]: V:" << register_file[jj][ii].busy << endl; 
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
  static uint64_t last_cycle_thread[HW_MAX_THREAD] ;
  static uint64_t last_inst_count_thread[HW_MAX_THREAD];

  for (int ii = 0; ii < (KNOB(KNOB_RUN_THREAD_NUM)->getValue()); ii++ ) {

    float temp_ipc = float(retired_instruction_thread[ii] - last_inst_count_thread[ii]) /(float)(cycle_count-last_cycle_thread[ii]) ;
    float ipc = float(retired_instruction_thread[ii]) /(float)(cycle_count) ;
    /* Do not modify this function. This messages will be used for grading */
    cout <<"**Heartbeat** cycle_count: " << cycle_count << " inst:" << retired_instruction_thread[ii] << " IPC: " << temp_ipc << " Overall IPC: " << ipc << " thread_id " << ii << endl;
    last_cycle_thread[ii] = cycle_count;
    last_inst_count_thread[ii] = retired_instruction_thread[ii];
  }
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
	generate_mcpat_xml();
	generate_power_counters();
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
	for(int j = 0 ; j < HW_MAX_THREAD ; j++)
	{
		for(i = 0 ; i < NUM_REG ; i++)
		{
			register_file[j][i].valid = false;
			register_file[j][i].busy = false;
		}
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
  hist_len = (type_bpred == BPRED_PERCEPTRON) ? KNOB(KNOB_PERCEP_HIS_LENGTH)->getValue() : KNOB(KNOB_BPRED_HIST_LEN)->getValue();
  percep_entries = KNOB(KNOB_PERCEP_TABLE_ENTRY)->getValue();
  threshold = KNOB(KNOB_PERCEP_THRESH_OVD)->getValue();
  int percep_ctr_bits = KNOB(KNOB_PERCEP_CTR_BITS)->getValue();
  percep_wt_max = (1 << percep_ctr_bits) - 1;
  percep_wt_min = -(1 << percep_ctr_bits);
  vmem_enabled = KNOB(KNOB_ENABLE_VMEM)->getValue();
  tlb_size = KNOB(KNOB_TLB_ENTRIES)->getValue();
  vmem_page_size = KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
  thread_count = KNOB(KNOB_RUN_THREAD_NUM)->getValue();
  pred_entries = (1 << hist_len);

  for(int i = 0 ; i < HW_MAX_THREAD ; i++)
	  end_of_stream[i] = false;

  data_cache = (Cache *) malloc(sizeof(Cache));
  cache_init(data_cache,cache_size,block_size,assoc,"dcache");
  main_memory->init_mem();

  if(use_bpred)
	  branchpred = bpred_new(type_bpred,hist_len);
  else
	  branchpred = NULL;

  if(vmem_enabled)
	  dtlb = tlb_new(tlb_size);
  else
	  dtlb = NULL;
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
		register_file[op->thread_id][op->dst].busy = false;
		uint8_t c = op->opcode;
		if((c == OP_FCVT) || (c == OP_FADD) || (c == OP_FMUL) || (c == OP_FDIV) || (c == OP_FCMP) ||
				(c == OP_FBIT) || (c == OP_FCMO) || op->is_fp)
			fp_reg_writes++;

		else if((c == OP_IADD) || (c == OP_IMUL) || (c == OP_ICMP) || (c == OP_IDIV))
			int_reg_writes++;
	  }
	  /* If it is a branch instruction, the address of branch is now available.
	     Tell the FE_stage to Fetch next instruction */
	  if((op->cf_type >= CF_BR) && (op->cf_type < NUM_CF_TYPES) && (FE_latch->stall_enforcer_thread[op->thread_id] == op->inst_id))
	  {
		FE_latch->pipeline_stall_enabled_thread[op->thread_id] = false;
		FE_latch->stall_enforcer_thread[op->thread_id] = 0;
	  }
	  
	  retired_instruction_thread[op->thread_id]++;

	  if(op->is_last_instruction)
	  {
		  last_instruction_recvd = true;
	  	  retired_instruction--;
		  retired_instruction_thread[op->thread_id]--;
		  num_of_inst_with_last_inst = MEM_latch->oplist.size();
	  }
  	  free_op(op);
   }
  
  retired_instruction += MEM_latch->oplist.size();
  MEM_latch->op_valid = false;
  MEM_latch->oplist.clear();

  /* setting sim end condition */
  bool l_inst,dram,fe,id,ex,mem,wb;
  l_inst = last_instruction_recvd;
  dram = main_memory->m_mshr.empty();
  wb = WB_pending.empty();
  mem = !MEM_latch->op_valid;
  ex = !EX_latch->op_valid;
  id = !ID_latch->op_valid;
  fe = true;
  for(int i = 0 ; i < thread_count ; i++)
  {
	  if(FE_latch->op_valid_thread[i])
		  fe = false;
  }

//  if(last_instruction_recvd && (main_memory->m_mshr.empty()) && (!MEM_latch->op_valid) && (WB_pending.empty()))

  if(l_inst  &&  dram  &&  wb  &&  mem  &&  ex  &&  id  &&  fe)
  {
	  sim_end_condition = true;
	  if(num_of_inst_with_last_inst == 1)
		  cycle_count--;
	  return;
  }

}

void MEM_stage(memory_c *main_memory)  // please modify MEM_stage function argument  /** NEW-LAB2 */ 
{
	if((MEM_latch->op_valid) || (!EX_latch->op_valid))
		return;

	Op *op = EX_latch->op;
	int threadid = op->thread_id;		/* keep this 0 for LAB 3 */
	uint64_t effective_latency = dcache_latency - 1;

	if(!((op->mem_type > NOT_MEM) && (op->mem_type < NUM_MEM_TYPES)))	// Not a memory op
	{
			MEM_latch->op = op;		// deprecated
			MEM_latch->oplist.push_back(op);
			MEM_latch->op_valid = true;
			EX_latch->op_valid = false;
			return;
	}
	
	UINT64 ac_addr = (op->mem_type == MEM_ST) ? op->st_vaddr : op->ld_vaddr;
	if(pteaddr_broadcasted)
	{
		/* We have all translations now. Set up the op to do cache access
		   so that actual data is now loaded */
		uint64_t vpn = ac_addr / KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
		uint64_t index = ac_addr % KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
		uint64_t pteaddr = vmem_get_pteaddr(vpn,threadid);
		uint64_t pfn = vmem_vpn_to_pfn(vpn,threadid);
		ac_addr = (pfn * KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()) | index;
		if(op->mem_type == MEM_ST)
			op->st_vaddr = ac_addr;
		else if(op->mem_type == MEM_LD)
			op->ld_vaddr = ac_addr;
		EX_latch->pipeline_stall_enabled = false;
		pteaddr_broadcasted = false;
		goto dcache_access_for_data;
	}

	if(vmem_enabled && (op->mem_type > NOT_MEM) && (op->mem_type < NUM_MEM_TYPES))
	{
		/* Making tlb access just to test if we get a hit.
		   This is not a real TLB access. It is done later.
		   This is just to take care of dcache_latency */
		uint64_t tvaddr = (op->mem_type == MEM_ST) ? op->st_vaddr : op->ld_vaddr;
		uint64_t tvpn = ac_addr / KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
		uint64_t tpfn;
		if(!tlb_access(dtlb,tvpn,threadid,&tpfn))
			effective_latency = 2 * (dcache_latency - 1);
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
			if(!returning_on_mshr_full && !pteop_returning_on_mshr_full)
			{
				op->wait_till_cycle = cycle_count + effective_latency;	// new op - set a deadline
				op->is_waiting = true;					// order it to wait
				return;
			}
		}
	}
		
	uint64_t tvpn,tpfn,tpteaddr,tindex,tphysical_addr;

	if(pteop_returning_on_mshr_full)
	{
		tvpn = ac_addr / KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
		tindex = ac_addr % KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
	 	tpteaddr = vmem_get_pteaddr(tvpn,threadid);
		bool b = tlb_access(dtlb,tvpn,threadid,&tpfn);
		if(b) 
		{
			dtlb_hit_count++;
			dtlb_hit_count_thread[threadid]++;
		}
		else
		{
			dtlb_miss_count++;
			dtlb_miss_count_thread[threadid]++;
		}
		if(b)
		{
			/* Got address translation in TLB */
			tphysical_addr = (tpfn * KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()) | tindex;
			ac_addr = tphysical_addr;
			if(op->mem_type == MEM_ST)
				op->st_vaddr = ac_addr;
			else if(op->mem_type == MEM_LD)
				op->ld_vaddr = ac_addr;
			/* Remove the flag that indicates that insert_mshr failed */
			pteop_returning_on_mshr_full = false;
			/* Unblock the stall as it is not applicable any more */
			EX_latch->pipeline_stall_enabled = false;
			goto dcache_access_for_data;
		}
		else if(dcache_access(tpteaddr))
		{
			/* we got a cache hit on address translation */
			dcache_hit_count++;
			dcache_hit_count_thread[threadid]++;
			cache_update(data_cache,tpteaddr);
			/* we got the pfn from dcache.
			   Here, we get it using vpn_to_pfn translation function */
			tpfn = vmem_vpn_to_pfn(tvpn,threadid);
			tphysical_addr = (tpfn * KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()) | tindex;
			tlb_install(dtlb,tvpn,threadid,tpfn);
			/* change the address accessed in cache as well as change mem request address
			   in case there is a cache miss */
			ac_addr = tphysical_addr;
			if(op->mem_type == MEM_ST)
				op->st_vaddr = ac_addr;
			else if(op->mem_type == MEM_LD)
				op->ld_vaddr = ac_addr;
			/* Remove the flag that indicates that insert_mshr failed */
			pteop_returning_on_mshr_full = false;
			/* Unblock the stall as it is not applicable any more */
			EX_latch->pipeline_stall_enabled = false;
			goto dcache_access_for_data; // add if needed
		}
		else
		{
			/* We got a cache miss for the address translation.
			   We will need to look up Page Table Entry in dram */
			dcache_miss_count++;
			dcache_miss_count_thread[threadid]++;
			if(op->opcode == OP_LD)
				dcache_read_misses++;
			else
				dcache_write_misses++;
			/* We need to stall the pipeline as we want to make dram
			   access for PTE */
			EX_latch->pipeline_stall_enabled = true;
			/* We also need a dummy load op that will go into memory */
			Op * pteop = get_free_op();
			pteop->is_pteop = true;
			pteop->mem_type = MEM_LD;
			pteop->ld_vaddr = tpteaddr;
			pteop->mem_read_size = VMEM_PTE_SIZE;
			pteop->vpn = tvpn;
			if(main_memory->store_load_forwarding(pteop))
			{
				tpfn = vmem_vpn_to_pfn(tvpn,threadid);
				tphysical_addr = (tpfn * KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()) | tindex;
				tlb_install(dtlb,tvpn,threadid,tpfn);	
				cache_update(data_cache,tpteaddr);
				ac_addr = tphysical_addr;
				if(op->mem_type == MEM_ST)
					op->st_vaddr = ac_addr;
				else if(op->mem_type == MEM_LD)
					op->ld_vaddr = ac_addr;
				/* Remove the flag that indicates that insert_mshr failed */
				pteop_returning_on_mshr_full = false;
				/* Unblock the stall as it is not applicable any more */
				EX_latch->pipeline_stall_enabled = false;
				goto dcache_access_for_data; // add if needed
			}
			else if(main_memory->check_piggyback(pteop))
			{
				pteop_returning_on_mshr_full = false;
				EX_latch->pipeline_stall_enabled = true;
				return;
			}
			else if(main_memory->insert_mshr(pteop))
			{
				pteop_returning_on_mshr_full = false;
				EX_latch->pipeline_stall_enabled = true;
				return;
			}
			else
			{
				dtlb_miss_count--;
				dtlb_miss_count_thread[threadid]--;
				pteop_returning_on_mshr_full = true;
				EX_latch->pipeline_stall_enabled = true;
				free_op(pteop);
				return;
			}

		}
	}

	/* If we came back here due to mshr was full during first attempt of getting translation,
	   and if the translation is not yet available,
	   then we should not execute further part of the function */
	if(EX_latch->pipeline_stall_enabled)
		return;

	/* Op has completed its wait for dcache latency amount of cycles */
	if(returning_on_mshr_full)
	{
		UINT64 ac_addr = (op->mem_type == MEM_ST) ? op->st_vaddr : op->ld_vaddr;
		if(dcache_access(ac_addr))
		{
			/* we got a cache hit - pass op to WB stage*/
			dcache_hit_count++;
			dcache_hit_count_thread[threadid]++;
			cache_update(data_cache,ac_addr);
			MEM_latch->op = op;		// deprecated
			MEM_latch->oplist.push_back(op);
			MEM_latch->op_valid = true;
			EX_latch->op_valid = false;	/* will help in handling Case #2 hit under miss */
			returning_on_mshr_full = false;		// XXX : check validity - added in lab 3
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

	ac_addr = (op->mem_type == MEM_ST) ? op->st_vaddr : op->ld_vaddr;
	UINT64 physical_addr;
	uint64_t vpn,pfn,pteaddr,index;
	/* If we are using the virtual memory -> then access TLB.
	   This is a real TLB access where we get data and decide whether to access dcache.
	   Cache is indexed using physical addresses now */
	if(vmem_enabled && (op->mem_type > NOT_MEM) && (op->mem_type < NUM_MEM_TYPES) && !pteop_returning_on_mshr_full)
	{
		vpn = ac_addr / KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
		index = ac_addr % KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
		bool b = tlb_access(dtlb,vpn,threadid,&pfn);
		if(b)
		{
			dtlb_hit_count++;
			dtlb_hit_count_thread[threadid]++;
		}
		else
		{
			dtlb_miss_count++;
			dtlb_miss_count_thread[threadid]++;
		}
		if(b)
		{
			/* Got address translation in TLB */
			physical_addr = (pfn * KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()) | index;
			/* change the address accessed in cache as well as change mem request address
			   in case there is a cache miss */
			ac_addr = physical_addr;
			if(op->mem_type == MEM_ST)
				op->st_vaddr = ac_addr;
			else if(op->mem_type == MEM_LD)
				op->ld_vaddr = ac_addr;
			EX_latch->pipeline_stall_enabled = false;
			/* No need to do anything else. Access dcache to get actual data 
			   GOTO dcache_access_for_data; // add if needed
			*/
		}
		else
		{
			/* We have a miss in TLB. Must access cache / page table in memory
			   to get the address translation */
			/* We have to get the PTE address first */
			vpn = ac_addr / KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
			index = ac_addr % KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
			pteaddr = vmem_get_pteaddr(vpn,threadid);
			if(dcache_access(pteaddr))
			{
				/* we got a cache hit on address translation */
				dcache_hit_count++;
				dcache_hit_count_thread[threadid]++;
				cache_update(data_cache,pteaddr);
				/* we got the pfn from dcache.
				   Here, we get it using vpn_to_pfn translation function */
				pfn = vmem_vpn_to_pfn(vpn,threadid);
				physical_addr = (pfn * KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()) | index;
				tlb_install(dtlb,vpn,threadid,pfn);
				/* change the address accessed in cache as well as change mem request address
				   in case there is a cache miss */
				ac_addr = physical_addr;
				if(op->mem_type == MEM_ST)
					op->st_vaddr = ac_addr;
				else if(op->mem_type == MEM_LD)
					op->ld_vaddr = ac_addr;
				EX_latch->pipeline_stall_enabled = false;
				/* No need to do anything else. Access dcache to get actual data 
				   GOTO dcache_access_for_data; // add if needed
				*/
			}
			else
			{
				/* We got a cache miss for the address translation.
				   We will need to look up Page Table Entry in dram */
				dcache_miss_count++;
				dcache_miss_count_thread[threadid]++;
				if(op->opcode == OP_LD)
					dcache_read_misses++;
				else
					dcache_write_misses++;

				/* We also need a dummy load op that will go into memory */
				Op * pteop = get_free_op();
				pteop->is_pteop = true;
				pteop->mem_type = MEM_LD;
				pteop->ld_vaddr = pteaddr;
				pteop->mem_read_size = VMEM_PTE_SIZE;
				pteop->vpn = vpn;
				if(main_memory->store_load_forwarding(pteop))
				{
					/* we got MSHR hit on store load forwarding */
					pfn = vmem_vpn_to_pfn(vpn,threadid);
					physical_addr = (pfn * KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()) | index;
					cache_update(data_cache,pteaddr);
					tlb_install(dtlb,vpn,threadid,pfn);
					ac_addr = physical_addr;
					if(op->mem_type == MEM_ST)
						op->st_vaddr = ac_addr;
					else if(op->mem_type == MEM_LD)
						op->ld_vaddr = ac_addr;
					EX_latch->pipeline_stall_enabled = false;
					/* No need to do anything else. Access dcache to get actual data 
					   GOTO dcache_access_for_data; // add if needed
					*/

				}
				else if(main_memory->check_piggyback(pteop))
				{
					/* We need to stall the pipeline as we want to make dram
					   access for PTE */
					EX_latch->pipeline_stall_enabled = true;
					return;
				}
				else if(main_memory->insert_mshr(pteop))
				{
					/* We need to stall the pipeline as we want to make dram
					   access for PTE */
					EX_latch->pipeline_stall_enabled = true;
					return;
				}
				else
				{
					EX_latch->pipeline_stall_enabled = true;
					pteop_returning_on_mshr_full = true;
					free_op(pteop);
					return;
				}

			}

		}
	}

	/* Check if we get the hit */

	dcache_access_for_data :

	if(dcache_access(ac_addr))
	{
		/* we got a cache hit - pass op to WB stage*/
		dcache_hit_count++;
		dcache_hit_count_thread[threadid]++;
		cache_update(data_cache,ac_addr);
		MEM_latch->op = op;		// deprecated
		MEM_latch->oplist.push_back(op);
		MEM_latch->op_valid = true;
		EX_latch->op_valid = false;	/* will help in handling Case #2 hit under miss */
		return;
	}
	
	/* We got a cache miss */
	dcache_miss_count++;
	dcache_miss_count_thread[threadid]++;
	if(op->opcode == OP_LD)
		dcache_read_misses++;
	else
		dcache_write_misses++;

	/* Store Load Forwarding */
	if(main_memory->store_load_forwarding(op))
	{
		/* We got MSHR Hit in store load forwarding */
		store_load_forwarding_count++;
		store_load_forwarding_count_thread[threadid]++;
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
	if(ID_latch->op_valid)
	{
		/* if the EX is still busy,
		   then, do not perform any work. */
		return;
	}

	Op* op;
	static UINT64 sched_counter = 0;
	int chosen_thread = -1;
	bool inst_found = false;
	/* Find instruction from one of the threads */
	for(int i = 0 ; i < thread_count ; i++)
	{
		chosen_thread = sched_counter++ % thread_count;
		if(FE_latch->op_valid_thread[chosen_thread])
		{
			op = FE_latch->op_queue[chosen_thread];
			inst_found = true;
			break;
		}
	}

	if(!inst_found)
		return;

	sched_access++;

	/* Now, we'll check for data dependency - Data Hazard MUST be checked before control hazard.*/
	int r;
	for(r = 0 ; r < 2 ; r++)
	{
		if((op->src[r] >= 0) && (op->src[r] < NUM_REG))
		{
			if(register_file[chosen_thread][(op->src[r])].busy)
			{
				/* Data Hazard detected. Do not perform any work */
				data_hazard_count++;
				data_hazard_count_thread[chosen_thread]++;
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

		FE_latch->pipeline_stall_enabled_thread[chosen_thread] = true;
		FE_latch->stall_enforcer_thread[chosen_thread] = op->inst_id;
		control_hazard_count++;
		control_hazard_count_thread[chosen_thread]++;
	}

	if(use_bpred && op->cf_type == CF_CBR)
	{
		control_hazard_count++;
		control_hazard_count_thread[chosen_thread]++;
	}
	/* Pass on the instruction to the EX stage now */

	if((op->dst >= 0) && (op->dst < NUM_REG))
	{
		/* Mark the destination register as busy */
		register_file[chosen_thread][op->dst].busy = true;
	}

	ID_latch->op = op;
	ID_latch->op_valid = true;
	FE_latch->op_valid_thread[chosen_thread] = false;
}


void FE_stage()
{

	if(stop_fetching)
		return;

	if(have_to_send_EOS)
	{
		if(sendEOS())
		{
			stop_fetching = true;
			have_to_send_EOS = false;
		}
		return;
	}

	#if 0
	if(FE_latch->op_valid || FE_latch->pipeline_stall_enabled)
	{
		/* Data inside the latch is valid and next stage is still using it.
		Or ID stage has enabled pipeline stalling because of a branch instruction.
		Do not fetch */
		return;
	}
	/* This condition is rewritten for multithreading. See following statements.
	~(a OR b) ===>  ~a AND ~b */
	#endif

	static UINT64 fetch_arbiter = 0;
	int stream_id = -1;
	Op *op;
	bool op_exists = false, stalled[HW_MAX_THREAD];

	for(int i = 0 ; i < HW_MAX_THREAD ; i++)
		stalled[i] = true;

	/* Find next available empty queue slot to fill */
	for(int i = 0 ; i < thread_count ; i++)
	{
		stream_id = fetch_arbiter++ % thread_count;
		if(!FE_latch->op_valid_thread[stream_id] && !FE_latch->pipeline_stall_enabled_thread[stream_id])
		{
			stalled[stream_id] = false;
			op = get_free_op();
			op_exists = get_op(op, stream_id);
			if(op_exists)
				break;
			else
				free_op(op);
		}
	}
	
	if(!op_exists)
	{
		/* No op fetched - this could be due to following : 
		   1. all threads were stalled
		   2. some threads were stalled and others have run out of instructions
		   3. no instructions available to fetch
		*/

		// checking case 1
		bool all_stalled = true;
		for(int i = 0 ; i < thread_count ; i++)
		{
			if(!stalled[i])
				all_stalled = false;
		}
		if(all_stalled)
			return;

		// checking case 2 & 3
		bool eois = true;	// end of instruction streams
		for(int i = 0 ; i < thread_count ; i++)
		{
			if(!end_of_stream[i])
				eois = false;
		}
		if(!eois)
			return;
		else
		{
			/* Must take actions for initiating simulator shut down */
			// first it should be seen if there is some space in queue.
			// if no, then try to send in next cycle
			if(sendEOS())
				stop_fetching = true;
			else
				have_to_send_EOS = true;
			return;
		}
	}
	
	int c = op->opcode;
	if((c == OP_FCVT) || (c == OP_FADD) || (c == OP_FMUL) || (c == OP_FDIV) || (c == OP_FCMP) ||
		(c == OP_FBIT) || (c == OP_FCMO) || op->is_fp)
	{
		fp_ops++;
		fp_reg_reads += op->num_src;
	}
	
	if((c == OP_IADD) || (c == OP_IMUL) || (c == OP_ICMP) || (c == OP_IDIV))
	{
		int_ops++;
		int_reg_reads += op->num_src;
	}
	if(c == OP_LD)
		ld_ops++;
	if(c == OP_ST)
		st_ops++;
	if((op->cf_type > NOT_CF) && (op->cf_type < NUM_CF_TYPES))
		br_ops++;
	
	if((c == OP_IMUL) || (c == OP_FMUL) || (c == OP_MM))
		mul_ops++;


	/* If the op is an branch other than conditional branch, assume that it is predicted correctly,
	if the branch predictor is used */
	//  if(use_bpred && (op->cf_type >= CF_BR) && (op->cf_type < NUM_CF_TYPES) && (op->cf_type != CF_CBR))
	//	  bpred_okpred_count++;
	/* Above 2 lines commented because its not the way solution is implemented */

	/* If we are using branch predictor and type of opcode is conditional branch,
	get a prediction and update GHR and PHT */
	if(use_bpred && (op->cf_type == CF_CBR))
	{
		int pred_val;
		int prediction = bpred_access(branchpred, op->instruction_addr, op->thread_id, &pred_val);
		if(prediction == op->actually_taken)
		{
			bpred_okpred_count++;
			bpred_okpred_count_thread[op->thread_id]++;
		}
		else
		{
			bpred_mispred_count++;
			bpred_mispred_count_thread[op->thread_id]++;
			/* stall the pipeline if we mispredict */
			FE_latch->pipeline_stall_enabled_thread[op->thread_id] = true;;
			FE_latch->stall_enforcer_thread[op->thread_id] = op->inst_id;
		}
		bpred_update(branchpred,op->instruction_addr,prediction,op->actually_taken, op->thread_id,pred_val);
	}

	/* hwsim : get the instruction and pass to ID phase */
	# if 0
	/* Deprecated  after adding MT support */
	FE_latch->op = op;				/* pass the op to ID stage */
	FE_latch->op_valid = true;			/* Mark it as valid */
	#endif

	FE_latch->op_queue[op->thread_id] = op;
	FE_latch->op_valid_thread[op->thread_id] = true;

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

  MEM_latch->stall_enforcer = 0;
  EX_latch->stall_enforcer = 0;
  ID_latch->stall_enforcer = 0;
  FE_latch->stall_enforcer = 0;


  MEM_latch->oplist.clear();
  EX_latch->oplist.clear();
  ID_latch->oplist.clear();
  FE_latch->oplist.clear();

  for(int i = 0 ; i < HW_MAX_THREAD ; i++)
  {
	MEM_latch->op_queue[i] = NULL;
	EX_latch->op_queue[i] = NULL;
	ID_latch->op_queue[i] = NULL;
	FE_latch->op_queue[i] = NULL;

	MEM_latch->op_valid_thread[i] = false;
	EX_latch->op_valid_thread[i] = false;
	ID_latch->op_valid_thread[i] = false;
	FE_latch->op_valid_thread[i] = false;

	MEM_latch->pipeline_stall_enabled_thread[i] = false;
	EX_latch->pipeline_stall_enabled_thread[i] = false;
	ID_latch->pipeline_stall_enabled_thread[i] = false;
	FE_latch->pipeline_stall_enabled_thread[i] = false;

	MEM_latch->stall_enforcer_thread[i] = 0;
	EX_latch->stall_enforcer_thread[i] = 0;
	ID_latch->stall_enforcer_thread[i] = 0;
	FE_latch->stall_enforcer_thread[i] = 0;

  }

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
  	dcache_reads++;
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
	/* If the op broadcasted is a dummy op,
	   then do not pass it to write back stage to retire.
	   Install it in TLB.
	 */
	if(op->is_pteop)
	{
		uint64_t pfn = vmem_vpn_to_pfn(op->vpn, op->thread_id);
		tlb_install(dtlb,op->vpn,op->thread_id,pfn);
		free_op(op);
		pteop_returning_on_mshr_full = false;
		EX_latch->pipeline_stall_enabled = false;
		pteaddr_broadcasted = true;
		return;
	}
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

/* send end of stream instruction in the pipeline */
bool sendEOS()
{
	int i;
	for(i = 0 ; i < thread_count ; i++)
	{
		if(!FE_latch->op_valid_thread[i] && !FE_latch->pipeline_stall_enabled_thread[i])
			break;
	}

	if(i == thread_count)
	{
		/* all slots are full */
		return false;
	}

	/* can put a last_instruction in stream 'i' */
	Op * op = get_free_op();
	op->is_last_instruction = true;	
	op->thread_id = i;
	
	FE_latch->op_queue[i] = op;
	FE_latch->op_valid_thread[i] = true;
	return true;
}



void generate_mcpat_xml()
{
		char filename[100];
		strcpy(filename,KNOB(KNOB_OUTPUT_FILE)->getValue().c_str());
		strcat(filename,".xml");
		FILE * fp = fopen(filename,"w\n");
		fprintf(fp,"<?xml version=\"1.0\" ?>\n");
		fprintf(fp,"<component id=\"root\" name=\"root\">\n");
		fprintf(fp,"<component id=\"system\" name=\"system\">\n");
		fprintf(fp,"<!--McPAT will skip the components if number is set to 0 -->\n");
		fprintf(fp,"<param name=\"number_of_cores\" value=\"1\"/>\n");
		fprintf(fp,"<param name=\"number_of_L1Directories\" value=\"1\"/>\n");
		fprintf(fp,"<param name=\"number_of_L2Directories\" value=\"0\"/>\n");
		fprintf(fp,"<param name=\"number_of_L2s\" value=\"4\"/>\n"); fprintf(fp,"<!-- This number means how many L2 clusters in each cluster there can be multiple banks/ports -->\n");
		fprintf(fp,"<param name=\"number_of_L3s\" value=\"0\"/>\n"); fprintf(fp,"<!-- This number means how many L3 clusters -->\n");
		fprintf(fp,"<param name=\"number_of_NoCs\" value=\"1\"/>\n");
		fprintf(fp,"<param name=\"homogeneous_cores\" value=\"1\"/>\n");fprintf(fp,"<!--1 means homo -->\n");
		fprintf(fp,"<param name=\"homogeneous_L2s\" value=\"1\"/>\n");
		fprintf(fp,"<param name=\"homogeneous_L1Directorys\" value=\"1\"/>\n");
		fprintf(fp,"<param name=\"homogeneous_L2Directorys\" value=\"1\"/>\n");
		fprintf(fp,"<param name=\"homogeneous_L3s\" value=\"1\"/>\n");
		fprintf(fp,"<param name=\"homogeneous_ccs\" value=\"1\"/>\n");fprintf(fp,"<!--cache coherece hardware -->\n");
		fprintf(fp,"<param name=\"homogeneous_NoCs\" value=\"1\"/>\n");
		fprintf(fp,"<param name=\"core_tech_node\" value=\"90\"/>\n");fprintf(fp,"<!-- nm -->\n");
		fprintf(fp,"<param name=\"target_core_clockrate\" value=\"%ld\"/>\n",KNOB(KNOB_CLOCK_FREQ)->getValue());fprintf(fp,"<!--MHz -->\n");
		fprintf(fp,"<param name=\"temperature\" value=\"380\"/>\n"); fprintf(fp,"<!-- Kelvin -->\n");
		fprintf(fp,"<param name=\"number_cache_levels\" value=\"2\"/>\n");
		fprintf(fp,"<param name=\"interconnect_projection_type\" value=\"0\"/>\n");fprintf(fp,"<!--0: agressive wire technology; 1: conservative wire technology -->\n");
		fprintf(fp,"<param name=\"device_type\" value=\"0\"/>\n");fprintf(fp,"<!--0: HP(High Performance Type); 1: LSTP(Low standby power) 2: LOP (Low Operating Power)  -->\n");
		fprintf(fp,"<param name=\"longer_channel_device\" value=\"1\"/>\n");fprintf(fp,"<!-- 0 no use; 1 use when possible -->\n");
		fprintf(fp,"<param name=\"machine_bits\" value=\"64\"/>\n");
		fprintf(fp,"<param name=\"virtual_address_width\" value=\"64\"/>\n");
		fprintf(fp,"<param name=\"physical_address_width\" value=\"52\"/>\n");
		fprintf(fp,"<param name=\"virtual_memory_page_size\" value=\"%ld\"/>\n",KNOB(KNOB_VMEM_PAGE_SIZE)->getValue());
		fprintf(fp,"<stat name=\"total_cycles\" value=\"%ld\"/>\n",cycle_count);
		fprintf(fp,"<stat name=\"idle_cycles\" value=\"0\"/>\n");
		fprintf(fp,"<stat name=\"busy_cycles\"  value=\"%ld\"/>\n",cycle_count);
			fprintf(fp,"<!--This page size(B) is complete different from the page size in Main memo secction. this page size is the size of virtual memory from OS/Archi perspective; the page size in Main memo secction is the actuall physical line in a DRAM bank  -->\n");
		fprintf(fp,"<!-- *********************** cores ******************* -->\n");
		fprintf(fp,"<component id=\"system.core0\" name=\"core0\">\n");
			fprintf(fp,"<!-- Core property -->\n");
			fprintf(fp,"<param name=\"clock_rate\" value=\"%ld\"/>\n",KNOB(KNOB_CLOCK_FREQ)->getValue());
			fprintf(fp,"<param name=\"instruction_length\" value=\"32\"/>\n");
			fprintf(fp,"<param name=\"opcode_width\" value=\"9\"/>\n");
			fprintf(fp,"<!-- address width determins the tag_width in Cache, LSQ and buffers in cache controller"
			"default value is machine_bits, if not set -->\n"); 
			fprintf(fp,"<param name=\"machine_type\" value=\"1\"/>\n");fprintf(fp,"<!-- 1 inorder; 0 OOO-->\n");
			fprintf(fp,"<!-- inorder/OoO -->\n");
			fprintf(fp,"<param name=\"number_hardware_threads\" value=\"%ld\"/>\n",KNOB(KNOB_RUN_THREAD_NUM)->getValue());
			fprintf(fp,"<!-- number_instruction_fetch_ports(icache ports) is always 1 in single-thread processor,"
			"it only may be more than one in SMT processors. BTB ports always equals to fetch ports since "
			"branch information in consective branch instructions in the same fetch group can be read out from BTB once.-->\n"); 
			fprintf(fp,"<param name=\"fetch_width\" value=\"1\"/>\n");
			fprintf(fp,"<!-- fetch_width determins the size of cachelines of L1 cache block -->\n");
			fprintf(fp,"<param name=\"number_instruction_fetch_ports\" value=\"1\"/>\n");
			fprintf(fp,"<param name=\"decode_width\" value=\"1\"/>\n");
			fprintf(fp,"<!-- decode_width determins the number of ports of the "
			"renaming table (both RAM and CAM) scheme -->\n");
			fprintf(fp,"<param name=\"issue_width\" value=\"1\"/>\n");
			fprintf(fp,"<!-- issue_width determins the number of ports of Issue window and other logic "
			"as in the complexity effective proccessors paper; issue_width==dispatch_width -->\n");
			fprintf(fp,"<param name=\"commit_width\" value=\"1\"/>\n");
			fprintf(fp,"<!-- commit_width determins the number of ports of register files -->\n");
			fprintf(fp,"<param name=\"fp_issue_width\" value=\"1\"/>\n");
			fprintf(fp,"<param name=\"prediction_width\" value=\"0\"/>\n"); 
			fprintf(fp,"<!-- number of branch instructions can be predicted simultannouesl-->\n");
			fprintf(fp,"<!-- Current version of McPAT does not distinguish int and floating point pipelines "
			"Theses parameters are reserved for future use.-->\n"); 
			fprintf(fp,"<param name=\"pipelines_per_core\" value=\"1,1\"/>\n");
			fprintf(fp,"<!--integer_pipeline and floating_pipelines, if the floating_pipelines is 0, then the pipeline is shared-->\n");
			fprintf(fp,"<param name=\"pipeline_depth\" value=\"5,5\"/>\n");
			fprintf(fp,"<!-- pipeline depth of int and fp, if pipeline is shared, the second number is the average cycles of fp ops -->\n");
			fprintf(fp,"<!-- issue and exe unit-->\n");
			fprintf(fp,"<param name=\"ALU_per_core\" value=\"1\"/>\n");
			fprintf(fp,"<!-- contains an adder, a shifter, and a logical unit -->\n");
			fprintf(fp,"<param name=\"MUL_per_core\" value=\"1\"/>\n");
			fprintf(fp,"<!-- For MUL and Div -->\n");
			fprintf(fp,"<param name=\"FPU_per_core\" value=\"0.125\"/>\n");		
			fprintf(fp,"<!-- buffer between IF and ID stage -->\n");
			fprintf(fp,"<param name=\"instruction_buffer_size\" value=\"16\"/>\n");
			fprintf(fp,"<!-- buffer between ID and sche/exe stage -->\n");
			fprintf(fp,"<param name=\"decoded_stream_buffer_size\" value=\"16\"/>\n");
			fprintf(fp,"<param name=\"instruction_window_scheme\" value=\"0\"/>\n");fprintf(fp,"<!-- 0 PHYREG based, 1 RSBASED-->\n");
			fprintf(fp,"<!-- McPAT support 2 types of OoO cores, RS based and physical reg based-->\n");
			fprintf(fp,"<param name=\"instruction_window_size\" value=\"16\"/>\n");
			fprintf(fp,"<param name=\"fp_instruction_window_size\" value=\"16\"/>\n");
			fprintf(fp,"<!-- the instruction issue Q as in Alpha 21264; The RS as in Intel P6 -->\n");
			fprintf(fp,"<param name=\"ROB_size\" value=\"80\"/>\n");
			fprintf(fp,"<!-- each in-flight instruction has an entry in ROB -->\n");
			fprintf(fp,"<!-- registers -->\n");
			fprintf(fp,"<param name=\"archi_Regs_IRF_size\" value=\"32\"/>\n");			
			fprintf(fp,"<param name=\"archi_Regs_FRF_size\" value=\"32\"/>\n");
			fprintf(fp,"<!--  if OoO processor, phy_reg number is needed for renaming logic, \
			renaming logic is for both integer and floating point insts.  -->\n");
			fprintf(fp,"<param name=\"phy_Regs_IRF_size\" value=\"80\"/>\n");
			fprintf(fp,"<param name=\"phy_Regs_FRF_size\" value=\"80\"/>\n");
			fprintf(fp,"<!-- rename logic -->\n");
			fprintf(fp,"<param name=\"rename_scheme\" value=\"0\"/>\n");
			fprintf(fp,"<!-- can be RAM based(0) or CAM based(1) rename scheme \
			RAM-based scheme will have free list, status table;\
			CAM-based scheme have the valid bit in the data field of the CAM \
			both RAM and CAM need RAM-based checkpoint table, checkpoint_depth=# of in_flight instructions;\
			Detailed RAT Implementation see TR -->\n");
			fprintf(fp,"<param name=\"register_windows_size\" value=\"8\"/>\n");
			fprintf(fp,"<!-- how many windows in the windowed register file, sun processors; \
			no register windowing is used when this number is 0 -->\n");
			fprintf(fp,"<!-- In OoO cores, loads and stores can be issued whether inorder(Pentium Pro) or (OoO)out-of-order(Alpha),\
			They will always try to exeute out-of-order though. -->\n");
			fprintf(fp,"<param name=\"LSU_order\" value=\"inorder\"/>\n");
			fprintf(fp,"<param name=\"store_buffer_size\" value=\"32\"/>\n");
			fprintf(fp,"<!-- By default, in-order cores do not have load buffers -->\n");
			fprintf(fp,"<param name=\"load_buffer_size\" value=\"32\"/>\n");	
			fprintf(fp,"<!-- number of ports refer to sustainable concurrent memory accesses -->\n"); 
			fprintf(fp,"<param name=\"memory_ports\" value=\"1\"/>\n");	
			fprintf(fp,"<!-- max_allowed_in_flight_memo_instructions determins the # of ports of load and store buffer\
			as well as the ports of Dcache which is connected to LSU -->\n");	
			fprintf(fp,"<!-- dual-pumped Dcache can be used to save the extra read/write ports -->\n");
			fprintf(fp,"<param name=\"RAS_size\" value=\"32\"/>\n");						
			fprintf(fp,"<!-- general stats, defines simulation periods;require total, idle, and busy cycles for senity check  -->\n");
			fprintf(fp,"<!-- please note: if target architecture is X86, then all the instrucions refer to (fused) micro-ops -->\n");
			fprintf(fp,"<stat name=\"total_instructions\" value=\"%ld\"/>\n",retired_instruction);
			fprintf(fp,"<stat name=\"int_instructions\" value=\"%ld\"/>\n",int_ops);
			fprintf(fp,"<stat name=\"fp_instructions\" value=\"%ld\"/>\n",fp_ops);
			fprintf(fp,"<stat name=\"branch_instructions\" value=\"%ld\"/>\n",br_ops);
			fprintf(fp,"<stat name=\"branch_mispredictions\" value=\"%ld\"/>\n",bpred_mispred_count);
			fprintf(fp,"<stat name=\"load_instructions\" value=\"%ld\"/>\n",ld_ops);
			fprintf(fp,"<stat name=\"store_instructions\" value=\"%ld\"/>\n",st_ops);
			fprintf(fp,"<stat name=\"committed_instructions\" value=\"%d\"/>\n",retired_instruction);
			fprintf(fp,"<stat name=\"committed_int_instructions\" value=\"%ld\"/>\n",int_ops);
			fprintf(fp,"<stat name=\"committed_fp_instructions\" value=\"%ld\"/>\n",fp_ops);
			fprintf(fp,"<stat name=\"pipeline_duty_cycle\" value=\"0.6\"/>\n");fprintf(fp,"<!-- <=1, runtime_ipc/peak_ipc; averaged for all cores if homogenous -->\n");
			fprintf(fp,"<!-- the following cycle stats are used for heterogeneouse cores only, \
				please ignore them if homogeneouse cores -->\n");
			fprintf(fp,"<stat name=\"total_cycles\" value=\"%ld\"/>\n",cycle_count);
			fprintf(fp,"<stat name=\"idle_cycles\" value=\"0\"/>\n");
			fprintf(fp,"<stat name=\"busy_cycles\"  value=\"%ld\"/>\n",cycle_count);
			fprintf(fp,"<!-- instruction buffer stats -->\n");
			fprintf(fp,"<!-- ROB stats, both RS and Phy based OoOs have ROB \
			performance simulator should capture the difference on accesses, \
			otherwise, McPAT has to guess based on number of commited instructions. -->\n");
			fprintf(fp,"<stat name=\"ROB_reads\" value=\"%ld\"/>\n",sched_access);
			fprintf(fp,"<stat name=\"ROB_writes\" value=\"%ld\"/>\n",sched_access);
			fprintf(fp,"<!-- RAT accesses -->\n");
			fprintf(fp,"<stat name=\"rename_accesses\" value=\"%ld\"/>\n",sched_access);
			fprintf(fp,"<stat name=\"fp_rename_accesses\" value=\"%ld\"/>\n",fp_ops);
			fprintf(fp,"<!-- decode and rename stage use this, should be total ic - nop -->\n");
			fprintf(fp,"<!-- Inst window stats -->\n");
			fprintf(fp,"<stat name=\"inst_window_reads\" value=\"%d\"/>\n",int_ops);
			fprintf(fp,"<stat name=\"inst_window_writes\" value=\"%ld\"/>\n",int_ops);
			fprintf(fp,"<stat name=\"inst_window_wakeup_accesses\" value=\"%ld\"/>\n",sched_access);
			fprintf(fp,"<stat name=\"fp_inst_window_reads\" value=\"%ld\"/>\n",fp_ops);
			fprintf(fp,"<stat name=\"fp_inst_window_writes\" value=\"%ld\"/>\n",fp_ops);
			fprintf(fp,"<stat name=\"fp_inst_window_wakeup_accesses\" value=\"%ld\"/>\n",fp_ops);
			fprintf(fp,"<!--  RF accesses -->\n");
			fprintf(fp,"<stat name=\"int_regfile_reads\" value=\"%ld\"/>\n",int_reg_reads);
			fprintf(fp,"<stat name=\"float_regfile_reads\" value=\"%ld\"/>\n",fp_reg_reads);
			fprintf(fp,"<stat name=\"int_regfile_writes\" value=\"%ld\"/>\n",int_reg_writes);
			fprintf(fp,"<stat name=\"float_regfile_writes\" value=\"%ld\"/>\n",fp_reg_writes);
			fprintf(fp,"<!-- accesses to the working reg -->\n");
			fprintf(fp,"<stat name=\"function_calls\" value=\"0\"/>\n");
			fprintf(fp,"<stat name=\"context_switches\" value=\"0\"/>\n");
			fprintf(fp,"<!-- Number of Windowes switches (number of function calls and returns)-->\n");
			fprintf(fp,"<!-- Alu stats by default, the processor has one FPU that includes the divider and \
			 multiplier. The fpu accesses should include accesses to multiplier and divider  -->\n");
			fprintf(fp,"<stat name=\"ialu_accesses\" value=\"%ld\"/>\n",int_reg_reads);			
			fprintf(fp,"<stat name=\"fpu_accesses\" value=\"%ld\"/>\n",fp_reg_reads);
			fprintf(fp,"<stat name=\"mul_accesses\" value=\"%ld\"/>\n",mul_ops);
			fprintf(fp,"<stat name=\"cdb_alu_accesses\" value=\"1000000\"/>\n");
			fprintf(fp,"<stat name=\"cdb_mul_accesses\" value=\"0\"/>\n");
			fprintf(fp,"<stat name=\"cdb_fpu_accesses\" value=\"0\"/>\n");
			fprintf(fp,"<!-- multiple cycle accesses should be counted multiple times, \
			otherwise, McPAT can use internal counter for different floating point instructions \
			to get final accesses. But that needs detailed info for floating point inst mix -->\n");
			fprintf(fp,"<!--  currently the performance simulator should \
			make sure all the numbers are final numbers, \
			including the explicit read/write accesses, \
			and the implicite accesses such as replacements and etc.\
			Future versions of McPAT may be able to reason the implicite access\
			based on param and stats of last level cache\
			The same rule applies to all cache access stats too!  -->\n");
			fprintf(fp,"<!-- following is AF for max power computation. \
				Do not change them, unless you understand them-->\n");
			fprintf(fp,"<stat name=\"IFU_duty_cycle\" value=\"0.25\"/>\n");			
			fprintf(fp,"<stat name=\"LSU_duty_cycle\" value=\"0.25\"/>\n");
			fprintf(fp,"<stat name=\"MemManU_I_duty_cycle\" value=\"1\"/>\n");
			fprintf(fp,"<stat name=\"MemManU_D_duty_cycle\" value=\"0.25\"/>\n");
			fprintf(fp,"<stat name=\"ALU_duty_cycle\" value=\"0.9\"/>\n");
			fprintf(fp,"<stat name=\"MUL_duty_cycle\" value=\"0.5\"/>\n");
			fprintf(fp,"<stat name=\"FPU_duty_cycle\" value=\"0.4\"/>\n");
			fprintf(fp,"<stat name=\"ALU_cdb_duty_cycle\" value=\"0.9\"/>\n");
			fprintf(fp,"<stat name=\"MUL_cdb_duty_cycle\" value=\"0.5\"/>\n");
			fprintf(fp,"<stat name=\"FPU_cdb_duty_cycle\" value=\"0.4\"/>\n");
			fprintf(fp,"<component id=\"system.core0.predictor\" name=\"PBT\">\n");
				fprintf(fp,"<!-- branch predictor; tournament predictor see Alpha implementation -->\n");
				fprintf(fp,"<param name=\"local_predictor_size\" value=\"10,3\"/>\n");
				fprintf(fp,"<param name=\"local_predictor_entries\" value=\"1024\"/>\n");
				fprintf(fp,"<param name=\"global_predictor_entries\" value=\"%ld\"/>\n",pred_entries);
				fprintf(fp,"<param name=\"global_predictor_bits\" value=\"2\"/>\n");
				fprintf(fp,"<param name=\"chooser_predictor_entries\" value=\"%ld\"/>\n",hist_len);
				fprintf(fp,"<param name=\"chooser_predictor_bits\" value=\"2\"/>\n");
				fprintf(fp,"<!-- These parameters can be combined like below in next version\
				<param name=\"load_predictor\" value=\"10,3,1024\"/>\
				<param name=\"global_predictor\" value=\"4096,2\"/>"
				"<param name=\"predictor_chooser\" value=\"4096,2\"/> -->\n");
			fprintf(fp,"</component>\n");
			fprintf(fp,"<component id=\"system.core0.itlb\" name=\"itlb\">\n");
				fprintf(fp,"<param name=\"number_entries\" value=\"64\"/>\n");
				fprintf(fp,"<stat name=\"total_accesses\" value=\"%ld\"/>\n",retired_instruction);
				fprintf(fp,"<stat name=\"total_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"conflicts\" value=\"0\"/>\n");	
				fprintf(fp,"<!-- there is no write requests to itlb although writes happen to itlb after miss, \
				which is actually a replacement -->\n");
			fprintf(fp,"</component>\n");
			fprintf(fp,"<component id=\"system.core0.icache\" name=\"icache\">\n");
				fprintf(fp,"<!-- there is no write requests to itlb although writes happen to it after miss, \
				which is actually a replacement -->\n");
				fprintf(fp,"<param name=\"icache_config\" value=\"%ld,%ld,%ld,1,1,%ld,8,1\"/>\n",KNOB(KNOB_DCACHE_SIZE)->getValue() * 1024, KNOB(KNOB_BLOCK_SIZE)->getValue(), KNOB(KNOB_DCACHE_WAY)->getValue(), KNOB(KNOB_ICACHE_LATENCY)->getValue());
				fprintf(fp,"<!-- the parameters are capacity,block_width, associativity, bank, throughput w.r.t. core clock, latency w.r.t. core clock,output_width, cache policy -->\n");
				fprintf(fp,"<!-- cache_policy;//0 no write or write-though with non-write allocate;1 write-back with write-allocate -->\n");
				fprintf(fp,"<param name=\"buffer_sizes\" value=\"16, 16, 16,0\"/>\n");
				fprintf(fp,"<!-- cache controller buffer sizes: miss_buffer_size(MSHR),fill_buffer_size,prefetch_buffer_size,wb_buffer_size-->\n"); 
				fprintf(fp,"<stat name=\"read_accesses\" value=\"%ld\"/>\n",retired_instruction);
				fprintf(fp,"<stat name=\"read_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"conflicts\" value=\"0\"/>\n");				
			fprintf(fp,"</component>\n");
			fprintf(fp,"<component id=\"system.core0.dtlb\" name=\"dtlb\">\n");
				fprintf(fp,"<param name=\"number_entries\" value=\"64\"/>\n");
				fprintf(fp,"<stat name=\"total_accesses\" value=\"%ld\"/>\n",dcache_reads + dcache_writes);
				fprintf(fp,"<stat name=\"total_misses\" value=\"4\"/>\n");
				fprintf(fp,"<stat name=\"conflicts\" value=\"0\"/>\n");	
			fprintf(fp,"</component>\n");
			fprintf(fp,"<component id=\"system.core0.dcache\" name=\"dcache\">\n");
			        fprintf(fp,"<!-- all the buffer related are optional -->\n");
				fprintf(fp,"<param name=\"dcache_config\" value=\"%ld,%ld,%ld,1,1,%ld,16,0\"/>\n",KNOB(KNOB_DCACHE_SIZE)->getValue() * 1024, KNOB(KNOB_BLOCK_SIZE)->getValue(), KNOB(KNOB_DCACHE_WAY)->getValue(), KNOB(KNOB_DCACHE_LATENCY)->getValue());
				fprintf(fp,"<param name=\"buffer_sizes\" value=\"%ld, 16, 16, 16\"/>\n",KNOB(KNOB_MSHR_SIZE)->getValue());
				fprintf(fp,"<!-- cache controller buffer sizes: miss_buffer_size(MSHR),fill_buffer_size,prefetch_buffer_size,wb_buffer_size-->\n");	
				fprintf(fp,"<stat name=\"read_accesses\" value=\"%ld\"/>\n",dcache_reads);
				fprintf(fp,"<stat name=\"write_accesses\" value=\"%ld\"/>\n",dcache_writes);
				fprintf(fp,"<stat name=\"read_misses\" value=\"%ld\"/>\n",dcache_read_misses);
				fprintf(fp,"<stat name=\"write_misses\" value=\"$ld\"/>\n",dcache_write_misses);
				fprintf(fp,"<stat name=\"conflicts\" value=\"0\"/>\n");	
			fprintf(fp,"</component>\n");
			fprintf(fp,"<component id=\"system.core0.BTB\" name=\"BTB\">\n");
			        fprintf(fp,"<!-- all the buffer related are optional -->\n");
				fprintf(fp,"<param name=\"BTB_config\" value=\"8192,4,2,1, 1,3\"/>\n");
				fprintf(fp,"<!-- the parameters are capacity,block_width,associativity,bank, throughput w.r.t. core clock, latency w.r.t. core clock,-->\n");
			fprintf(fp,"</component>\n");
	fprintf(fp,"</component>\n");
		fprintf(fp,"<component id=\"system.L1Directory0\" name=\"L1Directory0\">\n");
				fprintf(fp,"<param name=\"Directory_type\" value=\"0\"/>\n");
			    fprintf(fp,"<!--0 cam based shadowed tag. 1 directory cache -->\n");	
				fprintf(fp,"<param name=\"Dir_config\" value=\"2048,1,0,1, 4, 4,8\"/>\n");
				fprintf(fp,"<!-- the parameters are capacity,block_width, associativity,bank, throughput w.r.t. core clock, latency w.r.t. core clock,-->\n");
			    fprintf(fp,"<param name=\"buffer_sizes\" value=\"8, 8, 8, 8\"/>\n");	
				fprintf(fp,"<!-- all the buffer related are optional -->\n");
			    fprintf(fp,"<param name=\"clockrate\" value=\"1200\"/>\n");
				fprintf(fp,"<param name=\"ports\" value=\"1,1,1\"/>\n");
				fprintf(fp,"<!-- number of r, w, and rw search ports -->\n");
				fprintf(fp,"<param name=\"device_type\" value=\"0\"/>\n");
				fprintf(fp,"<!-- altough there are multiple access types, \
				Performance simulator needs to cast them into reads or writes\
				e.g. the invalidates can be considered as writes -->\n");
				fprintf(fp,"<stat name=\"read_accesses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"write_accesses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"read_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"write_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"conflicts\" value=\"0\"/>\n");	
				fprintf(fp,"<stat name=\"duty_cycle\" value=\"0.45\"/>\n");	
		fprintf(fp,"</component>\n");
		fprintf(fp,"<component id=\"system.L2Directory0\" name=\"L2Directory0\">\n");
				fprintf(fp,"<param name=\"Directory_type\" value=\"1\"/>\n");
			    fprintf(fp,"<!--0 cam based shadowed tag. 1 directory cache -->\n");	
				fprintf(fp,"<param name=\"Dir_config\" value=\"0,16,16,1,2, 100\"/>\n");
				fprintf(fp,"<!-- the parameters are capacity,block_width, associativity,bank, throughput w.r.t. core clock, latency w.r.t. core clock,-->\n");
			    fprintf(fp,"<param name=\"buffer_sizes\" value=\"8, 8, 8, 8\"/>\n");	
				fprintf(fp,"<!-- all the buffer related are optional -->\n");
			    fprintf(fp,"<param name=\"clockrate\" value=\"1200\"/>\n");
				fprintf(fp,"<param name=\"ports\" value=\"1,1,1\"/>\n");
				fprintf(fp,"<!-- number of r, w, and rw search ports -->\n");
				fprintf(fp,"<param name=\"device_type\" value=\"0\"/>\n");
				fprintf(fp,"<!-- altough there are multiple access types, \
				Performance simulator needs to cast them into reads or writes\
				e.g. the invalidates can be considered as writes -->\n");
				fprintf(fp,"<stat name=\"read_accesses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"write_accesses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"read_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"write_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"conflicts\" value=\"0\"/>\n");
			    fprintf(fp,"<stat name=\"duty_cycle\" value=\"0.45\"/>\n");		
		fprintf(fp,"</component>\n");
		fprintf(fp,"<component id=\"system.L20\" name=\"L20\">\n");
			fprintf(fp,"<!-- all the buffer related are optional -->\n");
				fprintf(fp,"<param name=\"L2_config\" value=\"786432,64,16,1, 4,23, 64, 1\"/>\n");
			    fprintf(fp,"<!-- consider 4-way bank interleaving for Niagara 1 -->\n");
				fprintf(fp,"<!-- the parameters are capacity,block_width, associativity, bank, throughput w.r.t. core clock, latency w.r.t. core clock,output_width, cache policy -->\n");
				fprintf(fp,"<param name=\"buffer_sizes\" value=\"16, 16, 16, 16\"/>\n");
				fprintf(fp,"<!-- cache controller buffer sizes: miss_buffer_size(MSHR),fill_buffer_size,prefetch_buffer_size,wb_buffer_size-->\n");	
				fprintf(fp,"<param name=\"clockrate\" value=\"1200\"/>\n");
				fprintf(fp,"<param name=\"ports\" value=\"1,1,1\"/>\n");
				fprintf(fp,"<!-- number of r, w, and rw ports -->\n");
				fprintf(fp,"<param name=\"device_type\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"read_accesses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"write_accesses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"read_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"write_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"conflicts\" value=\"0\"/>\n");	
			    fprintf(fp,"<stat name=\"duty_cycle\" value=\"0.5\"/>\n");	
		fprintf(fp,"</component>\n");
		
fprintf(fp,"<!--**********************************************************************-->\n");
fprintf(fp,"<component id=\"system.L30\" name=\"L30\">\n");
				fprintf(fp,"<param name=\"L3_config\" value=\"1048576,64,16,1, 2,100, 64,1\"/>\n");
				fprintf(fp,"<!-- the parameters are capacity,block_width, associativity, bank, throughput w.r.t. core clock, latency w.r.t. core clock,output_width, cache policy -->\n");
				fprintf(fp,"<param name=\"clockrate\" value=\"3500\"/>\n");
				fprintf(fp,"<param name=\"ports\" value=\"1,1,1\"/>\n");
				fprintf(fp,"<!-- number of r, w, and rw ports -->\n");
				fprintf(fp,"<param name=\"device_type\" value=\"0\"/>\n");
				fprintf(fp,"<param name=\"buffer_sizes\" value=\"16, 16, 16, 16\"/>\n");
				fprintf(fp,"<!-- cache controller buffer sizes: miss_buffer_size(MSHR),fill_buffer_size,prefetch_buffer_size,wb_buffer_size-->\n");	
				fprintf(fp,"<stat name=\"read_accesses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"write_accesses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"read_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"write_misses\" value=\"0\"/>\n");
				fprintf(fp,"<stat name=\"conflicts\" value=\"0\"/>\n");	
	            fprintf(fp,"<stat name=\"duty_cycle\" value=\"0.35\"/>\n");	
		fprintf(fp,"</component>\n");
fprintf(fp,"<!--**********************************************************************-->\n");
		fprintf(fp,"<component id=\"system.NoC0\" name=\"noc0\">\n");
			fprintf(fp,"<param name=\"clockrate\" value=\"1200\"/>\n");
			fprintf(fp,"<param name=\"type\" value=\"1\"/>\n");
			fprintf(fp,"<!-- 1 NoC, O bus -->\n");
			fprintf(fp,"<param name=\"horizontal_nodes\" value=\"2\"/>\n");
			fprintf(fp,"<param name=\"vertical_nodes\" value=\"1\"/>\n");
			fprintf(fp,"<param name=\"has_global_link\" value=\"0\"/>\n");
			fprintf(fp,"<!-- 1 has global link, 0 does not have global link -->\n");
			fprintf(fp,"<param name=\"link_throughput\" value=\"1\"/>\n");fprintf(fp,"<!--w.r.t clock -->\n");
			fprintf(fp,"<param name=\"link_latency\" value=\"1\"/>\n");fprintf(fp,"<!--w.r.t clock -->\n");
			fprintf(fp,"<!-- througput >= latency -->\n");
			fprintf(fp,"<!-- Router architecture -->\n");
			fprintf(fp,"<param name=\"input_ports\" value=\"8\"/>\n");
			fprintf(fp,"<param name=\"output_ports\" value=\"5\"/>\n");
			fprintf(fp,"<param name=\"virtual_channel_per_port\" value=\"1\"/>\n");
			fprintf(fp,"<!-- input buffer; in classic routers only input ports need buffers -->\n");
			fprintf(fp,"<param name=\"flit_bits\" value=\"136\"/>\n");
			fprintf(fp,"<param name=\"input_buffer_entries_per_vc\" value=\"2\"/>\n");fprintf(fp,"<!--VCs within the same ports share input buffers whose size is propotional to the number of VCs-->\n");
			fprintf(fp,"<param name=\"chip_coverage\" value=\"1\"/>\n");
			fprintf(fp,"<!-- When multiple NOC present, one NOC will cover part of the whole chip. chip_coverage <=1 -->\n");
			fprintf(fp,"<stat name=\"total_accesses\" value=\"%ld\"/>\n",4 * (dram_row_buffer_hit_count+dram_row_buffer_miss_count));
			fprintf(fp,"<!-- This is the number of total accesses within the whole network not for each router -->\n");
			fprintf(fp,"<stat name=\"duty_cycle\" value=\"0.6\"/>\n");
		fprintf(fp,"</component>\n");
		
fprintf(fp,"<!--**********************************************************************-->\n");
		fprintf(fp,"<component id=\"system.mem\" name=\"mem\">\n");
			fprintf(fp,"<!-- Main memory property -->\n");
			fprintf(fp,"<param name=\"mem_tech_node\" value=\"32\"/>\n");
			fprintf(fp,"<param name=\"device_clock\" value=\"200\"/>\n");fprintf(fp,"<!--MHz, this is clock rate of the actual memory device, not the FSB -->\n");
			fprintf(fp,"<param name=\"peak_transfer_rate\" value=\"3200\"/>\n");fprintf(fp,"<!--MB/S-->\n");
			fprintf(fp,"<param name=\"internal_prefetch_of_DRAM_chip\" value=\"4\"/>\n");
			fprintf(fp,"<!-- 2 for DDR, 4 for DDR2, 8 for DDR3...-->\n");
			fprintf(fp,"<!-- the device clock, peak_transfer_rate, and the internal prefetch decide the DIMM property -->\n");
			fprintf(fp,"<!-- above numbers can be easily found from Wikipedia -->\n");
			fprintf(fp,"<param name=\"capacity_per_channel\" value=\"4096\"/>\n"); fprintf(fp,"<!-- MB -->\n");
			fprintf(fp,"<!-- capacity_per_Dram_chip=capacity_per_channel/number_of_dimms/number_ranks/Dram_chips_per_rank\
			Current McPAT assumes single DIMMs are used.-->\n"); 		
			fprintf(fp,"<param name=\"number_ranks\" value=\"2\"/>\n");
			fprintf(fp,"<param name=\"num_banks_of_DRAM_chip\" value=\"%ld\"/>\n",KNOB(KNOB_DRAM_BANK_NUM)->getValue());
			fprintf(fp,"<param name=\"Block_width_of_DRAM_chip\" value=\"%ld\"/>\n",KNOB(KNOB_BLOCK_SIZE)->getValue()); 
			fprintf(fp,"<!-- B -->\n");
			fprintf(fp,"<param name=\"output_width_of_DRAM_chip\" value=\"8\"/>\n");
			fprintf(fp,"<!--number of Dram_chips_per_rank=\" 72/output_width_of_DRAM_chip-->\n");
			fprintf(fp,"<!--number of Dram_chips_per_rank=\" 72/output_width_of_DRAM_chip-->\n");
			fprintf(fp,"<param name=\"page_size_of_DRAM_chip\" value=\"%ld\"/>\n",KNOB(KNOB_DRAM_PAGE_SIZE)->getValue()); 
			fprintf(fp,"<!-- 8 or 16 -->\n");
			fprintf(fp,"<param name=\"burstlength_of_DRAM_chip\" value=\"8\"/>\n");
			fprintf(fp,"<stat name=\"memory_accesses\" value=\"%ld\"/>\n",(dram_row_buffer_miss_count+dram_row_buffer_hit_count));
			fprintf(fp,"<stat name=\"memory_reads\" value=\"%ld\"/>\n",dram_row_buffer_hit_count+dram_row_buffer_miss_count);
			fprintf(fp,"<stat name=\"memory_writes\" value=\"%ld\"/>\n",dram_row_buffer_miss_count+dram_row_buffer_hit_count);		
		fprintf(fp,"</component>\n");
		fprintf(fp,"<component id=\"system.mc\" name=\"mc\">\n");
			fprintf(fp,"<!-- Memeory controllers are for DDR(2,3...) DIMMs -->\n");
			fprintf(fp,"<!-- current version of McPAT uses published values for base parameters of memory controller\
			improvments on MC will be added in later versions. -->\n");
			fprintf(fp,"<param name=\"type\" value=\"0\"/>\n"); fprintf(fp,"<!-- 1: low power; 0 high performance -->\n");
			fprintf(fp,"<param name=\"mc_clock\" value=\"200\"/>\n");fprintf(fp,"<!--DIMM IO bus clock rate MHz DDR2-400 for Niagara 1-->\n"); 
			fprintf(fp,"<param name=\"peak_transfer_rate\" value=\"3200\"/>\n");fprintf(fp,"<!--MB/S-->\n");
			fprintf(fp,"<param name=\"block_size\" value=\"%ld\"/>\n",KNOB(KNOB_BLOCK_SIZE)->getValue());fprintf(fp,"<!--B-->\n");
			fprintf(fp,"<param name=\"number_mcs\" value=\"1\"/>\n");
			fprintf(fp,"<!-- current McPAT only supports homogeneous memory controllers -->\n");
			fprintf(fp,"<param name=\"memory_channels_per_mc\" value=\"1\"/>\n");
			fprintf(fp,"<param name=\"number_ranks\" value=\"2\"/>\n");
			fprintf(fp,"<param name=\"withPHY\" value=\"0\"/>\n");
			fprintf(fp,"<!-- # of ranks of each channel-->\n");
			fprintf(fp,"<param name=\"req_window_size_per_channel\" value=\"32\"/>\n");
			fprintf(fp,"<param name=\"IO_buffer_size_per_channel\" value=\"32\"/>\n");
			fprintf(fp,"<param name=\"databus_width\" value=\"128\"/>\n");
			fprintf(fp,"<param name=\"addressbus_width\" value=\"51\"/>\n");
			fprintf(fp,"<!-- McPAT will add the control bus width to the addressbus width automatically -->\n");
			fprintf(fp,"<stat name=\"memory_accesses\" value=\"%ld\"/>\n",4 * (dram_row_buffer_hit_count+dram_row_buffer_miss_count));
			fprintf(fp,"<stat name=\"memory_reads\" value=\"%ld\"/>\n",2 * (dram_row_buffer_miss_count+dram_row_buffer_hit_count));
			fprintf(fp,"<stat name=\"memory_writes\" value=\"%ld\"/>\n",2 * (dram_row_buffer_hit_count+dram_row_buffer_miss_count));
			fprintf(fp,"<!-- McPAT does not track individual mc, instead, it takes the total accesses and calculate \
			the average power per MC or per channel. This is sufficent for most application. \
			Further trackdown can be easily added in later versions. -->\n");  			
		fprintf(fp,"</component>\n");
fprintf(fp,"<!--**********************************************************************-->\n");
		fprintf(fp,"<component id=\"system.niu\" name=\"niu\">\n");
			fprintf(fp,"<!-- On chip 10Gb Ethernet NIC, including XAUI Phy and MAC controller  -->\n");
			fprintf(fp,"<!-- For a minimum IP packet size of 84B at 10Gb/s, a new packet arrives every 67.2ns. \
				 the low bound of clock rate of a 10Gb MAC is 150Mhz -->\n");
			fprintf(fp,"<param name=\"type\" value=\"0\"/>\n"); fprintf(fp,"<!-- 1: low power; 0 high performance -->\n");
			fprintf(fp,"<param name=\"clockrate\" value=\"350\"/>\n");
			fprintf(fp,"<param name=\"number_units\" value=\"0\"/>\n"); fprintf(fp,"<!-- unlike PCIe and memory controllers, each Ethernet controller only have one port -->\n");
			fprintf(fp,"<stat name=\"duty_cycle\" value=\"1.0\"/>\n"); fprintf(fp,"<!-- achievable max load <= 1.0 -->\n");
			fprintf(fp,"<stat name=\"total_load_perc\" value=\"0.7\"/>\n"); fprintf(fp,"<!-- ratio of total achived load to total achivable bandwidth  -->\n");
			fprintf(fp,"<!-- McPAT does not track individual nic, instead, it takes the total accesses and calculate \
			the average power per nic or per channel. This is sufficent for most application. -->\n");  			
		fprintf(fp,"</component>\n");
fprintf(fp,"<!--**********************************************************************-->\n");
		fprintf(fp,"<component id=\"system.pcie\" name=\"pcie\">\n");
			fprintf(fp,"<!-- On chip PCIe controller, including Phy-->\n");
			fprintf(fp,"<!-- For a minimum PCIe packet size of 84B at 8Gb/s per lane (PCIe 3.0), a new packet arrives every 84ns. \
				 the low bound of clock rate of a PCIe per lane logic is 120Mhz -->\n");
			fprintf(fp,"<param name=\"type\" value=\"0\"/>\n"); fprintf(fp,"<!-- 1: low power; 0 high performance -->\n");
			fprintf(fp,"<param name=\"withPHY\" value=\"1\"/>\n");
			fprintf(fp,"<param name=\"clockrate\" value=\"350\"/>\n");
			fprintf(fp,"<param name=\"number_units\" value=\"0\"/>\n");
			fprintf(fp,"<param name=\"num_channels\" value=\"8\"/>\n"); fprintf(fp,"<!-- 2 ,4 ,8 ,16 ,32 -->\n");
			fprintf(fp,"<stat name=\"duty_cycle\" value=\"1.0\"/>\n"); fprintf(fp,"<!-- achievable max load <= 1.0 -->\n");
			fprintf(fp,"<stat name=\"total_load_perc\" value=\"0.7\"/>\n"); fprintf(fp,"<!-- Percentage of total achived load to total achivable bandwidth  -->\n");
			fprintf(fp,"<!-- McPAT does not track individual pcie controllers, instead, it takes the total accesses and calculate \
			the average power per pcie controller or per channel. This is sufficent for most application. -->\n");  			
		fprintf(fp,"</component>\n");
fprintf(fp,"<!--**********************************************************************-->\n");
		fprintf(fp,"<component id=\"system.flashc\" name=\"flashc\">\n");
		    fprintf(fp,"<param name=\"number_flashcs\" value=\"0\"/>\n");
			fprintf(fp,"<param name=\"type\" value=\"1\"/>\n"); fprintf(fp,"<!-- 1: low power; 0 high performance -->\n");
            fprintf(fp,"<param name=\"withPHY\" value=\"1\"/>\n");
			fprintf(fp,"<param name=\"peak_transfer_rate\" value=\"200\"/>\n");fprintf(fp,"<!--Per controller sustainable reak rate MB/S -->\n");
			fprintf(fp,"<stat name=\"duty_cycle\" value=\"1.0\"/>\n"); fprintf(fp,"<!-- achievable max load <= 1.0 -->\n");
			fprintf(fp,"<stat name=\"total_load_perc\" value=\"0.7\"/>\n"); fprintf(fp,"<!-- Percentage of total achived load to total achivable bandwidth  -->\n");
			fprintf(fp,"<!-- McPAT does not track individual flash controller, instead, it takes the total accesses and calculate \
			the average power per fc or per channel. This is sufficent for most application -->\n");  			
		fprintf(fp,"</component>\n");
fprintf(fp,"<!--**********************************************************************-->\n");

		fprintf(fp,"</component>\n");
fprintf(fp,"</component>\n");
fclose(fp);
}

void generate_power_counters()
{
	std::ofstream out("power_counters.txt");
	out<< "Floating point instructions : "<< fp_ops<< endl;
	out<< "Integer instructions : "<< int_ops << endl;
	out<< "Load INstructions" << ld_ops << endl;
	out<< "Store Instructions : "<< st_ops << endl;
	out<< "Branch Instructions : " << br_ops << endl;
	out<< "Integer register reads : " << int_reg_reads<< endl;
	out<< "Floating point register reads : " << fp_reg_reads<< endl;
	out<< "Integer Register writes : "<< int_reg_writes<< endl;
	out<< "Floating Point Register writes : "<< fp_reg_writes << endl;
	out<< "Multiply Instructions : " << mul_ops << endl;
	out<< "Dcache Reads : " << dcache_reads << endl;
	out<< "Dcahe Writes : " << dcache_writes<< endl;
	out<< "Num Sched Accesses : "<< sched_access << endl;
	out<< "Num Mem Accesses : "<< (dram_row_buffer_miss_count+dram_row_buffer_hit_count) << endl;
	out<< "Dcache Read misses : "<< dcache_read_misses << endl;
	out<< "Dcache Write Misses : "<< dcache_write_misses << endl;
	out.close();
}
