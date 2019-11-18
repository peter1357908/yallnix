// Pipe.h
#ifndef _Pipe_h
#define _Pipe_h

#include "../Scheduler/Scheduler.h"  // PCB_t
#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/HashMap/HashMap.h"

/* manual page 41: "(See the header files for the length
 * of the pipe’s internal buffer.)", but... where is it??
 */
#define PIPE_MAX_BYTES        (TERMINAL_MAX_LINE / 2)

typedef struct pipe {
    // the base address of the buffer (should be malloc'd in kernel)
	void *buffer;
	
	// how many bytes are available (to be read)
    int numBytesWritten;
	
	// processes (by PCB) that are waiting to read from the pipe
	q_t *waitingQ;
} pipe_t;

HashMap_t *pipeMap;

// used in Kernel.c
void initLockMap(void);

/*  return ERROR/SUCCESS; initializes lock and saves
    lock_id to lock_idp. 
*/ 
int initLock(int *lock_idp);

// find and returns the lock associated with the lock_id (NULL if any error)
pipe_t *getPipe(int pipe_id);

/*  returns ERROR/SUCCESS; store specified bytes in the target pipe's buffer;
    only wakes a reader up if the line read is not just an EOF (i.e. wakes a
	reader up if received a blank line, which is not an EOF).
	
	Currently overwrites any remaining un-read bytes.
*/
int writePipe(int tty_id, void *buf, int len);

/*  returns ERROR/number of bytes written; copies at most [len] bytes from 
    tty_id's buffer to buf. If after reading, there are still bytes in the
	buffer, those bytes are copied to the beginning of the buffer.
	
	Assumes that the given len is positive (this is ensured by KernelTtyRead())
*/
int readPipe(int tty_id, void *buf, int len);

#endif /* _Pipe_h */
