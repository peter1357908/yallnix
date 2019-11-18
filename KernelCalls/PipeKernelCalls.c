#include "../KernelDataStructures/SyncObjects/Pipe.h"
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../GeneralDataStructures/Queue/Queue.h"
#include <hardware.h>  // TracePrintf
#include <yalnix.h>  // ERROR
#include <string.h>  // memmove
#include "../Kernel.h" // SUCCESS


int KernelPipeInit(int *pipe_idp){
    return initPipe(pipe_idp);
}

int KernelPipeRead(int pipe_id, void *buf, int len){
	TracePrintf(1, "Kernel_PipeRead() called; pipe_id = %d, buf = %x, len = %d\n", pipe_id, buf, len);
	
	if (len < 0) {
		TracePrintf(1, "the given len is negative, returning with ERROR\n");
		return ERROR;
	}
	if (len == 0) {
		TracePrintf(1, "the given len is 0, returning with 0\n");
		return SUCCESS;
	}
	
	pipe_t *pipep = getPipe(pipe_id);
	if (pipep == NULL) {
		TracePrintf(1, "getPipe(%d) returned NULL, returning with ERROR\n", pipe_id);
		return ERROR;
	}
	
	q_t *waitingQ = pipep->waitingQ;
	
	// first ensure that the queue itself exists
	if (waitingQ == NULL) return ERROR;
	
	// then, enqueue the current process
	// TOTHINK: better semantics here?
	if (enq_q(waitingQ, currPCB) == ERROR) return ERROR;

	/* block the reader if either some process is already waiting
	 * this condition won't be checked again (the next time this process
	 * wakes up, it should be the head of the queue)
	 */
	if ((PCB_t *) peek_q(waitingQ) != currPCB) {
		if (blockProcess() == ERROR) return ERROR;
	}
	
	/* --- the current process is the head of the waitingQ --- */
	
	/* block the reader until there are enough bytes to be read
	 * "if" is NOT sufficient since we don't keep track of the 
	 * len outside this function call.
	 */
    while (pipep->numBytesWritten < len) {
		TracePrintf(1, "target pipe (id = %d) does not have enough bytes (available = %d; requested len = %d), process (pid = %d) blocked\n", pipe_id, pipep->numBytesWritten, len, currPCB->pid);
        if (blockProcess() == ERROR) return ERROR;
    }
	
	// now the reader have enough bytes to read; dequeue it and read (len) bytes
	TracePrintf(1, "target tty (id = %d) now has enough bytes, process (pid = %d) now resuming in Kernel_PipeRead()\n", pipe_id, currPCB->pid);
	if (deq_q(waitingQ) == NULL) return ERROR;
	
	memmove(buf, pipep->buffer, len);
	
	int numRemainingBytes = pipep->numBytesWritten - len;
	pipep->numBytesWritten = numRemainingBytes;
	
	// if some bytes remain unread, move them to the base
	if (numRemainingBytes > 0) {
		void *baseOfRemaining = pipep->buffer + len;
		memmove(pipep->buffer, baseOfRemaining, numRemainingBytes);
		/* unblock the next waiter if there is one (whether there
		 * are enough bytes is when it runs)
		 */
		PCB_t *nextWaiterPcbp = peek_q(waitingQ);
		if (nextWaiterPcbp != NULL) {
			if (unblockProcess(nextWaiterPcbp->pid) == ERROR) return ERROR;
		}
	}
	
	return len;
}


int KernelPipeWrite(int pipe_id, void *buf, int len){
	TracePrintf(1, "Kernel_PipeWrite() called; pipe_id = %d, buf = %x, len = %d\n", pipe_id, buf, len);
	
	if (len < 0) {
		TracePrintf(1, "the given len is negative, returning with ERROR\n");
		return ERROR;
	}
	if (len == 0) {
		TracePrintf(1, "the given len is 0, returning with 0\n");
		return SUCCESS;
	}
	
	pipe_t *pipep = getPipe(pipe_id);
	if (pipep == NULL) {
		TracePrintf(1, "getPipe(%d) returned NULL, returning with ERROR\n", pipe_id);
		return ERROR;
	}
	if (pipep->numBytesWritten + len > PIPE_MAX_BYTES) {
		TracePrintf(1, "trying to write more than the buffer can hold (written = %d, len = %d, PIPE_MAX_BYTES = %d), returning with ERROR\n", pipep->numBytesWritten, len, PIPE_MAX_BYTES);
		return ERROR;
	}
	
	// the specified len is valid, append to the existing bytes
	void *nextFreeByte = pipep->buffer + pipep->numBytesWritten;
	memmove(nextFreeByte, buf, len);
	
	pipep->numBytesWritten += len;
	
	/* unblock the next waiter if there is one (whether there
	 * are enough bytes is checked when it runs)
	 */
	PCB_t *nextWaiterPcbp = peek_q(pipep->waitingQ);
	if (nextWaiterPcbp != NULL) {
		if (unblockProcess(nextWaiterPcbp->pid) == ERROR) return ERROR;
	}
		
	return len;
	
}