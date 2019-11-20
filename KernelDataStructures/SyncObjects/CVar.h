/* CVar.h
 *
 * a CVar is just a queue of PCB pointers, so its type is q_t
 */
#ifndef _CVar_h
#define _CVar_h

#include "../../GeneralDataStructures/HashMap/HashMap.h" // HashMap_t
#include "../../GeneralDataStructures/Queue/Queue.h" // q_t

HashMap_t *cvarMap;

/* initialize CvarMap
*/
int initCvarMap(void);

/*  returns ERROR/SUCCESS; intializes cvar and saves
    cvar_id to cvar_idp
*/
int initCvar(int *cvar_idp);

// find and returns the cvarQ associated with the cvar_id (NULL if any error)
q_t *getCvar(int cvar_id);

/* delete an individual cvar from the cvarMap if the cvar has no 
 * waiters. (the cvar, which is a queue, will be freed).
 * returns SUCCESS if deletion is successful; otherwise ERROR
 */
int deleteCvar(int cvar_id);

#endif /* _CVar_h */
