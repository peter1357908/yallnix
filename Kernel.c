#include <hardware.h>
#include "TrapHandlers/TrapHandlers.h"
#include "KernelDataStructures/PageTable/PageTable.h"
#include "KernelDataStructures/FrameList/FrameList.h"

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
    WriteRegister(REG_VECTOR_BASE, (unsigned int) &interruptVectorArray);

	// initialize FreePMList
	int numFrames = pmem_size / PAGESIZE;
	frame_t *FrameList[numFrames];
	frame_t *currFrame = FrameList;
	void* frameAddr = PMEM_BASE;
	int i;
	for (i = 0; i < numFrames; i++) {
		currFrame->addr = frameAddr;
		if (((int) frameAddr < (int) currKernelBrk) || (KERNEL_STACK_BASE <= (int) frameAddr && KERNEL_STACK_LIMIT > (int) frameAddr)) {
			currFrame->isFree = 0;
		} else {
			currFrame->isFree = 1;
		}
		frameAddr += PAGESIZE;
		currFrame += sizeof(frame_t);
	}

	// initialize the initial pagetable (think of this as for the initial idle process)
	struct pte *pageTable = initializePageTable();
	struct pte *currentPte = pageTable;

	int addr;
	// virtualize PMEM up to kernelDataStart
	for (addr = PMEM_BASE; addr < (int) kernelDataStart; addr += PAGESIZE) {
		setPageTableEntry(currentPte, 1, (PROT_READ|PROT_EXEC ), (addr & PAGEMASK));
		currentPte += PAGESIZE;
	}

	// virtualize kernelDataStart up to currKernelBrk
	for (addr = kernelDataStart; addr < (int) currKernelBrk; addr += PAGESIZE) {
		setPageTableEntry(currentPte, 1, (PROT_READ|PROT_WRITE), (addr & PAGEMASK));
		currentPte += PAGESIZE;
	}

	// set MMU registers 
	WriteRegister(REG_PTBR0, (unsigned int)  pageTable);
	WriteRegister(REG_PTLR0, (unsigned int) pageTable + MAX_PT_LEN);
	WriteRegister(REG_PTBR1, (unsigned int) pageTable + MAX_PT_LEN);
	WriteRegister(REG_PTLR1, (unsigned int) pageTable + 2 * MAX_PT_LEN);

	// enable virtual memory
	WriteRegister(REG_VM_ENABLE, 1);

	// initialize idle process
	struct pte * r1Base = ReadRegister(REG_PTBR1);
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

	// update page table if we're in VMM
	if (isVM) {
		struct pte * pageTable = ReadRegister(REG_PTBR0);
		struct pte currPte;
		int currAddr;

		// if addr lower than currKernelBrk, invalidate pages
		if ((int) addr < (int) currKernelBrk) {
			for (currAddr = (int) addr; currAddr < (int) currKernelBrk; currAddr += PAGESIZE ) {
				int vpn = (currAddr & PAGEMASK);
				int vpn0 = (VMEM_0_BASE & PAGEMASK); // first page in VMEM_0
				currPte = pageTable[vpn-vpn0];
				invalidatePageTableEntry(&currPte);
			}
		}
	
		// if addr higher than currKernelBrk, allocate new valid pages
		else {
			for (currAddr = (int) currKernelBrk; currAddr < (int) addr; currAddr += PAGESIZE) {
				int vpn = (currAddr & PAGEMASK);
				int vpn0 = (VMEM_0_BASE & PAGEMASK); // first page in VMEM_0
				currPte = pageTable[vpn-vpn0];
				// grab free frame

				// setPageTableEntry(&currPte, 1, (PROT_READ|PROT_WRITE), )

			}
		}
			// for each PTE *ptep in to-be-allocated pages:
				// grab pfn from FreePMList
				// validatePTE(ptep, prot, pfn);
	
	}

	currKernelBrk = addr;
	// return ERROR if error else 0
    // if you change a page->frame mapping in the MMU, flush its pte!
}
