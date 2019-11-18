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
void initPipeMap(void);

/*  return ERROR/SUCCESS; initializes lock and saves
    lock_id to lock_idp. 
*/ 
int initPipe(int *pipe_idp);

// find and returns the lock associated with the lock_id (NULL if any error)
pipe_t *getPipe(int pipe_id);

#endif /* _Pipe_h */
