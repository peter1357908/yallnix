#include <hardware.h>
#include "KernelDataStructures/FrameList/FrameList.h"

#define KILL 42
#define SUCCESS 0

// for convenience
#define KERNEL_STACK_BASE_VPN  (KERNEL_STACK_BASE >> PAGESHIFT)
#define KERNEL_BASE_VPN  (VMEM_0_BASE >> PAGESHIFT)
#define USER_BASE_VPN  (VMEM_1_BASE >> PAGESHIFT)

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