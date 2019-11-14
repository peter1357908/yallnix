#include "../KernelDataStructures/Lock/Lock.h"
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../GeneralDataStructures/Queue/Queue.h"
#include "../Kernel.h"
#include <hardware.h>
#include <yalnix.h>

int KernelLockInit(int *lock_idp) {
    TracePrintf(1, "KernelLockInit() starting... \n");
    return initLock(lock_idp);
}

int KernelAcquire(int lock_id) {
    return lockAcquire(lock_id, currPCB->pid);
}

int KernelRelease(int lock_id) { 
    releaseLock(lock_id);

    // if processes in LockQ, pop one & unblock it
    if (isLockQEmpty(lock_id) == 0) {
        int pid;
        if (popLockQ(lock_id, &pid) == ERROR) return ERROR;
        unblockProcess(pid);
	}

    return SUCCESS;
}
 