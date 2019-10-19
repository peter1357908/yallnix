#include <hardware.h>
#include <yalnix.h>
#include "TrapHandlers/TrapHandlers.h"
#include "KernelDataStructures/PageTable/PageTable.h"
#include "KernelDataStructures/FrameList/FrameList.h"

frame_t *FrameList;

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

	// initialize FreePMList (don't forget to free...?)
	if (initFrame(FrameList, pmem_size, currKernelBrk) == ERROR) {
		Halt();
	}; 

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
	for (addr = (int) kernelDataStart; addr < (int) currKernelBrk; addr += PAGESIZE) {
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
	struct pte * r1Base = (struct pte *) ReadRegister(REG_PTBR1);
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
		struct pte * pageTable = (struct pte *) ReadRegister(REG_PTBR0);
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
				frame_t * newFrame; 
				if (getFrame(FrameList, newFrame) == ERROR) {
					return ERROR;
				};
				setPageTableEntry(&currPte, 1, (PROT_READ|PROT_WRITE), ((u_long) newFrame & PAGEMASK));
			}
		}
	}

	currKernelBrk = addr;
	return 0;
}
