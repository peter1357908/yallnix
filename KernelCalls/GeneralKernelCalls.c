#include <hardware.h>
#include <yalnix.h>
#include <string.h>
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../KernelDataStructures/FrameList/FrameList.h"
#include "../KernelDataStructures/PageTable/PageTable.h"
#include "../KernelDataStructures/SyncObjects/Lock.h"
#include "../KernelDataStructures/SyncObjects/CVar.h"
#include "../KernelDataStructures/SyncObjects/Pipe.h"
#include "../GeneralDataStructures/HashMap/HashMap.h"
#include "KernelCalls.h"
#include "../LoadProgram.h"
#include "../Kernel.h"

int isValidBuffer(void *buffer, int len, unsigned long prot) {
	if (buffer == NULL) {
		TracePrintf(1, "isValidBuffer: the given buffer is NULL, returning 0 (FALSE)\n");
		return 0;
	}
	if (len <= 0) {
		TracePrintf(1, "isValidBuffer: given a non-positive len, returning 1 (TRUE)\n");
		return 1;
	}
	
	unsigned int firstPageVpn = ((unsigned int) buffer)>>PAGESHIFT;
	unsigned int maxVpn;
	struct pte *currPtep;
	// get the page table entry for the base of the string
	if (firstPageVpn < MAX_PT_LEN) {
		maxVpn = MAX_PT_LEN;
		currPtep = (struct pte *) ReadRegister(REG_PTBR0) + firstPageVpn - KERNEL_BASE_VPN;
	}
	else if (firstPageVpn < MAX_PT_LEN * 2) {
		maxVpn = MAX_PT_LEN * 2;
		currPtep = (struct pte *) ReadRegister(REG_PTBR1) + firstPageVpn - USER_BASE_VPN;
	}
	else {
		TracePrintf(1, "isValidBuffer: the buffer is not in the valid virtual address space; returning 0 (FALSE)\n");
		return 0;
	}
	
	unsigned int currVpn = firstPageVpn;
	int remainingLen = len;
	while (remainingLen > 0) {
		// first check if the page is even valid
		if (currPtep->valid != 1) {
			TracePrintf(1, "isValidBuffer: encountered an invalid page; returning 0 (FALSE)\n");
			return 0;
		}
		// then check if it INCLUDEs the right prot type(s)
		if ((currPtep->prot & prot) != prot) {
			TracePrintf(1, "isValidBuffer: encountered a page with incorrect prot (checking for \"%d\" but the page has \"%d\"); returning 0 (FALSE)\n", prot, currPtep->prot);
			return 0;
		}
		remainingLen -= PAGESIZE;
		currPtep++;
	}
	
	// no more bytes to check; the buffer is valid.
	return 1;
}

int isValidString(char *string) {
	if (string == NULL) {
		TracePrintf(1, "isValidString: the given string is NULL, returning 0 (FALSE)\n");
		return 0;
	}
	unsigned int firstPageBase = DOWN_TO_PAGE(string);
	unsigned int firstPageVpn = firstPageBase>>PAGESHIFT;
	
	unsigned int maxVpn;
	struct pte *currPtep;
	// get the page table entry for the base of the string
	if (firstPageVpn < MAX_PT_LEN) {
		maxVpn = MAX_PT_LEN;
		currPtep = (struct pte *) ReadRegister(REG_PTBR0) + firstPageVpn - KERNEL_BASE_VPN;
	}
	else if (firstPageVpn < MAX_PT_LEN * 2) {
		maxVpn = MAX_PT_LEN * 2;
		currPtep = (struct pte *) ReadRegister(REG_PTBR1) + firstPageVpn - USER_BASE_VPN;
	}
	else {
		TracePrintf(1, "isValidString: the string is not in the valid virtual address space; returning 0 (FALSE)\n");
		return 0;
	}
	
	// NOTE: cannot use UP_TO_PAGE for nextPageBase!
	char *currCharp = string;  // pointer to current character
	unsigned int nextPageBase = firstPageBase + PAGESIZE;
	unsigned int currVpn;
	for (currVpn = firstPageVpn; currVpn < maxVpn; currVpn++) {
		// first check if the page is even valid and readable
		if (currPtep->valid != 1 || \
			(currPtep->prot & PROT_READ) != PROT_READ) {
			TracePrintf(1, "isValidString: encountered a bad page (invalid or unREADable); returning 0 (FALSE)\n");
			return 0;
		}
		// check for the terminating null until we run into the next page
		while ((unsigned int) currCharp < nextPageBase) {
			if (*currCharp == '\0') return 1;
			currCharp++;
		}
		// increment the nextPageBase for the next iteration
		nextPageBase += PAGESIZE;
	}
	
	TracePrintf(1, "isValidString: the string is not null-terminated in the same region as its base; returning 0 (FALSE)\n");
	return 0;
}

