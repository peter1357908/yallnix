#include <hardware.h>

typedef struct PCB {
    int pid;
	void *currStackBase;
    void *brk;
    UserContext *uctxt;
    KernelContext *kctxt;
    // ZombieQueue *zq;
    int numChildren;
    PCB_t *parent; // -- currently only using this for KernelExit/KernelWait
	struct pte *pagetable;
} PCB_t;