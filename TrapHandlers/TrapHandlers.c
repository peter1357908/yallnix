#include <hardware.h>
#include <yalnix.h>
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../KernelDataStructures/PageTable/PageTable.h"
#include "../Kernel.h"
#include "TrapHandlers.h"
#include "../KernelCalls/KernelCalls.h"

void handleTrapKernel(UserContext *uctxt) {
    // use the code stored in uctxt to call the corresponding syscall functions.
    switch(uctxt->code) {
        case YALNIX_FORK:
            KernelFork();
            break;
        case YALNIX_EXEC:
            KernelExec((char *)(uctxt->regs)[0], (char **)(uctxt->regs)[1]);
            break;
        case YALNIX_EXIT:
            KernelExit((int)(uctxt->regs)[0]);
            break;
        case YALNIX_WAIT:
            KernelWait((int *)(uctxt->regs)[0]);
            break;
        case YALNIX_GETPID:
            KernelGetPid();
            break;
        case YALNIX_BRK:
            KernelBrk((void *)(uctxt->regs)[0]);
            break;
        case YALNIX_DELAY:
            KernelDelay((int)(uctxt->regs)[0]);
            break;
        // case YALNIX_TTY_READ:
        //     KernelTtyRead();
        // case YALNIX_TTY_WRITE:
        //     KernelTtyWrite();

        // #ifdef LINUX
        // case YALNIX_LOCK_INIT:
        //     KernelLockInit();
        // case YALNIX_LOCK_ACQUIRE:   
        //     KernelLockAcquire();
        // case YALNIX_LOCK_RELEASE:
        //     KernelLockRelease();
        // case YALNIX_CVAR_INIT:
        //     KernelCvarInit();
        // case YALNIX_CVAR_SIGNAL:
        //     KernelCvarSignal();
        // case YALNIX_CVAR_BROADCAST:
        //     KernelCvarBroadcast();
        // case YALNIX_CVAR_WAIT:
        //     kernelCvarWait();
        // case YALNIX_PIPE_INIT:
        //     KernelPipeInit();
        // case YALNIX_PIPE_READ:
        //     KernelPipeRead();
        // case YALNIX_PIPE_WRITE: 
        //     KernelPipeWrite();
        // #endif
    }
}

void handleTrapClock(UserContext *uctxt) {
    TracePrintf(1, "handleTrapClock() called\n");
 
    if (tickDownSleepers() == ERROR || \
		kickProcess() == ERROR) {
		Halt();
	}
}

void handleTrapIllegal(UserContext *uctxt) {
    // do this
}

void handleTrapMemory(UserContext *uctxt) {
	TracePrintf(1, "starting  handleTrapMemory()\n");
    void *addr = uctxt->addr;
    void *sp = uctxt->sp; 
	int targetPageNumber = (int) addr>>PAGESHIFT;
    int breakPageNumber = (int) (currPCB->brk)>>PAGESHIFT;

    // we leave 'REDZONE' of 1 page
    if (targetPageNumber <= breakPageNumber + 1) {
        // TODO: kill process
        return;
    }

    struct pte *currPageTable = currPCB->r1PageTable;
    struct pte *currPte = currPageTable + MAX_PT_LEN - 1;
    u_long pfn;
    void *currAddr = (void *) (VMEM_1_LIMIT - PAGESIZE);
    while (((int) currAddr>>PAGESHIFT) >= targetPageNumber) {
        if (currPte->valid == 0) {
            if (getFrame(FrameList, numFrames, &pfn) == ERROR) {
                // TODO: kill process;
                return;
            }
            setPageTableEntry(currPte, 1, (PROT_READ|PROT_WRITE), pfn);
        }
        currPte--;
        currAddr -= PAGESIZE;
    }
}

void handleTrapMath(UserContext *uctxt) {
    // printf(stderr, "Arithmetic error");
}

void handleTtyReceive(UserContext *uctxt) {
    // tty_id = uctxt->code
    // int lenOfInput = TtyReceive(tty_id, tty_idBuf, len)
    // (if lenOfInput > MAX_TERMINAL_LENGHT, we need to handle the input in batches)
}

void handleTtyTransmit(UserContext *uctxt) {
    // tty_id = uctxt->code
    // acquire lock corresponding to tty_id
    // tty_idCanTransfer = True
    // Kernel_CvarSignal(cvar_tty_id)
    // release lock
}

void handleTrapDisk(UserContext *uctxt) {
    // throw Error;
}

void handleNothing(UserContext *uctxt) {
	TracePrintf(1, "\nhanldNothing() was invoked...\n");
    // throw Error;
}