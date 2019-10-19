#include <stdlib.h>
#include <FrameList.h>
#include <yalnix.h>
#include <hardware.h>

int initFrame(frame_t *FrameList, int pmem_size, void * currKernelBrk) {
   	int numFrames = pmem_size / PAGESIZE;
	FrameList = malloc(numFrames * sizeof(frame_t));
    if (FrameList == NULL) {
        return ERROR;
    }
	frame_t *currFrame = FrameList;
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
		currFrame += sizeof(frame_t);
	}
    return 0;
}

int getFrame(frame_t * FrameList, frame_t * frame) {
    frame_t *currFrame = FrameList;
    int i = 0;
    int totalFrames = sizeof(FrameList) / sizeof(frame_t);
    while (!currFrame->isFree) {
        if (i >= totalFrames) {
            return ERROR;
        }
        currFrame += sizeof(frame_t);
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