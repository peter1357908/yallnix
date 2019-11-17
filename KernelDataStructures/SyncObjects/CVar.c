#include "../../GeneralDataStructures/Queue/Queue.h"
#include "../../GeneralDataStructures/HashMap/HashMap.h"
#include "../Scheduler/Scheduler.h"  // for nextSyncId
#include <yalnix.h> // for ERROR definition
#include "../../Kernel.h" // for SUCCESS definition
#include "Cvar.h"  // cvarMap

#define CVAR_MAP_HASH_BUCKETS 50

void initCvarMap() {
    cvarMap = HashMap_new(CVAR_MAP_HASH_BUCKETS);
}

int initCvar(int *cvar_idp) {
    // initialize cvar (which is actually just a queue of PCB pointers)
    q_t *cvarQ = make_q();
    if (cvarQ == NULL) return ERROR;
	
	*cvar_idp = nextSyncId++;

    // store cvar in cvarMap
    return HashMap_insert(cvarMap, *cvar_idp, cvarQ);
}

q_t *getCvarQ(int cvar_id) {
    return (q_t *) HashMap_find(cvarMap, cvar_id);
}
