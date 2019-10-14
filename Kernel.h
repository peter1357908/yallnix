// Scheduler (see KernelDataStructures/Scheduler/)

// int itemCount -- use this for lock, cvar, and pipe ids
// LockMap (HashMap: lock_id -> Lock *)
// CVarMap (HashMap: cvar_id -> CVar *)
// PipeMap (HashMap: pipe_id -> Pipe *)

// FreePMList (LinkedList)
    // a linked list of Frames
    // remove frames from List when we use them
    // append frames to tail when free'd

// Interrupt Vector Array
// Syscall Vector Array

// KernelStart()
// SetKernelData()
// SetKernelBreak()

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
