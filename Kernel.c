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

// the following variables stores info for building the initial page table
void *kernelDataStart;  // everything until this is READ and EXEC
void *currKernelBrk;  // everything from KernelDataStart until this is READ and WRITE

void DoIdle() {
	while(1) {
		TracePrintf(1, "DoIdle\n");
		Pause();
	}
}

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd) {
	kernelDataStart = _KernelDataStart;
	currKernelBrk = _KernelDataEnd;  // _KernelDataEnd is the lowest address NOT in use
}

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
	
	// initialize Interrupt Vector Array 
    void *interruptVectorArray[TRAP_VECTOR_SIZE];
    interruptVectorArray[TRAP_KERNEL] = handleTrapKernel;
    interruptVectorArray[TRAP_CLOCK] = handleTrapClock;
	interruptVectorArray[TRAP_ILLEGAL] = handleTrapIllegal;
    interruptVectorArray[TRAP_MEMORY] = handleTrapMemory;
    interruptVectorArray[TRAP_MATH] = handleTrapMath;
    interruptVectorArray[TRAP_TTY_RECEIVE] = handleTtyReceive;
    interruptVectorArray[TRAP_TTY_TRANSMIT] = handleTtyTransmit;
    interruptVectorArray[TRAP_DISK] = handleTrapDisk;
	
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

	struct pte *r1PageTable = initializeRegionPageTable();

	// set MMU registers 
	WriteRegister(REG_PTBR0, (unsigned int) r0PageTable);
	WriteRegister(REG_PTLR0, (unsigned int) MAX_PT_LEN); 
	WriteRegister(REG_PTBR1, (unsigned int) r1PageTable); 
	WriteRegister(REG_PTLR1, (unsigned int) MAX_PT_LEN); 

	// initialize FrameList; must happen after the pagetable is initialized and filled.
	numFrames = pmem_size / PAGESIZE;
	if (initFrameList(&FrameList, numFrames, currKernelBrk) == ERROR) {
		Halt();
	}

	// enable virtual memory
	WriteRegister(REG_VM_ENABLE, 1);

	// initialize the userland stack, with one page of memory at the very top
	struct pte *r1StackBasePtep = r1PageTable + MAX_PT_LEN - 1;
	frame_t *r1StackBaseFrame;
	if (getFrame(FrameList, numFrames, &r1StackBaseFrame) == ERROR) {
		Halt();
	}
	setPageTableEntry(r1StackBasePtep, 1, PROT_READ|PROT_WRITE, (int) (r1StackBaseFrame->addr)>>PAGESHIFT);
		
	// initialize the idle process by manipulating uctxt like in LoadProgram
	// DoIdle() has no arguments and needs no arguments
	int size = 0;
	int argcount = 0;
	char *cp = ((char *)VMEM_1_LIMIT) - size;

	char **cpp = (char **)
		(((int)cp - ((argcount + 3 + POST_ARGV_NULL_SPACE) *sizeof (void *))) & ~7);

  	char *cp2 = (caddr_t)cpp - INITIAL_STACK_FRAME_SIZE;
	
	uctxt->sp = cp2;
#ifdef LINUX
	uctxt->ebp = cp2;
#endif

	uctxt->pc = DoIdle;
	
	// idle process is the only process that is not initialized with initProcess 
	// (found in Scheduler.c)
	idlePCB = (PCB_t *) malloc(sizeof(PCB_t));
	idlePCB->pid = 0; 
	idlePCB->r1PageTable = r1PageTable;
	idlePCB->uctxt = uctxt;

	// initialize and run the `init` process (via scheduler, given uctxt)
	// PCB_t *initPCB;
	// if (initProcess(&initPCB, uctxt) == ERROR) {
	// 	Halt();
	// }
	// WriteRegister(REG_PTBR0, (unsigned int) initPCB->pagetable);
	// WriteRegister(REG_PTLR0, (unsigned int) MAX_PT_LEN); 
	// WriteRegister(REG_PTBR1, (unsigned int) (initPCB->pagetable + MAX_PT_LEN)); 
	// WriteRegister(REG_PTLR1, (unsigned int) MAX_PT_LEN); 
	// LoadProgram(cmd_args[0], cmd_args, initPCB);
	return;
}

// see manual page 46
/* special malloc (provided) will call this function to tell us about the heap segment
 * if it's before VM, we can safely assume that addr is higher than currKernelBrk (only malloc can call it)
 * if it's after VM, the procedure is analogous to KernelBrk() (since we can also call it)
 */
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
