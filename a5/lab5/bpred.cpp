#include <assert.h>
#include "bpred.h"

//Needs to add knobs for history bits


#define BPRED_PHT_CTR_MAX  3
#define BPRED_PHT_CTR_INIT 2

#define GSHARE_PHT_CTR_INIT 2

#define BPRED_SAT_INC(X,MAX)  (X<MAX)? (X+1)  : MAX
#define BPRED_SAT_DEC(X)      X?       (X-1)  : 0
#define ABS(X)		      (X<0)?   (-(X)) : X

int use_bpred, hist_len;
int percep_entries, percep_wt_max, percep_wt_min, threshold;
bpred_type type_bpred;

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bpred * bpred_new(bpred_type type, int hist_len){
  bpred *b = (bpred *) calloc(1, sizeof(bpred));
  
  b->type     = type;
  b->hist_len = hist_len;
  b->pht_entries = (1<<hist_len);
  b->perceptron_entries = percep_entries;
  b->threshold = threshold;

  switch(type){

  case BPRED_NOTTAKEN:
    // nothing to do
    break;

  case BPRED_TAKEN:
    // nothing to do
    break;

  case BPRED_BIMODAL:
    bpred_bimodal_init(b); 
    break;

  case BPRED_GSHARE:
    bpred_gshare_init(b); 
    break;

  case BPRED_PERCEPTRON:
    bpred_perceptron_init(b);
    break;

  default: 
    assert(0);
  }

  return b;
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int bpred_access(bpred *b, unsigned int pc, int tid, int *yout /* only for perceptron */){

  switch(b->type){

  case BPRED_NOTTAKEN:
    return 0;

  case BPRED_TAKEN:
    return 1;

  case BPRED_BIMODAL:
    return bpred_bimodal_access(b, pc, tid);

  case BPRED_GSHARE:
    return bpred_gshare_access(b, pc, tid);

  case BPRED_PERCEPTRON:
    return bpred_perceptron_access(b, pc, tid, yout);

  default: 
    assert(0);
  }

}


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void   bpred_update(bpred *b, unsigned int pc, 
		    int pred_dir, int resolve_dir, int tid, int pred_val){

  // update the stats
  if(pred_dir == resolve_dir){
    b->okpred++;
  }else{
    b->mispred++;
  }

  
  // update the predictor

  switch(b->type){
    
  case BPRED_NOTTAKEN:
    // nothing to do
    break; 

  case BPRED_TAKEN:
    // nothing to do
    break;

  case BPRED_BIMODAL:
    bpred_bimodal_update(b, pc, pred_dir, resolve_dir, tid);
    break;

  case BPRED_GSHARE:
    bpred_gshare_update(b, pc, pred_dir, resolve_dir, tid);
    break;

  case BPRED_PERCEPTRON:
    bpred_perceptron_update(b, pc, pred_dir, resolve_dir, tid, pred_val);
    break;

  default: 
    assert(0);
  }


}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_bimodal_init(bpred *b){

  b->pht = (unsigned int *) calloc(b->pht_entries, sizeof(unsigned int));

  for(int ii=0; ii< b->pht_entries; ii++){
    b->pht[ii]=BPRED_PHT_CTR_INIT; 
  }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int   bpred_bimodal_access(bpred *b, unsigned int pc, int tid){
  int pht_index = pc % (b->pht_entries);
  int pht_ctr = b->pht[pht_index];

  if(pht_ctr > BPRED_PHT_CTR_MAX/2){
    return 1; // Taken
  }else{
    return 0; // Not-Taken
  }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_bimodal_update(bpred *b, unsigned int pc, 
			    int pred_dir, int resolve_dir, int tid){
  int pht_index = pc % (b->pht_entries);
  int pht_ctr = b->pht[pht_index];
  int new_pht_ctr;

  if(resolve_dir==1){
    new_pht_ctr = BPRED_SAT_INC(pht_ctr, BPRED_PHT_CTR_MAX);
  }else{
    new_pht_ctr = BPRED_SAT_DEC(pht_ctr);
  }

  b->pht[pht_index] = new_pht_ctr;  
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_gshare_init(bpred *b)
{
	b->pht = (unsigned int *) calloc(b->pht_entries, sizeof(unsigned int));

	for(int ii=0; ii< b->pht_entries; ii++)
		b->pht[ii]= GSHARE_PHT_CTR_INIT; 
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int   bpred_gshare_access(bpred *b, unsigned int pc, int tid)
{
	int pht_index = (pc ^ b->ghr[tid]) % (b->pht_entries);
	int pht_ctr = b->pht[pht_index];
	
	if(pht_ctr >= 2)	// partially taken or taken
		return 1;
	else			// partially not taken or not taken
		return 0;
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_gshare_update(bpred *b, unsigned int pc, 
			    int pred_dir, int resolve_dir, int tid)
{
	// XXX : to take only hist_len number of bits from GHR before doing XOR?
	int pht_index = (pc ^ b->ghr[tid]) % (b->pht_entries);
	int pht_ctr = b->pht[pht_index];

	b->ghr[tid] = (((b->ghr[tid])<<1) | resolve_dir);

	b->pht[pht_index] = resolve_dir ? BPRED_SAT_INC(pht_ctr,3) : BPRED_SAT_DEC(pht_ctr);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_perceptron_init(bpred *b)
{
	b->perceptron = (int **) calloc(b->perceptron_entries,sizeof(int *));
	for(int i = 0 ; i < b->perceptron_entries ; i++)
		b->perceptron[i] = (int *) calloc((b->hist_len + 1), sizeof(int));	// last position is for w0
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int bpred_perceptron_access(bpred *b, unsigned int pc, int tid, int *yout)
{
	/* we need to consider only last (b->hist_len) bits of ghr */
	int filter = 0x00;
	for(int i = 0 ; i < b->hist_len ; i++)
		filter = (filter << 1) | 0x01;
	int filtered_ghr = b->ghr[tid] & filter;

	int index = (tid * b->perceptron_entries / thread_count) + ((pc) % (b->perceptron_entries / thread_count));

	int y = 0;
	for(int i = 0 ; i < b->hist_len ; i++)
		y += ((((filtered_ghr >> i) & 0x01) ? 1 : -1) * b->perceptron[index][i]);	// \sigma w_i * x_i
	
	y += b->perceptron[index][b->hist_len];		// adding w0

	*yout = y;
	
	if(y < 0)
		return 0;
	else
		return 1;	
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_perceptron_update(bpred *b, unsigned int pc, 
			    int pred_dir, int resolve_dir, int tid, int yout)
{
	/* we need to consider only last (b->hist_len) bits of ghr */
	int filter = 0x00;
	for(int i = 0 ; i < b->hist_len ; i++)
		filter = (filter << 1) | 0x01;
	int filtered_ghr = b->ghr[tid] & filter;
	int index = (tid * b->perceptron_entries / thread_count) + ((pc) % (b->perceptron_entries / thread_count));

	if((pred_dir != resolve_dir) || ((ABS(yout)) < b->threshold))
	{
		int i;
		for(i = 0 ; i < b->hist_len ; i++)
		{
			if((b->perceptron[index][i] < percep_wt_max) && (b->perceptron[index][i] > percep_wt_min))
				b->perceptron[index][i] +=  ( (resolve_dir ? 1 : -1) * (((filtered_ghr >> i) & 0x01) ? 1 : -1));
		}
		
		if((b->perceptron[index][i] < percep_wt_max) && (b->perceptron[index][i] > percep_wt_min))
			b->perceptron[index][i] += (resolve_dir ? 1 : -1);
	}

	b->ghr[tid] = (((b->ghr[tid])<<1) | resolve_dir);
}
