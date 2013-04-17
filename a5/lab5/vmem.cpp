//Author : Moinuddin K Qureshi, Prashant J Nair
//MS ECE : Georgia Institute of Technology

#include <assert.h>
#include "vmem.h"



int vmem_enabled, tlb_size, vmem_page_size;

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

tlb *tlb_new(int size){
  tlb *t = (tlb *) calloc(1, sizeof(tlb));

  t->size=size;
  t->entries = (tlb_entry *)calloc(size, sizeof(tlb_entry));
  for(int i = 0 ; i < size ; i++)
  {
	  t->entries[i].vpn = 0;
	  t->entries[i].pfn = 0;
	  t->entries[i].threadid = 0;
	  t->entries[i].last_access_time = 0;
	  t->entries[i].valid = false;
  }

  return t;
}



//////////////////////////////////////////////////////////////
// returns 1 if TLB hit, 0 otherwise
// If hit, the PFN field is changed to the translated PFN
//////////////////////////////////////////////////////////////

bool tlb_access (tlb *t, uint64_t vpn, int threadid, uint64_t *pfn){
  bool found=0;

  for(int ii=0; ii<t->size; ii++){
    tlb_entry *entry = &t->entries[ii];
    if( entry->valid &&
	(entry->threadid==threadid) &&
	(entry->vpn==vpn)){
      found=1;
      *pfn = entry->pfn;
      entry->last_access_time = t->s_access;
      break;
    }
  }

  t->s_access++;

  if(found==0){ 
    t->s_miss++;
  }

  return found;
}


//////////////////////////////////////////////////////////////
// Use this function only after TLB miss and DRAM access for PTE completes
// Get the actual PFN not from PTE but from vmem_vpn_to_pfn and then install
//////////////////////////////////////////////////////////////

void tlb_install(tlb *t, uint64_t vpn, int threadid, uint64_t  pfn){

	// Students to write this function.  
	// Use LRU replacement to find TLB victim
	// Initialize entry->threadid, entry->vpn, entry->pfn 
	// Hint: Check cache_install function 
	int lru_index = 0;
	for(int i = 0 ; i < t->size ; i++)
	{
		if(t->entries[i].last_access_time < t->entries[lru_index].last_access_time)
			lru_index = i;
	}

	t->entries[lru_index].vpn = vpn;
	t->entries[lru_index].pfn = pfn;
	t->entries[lru_index].threadid = threadid;
	t->entries[lru_index].valid = true;
	t->entries[lru_index].last_access_time = t->s_access;
	t->s_access++;
}


//////////////////////////////////////////////////////////////
// This function provides PTE address for a given VPN
// On TLB miss, use this function to get the DRAM address for PTE
// Do not change this function
//////////////////////////////////////////////////////////////

uint64_t vmem_get_pteaddr(uint64_t vpn, int threadid){

  assert(threadid < VMEM_MAX_THREADS);
  
  uint64_t pte_base_addr = threadid * VMEM_PAGE_TABLE_SIZE;
  uint64_t pte_offset = (vpn%(1<<20))* VMEM_PTE_SIZE;

  uint64_t pte = pte_base_addr + pte_offset;

  // XXX : what does following comment line mean?
  return pte; // provides a byte address of PTE (change to lineaddr)
}

//////////////////////////////////////////////////////////////
// This function provides the actual VPN to PFN translation
// Access this function once you get the PTE from DRAM, and before TLB install
// Do not change this function
//////////////////////////////////////////////////////////////

uint64_t vmem_vpn_to_pfn(uint64_t vpn, int threadid){

  assert(threadid < VMEM_MAX_THREADS);

  uint64_t total_os_reserved_pages = 16384;
  uint64_t pages_per_thread = (1<<20); // 4GB per thread
  uint64_t pfn_base = threadid*pages_per_thread;
  uint64_t pfn_thread = vpn % pages_per_thread; // simple function (not realistic)
  uint64_t pfn = total_os_reserved_pages + pfn_base + pfn_thread;

  return pfn;
}
