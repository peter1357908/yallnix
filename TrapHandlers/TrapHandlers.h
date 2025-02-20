#ifndef _TrapHandlers_h
#define _TrapHandlers_h

#include <hardware.h>

void handleTrapKernel(UserContext *uctxt);

void handleTrapClock(UserContext *uctxt);

void handleTrapIllegal(UserContext *uctxt);

void handleTrapMemory(UserContext *uctxt);

void handleTrapMath(UserContext *uctxt);

void handleTtyReceive(UserContext *uctxt);

void handleTtyTransmit(UserContext *uctxt);

void handleTrapDisk(UserContext *uctxt);

void handleNothing(UserContext *uctxt);

#endif /* _TrapHandlers_h */
