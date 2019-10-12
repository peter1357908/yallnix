#ifndef	_KERNELCALLS_H
#define	_KERNELCALLS_H

#include <hardware.h>

int Kernel_Fork(void);

    // generate new process ID
    // allocate new PCB for child
    // copy UserContext & Kernel Context to child's PCB
    // allocate physical memory for child process
    // copy parent's page tables (for validity & r/w/e bits) to child's
    // copy parent's contents to child's new frames
    // inform scheduler that child process is ready to run

int Kernel_Exec(char *filename, char **argvec, UserContext *uctxt);
    // uspace = NULL
    // uctxt = NULL
    // load program filename into current address space
    // copy arguments argvec to memory in address space
    // intialize hardware context to start execution at start

void Kernel_Exit(int status, UserContext *uctxt);

int Kernel_Wait(int *status_ptr, UserContext *user_context);

int Kernel_GetPid();
    // return current_process -> pid

int Kernel_Brk(void *addr);

int Kernel_Delay(UserContext *uctxt, int clock_ticks);

int Kernel_Reclaim(int id);


// lock stuff
int Kernel_LockInit(int *lock_idp);

int Kernel_Acquire(int lock_id);

int Kernel_Release(int lock_id); 


// conditional variable stuff
int Kernel_CvarInit(int *cvar_idp);

int Kernel_CvarSignal(int cvar_id);

int Kernel_CvarBroadcast(int cvar_id);

int Kernel_CvarWait(int cvar_id, int lock_id);


// tty stuff
int Kernel_TtyWrite(int tty_id, void *buf, int len);

int Kernel_TtyRead(int tty_id, void *buf, int len);

// pipe stuff
int Kernel_PipeInit(int *pip_idp);

int Kernel_PipeRead(int pipe_id, void *buf, int len);

int Kernel_PipeWrite(int pipe_id, void *buf, int len);

#endif /*!_KERNELCALLS_H*/
