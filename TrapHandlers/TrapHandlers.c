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

void handleTrapMemory(UserContext *uctxt) {
    // address = uctxt->addr
    // if address in Region 1 & in between stack & brk
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