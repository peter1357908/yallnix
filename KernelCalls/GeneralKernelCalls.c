#include <hardware.h>
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../Kernel.h"
#include <yalnix.h>

int KernelFork(void) {
    // initialize new PCB for child (with new pid)
    // copy Parent's PCB variables:
        // region1Base, region1Limit, KernelStackBase, uctxt, kctxt
    // increment Parent's PCB->numChildren
    // set Child's PCB->parent to Parent's PCB->pid

    // initialize child's page table
    // for each Parent's PTE in Region 1 & Kernel Stack
        // copy PTE to child's page table
        // newFrame = grab pfn from FreePMList
        // set child PTE pfn to newFrame
        // copy memory contents from parent page to child page
    
    // tell scheduler child is ready
}

void KernelExit(int status) {
    
}

int KernelExec(char *filename, char **argvec) {
    // delete page table
    // remove currentProcess's PCB from blockedMap (it should be there)
    // delete old PCB
    // initialize new PCB
    // initialize new page table
    // load program filename into page table
    // load arguments into page table
    // set UserContext & KernelContext (should be taken care of by PCB intialization)
    // tell scheduler program is Ready
}
    
void KernelExit(int status) {
    // if currentProcess->Parent != NULL
        // grab parentPCB
        // grab currentProcess PID
        // remove currentProcess from runningMap
        // add (PID, status) to Parent's ZombieQueue
    TracePrintf(1, "Exit\n");
}

int KernelWait(int *status_ptr) {
    /*
     *  if currentProcess -> numChildren equals 0:
     *      throw Error
     *  if zombie queue is empty:
     *  move currentProcess from running to blocked
     *  (process is now running * zombie queue has children)
     *  (pid, status) = zombieQueue.pop()
     *  &status_ptr = status
     *  return pid
     */
}

int KernelGetPid(void) {
    return currPCB->pid; 
}

// assumes that brk was in correct position (e.g. below: valid; above: invalid, etc.)
int KernelBrk(void *addr) {
    void *brk = currPCB->brk;
    struct pte *r1BasePtep = currPCB->r1PageTable;
    struct pte *targetPtep;
    int currAddr;
    int vpn1 = (VMEM_1_BASE>>PAGESHIFT); // first page in VMEM_0

    // if addr lower than brk, invalidate pages and free frames accordingly
    if ((int) addr < (int) brk) {
        for (currAddr = (int) addr; currAddr < (int) brk; currAddr += PAGESIZE) {
            int vpn = (currAddr>>PAGESHIFT);
            targetPtep = r1BasePtep + vpn - vpn1;
            freeFrame(FrameList, numFrames, targetPtep->pfn);
            invalidatePageTableEntry(targetPtep);
        }
    }

    // if addr higher than brk, validate pages and allocate frames accordingly
    else {
        for (currAddr = (int) brk; currAddr < (int) addr; currAddr += PAGESIZE) {
            int vpn = (currAddr>>PAGESHIFT);
            frame_t *newFrame;
            if (getFrame(FrameList, numFrames, &newFrame) == ERROR) {
                return ERROR;
            }
            targetPtep = r1BasePtep + vpn - vpn1;
            setPageTableEntry(targetPtep, 1, (PROT_READ|PROT_WRITE), (int) (newFrame->addr)>>PAGESHIFT);
        }
    }
    currPCB->brk = addr;
    return 0;
}

int KernelDelay(int clock_ticks){
    if (clock_ticks < 0) {
        return ERROR;
    }
    currPCB->numRemainingDelayTicks = clock_ticks;
    
    if (currPCB->pid == idlePCB->pid) {
        if (KernelContextSwitch(MyKCS, idlePCB, initPCB) == ERROR) { 
            return ERROR;
        }
    } 
    else if (currPCB->pid == initPCB->pid) {
        if (KernelContextSwitch(MyKCS, initPCB, idlePCB) == ERROR) { 
            return ERROR;
        }
    }
    return 0;
}

int KernelReclaim(int id) {
    // for each map (LockMap, CVarMap, PipeMap):
        // if id in map:
            // free(id)
    // return ERROR if error else 0
}