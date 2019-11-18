#ifndef	_KernelCalls_h
#define	_KernelCalls_h

#include <hardware.h>

int KernelFork(void);
int KernelExec(char *filename, char **argvec);
void KernelExit(int status);
int KernelWait(int *status_ptr);
int KernelGetPid(void);
int KernelBrk(void *addr);
int KernelDelay(int clock_ticks);
int KernelReclaim(int id);

// tty stuff
int KernelTtyWrite(int tty_id, void *buf, int len);
int KernelTtyRead(int tty_id, void *buf, int len);

#ifdef LINUX
// lock stuff
int KernelLockInit(int *lock_idp);
int KernelAcquire(int lock_id);
int KernelRelease(int lock_id); 

// cvar stuff
int KernelCvarInit(int *cvar_idp);
int KernelCvarSignal(int cvar_id);
int KernelCvarBroadcast(int cvar_id);
int KernelCvarWait(int cvar_id, int lock_id);

// pipe stuff
int KernelPipeInit(int *pip_idp);
int KernelPipeRead(int pipe_id, void *buf, int len);
int KernelPipeWrite(int pipe_id, void *buf, int len);
#endif /* LINUX */

#endif /* _KernelCalls_h */
