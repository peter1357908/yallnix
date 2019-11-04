/* assumes that only one process can be running at a time
 * assumes that the idle process is either running or in the readyQ
 */

#ifndef _Scheduler_h
#define _Scheduler_h

#include <hardware.h>
#include "../../GeneralDataStructures/Queue/Queue.h"

typedef struct PCB {
    int pid;
    void *brk;
    UserContext *uctxt;
    KernelContext *kctxt;
    unsigned int numChildren;
    struct PCB *parent; // only using this for KernelExit/KernelWait
	q_t *zombieQ;  // only used for KernelExit/KernelWait
	struct pte *r1PageTable;
    int numRemainingDelayTicks;
	u_long stackPfns[KERNEL_STACK_MAXSIZE / PAGESIZE];
} PCB_t;

typedef struct zombie {
	int pid;
	int exit_status;
} zombie_t;


PCB_t *currPCB;

int initPid;


/* --------- the following are for initialization --------- */

// the initialized process will be enqueued in readyQ
int initProcess(PCB_t **pcb);


// a special case of initProcess, only used for... the Init process.
int initInitProcess(struct pte *initR1PageTable, PCB_t **initPcbpp);


/* saves the kernel state necessary to make a new process run: the KernelContext
 * and the kernel stack.
 */
KernelContext *getStarterKernelState(KernelContext *currKctxt, void *nil0, void *nil1);


/* --------- the following are for scheduling --------- */

// return ERROR/0; initializes the scheduler (initializes the queues and the nextPid)
int initScheduler(void);


/* return ERROR/0; enqueue some process in sleepingQ and setting their 
 * numRemainingDelayTicks. Do kickProcess().
 */
int sleepProcess(int numRemainingDelayTicks);


// return ERROR/0; ticks down all sleepers and move to readyQ when necessary
int tickDownSleepers(void);


/* return ERROR/0; kicks the current process into readyQ and runs another ready process.
 * Theoretically, if the queue was empty, the same process is run again (which is impossible
 * for Yalnix, since the readyQ always has init or idle, and when init exits, Yalnix halts).
 */
int kickProcess(void);


/* returns ERROR/0; kicks the current process into readyQ and runs the target process
 * if the process is ready. If the target process is not ready, just run another process
 * like kickProcess().
 */
int runProcess(int pid);


// return ERROR/0; called after exec.
int execProcess(void);


/* returns ERROR/0; moves the current process into its parent's zombieQ and runs another 
 * ready process. If the current process has no parent, just delete the process and run
 * another process like kickProcess().
 */
int zombifyProcess(int exit_status);


// returns ERROR/0; blocks the current process and runs another ready process
int blockProcess(void);


// returns ERROR/0; moves a process from blockedQ to readyQ (does nothing else)
int unblockProcess(int pid);


/* when calling with currPcbP == NULL, assume that the currPCB is deleted already.
 */
KernelContext *switchBetween(KernelContext *currKctxt, void *currPcbP, void *nextPcbP);


/* delete_process() frees a PCB and all content within, including its uctxt,
 * kctxt, and r1PageTable. Also frees the allocated physical memory. Depends
 * on the FrameList module. Assumes that the call is followed by a "currPcbP=NULL"
 * KernelContextSwitch, and thus does not flush the TLB.
 *
 * Used along with the Queue module, and thus the signature.
 */
void delete_process(void *pcbp_item);


#endif /*_Scheduler_h*/