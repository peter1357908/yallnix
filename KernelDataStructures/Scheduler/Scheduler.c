#include <yalnix.h>
#include <hardware.h>
#include <stdio.h>
#include <string.h>
#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../PageTable/PageTable.h"
#include "../FrameList/FrameList.h"
#include "../../Kernel.h"
#include "Scheduler.h"


/* ------ the following declarations are only visible to Scheduler ------ */
int nextPid;
q_t *readyQ;
q_t *blockedQ;
q_t *sleepingQ;
typedef struct zombie {
	int pid;
	int exit_status;
} zombie_t;
int tick_down_sleepers_q();
PCB_t *remove_process_q(q_t *queue, int pid);


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
	newPCB->parent = NULL;
	newPCB->zombieQ = NULL;
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
	
	if (enq_q(readyQ, newPCB) == ERROR) return ERROR;
	
    return 0;
}


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
	initPCB->parent = NULL;
	initPCB->zombieQ = NULL;
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
	
	// no need to enqueue, as "init" will become the currPCB in Kernel Start
    
    return 0;
}


KernelContext *getStarterKernelState(KernelContext *currKctxt, void *nil0, void *nil1) {
	TracePrintf(1, "getStarterKernelState called, currPCB->pid = %d\n", currPCB->pid);
	
	memmove(starterKernelStack, (void *) KERNEL_STACK_BASE, KERNEL_STACK_MAXSIZE);
    memmove(starterKctxt, currKctxt, sizeof(KernelContext));
    return starterKctxt;
}


/* ------ the following are for scheduling ------ */

int initScheduler() {
	nextPid = 0;
	if ((readyQ = make_q()) == NULL || \
		(blockedQ = make_q()) == NULL || \
		(sleepingQ = make_q()) == NULL) {
		return ERROR;
	}
	return 0;
}


int tickDownSleepers(void) {
	return tick_down_sleepers_q();
}


