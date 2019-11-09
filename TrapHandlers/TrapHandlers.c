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
    // use the code stored in uctxt to call the corresponding syscall functions.
    switch(uctxt->code) {
        case YALNIX_FORK:
            (uctxt->regs)[0] = (u_long) KernelFork();
            break;
        case YALNIX_EXEC:
            (uctxt->regs)[0] = (u_long) KernelExec((char *)(uctxt->regs)[0], (char **)(uctxt->regs)[1]);
            break;
        case YALNIX_EXIT:
            KernelExit((int)(uctxt->regs)[0]);
            break;
        case YALNIX_WAIT:
            (uctxt->regs)[0] = (u_long) KernelWait((int *)(uctxt->regs)[0]);
            break;
        case YALNIX_GETPID:
            (uctxt->regs)[0] = (u_long) KernelGetPid();
            break;
        case YALNIX_BRK:
            (uctxt->regs)[0] = (u_long) KernelBrk((void *)(uctxt->regs)[0]);
            break;
        case YALNIX_DELAY:
            (uctxt->regs)[0] = (u_long) KernelDelay((int)(uctxt->regs)[0]);
            break;
        case YALNIX_TTY_READ:
            KernelTtyRead((int)(uctxt->regs)[0], (void *)(uctxt->regs)[1], (int)(uctxt->regs)[2]);
            break;
        case YALNIX_TTY_WRITE:
            KernelTtyWrite((int)(uctxt->regs)[0], (void *)(uctxt->regs)[1], (int)(uctxt->regs)[2]);
            break; 

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
    TracePrintf(1, "handleTrapClock() called, currPCB->pid = %d\n", currPCB->pid);
    if (tickDownSleepers() == ERROR || \
		kickProcess() == ERROR) {
		Halt();
	}
	
	TracePrintf(1, "handleTrapClock() execution finished; currPCB->pid = %d\n", currPCB->pid);
}

void handleTrapIllegal(UserContext *uctxt) {
    TracePrintf(1, "handleTrapIllegal() called, currPCB->pid = %d\n", currPCB->pid);
	if (exitProcess(ERROR) == ERROR) Halt();
}

void handleTrapMemory(UserContext *uctxt) {
	TracePrintf(1, "handleTrapMemory() called, currPCB->pid = %d\n", currPCB->pid);
    void *addr = uctxt->addr;
	TracePrintf(1, "Offending address is %x\n", addr);
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
    TracePrintf(1, "handleTrapMath() called, currPCB->pid = %d\n", currPCB->pid);
	if (exitProcess(ERROR) == ERROR) Halt();
}

void handleTtyReceive(UserContext *uctxt) {
    int tty_id = uctxt->code;
    void *buf = (void *) malloc(TERMINAL_MAX_LINE);
    int actualLen = TtyReceive(tty_id, buf, TERMINAL_MAX_LINE);
    writeBuffer(tty_id, buf, actualLen);
    free(buf);
}

void handleTtyTransmit(UserContext *uctxt) {
    int tty_id = uctxt->code;
    signalTransmitter(tty_id);
}

void handleTrapDisk(UserContext *uctxt) {
    TracePrintf(1, "handleTrapDisk() called, currPCB->pid = %d. Halting...\n", currPCB->pid);
	Halt();
}

void handleNothing(UserContext *uctxt) {
    TracePrintf(1, "handleTrapDisk() called, currPCB->pid = %d. Halting...\n", currPCB->pid);
	Halt();
}
