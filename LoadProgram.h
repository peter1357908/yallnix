#ifndef _LoadProgram_h
#define _LoadProgram_h

#include "KernelDataStructures/Scheduler/Scheduler.h"

// assumes that initProcess(&proc) is called beforehand;
int LoadProgram(char *name, char *args[], PCB_t *proc);

// assumes that initProcess(&idlePCB) is called beforehand;
int LoadIdle(void);


#endif /* _LoadProgram_h */
