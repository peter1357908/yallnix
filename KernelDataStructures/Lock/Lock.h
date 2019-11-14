// Lock.h
#ifndef _Lock_h
#define _Lock_h

#include "../../GeneralDataStructures/Queue/Queue.h"

/* initialize LockMap
*/
void initLockMap(void );

/*  return ERROR/SUCCESS; initializes lock and saves
    lock_id to lock_idp. 
*/ 
int initLock(int *lock_idp);

/*  return ERROR/SUCCESS;
    isFree = 1 if lock is free (no other process currently has lock)
    isFree 0 if lock is not free
*/
int isLockFree(int lock_id, int *isFree);

/*  return ERROR/SUCCESS; release's lock_id's lock by 
    setting owner to NULL
*/
int releaseLock(int lock_id);

/* return ERROR/SUCCESS; set's lock_id's lock->owner
*/
int setLockOwner(int lock_id, int pid);

/*  return 1 if lockQ is empty (no processes waiting to acquire)
    return 0 if lockQ has processes waiting to acquire it
*/
int isLockQEmpty(int lock_id);

/*  return ERROR/SUCCESS; pushes process (by pid) into 
    lock_id's queue
*/
int pushLockQ(int lock_id, int pid);

/*  return ERROR/SUCCESS; pops a process (by pid) from 
    the lock queue & assigns to pidP
*/
int popLockQ(int lock_id, int *pidP);

/*  returns ERROR/SUCCESS; acquires lock_id for process pid
    (we modularize this because Cvar & Lock use this method)
*/
int lockAcquire(int lock_id, int pid);

#endif /* _Lock_h */

