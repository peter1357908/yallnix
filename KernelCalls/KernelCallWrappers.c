#ifndef	_KernelCallWrappers_h
#define	_KernelCallWrappers_h

#include <yalnix.h>
#include "KernelCalls.h"

extern int Fork _PARAMS((void));
extern int Exec _PARAMS((char *, char **));
extern void Exit _PARAMS((int));
extern int Wait _PARAMS((int *));
extern int GetPid() {
    KernelGetPid();
}
extern int Brk(void *addr) {
    KernelBrk(addr);
}
extern int Delay(int clock_ticks) {
    KernelDelay(clock_ticks);
}
extern int TtyRead _PARAMS((int, void *, int));
extern int TtyWrite _PARAMS((int, void *, int));
extern int Register _PARAMS((unsigned int));
extern int Send _PARAMS((void *, int));
extern int Receive _PARAMS((void *));
extern int ReceiveSpecific _PARAMS((void *, int));
extern int Reply _PARAMS((void *, int));
extern int Forward _PARAMS((void *, int, int));
extern int CopyFrom _PARAMS((int, void *, void *, int));
extern int CopyTo _PARAMS((int, void *, void *, int));
extern int ReadSector _PARAMS((int, void *));
extern int WriteSector _PARAMS((int, void *));

extern int PipeInit _PARAMS((int *));
extern int PipeRead _PARAMS((int, void *, int));
extern int PipeWrite _PARAMS((int, void *, int));

extern int SemInit _PARAMS((int *, int));
extern int SemUp _PARAMS((int));
extern int SemDown _PARAMS((int));
extern int LockInit _PARAMS((int *));
extern int Acquire _PARAMS((int));
extern int Release _PARAMS((int));
extern int CvarInit _PARAMS((int *));
extern int CvarWait _PARAMS((int, int));
extern int CvarSignal _PARAMS((int));
extern int CvarBroadcast _PARAMS((int));

extern int Reclaim _PARAMS((int));


#endif /* _KernelCallWrappers_h */
