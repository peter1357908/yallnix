#include <hardware.h>

int KernelFork(void) {}

int KernelExec(char *filename, char **argvec) {}
    
void KernelExit(int status){}

int KernelWait(int *status_ptr){}

int KernelGetPid(void) {
    // return currentProcess->pid
}

int KernelBrk(void *addr){}

int KernelDelay(int clock_ticks){}

int KernelReclaim(int id){}