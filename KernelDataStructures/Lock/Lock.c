#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/Hashmap/Hashmap.h"
#include <yalnix.h>
#include "../../Kernel.h"
#include "./Lock.h"

/* ---------- only visible to Lock.c ------------ */

#define FREE -1
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

HashMap_t *lockMap;
int lockCount = 0;

lock_t *getLock(int lock_id) {
    char lock_id_char = (char) lock_id;
    return (lock_t *) HashMap_find(lockMap, &lock_id_char);
}


/* ------------------------- global ---------------------------*/
void initLockMap() {
    lockMap = HashMap_new(LOCK_MAX);
}

int initLock(int *lock_idp) {
    // initialize lock
    lock_t *lock;
    if ((lock = (lock_t *) malloc(sizeof(lock_t))) == NULL || \
		((lock->lockQ  = make_q())) == NULL ) {
		return ERROR;
	}
    lock->ownerPid = FREE;

    // get char from lockCount
    char lockCountChar = (char) lockCount;

    // store lock in LockMap
    if (HashMap_insert(lockMap, &lockCountChar, (void *) lock) == ERROR) return ERROR;

    // save lock_id in lock_idp & increment lockCounnt
    *lock_idp = lockCount++;

    return SUCCESS;
}

int isLockFree(int lock_id, int *isFree) {
    lock_t *lock;
    if ((lock = getLock(lock_id)) == NULL) return ERROR;
    if (lock->ownerPid == FREE) {
        *isFree = 1;
    } else {
        *isFree = 0;
    }
    return SUCCESS;
}

int releaseLock(int lock_id) {
    lock_t *lock;
    if ((lock = getLock(lock_id)) == NULL) return ERROR;
    lock->ownerPid = FREE;
    return SUCCESS;
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
    int pid = (int) deq_q(lock->lockQ);
    *pidP = pid; 
    return SUCCESS;
}

int pushLockQ(int lock_id, int pid) {
    lock_t *lock;
    if ((lock = getLock(lock_id)) == NULL) return ERROR;

    if (enq_q(lock->lockQ, (void *) pid) == ERROR) return ERROR;
    return SUCCESS;
}

int lockAcquire(int lock_id, int pid) {
    // while lock isn't free, push lock onto lockQ & block it
    // TOTHINK: might get away with an "if" here
    int isFree;
    if (isLockFree(lock_id, &isFree) == ERROR) return ERROR;
    while (isFree == 0) {
        if (pushLockQ(lock_id, pid) == ERROR || \
            blockProcess() == ERROR) return ERROR;
        if (isLockFree(lock_id, &isFree) == ERROR) return ERROR;
    }

    // set curr process as owner
    if (setLockOwner(lock_id, pid) == ERROR) return ERROR;
    return SUCCESS;
}
