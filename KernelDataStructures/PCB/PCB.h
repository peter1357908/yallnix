// PCB
    // int pid
    // void * region1Base
    // void * region1Limit
    // void * KernelStackBase
    // void * brk;
    // UserContext * uctxt;
    // KernelContext * kctxt;
    // ZombieQueue * zq;
    // int numChildren;
    // PCB *parent; -- currently only using this for KernelExit/KernelWait