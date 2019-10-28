#include <hardware.h>
#include <yalnix.h>
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../KernelDataStructures/PageTable/PageTable.h"
#include "../Kernel.h"

void handleTrapKernel(UserContext *uctxt) {
    // kernelCallNumber = uctxt->code
    /* use kernelCallNumber to call corresponding syscall function
       inside syscall vector array
    */
}

void handleTrapClock(UserContext *uctxt) {
    // in the future, do this to all PCBs
    if (currPCB->numRemainingDelayTicks > 0) {
        currPCB->numRemainingDelayTicks--;
    }
    if (idlePCB->numRemainingDelayTicks > 0) {
        idlePCB->numRemainingDelayTicks--;
    }
    // TODO: context switch between idle & init
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

    struct pte *currPageTable = currPCB->pagetable;
    struct pte *currPte = currPageTable + 2 * MAX_PT_LEN - 1;
    frame_t *newFrame;
    void *currAddr = (void *) (VMEM_1_LIMIT - PAGESIZE);
    while (((int) currAddr>>PAGESHIFT) >= targetPageNumber) {
        if (currPte->valid == 0) {
            if (getFrame(FrameList, numFrames, &newFrame) == ERROR) {
                // TODO: kill process;
                return;
            }
            setPageTableEntry(currPte, 1, (PROT_READ|PROT_WRITE), (int) (newFrame->addr)>>PAGESHIFT);
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