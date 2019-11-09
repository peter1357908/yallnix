#include "./TtyBuffer.h"
#include "../Scheduler/Scheduler.h"
#include <hardware.h>
#include <yalnix.h>
#include <string.h>
#include "../../Kernel.h"

typedef struct ttyBuf {
    void *base;
    int numBytesWritten;
} ttyBuf_t;

ttyBuf_t *buffers[NUM_TERMINALS];
int bufferSize = PAGESIZE / NUM_TERMINALS;

int initBuffers(void *base) {
    void *bufBase = base;
	ttyBuf_t *ttyBuf;
    int i;
    for (i = 0; i < NUM_TERMINALS; i++) {
        ttyBuf = (ttyBuf_t *) malloc(sizeof(ttyBuf_t));
        if (ttyBuf == NULL) return ERROR;
        
        ttyBuf->base = bufBase;
        ttyBuf->numBytesWritten = 0;
		
        buffers[i] = ttyBuf;
        bufBase += bufferSize;
    }
    return SUCCESS;
}

int writeBuffer(int tty_id, void *buf, int len) {
    if (len > bufferSize) return ERROR;
    ttyBuf_t *ttyBuf = buffers[tty_id];
    memmove(ttyBuf->base, buf, len);
    ttyBuf->numBytesWritten += len;

    /* this function only unblocks a reader if
        (1) there is a blocked reader
        (2) the reader w/ the highest priority has enough bytes to read
    */
    unblockReader(tty_id, ttyBuf->numBytesWritten);
    return SUCCESS;
}
int readBuffer(int tty_id, void *buf, int len) {
    if (len > bufferSize) return ERROR;
    
    ttyBuf_t *ttyBuf = buffers[tty_id];

    while (len > ttyBuf->numBytesWritten) {
        /*  blocks reader & associates it w/ 
            num bytes it's trying to read
        */
        blockReader(tty_id, len);
    }

    memmove(buf, ttyBuf->base, len);

    int numRemainingBytes = ttyBuf->numBytesWritten - len;
    if (numRemainingBytes > 0) {
        void *baseOfRemaining = ttyBuf->base + len;
        memmove(ttyBuf->base, baseOfRemaining, numRemainingBytes);
    }
    ttyBuf->numBytesWritten = numRemainingBytes;
    
    return SUCCESS;
}