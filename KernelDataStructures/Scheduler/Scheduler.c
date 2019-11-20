#include <yalnix.h>
#include <hardware.h>
#include <stdio.h>
#include <string.h>
#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/HashMap/HashMap.h"
#include "../PageTable/PageTable.h"
#include "../FrameList/FrameList.h"
#include "../../Kernel.h"
#include "Scheduler.h"

#define BLOCKED_MAP_HASH_BUCKETS 50
#define SLEEPING_MAP_HASH_BUCKETS 50

/* ------ the following declarations are only visible to Scheduler ------ */
int nextPid;
q_t *readyQ;
HashMap_t *blockedMap;
HashMap_t *sleepingMap;

KernelContext *switchBetween(KernelContext *, void *, void *);
KernelContext *forkTo(KernelContext *, void *, void *);
KernelContext *execTo(KernelContext *, void *, void *);
void free_zombie_t(void *);
void delete_process(void *);
void tickDownSleeper(void *, int, void *);

PCB_t *currTransmitters[NUM_TERMINALS]; // processes currently transmitting
q_t *transmittingQs[NUM_TERMINALS]; // processes waiting to transmit
q_t *readingQs[NUM_TERMINALS]; // processes waiting to read


/* ------ the following are for initialization ------ */

int initPCB(PCB_t **pcbpp) {
	PCB_t *newPCB = (PCB_t *) malloc(sizeof(PCB_t));
    if (newPCB == NULL) {
		TracePrintf(1, "initPCB: error malloc'ing PCB\n");
        return ERROR;
    }

    newPCB->pid = nextPid++;
    newPCB->uctxt = (UserContext *) malloc(sizeof(UserContext));
	if (newPCB->uctxt == NULL) {
		TracePrintf(1, "initPCB: error malloc'ing UserContext\n");
        return ERROR;
    }
    newPCB->kctxt = NULL;
    newPCB->numChildren = 0;
	newPCB->parent = NULL;
	newPCB->zombieQ = NULL;
	initializeRegionPageTable(&(newPCB->r1PageTable));
    newPCB->numRemainingDelayTicks = 0;

    int i;
    u_long pfn;
    for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
        if (getFrame(FrameList, numFrames, &pfn) == ERROR) {
            return ERROR;
	    }
        (newPCB->stackPfns)[i] = pfn;
    }
	
    *pcbpp = newPCB;
	return SUCCESS;
}

int initProcess(PCB_t **pcbpp) { 
	if (initPCB(pcbpp) == ERROR || enq_q(readyQ, *pcbpp) == ERROR) return ERROR;
    return SUCCESS;
}

