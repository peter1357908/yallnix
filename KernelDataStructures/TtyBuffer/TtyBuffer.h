#ifndef _TtyBuffer_h
#define _TtyBuffer_h

/* returns ERROR/SUCCESS; initializes tty buffers
 */
int initBuffers(void);

/*  returns ERROR/SUCCESS; store the ready-to-be-read line in our own buffer;
    only wakes a reader up if the line read is not just an EOF (i.e. wakes a
	reader up if received a blank line, which is not an EOF).
	
	Currently overwrites any remaining un-read bytes.
*/
int writeBuffer(int tty_id);

/*  returns ERROR/number of bytes written; copies at most [len] bytes from 
    tty_id's buffer to buf. If after reading, there are still bytes in the
	buffer, those bytes are copied to the beginning of the buffer.
	
	Assumes that the given len is positive (this is ensured by KernelTtyRead())
*/
int readBuffer(int tty_id, void *buf, int len);

#endif /*_TtyBuffer_h*/