#include <hardware.h>
#include <yalnix.h>
#include "../../Kernel.h" 


// TODO: return ERROR/0, and write to a struct pte **
struct pte *initializeRegionPageTable() {
    struct pte *pageTable = (struct pte *) malloc(sizeof(struct pte) * MAX_PT_LEN);
	struct pte *ptep = pageTable;
	int i;
	for (i = 0; i < MAX_PT_LEN; i++) {
		// we don't really have to care about prot and pfn here
		ptep->valid = 0;
		ptep->prot = PROT_NONE;
		ptep->pfn = 0;
		ptep++;
	}
    return pageTable;
}


// helper function for flushing individual PTEs; only visible in this module
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
		TracePrintf(1, "getVirtualAddr: invalid virtual address");
		return ERROR;
	}
	return vpn<<PAGESHIFT;
}


// valid = 1 bit; prot = 3 bits; pfn = 24 bits
void setPageTableEntry(struct pte *ptep, u_long valid, u_long prot, u_long pfn) {
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

int getAddressRegion(void *addr) {
	int intAddr = (int) addr;
	if (intAddr >= VMEM_1_BASE && intAddr < VMEM_1_LIMIT)
		return 1;
	else if (intAddr >= VMEM_0_BASE && intAddr < VMEM_0_LIMIT)
		return 0;
	else 
		return -1;
}

int getAddressProt(void *addr, int region, u_long *prot) {
	int baseVpn;
	struct pte *ptBase;
	if (region == 0) {
		baseVpn = (VMEM_0_BASE>>PAGESHIFT); 
		ptBase = (struct pte *) ReadRegister(REG_PTBR0);
	}
	else if (region == 1) {
		baseVpn = (VMEM_1_BASE>>PAGESHIFT); 
		ptBase = (struct pte *) ReadRegister(REG_PTBR1);
	}
	else {
		TracePrintf(1, "getAddressProt: invalid region given given to getAddressProt");
		return ERROR;
	}
	
    int addrVpn = ((int) addr)>>PAGESHIFT;
    struct pte *ptep = ptBase + addrVpn - baseVpn;
	*prot = ptep->prot;

	return SUCCESS;
}