// TODO: can probably modularize this more
int initInitProcess(struct pte *initR1PageTable, PCB_t **initPcbpp) {
    PCB_t *initPCB = (PCB_t *) malloc(sizeof(PCB_t));
    if (initPCB == NULL) {
		TracePrintf(1, "initInitProcess: error malloc'ing PCB\n");
        return ERROR;
    }

    initPCB->pid = nextPid++;
	initPid = initPCB->pid;
	
    initPCB->uctxt = (UserContext *) malloc(sizeof(UserContext));
	if (initPCB->uctxt == NULL) {
		TracePrintf(1, "initInitProcess: error malloc'ing UserContext\n");
        return ERROR;
    }
	
	/* NOTE: the init's kctxt will be given when it gets context-switched out,
	 * but we have to first allocate enough memory to hold that kctxt.
	 */
    initPCB->kctxt = (KernelContext *) malloc(sizeof(KernelContext));
	if (initPCB->kctxt == NULL) {
		TracePrintf(1, "initInitProcess: error malloc'ing KernelContext\n");
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
	nextSyncId = 0;

	if ((readyQ = make_q()) == NULL || \
		(blockedMap = HashMap_new(BLOCKED_MAP_HASH_BUCKETS)) == NULL || \
		(sleepingMap = HashMap_new(SLEEPING_MAP_HASH_BUCKETS)) == NULL) {
		return ERROR;
	}
	
	
	int i;
	for (i = 0; i < NUM_TERMINALS; i++) {
		if ((transmittingQs[i] = make_q()) == NULL || \
			(readingQs[i] = make_q()) == NULL) {
			return ERROR;
		}
	}
	
	return 0;
}

int sleepProcess(int numRemainingDelayTicks) {
	TracePrintf(1, "sleepProcess() called, currPCB->pid = %d, ticks = %d\n", currPCB->pid, numRemainingDelayTicks);
	
	if (numRemainingDelayTicks <= 0) {
		TracePrintf(1, "sleepProcess: exiting with SUCCESS because given non-positive ticks\n");
		return SUCCESS;
	}
	
	currPCB->numRemainingDelayTicks = numRemainingDelayTicks;

	if (HashMap_insert(sleepingMap, currPCB->pid, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) {
		TracePrintf(1, "sleepProcess: readyQ is empty\n");
		return ERROR;
	} 
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}

int tickDownSleepers(void) {
	TracePrintf(1, "tickDownSleepers() called, currPCB->pid = %d\n",  currPCB->pid);
	int errorCount = 0;
	HashMap_iterate(sleepingMap, &errorCount, tickDownSleeper);
	if (errorCount > 0) return ERROR;
	return SUCCESS;
}

int kickProcess() {
	TracePrintf(1, "kickProcess() called, currPCB->pid = %d\n",  currPCB->pid);
	
	// first make sure that the readyQ is not NULL
	if (readyQ == NULL) {
		TracePrintf(1, "kickProcess: readyQ is null\n");
		return ERROR;
	}
	
	// then, if we get NULL from peek_q, readyQ must be empty, so do nothing
	if (peek_q(readyQ) == NULL) {
		TracePrintf(1, "kickProcess() exiting with 0 because readyQ is empty\n");
		return 0;
	}
	
	if (enq_q(readyQ, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) { 
		TracePrintf(1, "kickProcess: readyQ is empty\n");
		return ERROR;
	}
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}

int execProcess() {
	TracePrintf(1, "execProcess() called, currPCB->pid = %d\n",  currPCB->pid);
	return KernelContextSwitch(execTo, NULL, NULL);
}


int forkProcess(PCB_t *childPCB) {
	TracePrintf(1, "forkProcess() called, currPCB->pid = %d, childPCB->pid = %d\n",  currPCB->pid, childPCB->pid);
	if (childPCB == NULL || \
		childPCB->parent == NULL || \
		childPCB->parent->pid != currPCB->pid) {
		TracePrintf(1, "forkProcess: childPCB is null or has bad parameters\n");
		return ERROR;
	}
	
	if (enq_q(readyQ, currPCB) == ERROR) return ERROR;
	
	return KernelContextSwitch(forkTo, NULL, childPCB);
}


int exitProcess(int exit_status) {
	TracePrintf(1, "exitProcess() called, currPCB->pid = %d, exit_status = %d\n",  currPCB->pid, exit_status);
	PCB_t *parentPcbp = currPCB->parent;
	
	if (parentPcbp != NULL) {
		// if the parent has no zombieQ yet, make one
		if (parentPcbp->zombieQ == NULL) {
			if ((parentPcbp->zombieQ = make_q()) == NULL) return ERROR;
		}
		
		// now it must have a zombieQ, enqueue the current process...
		zombie_t *childZombiep = (zombie_t *) malloc(sizeof(zombie_t));
		childZombiep->pid = currPCB->pid;
		childZombiep->exit_status = exit_status;
		
		if (enq_q(parentPcbp->zombieQ, childZombiep) == ERROR) return ERROR;
		
		unblockProcess(parentPcbp->pid);
	}
	
	delete_process(currPCB);
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) {
		TracePrintf(1, "exitProcess: readyQ is empty\n");
		return ERROR;
	}
	
	return KernelContextSwitch(switchBetween, NULL, nextPCB);
}


int blockProcess(void) {
	TracePrintf(1, "blockProcess() called, currPCB->pid = %d\n",  currPCB->pid);

	if (HashMap_insert(blockedMap, currPCB->pid, currPCB) == ERROR) return ERROR;
	
	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) {
		TracePrintf(1, "blockProcess: readyQ is empty\n");
		return ERROR;
	}
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}


int unblockProcess(int pid) {
	TracePrintf(1, "unblockProcess() called, currPCB->pid = %d, pid = %d\n",  currPCB->pid, pid);

	// first test if the map is NULL (fatal error)
	if (blockedMap == NULL) {
		TracePrintf(1, "unblockProcess: blockedMap is null\n");
		return ERROR;
	}
	
	PCB_t *unblockedPCB =  (PCB_t *) HashMap_remove(blockedMap, pid); 
	
	/* since we made sure the HashMap is not NULL, if unblockedPCB is NULL,
	 * then it just isn't blocked. Not an error (imagine multiple children,
	 * upon exiting, each tries to unblock a waiting parent even though
	 * the parent only waited for one).
	 */
	if (unblockedPCB != NULL) {
		return enq_q(readyQ, unblockedPCB);
	}
	
	return SUCCESS;
}


/*
    =========== TTY STUFF ===========
*/

int isTtyTransmitAvailable(int tty_id) {
	if (currTransmitters[tty_id] == NULL) return 1;
	return 0;
}

int blockTransmitter(int tty_id) {
	TracePrintf(1, "blockTransmitter() called, currPCB->pid = %d, tty_id = %d\n",  currPCB->pid, tty_id);
	q_t *transmittingQ = transmittingQs[tty_id];

	if (enq_q(transmittingQ, currPCB) == ERROR) return ERROR;

	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) {
		TracePrintf(1, "blockTransmitter: readyQ is empty\n");
		return ERROR;
	}
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}

