#include <hardware.h>
#include <TrapHandlers.h>
#include <PageTable.h>

// the following variables stores info for building the initial page table
void *kernelDataStart;  // everything until this is READ and EXEC
void *currKernelBrk;  // everything from KernelDataStart until this is READ and WRITE

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd) {
	kernelDataStart = _KernelDataStart;
	currKernelBrk = _KernelDataEnd;  // _KernelDataEnd is the lowest address NOT in use
}

void DoIdle(void) {
	while(1) {
		TracePrintf(1, "DoIdle\n");
		Pause();
	}
}

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
	
	// initialize Interrupt Vector Array 
    void * interruptVectorArray[TRAP_VECTOR_SIZE];
    interruptVectorArray[TRAP_KERNEL] = handleTrapKernel;
    interruptVectorArray[TRAP_CLOCK] = handleTrapClock;
	interruptVectorArray[TRAP_ILLEGAL] = handleTrapIllegal;
    interruptVectorArray[TRAP_MEMORY] = handleTrapMemory;
    interruptVectorArray[TRAP_MATH] = handleTrapMath;
    interruptVectorArray[TRAP_TTY_RECEIVE] = handleTtyReceive;
    interruptVectorArray[TRAP_TTY_TRANSMIT] = handleTtyTransmit;
    interruptVectorArray[TRAP_DISK] = handleTrapDisk;

    // write REG_VECTOR_BASE
    WriteRegister(REG_VECTOR_BASE, &interruptVectorArray);

	// initialize FreePMList
	int remainingMemory = pmem_size - ((int) currKernelBrk - PMEM_BASE);
	void * FreePMList[remainingMemory];
	for (int i = 0; i < remainingMemory; i++) {
	    FreePMList[i] = (void *) (currKernelBrk + i);
	}

	// initialize the initial pagetable (think of this as for the initial idle process)
	struct pte *pageTable = initializePageTable();
	struct pte *currentPte = pageTable;

	// virtualize PMEM up to kernelDataStart
	for (int addr = PMEM_BASE; addr < (int) kernelDataStart; addr += PAGESIZE) {
		currentPte->valid = 1;
		currentPte->prot = (PROT_READ|PROT_EXEC);
		currentPte->pfn = (addr & PAGEMASK);
		currentPte += PAGESIZE;
	}

	// virtualize kernelDataStart up to currKernelBrk
	for (int addr = kernelDataStart; addr < (int) currKernelBrk; addr += PAGESIZE) {
		currentPte->valid = 1;
		currentPte->prot = (PROT_READ|PROT_WRITE);
		currentPte->pfn = (addr & PAGEMASK);
		currentPte += PAGESIZE;
	}

	// set MMU registers 
	struct pte * r0Base = pageTable;
	struct pte * r0Limit = pageTable + MAX_PT_LEN;
	struct pte * r1Base = r0Limit;
	struct pte * r1Limit = r1Base + MAX_PT_LEN;

	WriteRegister(REG_PTBR0, (unsigned int) r0Base);
	WriteRegister(REG_PTLR0, (unsigned int) r0Limit);
	WriteRegister(REG_PTBR1, (unsigned int) r1Base);
	WriteRegister(REG_PTLR1, (unsigned int) r1Limit);

	// enable virtual memory
	WriteRegister(REG_VM_ENABLE, 1);

	// initialize idle process
	r1Base->pfn = (((int) DoIdle) & PAGEMASK);
	uctxt->sp = r1Base;
#ifdef LINUX
	uctxt->ebp = r1Base;
#endif
	uctxt->pc = DoIdle;

	// initialize and run the first process (via scheduler, given uctxt)

	return;
}

// see manual page 46
/* special malloc (provided) will call this function to tell us about the heap segment
 * if it's before VM, we can safely assume that addr is higher than currKernelBrk (only malloc can call it)
 * if it's after VM, the procedure is analogous to KernelBrk() (since we can also call it)
 */
int SetKernelBrk(void *addr) {
	unsigned int isVM = ReadRegister(REG_VM_ENABLE);
	if (isVM) {
		// if addr lower than currKernelBrk:
			// for each PTE *ptep in to-be-invalidated pages:
				// invalidatePTE(ptep);
		// if addr higher than currKernelBrk:
			// for each PTE *ptep in to-be-allocated pages:
				// grab pfn from FreePMList
				// validatePTE(ptep, prot, pfn);
	}
	currKernelBrk = addr;
	// return ERROR if error else 0
    // if you change a page->frame mapping in the MMU, flush its pte!
}
