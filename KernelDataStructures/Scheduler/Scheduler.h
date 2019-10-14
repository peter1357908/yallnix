<<<<<<< HEAD
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
=======
// Scheduler
    // scheduler will manage processes and moving them 
    // between states
    // implementation TBD

    // PCB_T * currentProcess
    // PCB_T * idleProcess -- should always be in readyMap

    // createProcess()
        // calls generateProcessID()

    // generateProcessID()
        // increment processCount
        // return processCount

    // int processCount 
    
    // runningMap (HashMap: pid -> PCB_T *)
    // readyMap (HashMap: pid -> PCB_T *) 
>>>>>>> 8d5601ea1601098457e04334edb81f12ba0c1a6f
    // blockedMap (HashMap: pid -> PCB_T *)