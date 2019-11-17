#include "../GeneralDataStructures/Queue/Queue.h"
#include "../GeneralDataStructures/HashMap/HashMap.h"
#include "../KernelDataStructures/SyncObjects/Lock.h"
#include "../KernelDataStructures/Scheduler/Scheduler.h"  // for PCB_t, and currPCB
#include "../Kernel.h"  // SUCCESS
#include <hardware.h>  // TracePrintf
#include <yalnix.h>  // ERROR


int KernelLockInit(int *lock_idp) {
    TracePrintf(1, "KernelLockInit() starting... \n");
    return initLock(lock_idp);
}

int KernelAcquire(int lock_id) {
	// get the lock first
    lock_t *lockp = getLock(lock_id);
    if (lockp == NULL) return ERROR;
	
    /* while the lock isn't free, mark the process as 
	 * waiting and block it
	 */
    // TOTHINK: might get away with an "if" here
    while (lockp->ownerPcbp != NULL) {
        if (enq_q(lockp->waitingQ, currPCB) == ERROR || \
			blockProcess() == ERROR) {
			return ERROR;
		}
    }

    // set curr process as owner
    lockp->ownerPcbp = currPCB;
	
    return SUCCESS;
}

int KernelRelease(int lock_id) { 
    // get the lock first
    lock_t *lockp = getLock(lock_id);
    // we COULD assume that currPCB is never NULL, but...
    if (lockp == NULL || \
		lockp->ownerPcbp == NULL || \
		lockp->ownerPcbp != currPCB || \
		lockp->waitingQ == NULL) {
		return ERROR;
	}
	
	/* the lock exists, is owned by the currPCB 
	 * and has a valid waitingQ, so we release the lock
	 */
	lockp->ownerPcbp = NULL;

    // if there are waiters, unblock one and lift its waiting status
    if (peek_q(lockp->waitingQ) != NULL) {
        PCB_t *waiterPcbp = deq_q(lockp->waitingQ);
        unblockProcess(waiterPcbp->pid);
	}

    return SUCCESS;
}
 