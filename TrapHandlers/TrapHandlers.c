#include <hardware.h>
#include <yalnix.h>
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../KernelDataStructures/PageTable/PageTable.h"
#include "../Kernel.h"
#include "TrapHandlers.h"
#include "../KernelCalls/KernelCalls.h"

void handleTrapKernel(UserContext *uctxt) {
    // kernelCallNumber = uctxt->code
    /* use kernelCallNumber to call corresponding syscall function
       inside syscall vector array
    // */
    // switch(uctxt->code) {
    //     case YALNIX_FORK:
    //         KernelFork();
    //     case YALNIX_EXEC:
    //         KernelExec();
    //     case YALNIX_EXIT:
    //         KernelExit();
    //     case YALNIX_WAIT:
    //         KernelWait();
    //     case YALNIX_GETPID:
    //         KernelGetPid();
    //     case YALNIX_BRK:
    //         KernelBrk();
    //     case YALNIX_DELAY:
    //         KernelDelay();
    //     case YALNIX_TTY_READ:
    //         KernelTtyRead();
    //     case YALNIX_TTY_WRITE:
    //         KernelTtyWrite();

    //     #ifdef LINUX
    //     case YALNIX_LOCK_INIT:
    //         KernelLockInit();
    //     case YALNIX_LOCK_ACQUIRE:   
    //         KernelLockAcquire();
    //     case YALNIX_LOCK_RELEASE:
    //         KernelLockRelease();
    //     case YALNIX_CVAR_INIT:
    //         KernelCvarInit();
    //     case YALNIX_CVAR_SIGNAL:
    //         KernelCvarSignal();
    //     case YALNIX_CVAR_BROADCAST:
    //         KernelCvarBroadcast();
    //     case YALNIX_CVAR_WAIT:
    //         kernelCvarWait();
    //     case YALNIX_PIPE_INIT:
    //         KernelPipeInit();
    //     case YALNIX_PIPE_READ:
    //         KernelPipeRead();
    //     case YALNIX_PIPE_WRITE: 
    //         KernelPipeWrite();
    //     #endif
    // }
}

void handleTrapClock(UserContext *uctxt) {
    // in the future, do this to all PCBs
    TracePrintf(1, "\nhandleTrapClock() called\n");
    if (initPCB->numRemainingDelayTicks > 0) {
        initPCB->numRemainingDelayTicks--;
    }
    
    if (currPCB->pid == idlePCB->pid && initPCB->numRemainingDelayTicks <= 0 ) {
        if (KernelContextSwitch(MyKCS, idlePCB, initPCB) == ERROR) { 
            // print error message
            Halt();
        }
    } 
    else if (currPCB->pid == initPCB->pid) {
        if (KernelContextSwitch(MyKCS, initPCB, idlePCB) == ERROR) { 
            // print error message
            Halt();
        }
    }
}

void handleTrapIllegal(UserContext *uctxt) {
    // do this
}

void handleTrapMemory(UserContext *uctxt) {
	TracePrintf(1, "\nhey i'm the memory trap handler\n");
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