#include <hardware.h>
#include <yalnix.h>
#include <string.h>
#include "../KernelDataStructures/Scheduler/Scheduler.h"
#include "../KernelDataStructures/FrameList/FrameList.h"
#include ".././LoadProgram.h"
#include "../Kernel.h"

int KernelFork(void) {
    PCB_t *parentPCB = currPCB;
    PCB_t *childPCB;  

    if (initProcess(&childPCB) == ERROR)
        return ERROR;

    // copy PCB instance variables
    memmmove(childPCB->kctxt, parentPCB->kctxt, sizeof(KernelContext));
    memmmove(childPCB->uctxt, parentPCB->uctxt, sizeof(UserContext));
    childPCB->brk = parentPCB->brk;

    // copy parent's region 1 PTEs & allocate new physical frame if valid == 1
    int i;
    u_long pfn; 
    struct pte *currParentPte = parentPCB->r1PageTable;
    struct pte *currChildPte = childPCB->r1PageTable;
    for (i = 0; i < MAX_PT_LEN; i++) {
        if (currParentPte->valid == 1) {
            if (getFrame(FrameList, numFrames, &pfn) == ERROR) return ERROR;
            setPageTableEntry(currChildPte, currParentPte->valid, currParentPte->prot, pfn);
        }

        currChildPte++;
        currParentPte++; 
    }
    
    // NOTE: 
    // virtual memory for parent & child process will be the same,
    // so we will use a "temp" page located below the Kernel Stack
    // to write to the child process's memory
    struct pte *tempPte = ((struct pte *) ReadRegister(REG_PTBR0)) + KERNEL_STACK_BASE_VPN - KERNEL_BASE_VPN - 1; 
    void *tempVAddr =  (void *) (KERNEL_STACK_BASE - PAGESIZE); 
    int addr;

    // copy R1 memory
    currChildPte = childPCB->r1PageTable;
    currParentPte = parentPCB->r1PageTable;
    for (addr = VMEM_1_BASE; addr < VMEM_1_LIMIT; addr += PAGESIZE) {
        if (currParentPte->valid == 1)
            // set tempPte's pfn to currChildPte's pfn
            setPageTableEntry(tempPte, 1, PROT_WRITE, currChildPte->pfn);
            /*  copy from parent v-addr to temp v-addr
                this should copy parent's memory contents to child's
            */
            memmove(tempVAddr, addr, PAGESIZE);
        currChildPte++; 
        currParentPte++;
    }

    // NOTE: the Kernel Stack is copied over during the context switch

    childPCB->parent = parentPCB->pid;
    parentPCB->numChildren++; 
    
    if (runProcess(childPCB->pid) == ERROR) return ERROR;

    if (currPCB->pid == parentPCB->pid) {
        return childPCB->pid;
    }
    else if (currPCB->pid == childPCB->pid)  {
        return 0;
    }
    else {
        return ERROR;
    }
}

int KernelExec(char *filename, char **argvec) {
    if (LoadProgram(filename, argvec, currPCB) == ERROR || execProcess() == ERROR) {
		return ERROR;
	}
	
	return 0;
}
    
void KernelExit(int status) {
	// TOTHINK: can we just check the PCB pointers?
	// TODO: wrap Halt() with free functions
	if (currPCB->pid == initPid) {
		TracePrintf(1, "Init process exited; Calling Halt()...");
		Halt();
	}
	
	if (zombifyProcess(status) == NULL) Halt();
}

int KernelWait(int *status_ptr) {
    if (currPCB->numChildren == 0) return ERROR;
    if (peek_q(currPCB->zombieQ) == NULL) {
        blockProcess();
	}
		
    // ** process is now running & zombieQueue has children ** 
    zombie_t *childPcbp = deq_q(currPCB->zombieQ);
	if (childPcbp == NULL) return ERROR;
	
    *status_ptr = childPcbp->exit_status;
    return childPcbp->pid;
}

int KernelGetPid() {
    return currPCB->pid;
}

// int KernelGetPid() {
//     return currPCB->pid; 
// }

// assumes that brk was in correct position (e.g. below: valid; above: invalid, etc.)
int KernelBrk(void *addr) {
    void *brk = currPCB->brk;
    struct pte *r1BasePtep = currPCB->r1PageTable;
    struct pte *targetPtep;
    int currAddr;
    int vpn1 = (VMEM_1_BASE>>PAGESHIFT); // first page in VMEM_0

    // if addr lower than brk, invalidate pages and free frames accordingly
    if ((int) addr < (int) brk) {
        for (currAddr = (int) addr; currAddr < (int) brk; currAddr += PAGESIZE) {
            int vpn = (currAddr>>PAGESHIFT);
            targetPtep = r1BasePtep + vpn - vpn1;
            freeFrame(FrameList, numFrames, targetPtep->pfn);
            invalidatePageTableEntry(targetPtep);
        }
    }

    // if addr higher than brk, validate pages and allocate frames accordingly
    else {
        for (currAddr = (int) brk; currAddr < (int) addr; currAddr += PAGESIZE) {
            int vpn = (currAddr>>PAGESHIFT);
            u_long pfn;
            if (getFrame(FrameList, numFrames, &pfn) == ERROR) return ERROR;
            targetPtep = r1BasePtep + vpn - vpn1;

            // TODO: fix this line
            setPageTableEntry(targetPtep, 1, (PROT_READ|PROT_WRITE), pfn);
        }
    }
	
    currPCB->brk = addr;
    return 0;
}

int KernelDelay(int clock_ticks) {
    if (clock_ticks < 0 || sleepProcess(clock_ticks) == ERROR) {
        return ERROR;
    }
	
    // scheduler should grab another ready process & context switch into it
    return 0;
}

int KernelReclaim(int id) {
    // 
    // for each map (LockMap, CVarMap, PipeMap):
        // if id in map:
            // free(id)
    // return ERROR if error else 0
}