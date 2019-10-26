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

    // write REG_VECTOR_BASE
    WriteRegister(REG_VECTOR_BASE, (unsigned int) &interruptVectorArray);

	// initialize the initial pagetable (think of this as for the initial idle process)
	struct pte *pageTable = initializePageTable();
	struct pte *currentPte = pageTable;

	int addr;

	// initialize page tables
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

	// set MMU registers 
	WriteRegister(REG_PTBR0, (unsigned int) pageTable);
	WriteRegister(REG_PTLR0, (unsigned int) MAX_PT_LEN); 
	WriteRegister(REG_PTBR1, (unsigned int) (pageTable + MAX_PT_LEN)); 
	WriteRegister(REG_PTLR1, (unsigned int) MAX_PT_LEN); 

	// initialize FrameList (are we supposed to free it somewhere?)
	numFrames = pmem_size / PAGESIZE;
	if (initFrameList(&FrameList, numFrames, currKernelBrk) == ERROR) {
		Halt();
	}

	// enable virtual memory
	WriteRegister(REG_VM_ENABLE, 1);
	
	// TODO: initialize idle process 

	// initialize the userland stack, with one page of memory at the very top
	struct pte *r1BasePtep = ((struct pte *) ReadRegister(REG_PTBR1));
	struct pte * r1StackBasePtep = r1BasePtep + MAX_PT_LEN - 1;
	frame_t *r1StackBaseFrame;
	if (getFrame(FrameList, numFrames, &r1StackBaseFrame) == ERROR) {
		Halt();
	}
	setPageTableEntry(r1StackBasePtep, 1, PROT_READ|PROT_WRITE, (int) (r1StackBaseFrame->addr)>>PAGESHIFT);
		
	// 	void **r1StackBase = (void **) (VMEM_1_LIMIT - PAGESIZE);	
	// 	void *r0DoIdle = DoIdle;
		
	// 	// TracePrintf(1, "r1StackBase = %p\n", r1StackBase);
	// 	// TracePrintf(1, "r0DoIdle = %p\n", r0DoIdle);
		
	// 	// TracePrintf(1, "*r1StackBase = %p\n", *r1StackBase);
		
	// 	uctxt->pc = r0DoIdle;
		
	// 	TracePrintf(1, "uctxt->pc = %p\n", uctxt->pc);
		
	// 	// *r1StackBase = r0DoIdle;

	// 	uctxt->sp = (void *) r1StackBase;
		
	// 	TracePrintf(1, "uctxt->sp = %p\n", uctxt->sp);
	// #ifdef LINUX
	// 	uctxt->sp = (void *) r1StackBase;
	// #endif

	// initialize and run the first process (via scheduler, given uctxt)
	
	PCB_t *idlePCB;
	if (initProcess(&idlePCB, uctxt) == ERROR) {
		Halt();
	}
	LoadProgram("idle", cmd_args, idlePCB);
	return;
}

// see manual page 46
/* special malloc (provided) will call this function to tell us about the heap segment
 * if it's before VM, we can safely assume that addr is higher than currKernelBrk (only malloc can call it)
 * if it's after VM, the procedure is analogous to KernelBrk() (since we can also call it)
 */
int SetKernelBrk(void *addr) {
	unsigned int isVM = ReadRegister(REG_VM_ENABLE);
	
	// update page table if we're in VM
	if (isVM == 1) {
		struct pte *pageTable = (struct pte *) ReadRegister(REG_PTBR0);
		struct pte currPte;
		int currAddr;

		// if addr lower than currKernelBrk, invalidate pages
		if ((int) addr < (int) currKernelBrk) {
			for (currAddr = (int) addr; currAddr < (int) currKernelBrk; currAddr += PAGESIZE) {
				int vpn = (currAddr>>PAGESHIFT);
				int vpn0 = (VMEM_0_BASE>>PAGESHIFT); // first page in VMEM_0
				currPte = pageTable[vpn-vpn0];
				invalidatePageTableEntry(&currPte);
			}
		}
	
		// if addr higher than currKernelBrk, validate pages and allocate frames accordingly
		else {
			for (currAddr = (int) currKernelBrk; currAddr < (int) addr; currAddr += PAGESIZE) {
				int vpn = (currAddr>>PAGESHIFT);
				int vpn0 = (VMEM_0_BASE>>PAGESHIFT); // first page in VMEM_0
				currPte = pageTable[vpn-vpn0];
				frame_t *newFrame;
				if (getFrame(FrameList, numFrames, &newFrame) == ERROR) {
					return ERROR;
				}
				
				setPageTableEntry(&currPte, 1, (PROT_READ|PROT_WRITE), (int) (newFrame->addr)>>PAGESHIFT);
			}
		}
	}

	currKernelBrk = addr;
	return 0;
}
