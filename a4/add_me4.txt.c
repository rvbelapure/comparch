// in all_knobs.h 

KnobTemplate< string >* KNOB_TRACE_FILE2;
KnobTemplate< string >* KNOB_TRACE_FILE3;
KnobTemplate< string >* KNOB_TRACE_FILE4;
KnobTemplate< unsigned >* KNOB_RUN_THREAD_NUM;

----------------------------------------------------------------------------------------------------------------
//in all_knobs.cpp 

// add the followings to all_knobs_c()
KNOB_TRACE_FILE2 = new KnobTemplate< string > ("trace_file2", "trace2.pzip");
KNOB_TRACE_FILE3 = new KnobTemplate< string > ("trace_file3", "trace3.pzip");
KNOB_TRACE_FILE4 = new KnobTemplate< string > ("trace_file4", "trace4.pzip");
KNOB_RUN_THREAD_NUM = new KnobTemplate< unsigned > ("run_thread_num", 3);

// add the followings to ~all_knobs_c()
delete KNOB_TRACE_FILE2;
delete KNOB_TRACE_FILE3;
delete KNOB_TRACE_FILE4;
delete KNOB_RUN_THREAD_NUM;

// add the followings to registerKnobs(KnobsContainer *container)
container->insertKnob( KNOB_TRACE_FILE2);
container->insertKnob( KNOB_TRACE_FILE3);
container->insertKnob( KNOB_TRACE_FILE4);
container->insertKnob( KNOB_RUN_THREAD_NUM);

// add the followings to ~display()
KNOB_TRACE_FILE2->display(cout); cout << endl;
KNOB_TRACE_FILE3->display(cout); cout << endl;
KNOB_TRACE_FILE4->display(cout); cout << endl;
KNOB_RUN_THREAD_NUM->display(cout); cout << endl;

----------------------------------------------------------------------------------------------------------------
// sim.h 

// ad thread_id into Op_struct. 

typedef struct Op_struct : public Trace_op{
  uint64_t inst_id; 
  Op_struct *op_pool_next; 
  uint32_t  op_pool_id; 
  bool valid; 
  int thread_id;  // new data structure 
  /* when you add new element, you must change the init_op function also */ 
} Op; 
----------------------------------------------------------------------------------------------------------------

// sim.cpp

#define HW_MAX_THREAD 4   // add this line 
gzFile g_stream[HW_MAX_THREAD];    // replace extern gzFile stream; 

// replace read_trace_file function with the following
void read_trace_file(void) {
  g_stream[0] = gzopen((KNOB(KNOB_TRACE_FILE)->getValue()).c_str(), "r");

  if ((KNOB(KNOB_RUN_THREAD_NUM)->getValue())<2) return;    /** NEW-LAB4 **/
   g_stream[1] = gzopen((KNOB(KNOB_TRACE_FILE2)->getValue()).c_str(), "r"); /** NEW-LAB4 **/
   if ((KNOB(KNOB_RUN_THREAD_NUM)->getValue())<3) return;  /** NEW-LAB4 **/
   g_stream[2] = gzopen((KNOB(KNOB_TRACE_FILE3)->getValue()).c_str(), "r"); /** NEW-LAB4 **/
   if ((KNOB(KNOB_RUN_THREAD_NUM)->getValue())<4) return;  /** NEW-LAB4 **/
   g_stream[3] = gzopen((KNOB(KNOB_TRACE_FILE4)->getValue()).c_str(), "r"); /** NEW-LAB4 **/
}

// add the followings
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

// replace get_op function in sim.cpp 
int get_op(Op *op)
{
  static UINT64 unique_count = 0; 
  static UINT64 fetch_arbiter; 
  
  Trace_op trace_op; 
  bool success = FALSE; 
  // read trace 
  // fill out op info 
  // return FALSE if the end of trace
  int read_size;
  int fetch_id = -1; 
  bool br_stall_fail = false; 
  // read trace 
  // fill out op info 
  // return FALSE if the end of trace 
  for (int jj = 0; jj < (KNOB(KNOB_RUN_THREAD_NUM)->getValue()); jj++) {
    fetch_id = (fetch_arbiter++%(KNOB(KNOB_RUN_THREAD_NUM)->getValue()));

    if (br_stall[fetch_id]) {
    br_stall_fail = true; 
    continue; 
    }

    read_size = gzread(g_stream[fetch_id], &trace_op, sizeof(Trace_op));
    success = read_size>0;
    if(read_size!=sizeof(Trace_op) && read_size>0) {
      printf( "ERROR!! gzread reads corrupted op! @cycle:%d\n", cycle_count);
      success = false;
    }

    /* copy trace structure to op */ 
    if (success) { 
	copy_trace_op(&trace_op, op); 

	op->inst_id  = unique_count++;
	op->valid    = TRUE; 
	op->thread_id = fetch_id; 
	    return success;  // get op so return 
    }
    
	// if not success and go to another trace. 
}
if (br_stall_fail) return -1; 
return success; 
}

/* you must replace this function in your source code */
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


// replace print_heartbeat function in sim.cpp 
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