int unblockTransmitter(int tty_id) {
	TracePrintf(1, "unblockTransmitter() called, currPCB->pid = %d, tty_id = %d\n",  currPCB->pid, tty_id);
	
	// make the tty available again.
	currTransmitters[tty_id] = NULL;
	
	q_t *transmittingQ = transmittingQs[tty_id];
	
	// first test if the queue is NULL (fatal error)
	if (transmittingQ == NULL) {
		TracePrintf(1, "unblockTransmitter: fatal error: transmittingQ is null\n");
		return ERROR;
	}

	PCB_t *unblockedPCB = (PCB_t *) deq_q(transmittingQ);

	// only move to readyQ if there is a blocked transmitter
	if (unblockedPCB != NULL) {
		return enq_q(readyQ, unblockedPCB);
	}

	return SUCCESS;
}

int waitTransmitter(int tty_id) {
	TracePrintf(1, "waitTransmitter() called, currPCB->pid = %d, tty_id = %d\n",  currPCB->pid, tty_id);
	
	currTransmitters[tty_id] = currPCB;

	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) {
		TracePrintf(1, "waitTransmitter: readyQ is empty\n");
		return ERROR;
	}
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}

int signalTransmitter(int tty_id) {
	TracePrintf(1, "signalTransmitter() called, currPCB->pid = %d, tty_id = %d\n",  currPCB->pid, tty_id);
	if (enq_q(readyQ, currPCB) == ERROR) return ERROR;

	PCB_t *nextPCB = currTransmitters[tty_id];
	
	if (nextPCB == NULL) {
		TracePrintf(1, "signalTransmitter: readyQ is empty\n");
		return ERROR;
	}
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}

int blockReader(int tty_id) {
	TracePrintf(1, "blockReader() called, currPCB->pid = %d, tty_id = %d\n",  currPCB->pid, tty_id);
	q_t *readingQ = readingQs[tty_id];
	
	if (enq_q(readingQ, currPCB) == ERROR) return ERROR;

	PCB_t *nextPCB = (PCB_t *) deq_q(readyQ);
	
	if (nextPCB == NULL) {
		TracePrintf(1, "blockReader: readyQ is empty\n");
		return ERROR;
	}
	
	return KernelContextSwitch(switchBetween, currPCB, nextPCB);
}

int unblockReader(int tty_id) {
	TracePrintf(1, "unblockReader() called, currPCB->pid = %d, tty_id = %d\n",  currPCB->pid, tty_id);
	q_t *readingQ = readingQs[tty_id];
	
	// first test if the queue is NULL (fatal error)
	if (readingQ == NULL) {
		TracePrintf(1, "unblockReader: fatal error: readyQ is null\n");
		return ERROR;
	}

	PCB_t *unblockedPCB = (PCB_t *) deq_q(readingQ);

	// only move to readyQ if there is a blocked reader
	if (unblockedPCB != NULL) {
		 return enq_q(readyQ, unblockedPCB);
	}

	return SUCCESS;
}


/* -------- scheduler-specific functions -------- */

