
#include <hardware.h>

// PageTable
    // array of PTEs (of a fixed number)

// TODO: return ERROR/0, and write to a struct pte **
struct pte *initializePageTable() {
	// NOTE: malloc appears to return an int, so we're casting it to a pointer
    struct pte *pageTable = (struct pte *) malloc(sizeof(struct pte) * NUM_VPN);
	struct pte *ptep = pageTable;
	int i;
	for (i = MIN_VPN; i < NUM_VPN; i++) {
		ptep->valid = 0;
		ptep->prot = PROT_NONE;
		ptep->pfn = 0;
	}
    return pageTable;
}

// valid = 1 bit; prot = 3 bits; pfn = 24 bits
void setPageTableEntry(struct pte * ptep, u_long valid, u_long prot, u_long pfn) {
    ptep->valid = valid;
    ptep->prot = prot;
    ptep->pfn = pfn;
	if (ReadRegister(REG_VM_ENABLE) == 1) {
		unsigned int virtualAddr = (ptep->pfn)<<PAGESHIFT;
		WriteRegister(REG_TLB_FLUSH, virtualAddr);
	}
}

void invalidatePageTableEntry(struct pte *ptep) {
	ptep->valid = 0;
	if (ReadRegister(REG_VM_ENABLE) == 1) {
		unsigned int virtualAddr = (ptep->pfn)<<PAGESHIFT;
		WriteRegister(REG_TLB_FLUSH, virtualAddr);
	}
}
