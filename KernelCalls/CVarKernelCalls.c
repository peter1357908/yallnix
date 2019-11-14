#include "../KernelDataStructures/Cvar/Cvar.h"
#include "../KernelDataStructures/Lock/Lock.h"
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include <yalnix.h>
#include "../Kernel.h"

int KernelCvarInit(int *cvar_idp) {
    return initCvar(cvar_idp);
}

int KernelCvarWait(int cvar_id, int lock_id){
    if (pushCvarQ(cvar_id, lock_id, currPCB->pid) == ERROR) return ERROR;
    KernelRelease(lock_id);
    if (blockProcess() == ERROR) return ERROR;
    return SUCCESS;
}

int KernelCvarSignal(int cvar_id){
    // pop process from Cvar queue & retrive pid & lock
    cvar_t *cvar;
    if (popCvarQ(cvar_id, &cvar) == ERROR) return ERROR;
    int lock_id = cvar->lock_id;
    int pid = cvar->pid;

    if (lockAcquire(lock_id, pid) == ERROR) return ERROR;
    unblockProcess(pid);
    return SUCCESS;
}

int KernelCvarBroadcast(int cvar_id){
    while (isCvarEmpty(cvar_id) == 0) {
        int signalStatus = KernelCvarSignal(cvar_id);
        if (signalStatus == ERROR) return ERROR;
    }
    return SUCCESS;
}