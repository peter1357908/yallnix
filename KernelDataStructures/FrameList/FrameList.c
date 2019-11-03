#include <stdlib.h>
#include "FrameList.h"
#include <yalnix.h>
#include <hardware.h>

int initFrameList(frame_t **FrameListp, int numFrames, void *currKernelBrk) {	
	*FrameListp = (frame_t *) malloc(sizeof(frame_t) * numFrames);
    if (*FrameListp == NULL) {
        return ERROR;
    }
	
	frame_t *currFrame = *FrameListp;
	int frameAddr = PMEM_BASE;
	int i;
	for (i = 0; i < numFrames; i++) {
		currFrame->pfn = frameAddr>>PAGESHIFT;
		if (frameAddr < (int) currKernelBrk || (KERNEL_STACK_BASE <= frameAddr && frameAddr < KERNEL_STACK_LIMIT)) {
			currFrame->isFree = 0;
		} else {
			currFrame->isFree = 1;
		}
		currFrame++;
		frameAddr += PAGESIZE;
	}
    return 0;
}

int getFrame(frame_t *FrameList, int numFrames, u_long *pfnp) {
    if (FrameList == NULL) {
        return ERROR;
    }
	
    frame_t *currFrame = FrameList;
	int i = 0;
    while (currFrame->isFree == 0) {
        if (i >= numFrames) {
            return ERROR;
        }
        currFrame++;
        i++;
    }
    currFrame->isFree = 0;
    *pfnp = currFrame->pfn;
    return 0;
}

void freeFrame(frame_t *FrameList, int numFrames, u_long pfn) {
    int i;
    frame_t *currFrame = FrameList;
    for (i = 0; i < numFrames; i++) {
        if (currFrame->pfn == pfn) {
            currFrame->isFree = 1;
            break;
        }
        currFrame++;
    }
}

void freeFrameList(frame_t *FrameList) {
    free(FrameList);
}