int KernelFork(void) {
	TracePrintf(1, "KernelFork() called, currPCB->pid = %d\n",  currPCB->pid);
    PCB_t *parentPCB = currPCB;
    PCB_t *childPCB;  

    if (initPCB(&childPCB) == ERROR) return ERROR;

    childPCB->kctxt = (KernelContext *) malloc(sizeof(KernelContext));
    memmove(childPCB->uctxt, parentPCB->uctxt, sizeof(UserContext));
    childPCB->brk = parentPCB->brk;

    /* Copy the parent's r1 pagetable and map each entry with a new frame
	 * then copy the parent's r1 content into those new frames.
	 *
	 * Since virtual memory for parent & child process will be the same,
     * we use a "temp" page located below the Kernel Stack to write to
     * the child process's physical memory.
	 */
    int addr;
    u_long pfn; 
    struct pte *currParentPte = parentPCB->r1PageTable;
    struct pte *currChildPte = childPCB->r1PageTable;
	
	for (addr = VMEM_1_BASE; addr < VMEM_1_LIMIT; addr += PAGESIZE) {
		if (currParentPte->valid == 1) {
			if (getFrame(FrameList, numFrames, &pfn) == ERROR) return ERROR;
			/* NOTE: unlike LoadProgram(), we don't have to worry about the stack
			 * being READ|EXEC, because we copy via the tempPte manipulation
			 */
			
			// set tempPte's pfn to currChildPte's pfn
			setPageTableEntry(tempPtep, 1, PROT_WRITE, pfn);
			/* copy from parent v-addr to temp v-addr
				this should copy parent's memory contents to child's memory
			*/
			memmove(tempVAddr, (void *) addr, PAGESIZE);
			
			setPageTableEntryNoFlush(currChildPte, 1, currParentPte->prot, pfn);
        }
		
		currParentPte++;
		currChildPte++;
    }
	
	// reset the tempPte
	setPageTableEntry(tempPtep, 0, PROT_NONE, 0);

    // NOTE: the Kernel Stack is copied over during the context switch

    childPCB->parent = parentPCB;
    HashMap_insert(parentPCB->childrenMap, childPCB->pid, childPCB);
    (parentPCB->numChildren)++; 
    
    if (forkProcess(childPCB) == ERROR) return ERROR;
	
	TracePrintf(1, "Inside KernelFork(), after forkProcess(), currPCB->pid = %d\n",  currPCB->pid);

    if (currPCB->pid == parentPCB->pid) {
        return childPCB->pid;
    }
    else if (currPCB->pid == childPCB->pid)  {
        return SUCCESS;
    }
    else {
        TracePrintf(1, "KernelFork: resuming with a process that's neither the child or the parent! Returning ERROR\n");
        return ERROR;
    }
}

