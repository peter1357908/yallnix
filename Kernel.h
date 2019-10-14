// Scheduler (see KernelDataStructures/Scheduler/)

int itemCount; // use this for lock, cvar, and pipe ids
// LockMap (HashMap: lock_id -> Lock *)
// CVarMap (HashMap: cvar_id -> CVar *)
// PipeMap (HashMap: pipe_id -> Pipe *)

// FreePMList (LinkedList)
    // a linked list of Frames
    // remove frames from List when we use them
    // append frames to tail when free'd

// Interrupt Vector Array
// Syscall Vector Array

// int cvar_clock_id; // id to clock's Cvar 
    // place this in CvarMap
    // handleTrapClock() will broadcast to this cVar

// lock_t tty0Lock
// bool tty0CanTransfer = true
// int cvar_tty0 // place in CVarMap
// void * tty0Buf -- buffer that can grow dynamically

// lock_t tty1Lock
// bool tty1CanTransfer = true
// int cvar_tty1 // place in CVarMap
// void * tty1Buf -- buffer that can grow dynamically

// lock_t tty2Lock
// bool tty2CanTransfer = true
// int cvar_tty2 // place in CVarMap
// void * tty2Buf -- buffer that can grow dynamically

// lock_t tty3Lock
// bool tty3CanTransfer = true
// int cvar_tty3 // place in CVarMap
// void * tty2Buf -- buffer that can grow dynamically

// the following variables stores info for building the initial page table
void *kernelDataStart;  // everything until this is READ and EXEC
void *currKernelBrk;  // everything from KernelDataStart until this is READ and WRITE

/* during bootstrap, the hardware calls this function to tell us about the
 * data segment, before virtual memory initialization. It initializes the 
 * CurrKernelBrk before the first call to SetKernelBrk
 */

// see manual page 46
void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd) {
	kernelDataStart = _KernelDataStart;
	currKernelBrk = _KernelDataEnd;  // _KernelDataEnd is the lowest address NOT in use
}

// see manual page 50
void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
	// initialize Interrupt Vector Array and REG_VECTOR_BASE
	// initialize FreePMList, given info such as currKernelBrk and pmem_size
	// initialize the initial pagetable (think of this as for the initial idle process)
	// virtualize everything according to the physical memory
	// set MMU registers (REG_PTBR0, REG_PTLR0, REG_PTBR1, REG_PTLR1)
	// WriteRegister(REG_VM_ENABLE, 1);
	// initialize the idleProcess (e.g. fill the pagetable accordingly)
	// initialize and run the first process (via scheduler, given uctxt)
}

// see manual page 46
/* special malloc (provided) will call this function to tell us about the heap segment
 * if it's before VM, we can safely assume that addr is higher than currKernelBrk (only malloc can call it)
 * if it's after VM, the procedure is analogous to KernelBrk() (since we can also call it)
 */
int SetKernelBrk(void *addr) {
	unsigned int isVM = ReadRegister(REG_VM_ENABLE);
	if (isVM) {
		// if addr lower than currKernelBrk:
			// for each PTE *ptep in to-be-invalidated pages:
				// invalidatePTE(ptep);
		// if addr higher than currKernelBrk:
			// for each PTE *ptep in to-be-allocated pages:
				// grab pfn from FreePMList
				// validatePTE(ptep, prot, pfn);
	}
	currKernelBrk = addr;
	// return ERROR if error else 0
}
