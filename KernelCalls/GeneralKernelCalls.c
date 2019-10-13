#include <hardware.h>

int Kernel_Fork(void) {
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

int Kernel_Exec(char *filename, char **argvec) {
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
    // return currentProcess->pid
}

int KernelBrk(void *addr){
    // pagesNeeded = calculatePagesNeeded(addr)
    // for each page in pageNeeded:
        // grab pfn from FreePMList
        // initializePTE(prot, pfn)
    // currentProcess->brk = addr
    // return ERROR if error else 0
}

int KernelDelay(int clock_ticks){
    // if clock_ticks < 0:
        // return Error
    // ticksLeft = clock_ticks
    // while ticksLeft > 0:
        // wait for TRAP_CLOCK
        // decrement ticksLeft
    // return 0
}

int KernelReclaim(int id) {
    // for each map (LockMap, CVarMap, PipeMap):
        // if id in map:
            // free(id)
    // return ERROR if error else 0
}