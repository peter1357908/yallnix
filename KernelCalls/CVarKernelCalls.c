int Kernel_CvarInit(int *cvar_idp) {
    // creates a CVar in the CVar Map
    // assigns cvar_id to cvar_idp
    // increment cvarCount
    // return 0 if no error
    // else return ERROR
}

int Kernel_CvarSignal(int cvar_id){
    // pid = Cvar.pop() (reminder that Cvar is a queue)
    // move process PID from blocked to ready
    // return 0 if no error
    // else return ERROR
}

int Kernel_CvarBroadcast(int cvar_id){
    // while !Cvar.isEmpty():
        // status = CvarSignal(cvar_id)
        // if status == ERROR:
            // return ERROR
    // return 0
}

int Kernel_CvarWait(int cvar_id, int lock_id){
    // cvar = getCVar(cvar_id)
    // cvar.addProcess(currentProcess->pid)
    // move process from running to blocked

    // acquire lock again
    // return 0 if no error
    // else return ERROR
}