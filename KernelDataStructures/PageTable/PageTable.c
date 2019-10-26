
#include <hardware.h>

// PageTable
    // array of PTEs (of a fixed number)

struct pte *initializePageTable() {
	// NOTE: malloc appears to return an int, so we're casting it to a pointer
    struct pte *pageTable = (struct pte *) malloc(sizeof(struct pte) * NUM_VPN);
	struct pte *ptep = pageTable;
	int i;
	for (i = MIN_VPN; i < NUM_VPN; i++) {
		ptep->valid = 0;
		ptep->prot = PROT_NONE;
		ptep->pfn = 0;
		ptep++;
	}
    return pageTable;
}

// valid = 1 bit; prot = 3 bits; pfn = 24 bits
void setPageTableEntry(struct pte * ptep, u_long valid, u_long prot, u_long pfn) {
    ptep->valid = valid;
    ptep->prot = prot;
    ptep->pfn = pfn;
}

// struct pte *getPte(struct pte *pageTable, void *addr) {
// 	int vpn = ((int) addr>>PAGESHIFT);
// 	int vpn0 = (VMEM_0_BASE>>PAGESHIFT); 
// 	return &(pageTable[vpn-vpn0]);
// }

void invalidatePageTableEntry(struct pte *ptep) {
	ptep->valid = 0;
}

// void deletePageTable() {
	// // for each PTE *ptep: free(ptep);
	// // free(pagetable)
// }
