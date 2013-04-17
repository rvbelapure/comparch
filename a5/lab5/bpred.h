//Author: Prashant J Nair, Georgia Tech
//Simulation of Branch Predictors

/****** Header file for branch prediction structures ********/

#ifndef __BPRED_H
#define __BPRED_H
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include "thread_support.h"

typedef enum bpred_type_enum{
  BPRED_NOTTAKEN, 
  BPRED_TAKEN,
  BPRED_BIMODAL,
  BPRED_GSHARE,
  BPRED_PERCEPTRON 
}bpred_type;

typedef struct bpred bpred;

extern int use_bpred, hist_len;
extern int percep_entries, percep_wt_max, percep_wt_min, threshold;
extern bpred_type type_bpred;

extern int thread_count;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

struct bpred{
  unsigned int  ghr[HW_MAX_THREAD]; // global history register
  unsigned int *pht; // pattern history table

  int **perceptron;

  bpred_type type; // type of branch predictor
  unsigned int hist_len; // history length
  unsigned int pht_entries; // entries in pht table
  unsigned int perceptron_entries;
  int threshold;

  long int mispred; // A counter to count mispredictions
  long int okpred; // A counter to count correct predictions
};


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bpred *bpred_new(bpred_type type, int hist_len);
int    bpred_access(bpred *b, unsigned int pc, int tid, int *yout /* only for perceptron */);
void   bpred_update(bpred *b, unsigned int pc, 
		    int pred_dir, int resolve_dir, int tid, int yout);

// bimodal predictor
void   bpred_bimodal_init(bpred *b);
int    bpred_bimodal_access(bpred *b, unsigned int pc, int tid);
void   bpred_bimodal_update(bpred *b, unsigned int pc, 
			    int pred_dir, int resolve_dir, int tid);


// gshare predictor
void   bpred_gshare_init(bpred *b);
int    bpred_gshare_access(bpred *b, unsigned int pc, int tid);
void   bpred_gshare_update(bpred *b, unsigned int pc, 
			   int pred_dir, int resolve_dir, int tid);

// perceptron predictor
void   bpred_perceptron_init(bpred *b);
int    bpred_perceptron_access(bpred *b, unsigned int pc, int tid, int *yout);
void   bpred_perceptron_update(bpred *b, unsigned int pc, 
			   int pred_dir, int resolve_dir, int tid, int yout);


/***********************************************************/
#endif
