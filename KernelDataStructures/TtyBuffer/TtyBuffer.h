#ifndef _TtyBuffer_h
#define _TtyBuffer_h

// returns ERROR/SUCCESS; initializes tty buffers at address "base"
int initBuffers(void *base);

/*  returns ERROR/SUCCESS; copies [len] bytes from buf to tty_id's buffer
    unblocks waiting readers if enough bytes have been written
*/
int writeBuffer(int tty_id, void *buf, int len);

/*  returns ERROR/SUCCESS; copies [len] bytes from tty_id's buffer to buf.
    blocks reader if not enough bytes are available.
    if after reading, there are still bytes in buffer, those bytes are copied
    to the start of the buffer.
*/
int readBuffer(int tty_id, void *buf, int len);

#endif /*_TtyBuffer_h*/