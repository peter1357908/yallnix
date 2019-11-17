#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/HashMap/HashMap.h"
#include <yalnix.h> // for ERROR definition
#include "../../Kernel.h" // for SUCCESS definition
#include "Cvar.h"
#include "Lock.h"

/* ---------- only visible to Cvar.c ------------ */

#define CVAR_MAX 50

HashMap_t *cvarMap;
int cvarCount = 0;

q_t *getCvarQ(int cvar_id) {
    char char_id_char = (char) cvar_id;
    q_t *cvarQ = (q_t *) HashMap_find(cvarMap, &char_id_char);
    return cvarQ;
}

/* ------------------------- global ---------------------------*/
void initCvarMap() {
    cvarMap = HashMap_new(CVAR_MAX);
}

int initCvar(int *cvar_idp) {
    // initialize lock
    q_t *cvarQ;
    if ((cvarQ = make_q()) == NULL) return ERROR;
	
    // get char from lockCount
    char cvarCountChar = (char) cvarCount;

    // store lock in LockMap
    if (HashMap_insert(cvarMap, &cvarCountChar, (void *) cvarQ) == ERROR) return ERROR;

    // save lock_id in lock_idp & increment lockCounnt
    *cvar_idp = cvarCount++;

    return SUCCESS;
}

int isCvarEmpty(int cvar_id) {
    q_t *cvarQ;
    if ((cvarQ = getCvarQ(cvar_id)) == NULL) return ERROR;
    if (peek_q(cvarQ) == NULL) return 1;
    return 0;
}

int pushCvarQ(int cvar_id, int lock_id, int pid) {
    q_t *cvarQ;
    if ((cvarQ = getCvarQ(cvar_id)) == NULL) return ERROR;
    cvar_t *cvar = (cvar_t *) malloc(sizeof(cvar_t));
    cvar->pid = pid;
    cvar->lock_id = lock_id;

    if (enq_q(cvarQ, (void *) cvar) == ERROR) return ERROR;
    return SUCCESS;
}

int popCvarQ(int cvar_id, cvar_t **cvarP) {
    q_t *cvarQ;
    if ((cvarQ = getCvarQ(cvar_id)) == NULL) return ERROR;
    *cvarP = (cvar_t *) deq_q(cvarQ);
    return SUCCESS;
}
