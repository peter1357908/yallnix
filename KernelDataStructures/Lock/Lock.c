#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/Hashmap/Hashmap.h"
#include <yalnix.h>
#include "../../Kernel.h"
#include "./Lock.h"

/* ---------- only visible to Lock.c ------------ */

#define LOCK_MAX 50

typedef struct lock {
    /*  process pid that currently has the lock
    */
    int ownerPid;
    /*  FIFO queue of processes (by pid) waiting to 
        acquire lock
    */
    q_t *lockQ; 
} lock_t;

HashMap_t *lockMap = HashMap_new(LOCK_MAX);
int lockCount = 0;

lock_t *getLock(int lock_id) {
    return (lock_t *) HashMap_find(lockMap, (char *) lock_id);
}


/* ------------------------- global ---------------------------*/
int initLock(int *lock_idp) {
    // initialize lock
    lock_t *lock;
    if ((lock = (lock_t *) malloc(sizeof(lock_t))) == NULL || \
		((lock->lockQ  = make_q())) == NULL ) {
		return ERROR;
	}

    // store lock in LockMap
    if (HashMap_insert(lockMap, (char *) lockCount, (void *) lock) == ERROR) return ERROR;

    // save lock_id in lock_idp & increment lockCounnt
    lock_idp = lockCount++;

    return SUCCESS;
}

int isLockFree(int lock_id) {
    lock_t *lock;
    if ((lock = getLock(lock_id)) == NULL) return ERROR;
    if (lock->ownerPid == NULL) {
        return 0;
    }
    return 1;
}

int releaseLock(int lock_id) {
    lock_t *lock;
    if ((lock = getLock(lock_id)) == NULL) return ERROR;
    lock->ownerPid = NULL;
}

int setLockOwner(int lock_id, int pid) {
    lock_t *lock;
    if ((lock = getLock(lock_id)) == NULL) return ERROR;
    lock->ownerPid = pid;
    return SUCCESS;
}

int isLockQEmpty(int lock_id) {
    lock_t *lock;
    if ((lock = getLock(lock_id)) == NULL) return ERROR;

    if (peek_q(lock->lockQ) == NULL) {
        return 1;
	}
    return 0;
}

int popLockQ(int lock_id, int *pidP) {
    lock_t *lock;
    if ((lock = getLock(lock_id)) == NULL) return ERROR;
    pidP = (int) deq_q(lock->lockQ);
    return SUCCESS;
}

int pushLockQ(int lock_id, int pid) {
    lock_t *lock;
    if ((lock = getLock(lock_id)) == NULL) return ERROR;

    if (enq_q(lock->lockQ, pid) == ERROR) return ERROR;
    return SUCCESS;
}
