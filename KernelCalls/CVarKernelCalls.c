#include "../KernelDataStructures/Cvar/Cvar.h"
#include "../KernelDataStructures/Lock/Lock.h"
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include <yalnix.h>
#include "../Kernel.h"

int Kernel_CvarInit(int *cvar_idp) {
    return initCvar(cvar_idp);
}

int Kernel_CvarWait(int cvar_id, int lock_id){
    addProcessToCvarQ(cvar_id, currPCB->pid);
    if (blockProcess() == ERROR) return ERROR;
    return SUCCESS;
}

int Kernel_CvarSignal(int cvar_id){
    // pop process from Cvar queue & retrive pid & lock
    cvar_t *cvar;
    if (popCvarQ(cvar_id, &cvar) == ERROR) return ERROR;
    int lock_id = cvar->lock_id;
    int pid = cvar->pid;
    pushLockQ(lock_id, pid);
    return SUCCESS;
}

int Kernel_CvarBroadcast(int cvar_id){
    while (isCvarEmpty(cvar_id) == 0) {
        int signalStatus = CvarSignal(cvar_id);
        if (signalStatus == ERROR) return ERROR;
    }
    return SUCCESS;
}