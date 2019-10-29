#include <hardware.h>
#include <yalnix.h>
#include <string.h>
#include "TrapHandlers/TrapHandlers.h"
#include "KernelDataStructures/PageTable/PageTable.h"
#include "KernelDataStructures/FrameList/FrameList.h"
#include "KernelDataStructures/Scheduler/Scheduler.h"
#include "LoadProgram.h"

frame_t *FrameList;
int numFrames;

KernelContext *starterKctxt;

// the following variables stores info for building the initial page table
void *kernelDataStart;  // everything until this is READ and EXEC
void *currKernelBrk;  // everything from KernelDataStart until this is READ and WRITE

// initialize Interrupt Vector Array 
void *interruptVectorArray[TRAP_VECTOR_SIZE];

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd) {
	kernelDataStart = _KernelDataStart;
	currKernelBrk = _KernelDataEnd;  // _KernelDataEnd is the lowest address NOT in use
}

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {

    interruptVectorArray[TRAP_KERNEL] = handleTrapKernel;
    interruptVectorArray[TRAP_CLOCK] = handleTrapClock;
	interruptVectorArray[TRAP_ILLEGAL] = handleTrapIllegal;
    interruptVectorArray[TRAP_MEMORY] = handleTrapMemory;
    interruptVectorArray[TRAP_MATH] = handleTrapMath;
    interruptVectorArray[TRAP_TTY_RECEIVE] = handleTtyReceive;
    interruptVectorArray[TRAP_TTY_TRANSMIT] = handleTtyTransmit;
    interruptVectorArray[TRAP_DISK] = handleTrapDisk;
	
	TracePrintf(1, "\n%p\n", interruptVectorArray);

	int i;
	for (i = 8; i < TRAP_VECTOR_SIZE; i++) {
		interruptVectorArray[i] = handleNothing;
	}

    // write REG_VECTOR_BASE
    WriteRegister(REG_VECTOR_BASE, (unsigned int) interruptVectorArray);

	// initialize the first pagetable (for the idle process)
	struct pte *r0PageTable = initializeRegionPageTable();
	struct pte *currentPte = r0PageTable;

	int addr;

	// fill the pagetable according to the current kernel state
	for (addr = VMEM_BASE; addr < VMEM_0_LIMIT; addr += PAGESIZE) {
		u_long prot;

		// r/e for text; the last segment before each cutoff may be an incomplete page (why does the code below work?)
		if (addr + PAGESIZE <= (int) kernelDataStart) {
			prot = (PROT_READ|PROT_EXEC);
		} 
		// r/w for data/heap
		else if (addr + PAGESIZE <= (int) currKernelBrk) {
			prot = (PROT_READ|PROT_WRITE);
		}
		// r/w for stack; KERNEL_STACK_BASE is actually always at the beginning of a page
		else if (KERNEL_STACK_BASE < addr + PAGESIZE) {
			prot = (PROT_READ|PROT_WRITE);
		}

		// unused page; skip
		else {
			currentPte++;
			continue;
		}
		
		setPageTableEntry(currentPte, 1, prot, (addr>>PAGESHIFT));
		currentPte++;
	}	

	struct pte *idleR1PageTable = initializeRegionPageTable();

	// set MMU registers 
	WriteRegister(REG_PTBR0, (unsigned int) r0PageTable);
	WriteRegister(REG_PTLR0, (unsigned int) MAX_PT_LEN); 

	// PTBR1 is initially idle's R1PageTable so we can enable VM 
	// we switch PTBR1 to init's r1PageTable before KernelStart returns via KernelContextSwitch
	WriteRegister(REG_PTBR1, (unsigned int) idleR1PageTable); 
	WriteRegister(REG_PTLR1, (unsigned int) MAX_PT_LEN); 

	// initialize FrameList; must happen after the pagetable is initialized and filled.
	numFrames = pmem_size / PAGESIZE;
	if (initFrameList(&FrameList, numFrames, currKernelBrk) == ERROR) {
		Halt();
	}

	// enable virtual memory
	WriteRegister(REG_VM_ENABLE, 1);

	if (initIdleProcess(idleR1PageTable) == ERROR) {
		Halt();
	}

	if (initProcess(&initPCB) == ERROR) {
		Halt();
	}

	WriteRegister(REG_PTBR1, (unsigned int) initPCB->r1PageTable);
	LoadProgram(cmd_args[0], cmd_args, initPCB);

	currPCB = initPCB;

	starterKctxt = (KernelContext *) malloc(sizeof(KernelContext));
	if (KernelContextSwitch(getStarterKctxt, starterKctxt, NULL) == ERROR) { 
		// print error message
		Halt();
	}

	memmove(currPCB->kctxt, starterKctxt, sizeof(KernelContext));
	memmove(uctxt, currPCB->uctxt, sizeof(KernelContext));

	return;
}

int SetKernelBrk(void *addr) {
	unsigned int isVM = ReadRegister(REG_VM_ENABLE);
	
	// if it were to grow into the kernel stack, return ERROR
	if ((int) addr >= KERNEL_STACK_BASE) {
		return ERROR;
	}
	
	// update page table if we're in VM
	if (isVM == 1) {
		struct pte *r0PageTable = (struct pte *) ReadRegister(REG_PTBR0);
		struct pte *targetPtep;
		int currAddr;
		int vpn0 = (VMEM_0_BASE>>PAGESHIFT); // first page in VMEM_0

		// if addr lower than currKernelBrk, invalidate pages and free frames accordingly
		if ((int) addr < (int) currKernelBrk) {
			for (currAddr = (int) addr; currAddr < (int) currKernelBrk; currAddr += PAGESIZE) {
				int vpn = (currAddr>>PAGESHIFT);
				targetPtep = r0PageTable + vpn - vpn0;
				freeFrame(FrameList, numFrames, targetPtep->pfn);
				invalidatePageTableEntry(targetPtep);
			}
		}
	
		// if addr higher than currKernelBrk, validate pages and allocate frames accordingly
		else {
			for (currAddr = (int) currKernelBrk; currAddr < (int) addr; currAddr += PAGESIZE) {
				int vpn = (currAddr>>PAGESHIFT);
				frame_t *newFrame;
				if (getFrame(FrameList, numFrames, &newFrame) == ERROR) {
					return ERROR;
				}
				targetPtep = r0PageTable + vpn - vpn0;
				setPageTableEntry(targetPtep, 1, (PROT_READ|PROT_WRITE), (int) (newFrame->addr)>>PAGESHIFT);
			}
		}
	}

	currKernelBrk = addr;
	return 0;
}
