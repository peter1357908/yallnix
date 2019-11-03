#include <yalnix.h>
#include <hardware.h>  
#include <stdio.h>
#include <string.h>
#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../PageTable/PageTable.h"
#include "../FrameList/FrameList.h"
#include "../../Kernel.h"
#include "Scheduler.h"


<<<<<<< Updated upstream
#define KERNEL_STACK_BASE_VPN  (KERNEL_STACK_BASE >> PAGESHIFT)
#define KERNEL_BASE_VPN  (VMEM_0_BASE >> PAGESHIFT)


/* -------- the following items are only visible to Scheduler -------- */
int nextPid;
q_t *readyQ;
q_t *blockedQ;
q_t *zombieQ;


/* ------ the following are for initialization ------ */

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
    u_long pfn;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        if (getFrame(FrameList, numFrames, &pfn) == ERROR) {
            return ERROR;
	    }
        (newPCB->stackPfns)[i] = pfn;
    }

    *pcb = newPCB;
    return 0;
}

=======
int nextPid = 0; 
>>>>>>> Stashed changes

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
    initPCB->parent = NULL;
	
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


<<<<<<< Updated upstream
KernelContext *getStarterKernelState(KernelContext *currKctxt, void *nil0, void *nil1) {
	TracePrintf(1, "getStarterKernelState called, currPCB->pid = %d\n", currPCB->pid);
=======
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
    newPCB->parent = NULL;

    int i;
    u_long pfn;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        if (getFrame(FrameList, numFrames, &pfn) == ERROR) {
            return ERROR;
	    }
        (newPCB->stackPfns)[i] = pfn;
    }

    *pcb = newPCB;
    return 0;
}


KernelContext *getStarterKctxt(KernelContext *currKctxt, void *nil0, void *nil1) {
	TracePrintf(1, "getStarterKctxt called, currPCB->pid = %d\n", currPCB->pid);
>>>>>>> Stashed changes
	
	memmove(starterKernelStack, (void *) KERNEL_STACK_BASE, KERNEL_STACK_MAXSIZE);
    memmove(starterKctxt, currKctxt, sizeof(KernelContext));
    return starterKctxt;
}


/* ------ the following are for scheduling ------ */

int initScheduler() {
	nextPid = 0;
	if ((readyQ = make_q()) == NULL || \
		(blockedQ = make_q()) == NULL || \
		(zombieQ = make_q()) == NULL) {
		
		return ERROR;
	}
	return 0;
}


KernelContext *MyKCS(KernelContext *currKctxt, void *currPcbP, void *nextPcbP) {
	TracePrintf(1, "MyKCS called, currPCB->pid = %d\n", currPCB->pid);
	
    // copy kctxt into currPcbP (after this, no more operation on currPcbP)
	// TOTHINK: is "currPcbP" always the same as "currPCB"?
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








void delete_process(void *pcbp_item) {
	PCB_t *pcbp = (PCB_t *) pcbp_item;
	free(pcbp->uctxt);
	free(pcbp->kctxt);
	
	// free the frames in the r1PageTable, and then the r1PageTable
	struct pte *ptep = pcbp->r1PageTable;
	int i;
	for (i = 0; i < MAX_PT_LEN; i++) {
		if (ptep->valid == 1) {
			freeFrame(FrameList, numFrames, ptep->pfn);
			// does not need to invalidate entries here
		}
		ptep++;
	}
	free(pcbp->r1PageTable);
	
	// free the frames in the kernel stack
	for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
		freeFrame(FrameList, numFrames, (pcbp->stackPfns)[i]);
	}
	// TOTHINK: is flushing necessary here?
	// Is called in another process, then no need to flush...
	
	free(pcbp);
}





