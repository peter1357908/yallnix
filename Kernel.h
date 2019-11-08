// Scheduler (see KernelDataStructures/Scheduler/)

int itemCount; // use this for lock, cvar, and pipe ids
// LockMap (HashMap: lock_id -> Lock *)
// CVarMap (HashMap: cvar_id -> CVar *)
// PipeMap (HashMap: pipe_id -> Pipe *)

// FreePMList (LinkedList)
    // a byte array 

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

#include <hardware.h>
#include "KernelDataStructures/FrameList/FrameList.h"

#define KILL 42
#define SUCCESS 0

// for convenience
#define KERNEL_STACK_BASE_VPN  (KERNEL_STACK_BASE >> PAGESHIFT)
#define KERNEL_BASE_VPN  (VMEM_0_BASE >> PAGESHIFT)

frame_t *FrameList;
int numFrames;

// for scheduler module
KernelContext *starterKctxt;
void *starterKernelStack;
struct pte *r0StackBasePtep;

// for kernel module
void *kernelDataStart;
void *currKernelBrk;

// spare pte and the associated address, for manipulation in fork() and forkTo()
struct pte *tempPtep;
void *tempVAddr;

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd) ;

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt);

int SetKernelBrk(void *addr) ;