#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/Hashmap/Hashmap.h"
#include <yalnix.h>
#include "../../Kernel.h"
#include "./Cvar.h"

/* ---------- only visible to Cvar.c ------------ */

#define CVAR_MAX 50

HashMap_t *cvarMap = HashMap_new(CVAR_MAX);
int cvarCount = 0;

q_t *getCvarQ(int cvar_id) {
    return (q_t *) HashMap_find(cvarMap, (char *) cvar_id);
}

/* ------------------------- global ---------------------------*/
int initCvar(int *cvar_idp) {
    // initialize lock
    q_t *cvarQ;
    if ((cvarQ = make_q()) == NULL ) return ERROR;
	
    // store lock in LockMap
    if (HashMap_insert(cvarMap, (char *) cvarCount, (void *) cvarQ) == 0) return ERROR;

    // save lock_id in lock_idp & increment lockCounnt
    cvar_idp = cvarCount++;
    return SUCCESS;
}

int isCvarEmpty(int cvar_id) {
    q_t *cvarQ = getCvarQ(cvar_id);
    if (peek_q(cvarQ) == NULL) return 1;
    return 0;
}

int pushCvarQ(int cvar_id, int lock_id, int pid) {
    q_t *cvarQ = getCvarQ(cvar_id);
    cvar_t *cvar = (cvar_t *) malloc(sizeof(cvar_t));
    cvar->pid = pid;
    cvar->lock_id = lock_id;

    if (enq_q(cvarQ, cvar) == ERROR) return ERROR;
    return SUCCESS;
}

int popCvarQ(int cvar_id, cvar_t *cvarP) {
    // needs to return NULL if nothing is in it
    q_t *cvarQ = getCvarQ(cvar_id);
    cvarP = (cvar_t *) deq_q(cvarQ);
    return SUCCESS;
}
