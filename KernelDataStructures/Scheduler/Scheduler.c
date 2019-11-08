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

KernelContext *switchBetween(KernelContext *, void *, void *);
KernelContext *forkTo(KernelContext *, void *, void *);
void free_zombie_t(void *);
void delete_process(void *);

PCB_t *remove_process_q(q_t *, int);
int tick_down_sleepers_q(void);


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


int initInitProcess(struct pte *initR1PageTable, PCB_t **initPcbpp) {
    PCB_t *initPCB = (PCB_t *) malloc(sizeof(PCB_t));
    if (initPCB == NULL) {
        return ERROR;
    }

    initPCB->pid = nextPid++;
	initPid = initPCB->pid;
	
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
    struct pte *currPtep = r0StackBasePtep;
    
	int i;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
		(initPCB->stackPfns)[i] = currPtep->pfn;
		currPtep++;
	}
	
	// no need to enqueue, as "init" will become the currPCB in Kernel Start
	*initPcbpp = initPCB;
	
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


int sleepProcess(int numRemainingDelayTicks) {
	TracePrintf(1, "sleepProcess() called, currPCB->pid = %d\n", currPCB->pid);
	currPCB->numRemainingDelayTicks = numRemainingDelayTicks;
	
	if (enq_q(sleepingQ, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) return ERROR;
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}


int tickDownSleepers(void) {
	return tick_down_sleepers_q();
}


int kickProcess() {
	TracePrintf(1, "kickProcess() called, currPCB->pid = %d\n",  currPCB->pid);
	if (enq_q(readyQ, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) return ERROR;
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}


int runProcess(int pid) {
	TracePrintf(1, "runProcess() called, currPCB->pid = %d, pid = %d\n",  currPCB->pid, pid);
	if (enq_q(readyQ, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = remove_process_q(readyQ, pid);
	
	/* nextPCB being NULL may mean that the target process is not ready yet,
	 * so we try to run something else instead.
	 */
	if (nextPCB == NULL) {
		nextPCB = (PCB_t *) deq_q(readyQ);
		
		if (nextPCB == NULL) return ERROR;
	}
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}


int execProcess() {
	TracePrintf(1, "execProcess() called, currPCB->pid = %d\n",  currPCB->pid);
	return KernelContextSwitch(switchBetween, NULL, currPCB);
}


int forkProcess(int pid) {
	TracePrintf(1, "forkProcess() called, currPCB->pid = %d, pid = %d\n",  currPCB->pid, pid);
	if (enq_q(readyQ, currPCB) == ERROR) return ERROR;
	
	/* TOTHINK: at least for fork(), there is no point to enq_q() into the readyQ
	 * only to get pulled back out here...
	 */
	PCB_t *childPCB = remove_process_q(readyQ, pid);
	
	/* if we can't find the child in the readyQ, or that the child we found is 
	 * actually not a child of the current process... then we have a problem.
	 */
	if (childPCB == NULL || \
		childPCB->parent == NULL || \
		childPCB->parent->pid != currPCB->pid) {
		return ERROR;
	}
	return KernelContextSwitch(forkTo, NULL, childPCB);
}


int exitProcess(int exit_status) {
	TracePrintf(1, "exitProcess() called, currPCB->pid = %d\n",  currPCB->pid);
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
		
		unblockProcess(parentPcbp->pid);
	}
	
	delete_process(currPCB);
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) return ERROR;
	
	return KernelContextSwitch(switchBetween, NULL, nextPCB);
}


int blockProcess(void) {
	TracePrintf(1, "blockProcess() called, currPCB->pid = %d\n",  currPCB->pid);
	if (enq_q(blockedQ, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) return ERROR;
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}


int unblockProcess(int pid) {
	TracePrintf(1, "forkProcess() called, currPCB->pid = %d, pid = %d\n",  currPCB->pid, pid);
	// first test if the queue is NULL (fatal error)
	if (blockedQ == NULL) {
		return ERROR;
	}
	
	PCB_t *unblockedPCB = remove_process_q(blockedQ, pid);
	
	/* since we made sure the queue is not NULL, if unblockedPCB is NULL,
	 * then it just isn't blocked. Not an error (imagine multiple children,
	 * upon exiting, each tries to unblock a waiting parent even though
	 * the parent only waited for one).
	 */
	if (unblockedPCB != NULL) {
		if (enq_q(readyQ, unblockedPCB) == ERROR) return ERROR;
	}
	
	return 0;
}


/* -------- scheduler-specific functions -------- */

/* when calling with currPcbP == NULL, assume that the currPCB is deleted already. */
KernelContext *switchBetween(KernelContext *currKctxt, void *currPcbP, void *nextPcbP) {
	TracePrintf(1, "switchBetween() called, currPCB->pid = %d, nextPcbP->pid = %d\n", currPCB->pid, ((PCB_t *) nextPcbP)->pid);
	
	if (currPcbP != NULL) {
		// copy kctxt into currPcbP (after this, no more operation on currPcbP)
		memmove((((PCB_t *) currPcbP)->kctxt), currKctxt, sizeof(KernelContext));
	}
	
    // set the global currPCB to be the PCB for the next process
    currPCB = (PCB_t *) nextPcbP;
	
	// switch kernel stack by changing the pfn mappings for corresponding PTEs
    struct pte *currPtep = r0StackBasePtep;
	
    int i;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        // NOTE: kernel stack MMU entries are flushed inside setPageTableEntry() method
        setPageTableEntry(currPtep, 1, (PROT_READ|PROT_WRITE), (currPCB->stackPfns)[i]);
        currPtep++;
    }

	/* if the currPCB (now nextPcbP) is a new process, copy into it the
	 * starter kernel context and the starter kernel stack (must happen
	 * after kernel stack switching)
	 */ 
	if (currPCB->kctxt == NULL) {
		currPCB->kctxt = (KernelContext *) malloc(sizeof(KernelContext));
		memmove(currPCB->kctxt, starterKctxt, sizeof(KernelContext));
		memmove((void *) KERNEL_STACK_BASE, starterKernelStack, KERNEL_STACK_MAXSIZE);
	}
	
    // change MMU registers for R1 & flush R1 TLB
    WriteRegister(REG_PTBR1, (unsigned int) (currPCB->r1PageTable));
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
	
    // return pointer to kctxt of nextPcbP
    return currPCB->kctxt;
}


/* assumes that the currPCB is of the parent process; copy currKctxt to the child
 * because currKctxt is exactly what the parent has for kctxt. Only used in
 * forkProcess().
 */
KernelContext *forkTo(KernelContext *currKctxt, void *nil0, void *childPcbP) {
	TracePrintf(1, "forkTo() called, currPCB->pid = %d, childPcbP->pid = %d\n", currPCB->pid, ((PCB_t *) childPcbP)->pid);
	
	// copy kctxt into the parent (which is assumed to be the currPCB)
	memmove(currPCB->kctxt, currKctxt, sizeof(KernelContext));
	
	// the child will be running next
    currPCB = (PCB_t *) childPcbP;
	
	// switch kernel stack AND populate the child's with the content of the parent's
	int i;
	u_long pfn;
	int addr = KERNEL_STACK_BASE;
	struct pte *currPtep = r0StackBasePtep;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
		/* NOTE: the following is analogous to the tempPte manipulation in
		 * fork(), but setPageTableEntry() MUST come last, because otherwise
		 * we will lose track of the parent's content (whereas in fork(), we
		 * do not modify the current page table, which is the parent's).
		 */
		pfn = (currPCB->stackPfns)[i];
		
		setPageTableEntry(tempPtep, 1, PROT_WRITE, pfn);
		memmove(tempVAddr, (void *) addr, PAGESIZE);
		
        // NOTE: kernel stack MMU entries are flushed inside setPageTableEntry() method
        setPageTableEntry(currPtep, 1, (PROT_READ|PROT_WRITE), pfn);
        currPtep++;
		addr += PAGESIZE;
    }
	
	// reset the tempPte
	setPageTableEntry(tempPtep, 0, PROT_NONE, 0);

	// copy the parent's current kernel context, which is currKctxt
	memmove(currPCB->kctxt, currKctxt, sizeof(KernelContext));
	
    // change MMU registers for R1 & flush R1 TLB
    WriteRegister(REG_PTBR1, (unsigned int) (currPCB->r1PageTable));
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
	
    // return pointer to kctxt of the childPcbP
    return currPCB->kctxt;
}


// only used in delete_process()
void free_zombie_t(void *zombiePcbp_item) {
	free(zombiePcbp_item);
}


/* delete_process() frees a PCB and all content within, including its uctxt,
 * kctxt, and r1PageTable. Also frees the allocated physical memory. Depends
 * on the FrameList module. Assumes that the call is followed by a "currPcbP=NULL"
 * KernelContextSwitch, and thus does not flush the TLB.
 *
 * Used along with the Queue module, and thus the signature.
 */
void delete_process(void *pcbp_item) {
	PCB_t *pcbp = (PCB_t *) pcbp_item;
	free(pcbp->uctxt);
	free(pcbp->kctxt);
	free_q(pcbp->zombieQ, free_zombie_t);
	
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
	TracePrintf(1, "tick_down_sleepers_q starting...\n");
	if (sleepingQ == NULL) return ERROR;
	qnode_t *currNode = sleepingQ->head;
	if (currNode == NULL) return 0;
	
	PCB_t *pcbp = (PCB_t *) (currNode->item);
	(pcbp->numRemainingDelayTicks)--;
	
	// keep removing heads; if no ticks remain, move the current pcbp to readyQ
	while (pcbp->numRemainingDelayTicks <= 0) {
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
		(pcbp->numRemainingDelayTicks)--;
	}
	
	
	// start checking each node_behind
	qnode_t *node_behind = currNode->node_behind;
	qnode_t *node_doubly_behind;
	while (node_behind != NULL) {
		node_doubly_behind = node_behind->node_behind;
		pcbp = (PCB_t *) (node_behind->item);
		(pcbp->numRemainingDelayTicks)--;
		
		// if no ticks remain, move the current pcbp to readyQ
		if (pcbp->numRemainingDelayTicks <= 0) {
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

