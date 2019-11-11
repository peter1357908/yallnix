#include "./TtyBuffer.h"
#include "../Scheduler/Scheduler.h"
#include <hardware.h>
#include <yalnix.h>
#include <string.h>
#include "../../Kernel.h"

#define TOTAL_READ_BUFFER_PAGES 1
#define TTY_READ_BUFFER_SIZE (TOTAL_READ_BUFFER_PAGES * PAGESIZE / NUM_TERMINALS)

typedef struct ttyBuf {
    void *base;
    int numBytesWritten;
	int isLineReady;
} ttyBuf_t;

ttyBuf_t *buffers[NUM_TERMINALS];

int initBuffers() {
	// make that many pages available
	void *totalBufferBase = tempVAddr - TOTAL_READ_BUFFER_PAGES * PAGESIZE;
	struct pte *totalBufferBasePtep = tempPtep - TOTAL_READ_BUFFER_PAGES;
	
	int pageCount;
	u_long pfn;
	for (pageCount = 0; pageCount < TOTAL_READ_BUFFER_PAGES; pageCount++) {
		if (getFrame(FrameList, numFrames, &pfn) == ERROR) Halt();
		
		setPageTableEntry(totalBufferBasePtep, 1, (PROT_READ|PROT_WRITE), pfn);
		
		totalBufferBasePtep++;
	}
	
    void *currBufferBase = totalBufferBase;
	ttyBuf_t *ttyBuf;
    int i;
    for (i = 0; i < NUM_TERMINALS; i++) {
        ttyBuf = (ttyBuf_t *) malloc(sizeof(ttyBuf_t));
        if (ttyBuf == NULL) return ERROR;
        
        ttyBuf->base = currBufferBase;
        ttyBuf->numBytesWritten = 0;
		ttyBuf->isLineReady = 0;
		
        buffers[i] = ttyBuf;
        currBufferBase += TTY_READ_BUFFER_SIZE;
    }
	
    return SUCCESS;
}

int writeBuffer(int tty_id) {
	ttyBuf_t *ttyBuf = buffers[tty_id];
	int lenWithNull = TtyReceive(tty_id, ttyBuf->base, TERMINAL_MAX_LINE);
	TracePrintf(1, "in writeBuffer(); tty_id = %d, lenWithNull = %d\n", tty_id, lenWithNull);
	
	/* per page 33, NULL is not copied into the buffer. lenWithNull will be
	 * 1 if a blank line is returned, and 0 for an EOF. Only wake a reader
	 * up if the line read is not just an EOF.
	 */
	if (lenWithNull > 0) {
		ttyBuf->numBytesWritten = lenWithNull - 1;
		ttyBuf->isLineReady = 1;
		unblockReader(tty_id);
	}
	else {
		ttyBuf->numBytesWritten = 0;
		ttyBuf->isLineReady = 0;
	}
	
    return SUCCESS;
}

int readBuffer(int tty_id, void *buf, int len) {    
    ttyBuf_t *ttyBuf = buffers[tty_id];
	TracePrintf(1, "in readBuffer(); tty_id = %d, buf = %x, len = %d\n", tty_id, buf, len);

	// block the reader if there is no line to be read yet
	// TOTHINK: is "if" sufficient?
    while (ttyBuf->isLineReady == 0) {
		TracePrintf(1, "target tty (id = %d) not ready, process (pid = %d) blocked\n", tty_id, currPCB->pid);
        if (blockReader(tty_id) == ERROR) return ERROR;
    }
	
	/* now the reader can read something; read as much as possible -
	 * either the length wanted, or the available length
	 */
	
	TracePrintf(1, "target tty (id = %d) is now ready, process (pid = %d) now resuming in readBuffer()\n", tty_id, currPCB->pid);
	if (len < ttyBuf->numBytesWritten) {
		memmove(buf, ttyBuf->base, len);
		
		// some bytes remain unread, move them to the base
		int numRemainingBytes = ttyBuf->numBytesWritten - len;
		void *baseOfRemaining = ttyBuf->base + len;
        memmove(ttyBuf->base, baseOfRemaining, numRemainingBytes);
		ttyBuf->numBytesWritten = numRemainingBytes;
		
		// attempts to unblock a reader because some bytes remain
		if (unblockReader(tty_id) == ERROR) return ERROR;
		
		return len;
	}
	else {
		int bytes_read = ttyBuf->numBytesWritten;
		
		memmove(buf, ttyBuf->base, ttyBuf->numBytesWritten);
		
		// all available bytes are read, set the numBytesWritten to 0
		ttyBuf->numBytesWritten = 0;
		ttyBuf->isLineReady = 0;
		
		return bytes_read;
	}
}
