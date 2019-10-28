#include <hardware.h>
#include <yalnix.h>

// PageTable
    // array of PTEs (of a fixed number)

// TODO: return ERROR/0, and write to a struct pte **
struct pte *initializeRegionPageTable() {
	// NOTE: malloc appears to return an int, so we're casting it to a pointer
    struct pte *pageTable = (struct pte *) malloc(sizeof(struct pte) * MAX_PT_LEN);
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

int getVirtualAddr(struct pte *ptep) {
	int r0Base = ReadRegister(REG_PTBR0);
	int r0Limit = r0Base + MAX_PT_LEN * sizeof(struct pte);
	int r1Base = ReadRegister(REG_PTBR1);
	int r1Limit = r1Base + MAX_PT_LEN * sizeof(struct pte);
	int vpn;
	if ((int) ptep >= r0Base && (int) ptep < r0Limit) {
		vpn = ((int) ptep - r0Base) / sizeof(struct pte);
	}
	else if ((int) ptep >= r1Base && (int) ptep < r1Limit) {
		vpn = ((int) ptep - r1Base) / sizeof(struct pte) + MAX_PT_LEN;
	}
	else {
		return ERROR;
	}
	return vpn<<PAGESHIFT;
}

// valid = 1 bit; prot = 3 bits; pfn = 24 bits
void setPageTableEntry(struct pte * ptep, u_long valid, u_long prot, u_long pfn) {
    ptep->valid = valid;
    ptep->prot = prot;
    ptep->pfn = pfn;
	if (ReadRegister(REG_VM_ENABLE) == 1) {
		int virtualAddr = getVirtualAddr(ptep);
		WriteRegister(REG_TLB_FLUSH, (unsigned int) virtualAddr);
	}
}

void invalidatePageTableEntry(struct pte *ptep) {
	ptep->valid = 0;
	if (ReadRegister(REG_VM_ENABLE) == 1) {
		int virtualAddr = getVirtualAddr(ptep);
		WriteRegister(REG_TLB_FLUSH, (unsigned int) virtualAddr);
	}
}
