#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/HashMap/HashMap.h"
#include "../Scheduler/Scheduler.h"  // for nextSyncId
#include <yalnix.h> // for ERROR definition
#include "../../Kernel.h" // for SUCCESS definition
#include "Pipe.h"  // PIPE_MAX_BYTES, pipe_t and pipeMap

#define PIPE_MAP_HASH_BUCKETS 50

void initPipeMap() {
    pipeMap = HashMap_new(PIPE_MAP_HASH_BUCKETS);
}

int initPipe(int *pipe_idp) {
    pipe_t *pipep = (pipe_t *) malloc(sizeof(pipe_t));
	
	// return ERROR if malloc fails
    if ( pipep == NULL || \
		(pipep->buffer = (void *) malloc(PIPE_MAX_BYTES)) == NULL || \
		(pipep->waitingQ = make_q()) == NULL ) {
		return ERROR;
	}
    pipep->numBytesWritten = 0;

    // get an ID for the pipe and increment the sync object count
    *pipe_idp = nextSyncId++;

    // store pipe in pipeMap
    return HashMap_insert(pipeMap, *pipe_idp, pipep);
}

pipe_t *getPipe(int pipe_id) {
	return (pipe_t *) HashMap_find(pipeMap, pipe_id);
}

int deletePipe(int pipe_id) {
	pipe_t *pipep = (pipe_t *) HashMap_remove(pipeMap, pipe_id);
	
	// if the pipep, its buffer, or its waitingQ is NULL, return ERROR
	if (pipep == NULL || \
		pipep->buffer == NULL || \
		pipep->waitingQ == NULL) {
		return ERROR;
	}
	
	// if it has some waiters, return ERROR;
	if (peek_q(pipep->waitingQ) != NULL) {
		return ERROR;
	}
	
	// otherwise, the pipe is safe to be deleted
	free(pipep->buffer);
	delete_q(pipep->waitingQ, NULL);
	free(pipep);
	
	return SUCCESS;
}
