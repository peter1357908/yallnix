#include "../KernelDataStructures/Lock/Lock.h"
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../Kernel.h"
#include <hardware.h>
#include <yalnix.h>

int Kernel_LockInit(int *lock_idp) {
    return initLock(lock_idp);
}

int Kernel_Acquire(int lock_id) {
    // while lock isn't free, push lock onto lockQ & block it
    while (isLockFree(lock_id) == 0) {
        if (pushLockQ(lock_id, currPCB->pid) == ERROR || \
            blockProcess() == ERROR) return ERROR;
    }

    // set curr process as owner
    if (setLockOwner(lock_id, currPCB->pid) == ERROR) return ERROR;

    return SUCCESS;
}

int Kernel_Release(int lock_id) { 
    releaseLock(lock_id);

    // if processes in LockQ, pop one & unblock it
    if (isLockQEmpty(lock_id) == 0) {
        int pid;
        if (popLockQ(lock_id, &pid) == ERROR) return ERROR;
        unblockProcess(pid);
	}

    return SUCCESS;
}
 