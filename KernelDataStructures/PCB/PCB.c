#include <hardware.h>

typedef struct PCB {
    int pid;
    void * region1PageTable;
    void *brk;
    UserContext *uctxt;
    KernelContext *kctxt;
    // ZombieQueue *zq;
    int numChildren;
    PCB_t *parent; // -- currently only using this for KernelExit/KernelWait
	struct pte *pagetable;
} PCB_t;