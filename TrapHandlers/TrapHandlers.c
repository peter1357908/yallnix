#include <hardware.h>
#include <yalnix.h>

void handleTrapKernel(UserContext *uctxt) {
    // kernelCallNumber = uctxt->code
    /* use kernelCallNumber to call corresponding syscall function
       inside syscall vector array
    */
}

void handleTrapClock(UserContext *uctxt) {
    // Kernel_CvarBroadcast(clock_id)
}

void handleTrapIllegal(UserContext *uctxt) {
    // do this
}

void handleTrapMemory(UserContext *uctxt) {
	TracePrintf(1, "\nhey i'm the memory trap handler\n");
    void *addr = uctxt->addr;
	void *sp = uctxt->sp;
	
    // if address in Region 1 & in between stack & brk
	// if ((int) addr < (int) sp && (int) )
        // grow the stack to cover this address
        // (leave at least 1 page unmapped with valid bit == 0, 
        //  so that we don't silently overlap into heap without triggering TRAP_MEMORY)
    // else:
        // abort currently running process
        // segmentation fault?
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