int KernelExec(char *filename, char **argvec) {
	TracePrintf(1, "KernelExec() called, currPCB->pid = %d\n",  currPCB->pid);
	
	// validate the input first. Check both the filename and the argvec.
	if (isValidString(filename) == 0) {
		TracePrintf(1, "KernelExec: input filename is not a valid string\n");
		return ERROR;
	}
	
	/* a similar part in LoadProgram assumes that the last element of the argvec
	 * is a NULL... The following loop implicitly makes sure that it is 
	 * (if it's not, we will keep going until running into an invalid string`)
	 */
	unsigned int i;
	for (i = 0; argvec[i] != NULL; i++) {
		if (isValidString(argvec[i]) == 0) {
			TracePrintf(1, "KernelExec: the %d-th string in the input argvec is invalid\n", i);
			return ERROR;
		}
	}

    int loadProgramStatus = LoadProgram(filename, argvec, currPCB);
	
	if (loadProgramStatus == KILL) {
		TracePrintf(1, "KernelExec: LoadProgram returned KILL; calling KernelExit\n");
		KernelExit(ERROR);
		
	}
    if (loadProgramStatus == ERROR || execProcess() == ERROR) {
		TracePrintf(1, "KernelExec: LoadProgram or execProcess returned ERROR; returning ERROR\n");
		return ERROR;
	}

    
	
	return SUCCESS;
}
    
void KernelExit(int status) {
	TracePrintf(1, "KernelExit() called, currPCB->pid = %d, status = %d\n",  currPCB->pid, status);
	if (currPCB->pid == initPid) {
		TracePrintf(1, "init process exited; Calling Halt()...\n");
		Halt();
	}
	
	if (exitProcess(status) == ERROR) Halt();
}

int KernelWait(int *status_ptr) {
	TracePrintf(1, "KernelWait() called, currPCB->pid = %d\n",  currPCB->pid);
    int region = getAddressRegion(status_ptr);
    if (region != 1) {
        TracePrintf(1, "KernelWait: status_ptr not in region 1\n");
        return ERROR; 
    } 
    
    u_long prot;
	// the prot must be exactly READ|WRITE
    if (getAddressProt(status_ptr, 1, &prot) == ERROR || prot != (PROT_READ | PROT_WRITE)) {
            TracePrintf(1, "KernelWait: status_ptr not READ|WRITE\n");
            return ERROR;
    }
    if (currPCB->numChildren <= 0) {
        TracePrintf(1, "KernelWait: no children to wait for\n");
        return ERROR;
	}
	
	// if the parent has no exited children yet, block the parent...
    if (peek_q(currPCB->zombieQ) == NULL) {
        if (blockProcess() == ERROR) {
			Halt();
		}
	}
    // ** process is now running & zombieQueue has children ** 
    zombie_t *childZombiep = deq_q(currPCB->zombieQ);
	if (childZombiep == NULL) {
		TracePrintf(1, "KernelWait: is returning with ERROR because currPCB (pid = %d) should be a parent unblocked from Wait(), but dequeuing its zombieQ somehow returned NULL\n", currPCB->pid);
		return ERROR;
	}
	
    *status_ptr = childZombiep->exit_status;
    
	int childPid = childZombiep->pid;
	
	free(childZombiep);
	
	return childPid;
}

int KernelGetPid() {
    TracePrintf(1, "KernelGetPid() called, currPCB->pid = %d\n",  currPCB->pid);
    return currPCB->pid;
}