/* when calling with currPcbP == NULL, assume that the caller is exitProcess(). */
KernelContext *switchBetween(KernelContext *currKctxt, void *currPcbP, void *nextPcbP) {	
	if (currPcbP != NULL) {
		TracePrintf(1, "switchBetween() called, currPcbP->pid = %d, nextPcbP->pid = %d\n", ((PCB_t *) currPcbP)->pid, ((PCB_t *) nextPcbP)->pid);
		// copy kctxt into currPcbP (after this, no more operation on currPcbP)
		memmove((((PCB_t *) currPcbP)->kctxt), currKctxt, sizeof(KernelContext));
	}
	else {
		TracePrintf(1, "switchBetween() called, currPcbP = NULL, nextPcbP->pid = %d\n", ((PCB_t *) nextPcbP)->pid);
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


/* assumes that the currPCB is the process calling Exec(). Give it the starter kernel
 * state so it can resume running.
 */
KernelContext *execTo(KernelContext *currKctxt, void *nil0, void *nil1) {	
	TracePrintf(1, "execTo() called, currPCB->pid = %d\n", currPCB->pid);
	
    // unlike switchBetween, no need to change kernel stacks
	
	// the process must already have its kctxt and kernel stack allocated...
	memmove(currPCB->kctxt, starterKctxt, sizeof(KernelContext));
	memmove((void *) KERNEL_STACK_BASE, starterKernelStack, KERNEL_STACK_MAXSIZE);
	
    /* no need to change the MMU register for R1 or flushing because
	 * it was done in LoadProgram() (TOTHINK: is this true, though?)
	 */
	
    return currPCB->kctxt;
}


// only used in delete_process()
void free_zombie_t(void *zombiep_item) {
	free(zombiep_item);
}


/* delete_process() frees a PCB and all content within, including its uctxt,
 * kctxt, and r1PageTable. Also frees the allocated physical memory. Depends
 * on the FrameList module. Assumes that the call is followed by a "currPcbP=NULL"
 * KernelContextSwitch, and thus does not flush the TLB.
 *
 * Used along with the Queue module, and thus the signature.
 */
void delete_process(void *pcbp_item) {
	if (pcbp_item == NULL) {
		TracePrintf(1, "delete_process() trying to delete a NULL, halting.\n");
		Halt();
	}
	
	TracePrintf(1, "delete_process() called, deleting process whose pid = %d\n", ((PCB_t *) pcbp_item)->pid);
	PCB_t *pcbp = (PCB_t *) pcbp_item;
	free(pcbp->uctxt);
	free(pcbp->kctxt);
	delete_q(pcbp->zombieQ, free_zombie_t);
	
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

/* errorCountp is an integer pointer; used in tickDownSleepers as
 * function to iterate over the sleepingMap, decrementing each sleeper's
 * numRemainingDelayTicks.
 */
void tickDownSleeper(void *errorCountp, int key, void *sleeper) {
	PCB_t *sleeperPCB = (PCB_t *) sleeper;
	TracePrintf(1, "tickDownSleeper called; errorCount = %d, key pid = %d, sleeper's pid = %d,  called\n", *((int *)errorCountp), key, sleeperPCB->pid);
	
	// tick down the sleeper...
	sleeperPCB->numRemainingDelayTicks--;
	
	/* 	if remaining ticks <= 0, move to readyQ and remove from
		sleepingMap; we SHOULD only be dealing with non-negative
		ticks, since sleepProcess() ensures that the PCBs that
		enter the sleepingMap have numRemainingDelayTicks > 0.
	*/
	if (sleeperPCB->numRemainingDelayTicks <= 0) {
		/* TOTHINK: removing a HashMap node while iterating through it is bad practice,
		 * though the HashMap module has been updated to tolerate this behavior.
		 * Maybe a special data structure should be made for sleepingQ, since multiple
		 * processes might wake up in one tick-down iteration...
		 *
		 * This is cleaner than the original tick_down_sleepers_q, but also less
		 * efficient (remove is linear-time!).
		 */
		if (HashMap_remove(sleepingMap, sleeperPCB->pid) == NULL || \
			enq_q(readyQ, sleeperPCB) == ERROR) {
			// back to an integer pointer and then dereferenced...
			(*((int *)errorCountp))++;
			return;
		}
	}
}
