#include "Scheduler.h"
#include <yalnix.h>
#include "../PageTable/PageTable.h"

int nextPid = 0;

int initProcess(PCB_t **pcb, UserContext *uctxt) {
    PCB_t *newPCB = (PCB_t *) malloc(sizeof(PCB_t));
    if (newPCB == NULL) {
        return ERROR;
    }
    newPCB->pid = nextPid++;
    newPCB->pagetable = initializePageTable();
    newPCB->uctxt = uctxt;
    *pcb = newPCB;
    return 0;
}