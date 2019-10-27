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
	void* frameAddr = (void *) PMEM_BASE;
	int i;
	for (i = 0; i < numFrames; i++) {
		currFrame->addr = frameAddr;
		if (((int) frameAddr < (int) currKernelBrk) || (KERNEL_STACK_BASE <= (int) frameAddr && (int) frameAddr < KERNEL_STACK_LIMIT)) {
			currFrame->isFree = 0;
		} else {
			currFrame->isFree = 1;
		}
		currFrame++;
		frameAddr += PAGESIZE;
		
	}
    return 0;
}

int getFrame(frame_t *FrameList, int numFrames, frame_t **frame) {
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
    *frame = currFrame;
    return 0;
}

void freeFrame(frame_t *FrameList, int numFrames, u_long pfn) {
    int i;
    frame_t *currFrame = FrameList;
    void *targetFrameAddr = (void *) (pfn<<PAGESHIFT);
    for (i = 0; i < numFrames; i++) {
        if (targetFrameAddr == currFrame->addr) {
            currFrame->isFree = 1;
            break;
        }
        currFrame++;
    }
}

void freeFrameList(frame_t *FrameList) {
    free(FrameList);
}