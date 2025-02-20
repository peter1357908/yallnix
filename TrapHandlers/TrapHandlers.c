// see manual page 34 and 44
#include <hardware.h>
#include <yalnix.h>
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../KernelDataStructures/PageTable/PageTable.h"
#include "../Kernel.h"
#include "TrapHandlers.h"
#include "../KernelCalls/KernelCalls.h"
#include "../KernelDataStructures/TtyBuffer/TtyBuffer.h"

void handleTrapKernel(UserContext *uctxt) {
	// return_code defaults to ERROR
	int return_code = ERROR;
	
    // use the code stored in uctxt to call the corresponding syscall functions.
    switch(uctxt->code) {
        case YALNIX_FORK:
            return_code = KernelFork();
            break;
        case YALNIX_EXEC:
            return_code = KernelExec((char *)(uctxt->regs)[0], (char **)(uctxt->regs)[1]);
            break;
        case YALNIX_EXIT:
            KernelExit((int)(uctxt->regs)[0]);
			return_code = 0;
            break;
        case YALNIX_WAIT:
            return_code = KernelWait((int *)(uctxt->regs)[0]);
            break;
        case YALNIX_GETPID:
            return_code = KernelGetPid();
            break;
        case YALNIX_BRK:
            return_code = KernelBrk((void *)(uctxt->regs)[0]);
            break;
        case YALNIX_DELAY:
            return_code = KernelDelay((int)(uctxt->regs)[0]);
            break;
		case YALNIX_RECLAIM:
			return_code = KernelReclaim((int)(uctxt->regs)[0]);
			break;
        case YALNIX_TTY_READ:
            return_code = KernelTtyRead((int)(uctxt->regs)[0], \
			(void *)(uctxt->regs)[1], (int)(uctxt->regs)[2]);
            break;
        case YALNIX_TTY_WRITE:
            return_code = KernelTtyWrite((int)(uctxt->regs)[0], \
			(void *)(uctxt->regs)[1], (int)(uctxt->regs)[2]);
            break;

        #ifdef LINUX
        case YALNIX_LOCK_INIT:
            return_code = KernelLockInit((int *)(uctxt->regs)[0]);
			break;
        case YALNIX_LOCK_ACQUIRE:   
            return_code = KernelAcquire((int)(uctxt->regs)[0]);
			break;
        case YALNIX_LOCK_RELEASE:
            return_code = KernelRelease((int)(uctxt->regs)[0]);
			break;
        case YALNIX_CVAR_INIT:
            return_code = KernelCvarInit((int *)(uctxt->regs)[0]);
			break;
        case YALNIX_CVAR_SIGNAL:
            return_code = KernelCvarSignal((int)(uctxt->regs)[0]);
			break;
        case YALNIX_CVAR_BROADCAST:
            return_code = KernelCvarBroadcast((int)(uctxt->regs)[0]);
			break;
        case YALNIX_CVAR_WAIT:
            return_code = KernelCvarWait((int)(uctxt->regs)[0], (int)(uctxt->regs)[1]);
			break;
        case YALNIX_PIPE_INIT:
            return_code = KernelPipeInit((int *)(uctxt->regs)[0]);
			break;
        case YALNIX_PIPE_READ:
            return_code = KernelPipeRead((int)(uctxt->regs)[0], \
			(void *)(uctxt->regs)[1], (int)(uctxt->regs)[2]);
			break;
        case YALNIX_PIPE_WRITE: 
            return_code = KernelPipeWrite((int)(uctxt->regs)[0], \
			(void *)(uctxt->regs)[1], (int)(uctxt->regs)[2]);
			break;
        #endif /* LINUX */
		
		default:
			TracePrintf(1, "handleTrapKernel: trying to handle a non-existent trap (uctxt->code = %d)\n, halting Yalnix.\n", uctxt->code);
			Halt();
			/* --- NOT REACHED ---*/
			break;
    }
	
	(uctxt->regs)[0] = (u_long) return_code;
}

void handleTrapClock(UserContext *uctxt) {
    TracePrintf(1, "handleTrapClock() called, currPCB->pid = %d\n", currPCB->pid);
    if (tickDownSleepers() == ERROR || \
		kickProcess() == ERROR) {
		Halt();
	}
	
	TracePrintf(1, "handleTrapClock() execution finished; currPCB->pid = %d\n", currPCB->pid);
}

void handleTrapIllegal(UserContext *uctxt) {
    TracePrintf(1, "handleTrapIllegal() called, currPCB->pid = %d\n", currPCB->pid);
    KernelExit(ERROR);
}

void handleTrapMemory(UserContext *uctxt) {
	TracePrintf(1, "handleTrapMemory() called, currPCB->pid = %d\n", currPCB->pid);
    void *addr = uctxt->addr;
	TracePrintf(1, "Offending address is %x\n", addr);
	unsigned int targetPageNumber = ((unsigned int) addr)>>PAGESHIFT;
    unsigned int breakPageNumber = ((unsigned int) currPCB->brk)>>PAGESHIFT;

    /* we leave 'REDZONE' of 1 page. This implicitly makes
	 * sure that the target addr is in Region 1, above the user
	 * brk's page.
	 */
    if (targetPageNumber <= breakPageNumber + 1) {
		// if we can't, then abort the current process
        KernelExit(ERROR);
    }
	
    struct pte *currPageTable = currPCB->r1PageTable;
    struct pte *currPtep = currPageTable + MAX_PT_LEN - 1;
    u_long pfn;
    unsigned int currAddr = (VMEM_1_LIMIT - PAGESIZE);
    /* and the while loop implicitely makes sure that we only
	 * change the pagetable if the target addr is in Region 1,
	 * below the already validated stack pages.
	 */
	while ((currAddr>>PAGESHIFT) >= targetPageNumber) {
        if (currPtep->valid == 0) {
            if (getFrame(FrameList, numFrames, &pfn) == ERROR) {
                KernelExit(ERROR);
            }
            setPageTableEntry(currPtep, 1, (PROT_READ|PROT_WRITE), pfn);
        }
        currPtep--;
        currAddr -= PAGESIZE;
    }
}

void handleTrapMath(UserContext *uctxt) {
    TracePrintf(1, "handleTrapMath() called, currPCB->pid = %d\n", currPCB->pid);
    KernelExit(ERROR);
}

void handleTtyReceive(UserContext *uctxt) {
    int tty_id = uctxt->code;
    if (writeBuffer(tty_id) == ERROR) Halt();
}

void handleTtyTransmit(UserContext *uctxt) {
    int tty_id = uctxt->code;
    if (signalTransmitter(tty_id) == ERROR) Halt();
}

void handleTrapDisk(UserContext *uctxt) {
    TracePrintf(1, "handleTrapDisk() called, currPCB->pid = %d. Halting...\n", currPCB->pid);
	Halt();
}

void handleNothing(UserContext *uctxt) {
    TracePrintf(1, "handleNothing() called, currPCB->pid = %d. Halting...\n", currPCB->pid);
	Halt();
}
