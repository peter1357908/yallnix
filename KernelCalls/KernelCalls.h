#ifndef	_KernelCalls_h
#define	_KernelCalls_h

#include <hardware.h>

int Kernel_Fork(void);
int Kernel_Exec(char *filename, char **argvec);
void Kernel_Exit(int status);
int Kernel_Wait(int *status_ptr);
int Kernel_GetPid();
int Kernel_Brk(void *addr);
int Kernel_Delay(int clock_ticks);
int Kernel_Reclaim(int id);

// lock stuff
int Kernel_LockInit(int *lock_idp);
int Kernel_Acquire(int lock_id);
int Kernel_Release(int lock_id); 

// cvar stuff
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

#endif /* _KernelCalls_h */