int KernelBrk(void *addr) {
    TracePrintf(1, "KernelBrk() called, currPCB->pid = %d, addr = %x\n",  currPCB->pid, addr);
	
	/* only care about if the future brk is in region 1 (we do not care
	 * if it will grow into the user stack or lower into the user data)
	 */
    int region = getAddressRegion(addr);
    if (region != 1) {
        TracePrintf(1, "KernelBrk: addr not in region 1\n");
        return ERROR; 
    }

    void *currBrk = currPCB->brk;
	
	unsigned int addr_ui = (unsigned int) addr;
	unsigned int currBrk_ui = (unsigned int) currBrk;
	unsigned int addrVpn = addr_ui>>PAGESHIFT;
	unsigned int currBrkVpn = currBrk_ui>>PAGESHIFT;
	
	struct pte *r1PageTable = currPCB->r1PageTable;
	struct pte *targetPtep;
	unsigned int currVpn;

	/* if addr's page is in a lower page than currBrk, 
	 * invalidate pages and free frames accordingly
	 */
	if (addrVpn < currBrkVpn) {
		/*	we only want to free the page containing addr if addr 
			is at the bottom of the page. If so, we want to invalidate 
			pages starting with addr's page. Otherwise, we want to 
			invalidate pages starting with the following page.
		*/
		if (DOWN_TO_PAGE(addr_ui) == addr_ui) { 
			currVpn = addrVpn;
		}
		else {
			currVpn = addrVpn + 1;
		}
		/* Also, we don't want to invalidate the page the currBrk is in
		 * if it's already invalid (currBrk at the beginning of a page)
		 */
		if (DOWN_TO_PAGE(currBrk_ui) == currBrk_ui) {
			currBrkVpn--;  // so the upcoming loop terminates one iteration earlier
		}
		
		targetPtep = r1PageTable + currVpn - USER_BASE_VPN;
		while (currVpn <= currBrkVpn) {
			freeFrame(FrameList, numFrames, targetPtep->pfn);
			invalidatePageTableEntry(targetPtep);
			
			currVpn++;
			targetPtep++;
		}
	}

	// if addr is in the same page... it depends...
	else if (addrVpn == currBrkVpn) {
		/* if addr is lower, then we need to invalidate the page if
		 * addr is at the beginning of the page
		 */ 
		if (addr_ui < currBrk_ui && DOWN_TO_PAGE(addr_ui) == addr_ui) {
			targetPtep = r1PageTable + addrVpn - USER_BASE_VPN;
			freeFrame(FrameList, numFrames, targetPtep->pfn);
			invalidatePageTableEntry(targetPtep);
		}
		/* if addr is higher, then we need to validate the page if brk was
		 * at the beginning of the page
		 */
		else if (addr_ui > currBrk_ui && DOWN_TO_PAGE(currBrk_ui) == currBrk_ui) {
			u_long pfn;
			targetPtep = r1PageTable + addrVpn - USER_BASE_VPN;
			if (getFrame(FrameList, numFrames, &pfn) == ERROR) return ERROR;
			setPageTableEntry(targetPtep, 1, (PROT_READ|PROT_WRITE), pfn);
		}
		// if addr is the same as the currBrk, nothing happens.
	}
	
	// if addr is in a higher page...
	else {
		// similarly, we don't want to re-allocate an existing page...
		if (DOWN_TO_PAGE(currBrk_ui) == currBrk_ui) { 
			currVpn = currBrkVpn;
		}
		else {
			currVpn = currBrkVpn + 1;
		}
		/* Also, we don't want to validate another page if addr is at
		 * the beginning of a page...
		 */
		if (DOWN_TO_PAGE(addr_ui) == addr_ui) {
			addrVpn--;  // so the upcoming loop terminates one iteration earlier
		}
		
		u_long pfn;
		targetPtep = r1PageTable + currVpn - USER_BASE_VPN;
		while (currVpn <= addrVpn) {
			if (getFrame(FrameList, numFrames, &pfn) == ERROR) return ERROR;
			setPageTableEntry(targetPtep, 1, (PROT_READ|PROT_WRITE), pfn);
			
			currVpn++;
			targetPtep++;
		}
	}

    currPCB->brk = addr;
    return SUCCESS;
}

int KernelDelay(int clock_ticks) {
    TracePrintf(1, "KernelDelay() called, currPCB->pid = %d, clock_ticks = %d\n",  currPCB->pid, clock_ticks);
	if (clock_ticks != 0) {
		if (clock_ticks < 0) {
            TracePrintf(1, "KernelDelay: negative clock_ticks\n");
			return ERROR;
        } 
        if (sleepProcess(clock_ticks) == ERROR) {
            return ERROR;
		}
	}
    // scheduler should grab another ready process & context switch into it
    return SUCCESS;
}


int KernelReclaim(int id) {
	/* since deletion functions return ERROR if the Map does not have the item
	 * corresponding to the given id, we need to first make sure that
	 * the Map has the item, before returning with the deletion function
	 */
	if (getLock(id) != NULL) return deleteLock(id);
	if (getCvar(id) != NULL) return deleteCvar(id);
	if (getPipe(id) != NULL) return deletePipe(id);
	
	// the given id does not correspond to any sync object; return ERROR
    TracePrintf(1, "KernelReclaim: id does not correspond to any sync object\n");
	return ERROR;
}