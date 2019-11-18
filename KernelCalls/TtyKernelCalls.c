#include "KernelCalls.h"
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../KernelDataStructures/TtyBuffer/TtyBuffer.h"
#include <hardware.h>
#include <yalnix.h>
#include <string.h>

#define min(a, b) (a < b ? a : b) 

int KernelTtyWrite(int tty_id, void *buf, int len) {
	TracePrintf(1, "KernelTtyWrite() called; tty_id = %d, buf = %x, len = %d\n", tty_id, buf, len);
	
	if (len < 0) {
		TracePrintf(1, "the given len is negative, returning with ERROR\n");
		return ERROR;
	}
	if (len == 0) {
		TracePrintf(1, "the given len is 0, returning with 0\n");
		return 0;
	}
	
    // copy buf into R0 so we don't lose it on context switch, see page 32
    void *r0Buf = (void *) malloc(len);
    memmove(r0Buf, buf, len);

	// while the target tty is not available, block the current process
	// (TOTHINK: using "if" is sufficient?)
    while (isTtyTransmitAvailable(tty_id) == 0) {
        blockTransmitter(tty_id);
    }

    void *batchStartAddress = r0Buf;
    int batchSize;
    int bytesTransmitted = 0;
    while (bytesTransmitted < len) {
		/* transmit as much as possible - either the max length
		 * allowed, or all of the remaining bytes.
		 */
        batchSize = min(TERMINAL_MAX_LINE, len - bytesTransmitted);
        
        TtyTransmit(tty_id, batchStartAddress, batchSize);
		
		/* mark the terminal as busy and wait until the current transmission finishes;
		 * the waiting process should be woken up by the TRAP_TTY_TRANSMIT
		 */
        waitTransmitter(tty_id);
		
        batchStartAddress += batchSize;
        bytesTransmitted += batchSize;
    }
	
	free(r0Buf);
	
    // mark the terminal as free and wake up another waiting writer if one exists
    unblockTransmitter(tty_id);
    return len;
}

int KernelTtyRead(int tty_id, void *buf, int len) {
	TracePrintf(1, "KernelTtyRead() called; tty_id = %d, buf = %x, len = %d\n", tty_id, buf, len);
	
	if (len < 0) {
		TracePrintf(1, "the given len is negative, returning with ERROR\n");
		return ERROR;
	}
	if (len == 0) {
		TracePrintf(1, "the given len is 0, returning with 0\n");
		return 0;
	}
	
    return readBuffer(tty_id, buf, len);
}
