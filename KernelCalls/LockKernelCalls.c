int Kernel_LockInit(int *lock_idp) {
    // creates a lock in LockMap
    // assigns lock_id to lock_idp
    // increment lockCount
    // return 0 if no error
    // else return ERROR
}

int Kernel_Acquire(int lock_id) {
    // lock = LockMap[lock_id]
    // throw error if lock_id doesn't exist
    // while lock.isLocked:
        // move process from running to blocked

    // (below has to be atomic -- ask Sean)
    // set lock->isLocked = True
    // set lock->ownerPID = currentProcess->pid
}

int Kernel_Release(int lock_id) {
    // lock = LockMap[lock_id]
    // throw error if lock_id doesn't exist
    // set lock->isLocked = False
}
