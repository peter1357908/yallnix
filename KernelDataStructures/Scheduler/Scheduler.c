#include "Scheduler.h"
#include <yalnix.h>
#include <stdio.h>
#include <string.h>
#include "../PageTable/PageTable.h"
#include "../FrameList/FrameList.h"
#include "../../Kernel.h"

int nextPid = 0; 

void DoIdle() {
	while(1) {
		TracePrintf(1, "DoIdle\n");
		Pause();
	}
}

int initIdleProcess(struct pte *r1PageTable) {
    idlePCB = (PCB_t *) malloc(sizeof(PCB_t));
    if (idlePCB == NULL) {
        return ERROR;
    }

    idlePCB->pid = nextPid++;
    idlePCB->uctxt = (UserContext *) malloc(sizeof(UserContext));
    idlePCB->kctxt = (KernelContext *) malloc(sizeof(KernelContext));
    idlePCB->numChildren = 0;
    idlePCB->r1PageTable = r1PageTable;
    idlePCB->numRemainingDelayTicks = 0;

    int i;
    frame_t *newFrame;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        if (getFrame(FrameList, numFrames, &newFrame) == ERROR) {
            return ERROR;
	    }
        (idlePCB->stackPfns)[i] = (u_long) (newFrame->addr)>>PAGESHIFT;
    }

	struct pte *r1StackBasePtep = idlePCB->r1PageTable + MAX_PT_LEN - 1;
	frame_t *r1StackBaseFrame;
	if (getFrame(FrameList, numFrames, &r1StackBaseFrame) == ERROR) {
		return ERROR;
	}
	setPageTableEntry(r1StackBasePtep, 1, PROT_READ|PROT_WRITE, (int) (r1StackBaseFrame->addr)>>PAGESHIFT);


    int size = 0;
	int argcount = 0;
	char *cp = ((char *) VMEM_1_LIMIT) - size;
	char **cpp = (char **) (((int)cp - ((argcount + 3 + POST_ARGV_NULL_SPACE) *sizeof (void *))) & ~7);
  	char *cp2 = (caddr_t)cpp - INITIAL_STACK_FRAME_SIZE;

    idlePCB->uctxt->sp = cp2;
	idlePCB->uctxt->pc = DoIdle;
#ifdef LINUX
	idlePCB->uctxt->ebp = cp2;
#endif
    
    return 0;
}

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
    newPCB->numRemainingDelayTicks = 0;

    int i;
    frame_t *newFrame;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        if (getFrame(FrameList, numFrames, &newFrame) == ERROR) {
            return ERROR;
	    }
        (newPCB->stackPfns)[i] = (u_long) (newFrame->addr)>>PAGESHIFT;
    }

    *pcb = newPCB;
    return 0;
}

// NOTE: we don't use nil, but we need it to match KernelContextSwitch's desired signature
KernelContext *getStarterKctxt(KernelContext *currKctxt, void *starterKctxt, void *nil) {
    memmove((KernelContext *) starterKctxt, currKctxt, sizeof(KernelContext));
    return (KernelContext *) starterKctxt;
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
