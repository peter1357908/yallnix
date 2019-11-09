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
 * numRemainingDelayTicks. Do kickProcess(). Do nothing if input integer is non-positive.
 */
int sleepProcess(int numRemainingDelayTicks);


// return ERROR/0; ticks down all sleepers and move to readyQ when necessary
int tickDownSleepers(void);


/* return ERROR/0; kicks the current process into readyQ and runs another ready process.
 * If the readyQ was empty, does nothing (for example, when init is the only other 
 * process and sleeps, then idle gets the CPU repeatedly).
 */
int kickProcess(void);


/* returns ERROR/0; kicks the current process into readyQ and runs the target process
 * if the process is ready. If the target process is not ready, just run another process
 * like kickProcess(). If the readyQ was empty, does nothing (like in kickProcess()).
 * If the pid given is the same as the currPCB->pid, does nothing.
 */
int runProcess(int pid);


// return ERROR/0; called in KernelExec().
int execProcess(void);


/* return ERROR/0; called by the end of KernelFork() (after allocating 
 * kctxt for the child, filling up the child's r1PageTable, etc.)
 *
 * currently blocks the parent and runs the child process next (TOTHINK: this
 * behavior also depends on forkTo(); do we really want to switch to child?)
 */
int forkProcess(int pid);


/* returns ERROR/0; moves the current process into its parent's zombieQ and runs another 
 * ready process. If the current process has no parent, just delete the process and run
 * another process like kickProcess().
 * 
 * assumes that if the parent attribute is not NULL, then the parent is still in operation
 * this invariant is maintained by exitProcess() itself, currently.
 */
int exitProcess(int exit_status);


// returns ERROR/0; blocks the current process and runs another ready process
int blockProcess(void);


/* returns ERROR/0; moves a process from blockedQ to readyQ (does nothing else)
 * if the process is not in the blockedQ, nothing happens (returns 0).
 */
int unblockProcess(int pid);


#endif /*_Scheduler_h*/