#include <hardware.h>

void handleTrapKernel(UserContext *uctxt);

void handleTrapClock(UserContext *uctxt);

void handleTrapIllegal(UserContext *uctxt);

void handleTrapMemory(UserContext *uctxt);

void handleTrapMath(UserContext *uctxt);

void handleTtyReceive(UserContext *uctxt);

void handleTtyTransmit(UserContext *uctxt);

void handleTrapDisk(UserContext *uctxt);