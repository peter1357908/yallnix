#ifndef _LoadProgram_h
#define _LoadProgram_h

#include "KernelDataStructures/Scheduler/Scheduler.h"

// assumes that proc is already initialized and the MMU points to its r1PageTable
int LoadProgram(char *name, char *args[], PCB_t *proc);

// assumes that initProcess(&idlePCB) is called beforehand;
int LoadIdle(void);


#endif /* _LoadProgram_h */
