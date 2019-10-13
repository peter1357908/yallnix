// Scheduler
    // PCB_T * currentProcess
    // PCB_T * idleProcess

    // createProcess()
        // calls generateProcessID()

    // generateProcessID()
        // increment processCount
        // return processCount

    // int processCount 
    
    // runningMap (HashMap: pid -> PCB_T *)
    // readyMap (HashMap: pid -> PCB_T *)
    // blockedMap (HashMap: pid -> PCB_T *)