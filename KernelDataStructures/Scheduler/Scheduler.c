#include "Scheduler.h"
#include <yalnix.h>
#include <stdio.h>
#include <string.h>
#include "../PageTable/PageTable.h"
#include "../FrameList/FrameList.h"
#include "../../Kernel.h"

int nextPid = 1; // we're starting at 1 because the idle pid = 0

int initProcess(PCB_t **pcb) {
    PCB_t *newPCB = (PCB_t *) malloc(sizeof(PCB_t));
    if (newPCB == NULL) {
        return ERROR;
    }

    newPCB->pid = nextPid++;
  
    newPCB->uctxt = (UserContext *) malloc(sizeof(UserContext));
    newPCB->kctxt = (KernelContext *) malloc(sizeof(KernelContext));
    newPCB->numChildren = 0;
    newPCB->r1PageTable = initializeRegionPageTable();

    // grab KERNEL_STACK_MAXSIZE / PAGESIZE frames for Kernel
    int i;
    frame_t *newFrame;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        if (getFrame(FrameList, numFrames, &newFrame) == ERROR) {
            return ERROR;
	    }
        (newPCB->stackPfns)[i] = (u_long) (newFrame->addr)>>PAGESHIFT;
    }

    newPCB->numRemainingDelayTicks = 0;

    *pcb = newPCB;

    return 0;
}

KernelContext *MyKCS(KernelContext *currKctxt, void *currPcbP , void *nextPcbP) {
    // switch kernel stack
    int r0StackBaseVpn = KERNEL_STACK_BASE>>PAGESHIFT;
    int r0BaseVpn = VMEM_0_BASE>>PAGESHIFT;
    struct pte *r0StackBasePtep = (struct pte *) ReadRegister(REG_PTBR0) + r0StackBaseVpn - r0BaseVpn;
    struct pte *currPte = r0StackBasePtep;
    int i;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        // NOTE: kernel stack PTEs are flushed inside setPageTableEntry() method
        setPageTableEntry(currPte, 1, (PROT_READ|PROT_WRITE), (((PCB_t *) nextPcbP)->stackPfns)[i]);
        currPte++;
    } 

    // change MMU registers for R1 & flush R1 TLB
    WriteRegister(REG_PTBR1, (unsigned int) ((PCB_t *) nextPcbP)->r1PageTable);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    // set currPCB
    currPCB = (PCB_t *) nextPcbP;

    // copy kctxt into currPcbP
    memmove((((PCB_t *)currPcbP)->kctxt), currKctxt, sizeof(KernelContext));
    // return pointer to kctxt of nextPcbP
    return ((PCB_t *) nextPcbP)->kctxt;
}
