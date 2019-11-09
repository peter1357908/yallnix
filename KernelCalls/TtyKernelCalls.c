#include <hardware.h>
#include <yalnix.h>
#include <string.h>
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "./KernelCalls.h"
#include "../KernelDataStructures/TtyBuffer/TtyBuffer.h"

#define min(a, b) (a < b ? a : b) 

int KernelTtyWrite(int tty_id, void *buf, int len) {
    // copy buf into R0 so we don't lose it on context switch
    void *r0Buf = (void *) malloc(len);
    memmove(r0Buf, buf, len);

    if (!isTtyTransmitAvailable(tty_id)) {
        blockTransmitter(tty_id);
    }

    void *batchStartAddress = r0Buf;
    int batchSize;
    int bytesTransmitted = 0;
    while (bytesTransmitted < len) {
        batchSize = min(TERMINAL_MAX_LINE, len - bytesTransmitted);
        
        TtyTransmit(tty_id, batchStartAddress, batchSize);
        waitTransmitter(tty_id);

        batchStartAddress += batchSize;
        bytesTransmitted += batchSize;
    }

    // unblocks processes waiting to transmit to the same tty (if they exist)
    unblockTransmitter(tty_id);
    return len;
}

int KernelTtyRead(int tty_id, void *buf, int len){  
    readBuffer(tty_id, buf, len);
}