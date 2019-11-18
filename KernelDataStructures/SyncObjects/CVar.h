// Cvar.h
#ifndef _CVar_h
#define _CVar_h

#include "../../GeneralDataStructures/HashMap/HashMap.h" // HashMap_t
#include "../../GeneralDataStructures/Queue/Queue.h" // q_t

HashMap_t *cvarMap;

/* initialize CvarMap
*/
void initCvarMap(void);

/*  returns ERROR/SUCCESS; intializes cvar and saves
    cvar_id to cvar_idp
*/
int initCvar(int *cvar_idp);

// find and returns the cvarQ associated with the cvar_id (NULL if any error)
q_t *getCvarQ(int cvar_id);

#endif /* _CVar_h */
