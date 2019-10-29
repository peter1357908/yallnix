// Scheduler
    // scheduler will manage processes and moving them 
    // between states
    // implementation TBD

    // PCB_T * currentProcess
    // PCB_T * idleProcess -- should always be in readyMap

    // createProcess()
        // calls generateProcessID()

    // generateProcessID()
        // increment processCount
        // return processCount

    // int processCount 
    
    // runningMap (HashMap: pid -> PCB_T *)
    // readyMap (HashMap: pid -> PCB_T *) 
    // blockedMap (HashMap: pid -> PCB_T *)

#ifndef _Scheduler_h
#define _Scheduler_h
#include <hardware.h>

typedef struct PCB {
    int pid;
    void *brk;
    UserContext *uctxt;
    KernelContext *kctxt;
    // ZombieQueue *zq;
    unsigned int numChildren;
    struct PCB *parent; // -- currently only using this for KernelExit/KernelWait
	struct pte *r1PageTable;
    u_long stackPfns[KERNEL_STACK_MAXSIZE / PAGESIZE];
    unsigned int numRemainingDelayTicks;
} PCB_t;

PCB_t *currPCB;
PCB_t *idlePCB;
PCB_t *initPCB;

int initProcess(PCB_t **);


KernelContext *getKernelContext(KernelContext *currKctxt, void *kctxt, void *nextPcbP);
KernelContext *MyKCS(KernelContext *kc_in, void *curr_pcb_b, void *next_pcb_p);

#endif /*_Scheduler_h*/