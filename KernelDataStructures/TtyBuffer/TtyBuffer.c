#include "./TtyBuffer.h"
#include "../Scheduler/Scheduler.h"
#include <hardware.h>
#include <yalnix.h>
#include <string.h>
#include "../../Kernel.h"

#define TTY_READ_BUFFER_SIZE (PAGESIZE / NUM_TERMINALS)

typedef struct ttyBuf {
    void *base;
    int numBytesWritten;
	int isLineReady;
} ttyBuf_t;

ttyBuf_t *buffers[NUM_TERMINALS];

int initBuffers(void *base) {
    void *bufBase = base;
	ttyBuf_t *ttyBuf;
    int i;
    for (i = 0; i < NUM_TERMINALS; i++) {
        ttyBuf = (ttyBuf_t *) malloc(sizeof(ttyBuf_t));
        if (ttyBuf == NULL) return ERROR;
        
        ttyBuf->base = bufBase;
        ttyBuf->numBytesWritten = 0;
		ttyBuf->isLineReady = 0;
		
        buffers[i] = ttyBuf;
        bufBase += TTY_READ_BUFFER_SIZE;
    }
    return SUCCESS;
}

int writeBuffer(int tty_id) {	
    ttyBuf_t *ttyBuf = buffers[tty_id];
	
	int lenWithNull = TtyReceive(tty_id, ttyBuf->base, TERMINAL_MAX_LINE);
	
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

	// block the reader if there is no line to be read yet
	// TOTHINK: is "if" sufficient?
    while (ttyBuf->isLineReady == 0) {
        if (blockReader(tty_id) == ERROR) return ERROR;
    }
	
	/* now the reader can read something; read as much as possible -
	 * either the length wanted, or the available length
	 */
	 
	if (len < ttyBuf->numBytesWritten) {
		memmove(buf, ttyBuf->base, len);
		
		// some bytes remain unread, move them to the base
		int numRemainingBytes = ttyBuf->numBytesWritten - len;
		void *baseOfRemaining = ttyBuf->base + len;
        memmove(ttyBuf->base, baseOfRemaining, numRemainingBytes);
		ttyBuf->numBytesWritten = numRemainingBytes;
		
		// attempts to unblock a reader because some bytes remain
		unblockReader(tty_id);
		
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
