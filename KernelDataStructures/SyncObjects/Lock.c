#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/HashMap/HashMap.h"
#include "../Scheduler/Scheduler.h"  // for nextSyncId
#include <yalnix.h> // for ERROR definition
#include "Lock.h" // lock_t and lockMap

#define LOCK_MAP_HASH_BUCKETS 50

void initLockMap() {
    lockMap = HashMap_new(LOCK_MAP_HASH_BUCKETS);
}

int initLock(int *lock_idp) {
    lock_t *lockp = (lock_t *) malloc(sizeof(lock_t));
	
	// return ERROR if malloc'ing for the lock or the waitingQ fails
    if (lockp == NULL || (lockp->waitingQ = make_q()) == NULL) {
		return ERROR;
	}
    lockp->ownerPcbp = NULL;

    // get an ID for the lock and increment the sync object count
    *lock_idp = nextSyncId++;

    // store lock in lockMap
    return HashMap_insert(lockMap, *lock_idp, lockp);
}

lock_t *getLock(int lock_id) {
	return (lock_t *) HashMap_find(lockMap, lock_id);
}
