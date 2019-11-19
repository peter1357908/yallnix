#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/HashMap/HashMap.h"
#include "../Scheduler/Scheduler.h"  // for nextSyncId
#include <yalnix.h> // for ERROR definition
#include "../../Kernel.h" // for SUCCESS definition
#include "Lock.h" // lock_t and lockMap

#define LOCK_MAP_HASH_BUCKETS 50

void initLockMap() {
    lockMap = HashMap_new(LOCK_MAP_HASH_BUCKETS);
}

int initLock(int *lock_idp) {
    lock_t *lockp = (lock_t *) malloc(sizeof(lock_t));
	
	if (lockp == NULL) {
		TracePrintf(1, "initLock: malloc failed for lock");
		return ERROR;
	}

	// return ERROR if malloc'ing for the lock or the waitingQ fails
    if (lockp->waitingQ = make_q() == NULL) return ERROR;

    lockp->ownerPcbp = NULL;

    // get an ID for the lock and increment the sync object count
    *lock_idp = nextSyncId++;

    // store lock in lockMap
    return HashMap_insert(lockMap, *lock_idp, lockp);
}

lock_t *getLock(int lock_id) {
	return (lock_t *) HashMap_find(lockMap, lock_id);
}

int deleteLock(int lock_id) {
	lock_t *lockp = (lock_t *) HashMap_remove(lockMap, lock_id);

	if (lockp == NULL) {
		TracePrintf(1, "deleteLock: lock is null");
		return ERROR;
	}

	// return ERROR if malloc'ing for the lock or the waitingQ fails
    if (lockp->waitingQ = make_q() == NULL) return ERROR;
	
	// if it has an owner or some waiters, return ERROR;
	if (lockp->ownerPcbp != NULL || peek_q(lockp->waitingQ) != NULL) {
		return ERROR;
	}
	
	// otherwise, the lock is safe to be deleted
	delete_q(lockp->waitingQ, NULL);
	free(lockp);
	
	return SUCCESS;
}