#define MAX_TR_REG 32
#define MAX_TR_OPCODE_NAME 24
const char *g_tr_reg_names[MAX_TR_REG] = {
    "r0", 
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15",
    "r16",
    "r17",
    "r18",
    "r19",
    "r20",
    "r21",
    "r22",
    "r23",
    "r24",
    "r25",
    "r26",
    "r27",
    "r28",
    "r29",
    "r30",
    "r31",
};

const char* g_tr_opcode_names[MAX_TR_OPCODE_NAME] = {
  "INV",       // invalid opcode            0 
  "NOP",       // is a decoded nop
  "CF",        // change of flow           
  "CMOV",      // conditional move         
  "LDA",       // load address             
  "LD",        // load operation 
  "ST",        // store operation 
  "IADD",      // integer add              
  "IMUL",      // integer multiply         
  "ICMP",      // integer compare          
  "IDIV",      // integer divide   // 10 
  "LOGIC",     // logical                  
  "SHIFT",     // shift                    
  "BYTE",      // byte manipulation        
  "MM",        // multimedia instructions  
  "FMEM",      // fp memory instruction     // 15 
  "FCF",
  "FCVT",      // floating point convert   
  "FADD",      // floating point add       
  "FMUL",      // floating point multiply  
  "FDIV",      // floating point divide     // 20 
  "FCMP",      // floating point compare   
  "FBIT",      // floating point bit       
  "FCMO",      // floating point cond move // 23 
  
};

const char* g_tr_cf_names[10] = {
  "NOT_CF",			// not a control flow instruction
  "CF_BR",			// an unconditional branch
  "CF_CBR",			// a conditional branch
  "CF_CALL",			// a call
  "CF_IBR",			// an indirect branch
  "CF_ICALL",			// an indirect call
  "CF_ICO",			// an indirect jump to co-routine
  "CF_RET",			// a return
  "CF_SYS",
  "CF_ICBR"
};

const char *g_optype_names[24] = {
  "OP_INV",			// invalid opcode
  "OP_SPEC",			// something weird (rpcc)
  "OP_NOP",			// is a decoded nop
  "OP_CF",			// change of flow
  "OP_CMOV",			// conditional move
  "OP_LDA",                     // load address
  "OP_IMEM",			// int memory instruction
  "OP_IADD",			// integer add
  "OP_IMUL",			// integer multiply
  "OP_ICMP",			// integer compare
  "OP_LOGIC",			// logical
  "OP_SHIFT",			// shift
  "OP_BYTE",			// byte manipulation
  "OP_MM",			// multimedia instructions
  "OP_FMEM",			// fp memory instruction
  "OP_FCF",
  "OP_FCVT",			// floating point convert
  "OP_FADD",			// floating point add
  "OP_FMUL",			// floating point multiply
  "OP_FDIV",			// floating point divide
  "OP_FCMP",			// floating point compare
  "OP_FBIT",			// floating point bit
  "OP_FCMOV"      // floating point cond move
};

const char *g_mem_type_names[3] = {
  "NOT_MEM",			// not a memory instruction
  "MEM_LD",			// a load instruction
  "MEM_ST",			// a store instruction
};


