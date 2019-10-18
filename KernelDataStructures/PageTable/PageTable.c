
#include <hardware.h>

// PageTable
    // array of PTEs (of a fixed number)

struct pte * initializePageTable() {
    struct pte *pageTable = malloc(sizeof(struct pte) * NUM_VPN);
	struct pte *ptep = pageTable;
	for (int i = MIN_VPN; i < NUM_VPN; i++) {
		ptep->valid = 0;
		ptep->prot = PROT_NONE;
		ptep += sizeof(struct pte);
	}
    return pageTable;
}

// bool chunkValidityCheck(void * base, void * limit, bool validity) {
// 	// check every PTE from base to limit
// 	// return FALSE if any PTE's validity != validity
// 	// otherwise return TRUE
// }


void deletePageTable() {
	// for each PTE *ptep: free(ptep);
	// free(pagetable)
}
