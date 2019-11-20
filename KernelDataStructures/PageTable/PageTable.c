#include <hardware.h>
#include <yalnix.h>
#include <stdlib.h>  // malloc
#include "../../Kernel.h"
#include "PageTable.h"


int initializeRegionPageTable(struct pte **pageTablep) {
    *pageTablep = (struct pte *) malloc(sizeof(struct pte) * MAX_PT_LEN);
	if (*pageTablep == NULL) {
		TracePrintf(1, "initializeRegionPageTable: malloc'ing for pageTable failed\n");
		return ERROR;
	}
	struct pte *ptep = *pageTablep;
	int i;
	for (i = 0; i < MAX_PT_LEN; i++) {
		// we don't really have to care about prot and pfn here
		ptep->valid = 0;
		ptep->prot = PROT_NONE;
		ptep->pfn = 0;
		ptep++;
	}
    return SUCCESS;
}


/* helper function for flushing individual PTEs; only visible in this module
 * returning int instead of unsigned int so we can return ERROR (supposedly -1)
 *
 * ASSUMES THAT THE CURRENT MMU REGISTERS ARE CONSISTENT (see the comments for
 * setPageTableEntry())
 */
int getVirtualAddr(struct pte *ptep) {
	unsigned int ptep_int = (unsigned int) ptep;
	unsigned int r0PtBase = ReadRegister(REG_PTBR0);
	unsigned int r0PtLimit = r0PtBase + ReadRegister(REG_PTLR0) * sizeof(struct pte);
	unsigned int r1PtBase = ReadRegister(REG_PTBR1);
	unsigned int r1PtLimit = r1PtBase + ReadRegister(REG_PTLR1) * sizeof(struct pte);
	TracePrintf(2, "getVirtualAddr: ptep_int = %d, r0PtBase = %d, r0PtLimit = %d, r1PtBase = %d, r1PtLimit = %d\n", ptep_int, r0PtBase, r0PtLimit, r1PtBase, r1PtLimit);
	int vpn;
	if (ptep_int >= r0PtBase && ptep_int < r0PtLimit) {
		TracePrintf(2, "It's region 0!\n");
		vpn = (ptep_int - r0PtBase) / sizeof(struct pte);
	}
	else if (ptep_int >= r1PtBase && ptep_int < r1PtLimit) {
		TracePrintf(2, "It's region 1!\n");
		vpn = (ptep_int - r1PtBase) / sizeof(struct pte) + MAX_PT_LEN;
	}
	else {
		TracePrintf(1, "getVirtualAddr: invalid virtual address\n");
		return ERROR;
	}
	TracePrintf(1, "returning %d\n", vpn<<PAGESHIFT);
	return vpn<<PAGESHIFT;
}


/* this function will automatically flush the affected pte mapping in MMU,
 * assuming that the current MMU registers have the pageTable for the intended
 * ptep. Otherwise, (like in LoadIdle() and KernelFork()), use 
 * setPageTableEntryNoFlush() instead.
 */
void setPageTableEntry(struct pte *ptep, u_long valid, u_long prot, u_long pfn) {
    setPageTableEntryNoFlush(ptep, valid, prot, pfn);
	
	int virtualAddr = getVirtualAddr(ptep);
	if (virtualAddr == ERROR) Halt();
	WriteRegister(REG_TLB_FLUSH, (unsigned int) virtualAddr);
}

/* this function will NOT flush the MMU in any way, assuming that the affected
 * ptep is not in the pageTables currently in the MMU registers. Used in
 * LoadIdle() and KernelFork(), as well as before we enable virtual memory.
 */
void setPageTableEntryNoFlush(struct pte *ptep, u_long valid, u_long prot, u_long pfn) {
    ptep->valid = valid;
    ptep->prot = prot;
    ptep->pfn = pfn;
}


void invalidatePageTableEntry(struct pte *ptep) {
	ptep->valid = 0;
	if (ReadRegister(REG_VM_ENABLE) == 1) {
		int virtualAddr = getVirtualAddr(ptep);
		if (virtualAddr == ERROR) Halt();
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
		baseVpn = KERNEL_BASE_VPN; 
		ptBase = (struct pte *) ReadRegister(REG_PTBR0);
	}
	else if (region == 1) {
		baseVpn = USER_BASE_VPN; 
		ptBase = (struct pte *) ReadRegister(REG_PTBR1);
	}
	else {
		TracePrintf(1, "getAddressProt: invalid region given to getAddressProt\n");
		return ERROR;
	}
	
    int addrVpn = ((int) addr)>>PAGESHIFT;
    struct pte *ptep = ptBase + addrVpn - baseVpn;
	*prot = ptep->prot;

	return SUCCESS;
}
