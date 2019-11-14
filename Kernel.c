#include <hardware.h>
#include <yalnix.h>
#include <string.h>
#include "TrapHandlers/TrapHandlers.h"
#include "KernelDataStructures/PageTable/PageTable.h"
#include "KernelDataStructures/FrameList/FrameList.h"
#include "KernelDataStructures/Scheduler/Scheduler.h"
#include "KernelDataStructures/TtyBuffer/TtyBuffer.h"
#include "KernelDataStructures/Lock/Lock.h"
#include "KernelDataStructures/Cvar/Cvar.h"
#include "LoadProgram.h"
#include "Kernel.h"

// create the Interrupt Vector Array globally (instead of inside KernelStart)
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

	int i;
	for (i = TRAP_DISK + 1; i < TRAP_VECTOR_SIZE; i++) {
		interruptVectorArray[i] = handleNothing;
	}

    // write REG_VECTOR_BASE
    WriteRegister(REG_VECTOR_BASE, (unsigned int) interruptVectorArray);
	
	// initialize the general r0PageTable (whose stack is for "init")
	struct pte *r0PageTable = initializeRegionPageTable();
	
	/* initialize an r1PageTable so we can enable VM; that page table will be
	 * for the first ever process to run (i.e. "init")
	 */
	struct pte *initR1PageTable = initializeRegionPageTable();
	
	/* initialize FrameList; must happen after the pagetables are initialized,
	 * and before the r0PageTable is filled.
	 */
	numFrames = pmem_size / PAGESIZE;
	if (initFrameList(&FrameList, numFrames, currKernelBrk) == ERROR) {
		Halt();
	}
	
	/* ------ no more malloc(), starting here, until VM is enabled!! ------ */
	
	// fill the r0PageTable according to the current kernel state
	struct pte *currentPte = r0PageTable;
	int addr;
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
	
	// set MMU registers and enable VM
	WriteRegister(REG_PTBR0, (unsigned int) r0PageTable);
	WriteRegister(REG_PTLR0, (unsigned int) MAX_PT_LEN);
	WriteRegister(REG_PTBR1, (unsigned int) initR1PageTable);
	WriteRegister(REG_PTLR1, (unsigned int) MAX_PT_LEN);
	
	WriteRegister(REG_VM_ENABLE, 1);
	
	// calculate a few global addresses
	r0StackBasePtep = r0PageTable + KERNEL_STACK_BASE_VPN - KERNEL_BASE_VPN;
	tempPtep = r0StackBasePtep - 1;
	tempVAddr =  (void *) (KERNEL_STACK_BASE - PAGESIZE);

	// initialize ttyBuffers
	initBuffers();

	// initialize Lock Map
	initLockMap();

	// initialize Cvar Map
	initCvarMap();
	
	/* initialization logic: "init" requires special initialization because
	 * its kernel stack is the same as the current kernel stack, and its
	 * r1PageTable is already initialized as initR1PageTable; while "idle"
	 * will be initialized as any future new process. However, for "idle" to run, 
	 * we need to manually "load" DoIdle() instead of relying on LoadProgram().
	 * Consequently, we make a special initInitProcess() and a special LoadIdle() while using
	 * the regular LoadProgram() for "init," and the regular initProcess() for "idle"
	 *
	 * Further, "init" won't be enqueued in the readyQ; it will be currPCB. "init"
	 * will also have its kctxt malloc'd already, as it's not considered a new process.
	 */
	 
	if (initScheduler() == ERROR) Halt();
	
	PCB_t *idlePCB;
	PCB_t *initPCB;
	
	if (initProcess(&idlePCB) == ERROR || LoadIdle(idlePCB) == ERROR) {
		Halt();
	}
	
	// per page 61, we default to "init" with no arguments if cmg_args[0] == NULL
	if (cmd_args[0] == NULL) {
		char* default_args[] = {"init", NULL};
		cmd_args = default_args;
	}
	
	// be mindful that LoadProgram may return KILL; but as long as it's not SUCCESS...
	if (initInitProcess(initR1PageTable, &initPCB) == ERROR || LoadProgram(cmd_args[0], cmd_args, initPCB) != SUCCESS) {
		Halt();
	}

	// mimic context switch, as if switching into the "init" process
	currPCB = initPCB;
	
	// no need to switch kernel stack, since init already uses the current kernel stack
	
	// no need to change MMU registers since REG_PTBR1 already points to initR1PageTable
	
	// allocate memory for starterKernelStack and starterKctxt
	starterKernelStack = (void *) malloc(KERNEL_STACK_MAXSIZE);
	if (starterKernelStack == NULL) {
		Halt();
	}
	starterKctxt = (KernelContext *) malloc(sizeof(KernelContext));
	if (starterKctxt == NULL) {
		Halt();
	}
	
	
	/* ------------ current Kctxt and Kernel Stack are copied ------------ */
	if (KernelContextSwitch(getStarterKernelState, NULL, NULL) == ERROR) {
		Halt();
	}
	
	/* ------------ every new-process-to-run-next starts here ------------ */
	
	memmove(uctxt, currPCB->uctxt, sizeof(UserContext));

	return;
}

int SetKernelBrk(void *addr) {
	unsigned int isVM = ReadRegister(REG_VM_ENABLE);
	
	// if it were to grow into the kernel stack, return ERROR
	if ((int) addr >= KERNEL_STACK_BASE) {
		return ERROR;
	}
	
	// update page table if we're in virtual memory mode
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
		else if ((int) addr > (int) currKernelBrk) {
			for (currAddr = (int) currKernelBrk; currAddr < (int) addr; currAddr += PAGESIZE) {
				int vpn = (currAddr>>PAGESHIFT);
				u_long pfn;
				if (getFrame(FrameList, numFrames, &pfn) == ERROR) return ERROR;
				targetPtep = r0PageTable + vpn - vpn0;
				setPageTableEntry(targetPtep, 1, (PROT_READ|PROT_WRITE), pfn);
			}
		}
	}

	currKernelBrk = addr;
	return 0;
}