int kickProcess() {
	if (enq_q(readyQ, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) return ERROR;
	
	if (KernelContextSwitch(switchBetween, currPCB, nextPCB) == ERROR) return ERROR;
	
	return 0;
}


int runProcess(int pid) {
	if (enq_q(readyQ, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = remove_process_q(readyQ, pid);
	
	if (nextPCB == NULL) {
		nextPCB = (PCB_t *) deq_q(readyQ);
		
		if (nextPCB == NULL) return ERROR;
	}
	
	if (KernelContextSwitch(switchBetween, currPCB, nextPCB) == ERROR) return ERROR;
	
	return 0;
}


int zombifyProcess(int exit_status) {
	PCB_t *parentPcbp = currPCB->parent;
	
	if (parentPcbp != NULL) {
		// if the parent has no zombieQ yet, make one
		if (parentPcbp->zombieQ == NULL) {
			if ((parentPcbp->zombieQ = make_q()) == NULL) return ERROR;
		}
		
		// now it must have a zombieQ, enqueue the current process...
		zombie_t *zombiePcbp = (zombie_t *) malloc(sizeof(zombie_t));
		zombiePcbp->pid = currPCB->pid;
		zombiePcbp->exit_status = exit_status;
		if (enq_q(parentPcbp->zombieQ, zombiePcbp) == ERROR) return ERROR;
	}
	
	delete_process(currPCB);
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) return ERROR;
	
	return KernelContextSwitch(switchBetween, NULL, nextPCB);
}


// returns ERROR/0; blocks the current process and runs another ready process
int blockProcess(void) {
	if (enq_q(blockedQ, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) return ERROR;
	
	if (KernelContextSwitch(switchBetween, currPCB, nextPCB) == ERROR) return ERROR;
	
	return 0;
}


// returns ERROR/0; moves a process from blockedQ to readyQ (does nothing else)
int unblockProcess(int pid) {
	PCB_t *unblockedPCB = remove_process_q(blockedQ, pid);
	
	// TODO: may require discerning between no PCB with pid and bad queue
	if (unblockedPCB == NULL) return ERROR;
	
	if (enq_q(readyQ, unblockedPCB) == ERROR) return ERROR;
	
	return 0;
}


KernelContext *switchBetween(KernelContext *currKctxt, void *currPcbP, void *nextPcbP) {
	TracePrintf(1, "switchBetween() called, currPCB->pid = %d, nextPcbP->pid = %d\n", currPCB->pid, ((PCB_t *) nextPcbP)->pid);
	
	if (currPcbP != NULL) {
		// copy kctxt into currPcbP (after this, no more operation on currPcbP)
		// TOTHINK: is "currPcbP" always the same as "currPCB"?
		memmove((((PCB_t *) currPcbP)->kctxt), currKctxt, sizeof(KernelContext));
	}
	
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


void free_zombieQ(void *zombiePcbp_item) {
	free(zombiePcbp_item);
}


void delete_process(void *pcbp_item) {
	PCB_t *pcbp = (PCB_t *) pcbp_item;
	free(pcbp->uctxt);
	free(pcbp->kctxt);
	free_q(pcbp->zombieQ, free_zombieQ);
	
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
	
	free(pcbp);
}


/* -------- scheduler-specific queue operations -------- */

/* this function treats the queue as a linked list and removes
 * a PCB based on the given pid; return NULL if no PCB with the
 * given pid is found, or if the queue is NULL.
 */
PCB_t *remove_process_q(q_t *queue, int pid) {
	if (queue == NULL) return NULL;
	qnode_t *currNode = queue->head;
	if (currNode == NULL) return NULL;

	// checks the head first
	PCB_t *pcbp;
	if (((PCB_t *) (currNode->item))->pid == pid) {
		// removing head, similar to deq_q
		pcbp = (PCB_t *) (currNode->item);
		qnode_t *new_head = currNode->node_behind;
		free(currNode);
		queue->head = new_head;
		// if now the queue is empty...
		if (new_head == NULL) {
			queue->tail = NULL;
		}
		return pcbp;
	}
	
	// start checking each node_behind
	qnode_t *node_behind = currNode->node_behind;			
	while (node_behind != NULL) {
		// if the target PCB is found, remove and free the node before breaking
		if (((PCB_t *) (node_behind->item))->pid == pid) {
			pcbp = (PCB_t *) (node_behind->item);
			qnode_t *node_doubly_behind = node_behind->node_behind;
			free(node_behind);
			currNode->node_behind = node_doubly_behind;
			// if we just removed the tail of the queue...
			if (node_doubly_behind == NULL) {
				queue->tail = currNode;
			}
			return pcbp;
		}
		// else, advance to the next node behind...
		currNode = node_behind;
		node_behind = node_behind->node_behind;
	}

}


/* similar to remove_process_q but messier, because multiple
 * nodes can be removed in one tick_down_sleepers_q() call.
 */
int tick_down_sleepers_q() {
	if (sleepingQ == NULL) return ERROR;
	qnode_t *currNode = sleepingQ->head;
	if (currNode == NULL) return 0;
	
	PCB_t *pcbp = (PCB_t *) (currNode->item);
	pcbp->numRemainingDelayTicks--;
	
	// keep removing heads; if no ticks remain, move the current pcbp to readyQ
	while (pcbp->numRemainingDelayTicks == 0) {
		// remove the current head from sleepingQ
		qnode_t *new_head = currNode->node_behind;
		free(currNode);		
		// and enqueue it to readyQ
		enq_q(readyQ, pcbp);
		
		sleepingQ->head = new_head;
		// if now the queue is empty...
		if (new_head == NULL) {
			sleepingQ->tail = NULL;
			return 0;
		}
		
		// the queue is not empty yet, next iteration starts here
		currNode = new_head;
		pcbp = (PCB_t *) (currNode->item);
		pcbp->numRemainingDelayTicks--;
	}
	
	
	// start checking each node_behind
	qnode_t *node_behind = currNode->node_behind;
	qnode_t *node_doubly_behind;
	while (node_behind != NULL) {
		node_doubly_behind = node_behind->node_behind;
		pcbp = (PCB_t *) (node_behind->item);
		pcbp->numRemainingDelayTicks--;
		
		// if no ticks remain, move the current pcbp to readyQ
		if (pcbp->numRemainingDelayTicks == 0) {
			free(node_behind);
			enq_q(readyQ, pcbp);
			
			currNode->node_behind = node_doubly_behind;
			// if we just removed the tail of the queue...
			if (node_doubly_behind == NULL) {
				sleepingQ->tail = currNode;
				return 0;
			}
			
			/* if the node removed was not the tail of the queue...
			 * currNode is unchanged, since we removed the node_behind;
			 */
			node_behind = node_doubly_behind;
			continue;
			
		}
		
		// if we didn't remove a node, the currNode is now the node_behind		
		currNode = node_behind;
		node_behind = node_doubly_behind;
	}
	
	return 0;
}

