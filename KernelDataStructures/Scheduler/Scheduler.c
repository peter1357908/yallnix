#include "Scheduler.h"
#include <yalnix.h>
#include <hardware.h>  
#include <stdio.h>
#include <string.h>
#include "../PageTable/PageTable.h"
#include "../FrameList/FrameList.h"
#include "../../Kernel.h"

#define KERNEL_STACK_BASE_VPN  (KERNEL_STACK_BASE >> PAGESHIFT)
#define KERNEL_BASE_VPN  (VMEM_0_BASE >> PAGESHIFT)

int nextPid = 0; 

int initInitProcess(struct pte *initR1PageTable) {
    initPCB = (PCB_t *) malloc(sizeof(PCB_t));
    if (initPCB == NULL) {
        return ERROR;
    }

    initPCB->pid = nextPid++;
    initPCB->uctxt = (UserContext *) malloc(sizeof(UserContext));
	if (initPCB->uctxt == NULL) {
        return ERROR;
    }
	
	/* NOTE: the init's kctxt will be given when it gets context-switched out,
	 * but we have to first allocate enough memory to hold that kctxt.
	 */
    initPCB->kctxt = (KernelContext *) malloc(sizeof(KernelContext));
	if (initPCB->kctxt == NULL) {
		return ERROR;
	}
	
    initPCB->numChildren = 0;
    initPCB->r1PageTable = initR1PageTable;
    initPCB->numRemainingDelayTicks = 0;
	
	// calculate r0StackBasePtep;
    struct pte *r0StackBasePtep = ((struct pte *) ReadRegister(REG_PTBR0)) + KERNEL_STACK_BASE_VPN - KERNEL_BASE_VPN;
    struct pte *currPtep = r0StackBasePtep;
    
	int i;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
		(initPCB->stackPfns)[i] = currPtep->pfn;
		currPtep++;
	}
    
    return 0;
}

int initProcess(PCB_t **pcb) {
    PCB_t *newPCB = (PCB_t *) malloc(sizeof(PCB_t));
    if (newPCB == NULL) {
        return ERROR;
    }

    newPCB->pid = nextPid++;
    newPCB->uctxt = (UserContext *) malloc(sizeof(UserContext));
	if (newPCB->uctxt == NULL) {
        return ERROR;
    }
    newPCB->kctxt = NULL;
    newPCB->numChildren = 0;
    newPCB->r1PageTable = initializeRegionPageTable();
    newPCB->numRemainingDelayTicks = 0;

    int i;
    frame_t *newFrame;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        if (getFrame(FrameList, numFrames, &newFrame) == ERROR) {
            return ERROR;
	    }
        (newPCB->stackPfns)[i] = (u_long) ((int) (newFrame->addr)>>PAGESHIFT);
    }

    *pcb = newPCB;
    return 0;
}


KernelContext *getStarterKctxt(KernelContext *currKctxt, void *nil0, void *nil1) {
	TracePrintf(1, "getStarterKctxt called, currPCB->pid = %d\n", currPCB->pid);
	
	memmove(starterKernelStack, (void *) KERNEL_STACK_BASE, KERNEL_STACK_MAXSIZE);
    memmove(starterKctxt, currKctxt, sizeof(KernelContext));
    return starterKctxt;
}


KernelContext *MyKCS(KernelContext *currKctxt, void *currPcbP, void *nextPcbP) {
	TracePrintf(1, "MyKCS called, currPCB->pid = %d\n", currPCB->pid);
	
    // copy kctxt into currPcbP (after this, no more operation on currPcbP)
	// TODO: is "currPcbP" always the same as "currPCB"?
    memmove((((PCB_t *) currPcbP)->kctxt), currKctxt, sizeof(KernelContext));

    // set the global currPCB to be the PCB for the next process
    currPCB = (PCB_t *) nextPcbP;
	
	// switch kernel stack, populate PTEs from r0StackBase to r0StackLimit
	struct pte *r0StackBasePtep = ((struct pte *) ReadRegister(REG_PTBR0)) + KERNEL_STACK_BASE_VPN - KERNEL_BASE_VPN;
    struct pte *currPtep = r0StackBasePtep;
	
    int i;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        // NOTE: kernel stack PTEs are flushed inside setPageTableEntry() method
        setPageTableEntry(currPtep, 1, (PROT_READ|PROT_WRITE), (currPCB->stackPfns)[i]);
        currPtep++;
    }

	/* if the currPCB (nextPcbP) is a new process, copy into it the starter kernel context
	 * and the starter kernel stack (must happen after kernel stack switching)
	 */ 
	if (currPCB->kctxt == NULL) {
		currPCB->kctxt = (KernelContext *) malloc(sizeof(KernelContext));
		memmove(currPCB->kctxt, starterKctxt, sizeof(KernelContext));
		memmove((void *) KERNEL_STACK_BASE, starterKernelStack, KERNEL_STACK_MAXSIZE);
	}
	
    // change MMU registers for R1 & flush R1 TLB
    WriteRegister(REG_PTBR1, (unsigned int) ((PCB_t *) nextPcbP)->r1PageTable);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
	
    // return pointer to kctxt of nextPcbP
    return currPCB->kctxt;
}
