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

int nextSyncId;

/* --------- the following are for initialization --------- */

// initializes PCB
int initPCB(PCB_t **pcb);

// calls initPCB & places process in readyQ
int initProcess(PCB_t **pcb);

int initChildProcess(PCB_t **pcb);

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
int forkProcess(PCB_t *childPCB);


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

/*
    =========== TTY STUFF ===========
*/

/*  return 1 if no other process is currently transmitting to tty_id
    else return 0
*/
int isTtyTransmitAvailable(int tty_id);

/*  return ERROR/0; blocks current process by placing it in tty_id's transmittingQ.
    Then, runs another ready process.
*/
int blockTransmitter(int tty_id);

/*  return ERROR/0; makes the target tty available again, then pops process from 
	tty_id's transmitting queue and moves it to readyQ.
*/
int unblockTransmitter(int tty_id);

/*  return ERROR/0; blocks process that is currently transmitting to tty_id 
    by placing it in currTransmitters[tty_id]
*/
int waitTransmitter(int tty_id);

/*  return ERROR/0; kicks the current process into readyQ,
    removes process from currTransmitters[tty_id], and runs that process
*/
int signalTransmitter(int tty_id);

/*  return ERROR/0; blocks current process by placing it in tty_id's readingQ.
*/
int blockReader(int tty_id);

/*  return ERROR/0; if there is someone in the tty_id's readingQ, move it to
	the readyQ instead.
*/
int unblockReader(int tty_id);

#endif /*_Scheduler_h*/
