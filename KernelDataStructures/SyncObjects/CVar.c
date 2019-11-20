#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/HashMap/HashMap.h"
#include "../Scheduler/Scheduler.h"  // for nextSyncId
#include <yalnix.h> // for ERROR definition
#include "../../Kernel.h" // for SUCCESS definition
#include "Cvar.h"  // cvarMap

#define CVAR_MAP_HASH_BUCKETS 50

int initCvarMap() {
    cvarMap = HashMap_new(CVAR_MAP_HASH_BUCKETS);
	if (cvarMap == NULL) return ERROR;
	return SUCCESS;
}

int initCvar(int *cvar_idp) {
    // initialize cvar (which is actually just a queue of PCB pointers)
    q_t *cvarQ = make_q();
    if (cvarQ == NULL) {
		TracePrintf(1, "initCvar: make_q() failed & cvarQ is null");
		return ERROR;
	}
	
	*cvar_idp = nextSyncId++;

    // store cvar in cvarMap
    return HashMap_insert(cvarMap, *cvar_idp, cvarQ);
}

q_t *getCvar(int cvar_id) {
    return (q_t *) HashMap_find(cvarMap, cvar_id);
}

int deleteCvar(int cvar_id) {
	q_t *cvarQ = (q_t *) HashMap_remove(cvarMap, cvar_id);
	
	// if the cvarQ is NULL, return ERROR
	if (cvarQ == NULL) {
		TracePrintf(1, "deleteCvar: cvarQ is null");
		return ERROR;
	}
	
	// if it has some waiters, return ERROR;
	if (peek_q(cvarQ) != NULL) {
		TracePrintf(1, "deleteCvar: cvarQ has waiters");
		return ERROR;
	}
	
	// otherwise, the cvar is safe to be deleted
	delete_q(cvarQ, NULL);
	
	return SUCCESS;
}
