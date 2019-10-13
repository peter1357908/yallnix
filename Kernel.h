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
