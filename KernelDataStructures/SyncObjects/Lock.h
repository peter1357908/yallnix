// Lock.h
#ifndef _Lock_h
#define _Lock_h

#include "../Scheduler/Scheduler.h"  // PCB_t
#include "../../GeneralDataStructures/HashMap/HashMap.h"

typedef struct lock {
    /*  pointer to the PCB of the lock's current owner
		NULL means the lock has no owner at the moment
	*/
    PCB_t *ownerPcbp;
    /*  FIFO queue of processes (by PCB pointer) waiting to 
        acquire lock
    */
    q_t *waitingQ;
} lock_t;

HashMap_t *lockMap;

// used in Kernel.c
int initLockMap(void);

/*  return ERROR/SUCCESS; initializes lock and saves
    lock_id to lock_idp. 
*/ 
int initLock(int *lock_idp);

// find and returns the lock associated with the lock_id (NULL if any error)
lock_t *getLock(int lock_id);

/* delete an individual lock from the lockMap if the lock has no 
 * owner nor waiters. (the lock and the waitingQ will be freed).
 * returns SUCCESS if deletion is successful; otherwise ERROR
 */
int deleteLock(int lock_id);

#endif /* _Lock_h */
