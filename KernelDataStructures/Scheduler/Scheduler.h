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

// PROCESSES_SUCK

typedef struct PCB {
    int pid;
    void *brk;
    UserContext *uctxt;
    KernelContext *kctxt;
    // ZombieQueue *zq;
    int numChildren;
    struct PCB *parent; // -- currently only using this for KernelExit/KernelWait
	struct pte *pagetable;
} PCB_t;

int initProcess(PCB_t **, UserContext *uctxt);
#endif /*_Scheduler_h*/