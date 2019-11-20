#include "../KernelDataStructures/SyncObjects/Cvar.h"
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "KernelCalls.h" // for KernelRelease and KernelAcquire
#include "../Kernel.h"  // SUCCESS
#include <hardware.h>  // TracePrintf
#include <yalnix.h>  // ERROR


int KernelCvarInit(int *cvar_idp) {
	TracePrintf(1, "KernelCvarInit() starting... \n");
    return initCvar(cvar_idp);
}

int KernelCvarWait(int cvar_id, int lock_id){
    q_t *cvarQ = getCvar(cvar_id);
     if (cvarQ == NULL) {
        TracePrintf(1, "KernelCvarWait: cvar %d is null\n", cvar_id);
        return ERROR;
    }
	
	
	// enqueue the currPCB in the cvarQ
	// dequeuing happens during Signal()/Broadcast()
    if (enq_q(cvarQ, currPCB) == ERROR) return ERROR;
	
	// release the lock and block the process
    KernelRelease(lock_id);
    if (blockProcess() == ERROR) return ERROR;
	
	/* --- woke up and running; reacquire the lock --- */
	
	/* the Lock module handles further possible blocking 
	 * due to lock being already acquired by some other process
	 */
	return KernelAcquire(lock_id);
}

int KernelCvarSignal(int cvar_id){
	q_t *cvarQ = getCvar(cvar_id);
    if (cvarQ == NULL) {
        TracePrintf(1, "KernelCvarSignal: cvar %d is null\n", cvar_id);
        return ERROR;
    }
	
    // pop one waiting process from cvarQ & retrive pid & lock
    PCB_t *waitingPcbp = (PCB_t *) deq_q(cvarQ);
	
	// since cvarQ is not NULL, waitingPcbp being NULL means there is no waiter
    if (waitingPcbp == NULL) return SUCCESS;
	
	// waitingPcbp is not NULL, then wake the process up
    return unblockProcess(waitingPcbp->pid);
}

int KernelCvarBroadcast(int cvar_id){
	q_t *cvarQ = getCvar(cvar_id);
    if (cvarQ == NULL) {
        TracePrintf(1, "KernelCvarBroadcast: cvar %d is null\n", cvar_id);
        return ERROR;
    }
	
	// keep dequeuing until the cvarQ is empty
    PCB_t *waitingPcbp;
    while ((waitingPcbp = (PCB_t *) deq_q(cvarQ)) != NULL) {
        if (unblockProcess(waitingPcbp->pid) == ERROR) {
			return ERROR;
		}
    }
    return SUCCESS;
}