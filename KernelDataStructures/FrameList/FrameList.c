#include <stdlib.h>
#include "FrameList.h"
#include <yalnix.h>
#include <hardware.h>

int initFrameList(frame_t **FrameListp, int numFrames, void *currKernelBrk) {	
	*FrameListp = malloc(numFrames * sizeof(frame_t));
    if (*FrameListp == NULL) {
        return ERROR;
    }
	
	frame_t *currFrame = *FrameListp;
	void* frameAddr = PMEM_BASE;
	int i;
	for (i = 0; i < numFrames; i++) {
		currFrame->addr = frameAddr;
		if (((int) frameAddr < (int) currKernelBrk) || (KERNEL_STACK_BASE <= (int) frameAddr && KERNEL_STACK_LIMIT > (int) frameAddr)) {
			currFrame->isFree = 0;
		} else {
			currFrame->isFree = 1;
		}
		frameAddr += PAGESIZE;
		currFrame++;
	}
    return 0;
}

int getFrame(frame_t *FrameList, int numFrames, frame_t *frame) {
    if (FrameList == NULL) {
        return ERROR;
    }
	
    frame_t *currFrame = FrameList;
	int i = 0;
    while (!currFrame->isFree) {
        if (i >= numFrames) {
            return ERROR;
        }
        currFrame++;
        i++;
    }
    currFrame->isFree = 0;
    frame = currFrame;
    return 0;
}

void freeFrame(frame_t * frame) {
    frame->isFree = 1;
}

void freeFrameList(frame_t *FrameList) {
    free(FrameList);
}