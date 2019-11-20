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
#include "KernelCalls.h"  // KernelExit
#include "../LoadProgram.h"
#include "../Kernel.h"

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

// assumes that brk was in correct position (e.g. below: valid; above: invalid, etc.)
int KernelBrk(void *addr) {
    
    TracePrintf(1, "KernelBrk() called, currPCB->pid = %d, addr = %x\n",  currPCB->pid, addr);
    int region = getAddressRegion(addr);
    if (region != 1) {
        TracePrintf(1, "KernelBrk: addr not in region 1\n");
        return ERROR; 
    }

    void *brk = currPCB->brk;

    /*  only update page table if addr & brk are in
        different pages
    */
    if (((int) addr>>PAGESHIFT) != ((int) brk>>PAGESHIFT)) {

        struct pte *r1BasePtep = currPCB->r1PageTable;
        struct pte *targetPtep;
        int currAddr;
        int vpn;
        int vpn1 = (VMEM_1_BASE>>PAGESHIFT); // first page in VMEM_0

        if ((int) addr < (int) currKernelBrk) {
			
			// get vpn & ptep of addr (the new brk)
			vpn = (((int) addr)>>PAGESHIFT);
			targetPtep = r1BasePtep + vpn - vpn1;
			
			/*	we only want to free the page containing addr if if addr is at the bottom 
				of the page. So, if targetPtep->pfn equals add (addr is at the bottom of its own page),
				we want to invalidate pages starting with addr's page. Otherwise, we want to invalidate
				pages starting with the following page.
			*/
			if (targetPtep->pfn == (int) addr) 
				currAddr = (int) addr;
			else
				currAddr = ((int) addr) + PAGESIZE;
			
			while (currAddr < (int) currKernelBrk) {
				freeFrame(FrameList, numFrames, targetPtep->pfn);
				invalidatePageTableEntry(targetPtep);
				vpn = (currAddr>>PAGESHIFT);
				targetPtep = r1BasePtep + vpn - vpn1;
				currAddr += PAGESIZE;
			}
		}

        // if addr higher than brk, validate pages and allocate frames accordingly
       else if ((int) addr > (int) brk) {
            for (currAddr = (int) brk; currAddr < (int) addr; currAddr += PAGESIZE) {
                vpn = (currAddr>>PAGESHIFT);
                u_long pfn;
                if (getFrame(FrameList, numFrames, &pfn) == ERROR) return ERROR;
                targetPtep = r1BasePtep + vpn - vpn1;

                // TODO: fix this line
                setPageTableEntry(targetPtep, 1, (PROT_READ|PROT_WRITE), pfn);
            }
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