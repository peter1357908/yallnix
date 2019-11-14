// Cvar.h
#ifndef _CVar_h
#define _CVar_h

#include "../../GeneralDataStructures/Queue/Queue.h"

typedef struct cvar {
    /*  pid of process waiting on cvar
    */
    int pid;
    /*  lock_id for the lock process
        is waiting to acquire
    */
    int lock_id;
} cvar_t;

/*  returns ERROR/SUCCESS; intializes cvar and saves
    cvar_id to cvar_idp
*/
int initCvar(int *cvar_idp);

/*  return 1 if cvar queue is empty (no processes waiting on cvar)
    return 0 if cvar queue has processes waiting
*/ 
int isCvarEmpty(int cvar_id);

/*  return ERROR/SUCCESS; pushes process (by pid)
    into cvar_id's queue
*/
int pushCvarQ(int cvar_id, int lock_id, int pid);

/*  return ERROR/SUCCESS pops a process (by pid & lock_id) from
    the cvar queue & assigns to cvarP
*/
int popCvarQ(int cvar_id, cvar_t *cvarP);

#endif /* _CVar_h */