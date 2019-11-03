// assumes that only one process can be running at a time

#ifndef _Scheduler_h
#define _Scheduler_h

#include <hardware.h>

typedef struct PCB {
    int pid;
    void *brk;
    UserContext *uctxt;
    KernelContext *kctxt;
    unsigned int numChildren;
    struct PCB *parent; // -- currently only using this for KernelExit/KernelWait
	struct pte *r1PageTable;
    u_long stackPfns[KERNEL_STACK_MAXSIZE / PAGESIZE];
    unsigned int numRemainingDelayTicks;
} PCB_t;


PCB_t *currPCB;
PCB_t *idlePCB;
PCB_t *initPCB;


/* --------- the following are for initialization --------- */

// the initialized process will be enqueued in readyQ
int initProcess(PCB_t **pcb);


// a special case of initProcess, only used for... the Init process.
int initInitProcess(struct pte *initR1PageTable);


/* saves the kernel state necessary to make a new process run: the KernelContext
 * and the kernel stack.
 */
KernelContext *getStarterKernelState(KernelContext *currKctxt, void *nil0, void *nil1);


/* --------- the following are for scheduling --------- */

// initializes the scheduler (initializes the queues and the nextPid)
int initScheduler(void);


// return ERROR/0; kicks the current process into readyQ and runs another ready process
int kickProcess(void);


/* returns ERROR/0; kicks the current process into readyQ and runs the target process
 * if the process is ready. If the target process is not ready, do kickProcess().
 */
int runProcess(int pid);


// returns ERROR/0; moves the current process into zombieQ and runs another ready process
int zombifyProcess(void);


// returns ERROR/0; blocks the current process and runs another ready process
int blockProcess(void);


// returns ERROR/0; moves a process from blockedQ to readyQ (does nothing else)
int unblockProcess(int pid);


// for use with KernelContextSwitch
KernelContext *MyKCS(KernelContext *kc_in, void *currPcbP , void *nextPcbP);


/* delete_process() frees a PCB and all content within, including its uctxt,
 * kctxt, and r1PageTable. Also frees the allocated physical memory. Depends
 * on the FrameList module. Assumes the caller is outside the original process
 * and thus does not flush the TLB.
 *
 * Used along with the Queue module, and thus the signature.
 */
void delete_process(void *pcbp_item);


#endif /*_Scheduler_h*/