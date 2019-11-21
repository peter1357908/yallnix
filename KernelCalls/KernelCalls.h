#ifndef	_KernelCalls_h
#define	_KernelCalls_h

#include <hardware.h>

/* -------- utility functions -------- */

/* a utility function asserting that the first (len) bytes 
 * of the buffer INCLUDE the given prot type(s),
 * as defined in hardware.h
 * return 1 (TRUE) if test passed; 0 (FALSE) otherwise.
 * (a non-positive len guarantees a TRUE return value)
 */
int isValidBuffer(void *buffer, int len, unsigned long prot);

/* a utility function asserting that the string is not NULL, in
 * one region (kernel / user), and completely readable.
 * return 1 (TRUE) if test passed; 0 (FALSE) otherwise.
 *
 * the sole point of this function is for robustness, not security
 * (just so that Kernel doesn't die due to a bad memory access)
 *
 * see Yalnix Manual Page 43.
 *
 * can be easily tuned to return the length of the string, too...
 */
int isValidString(char *string);

/* -------- KernelCall functions -------- */

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
