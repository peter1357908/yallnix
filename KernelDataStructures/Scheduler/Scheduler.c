#include "Scheduler.h"
#include <yalnix.h>
#include <stdio.h>
#include "../PageTable/PageTable.h"

int nextPid = 1; // we're starting at 1 because the idle pid = 0

int initProcess(PCB_t **pcb, UserContext *uctxt) {
    PCB_t *newPCB = (PCB_t *) malloc(sizeof(PCB_t));
    if (newPCB == NULL) {
        return ERROR;
    }

    newPCB->pid = nextPid++;
    newPCB->pagetable = initializePageTable();
    newPCB->uctxt = uctxt;
    newPCB->numRemainingDelayTicks = 0;
    newPCB->numChildren = 0;
    *pcb = newPCB;


    return 0;
}