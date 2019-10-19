#include <stdlib.h>
#include <FrameList.h>
#include <yalnix.h>

int getFrame(frame_t *FrameList) {
    frame_t *currFrame = FrameList;
    int i;
    int totalFrames = sizeof(FrameList) / sizeof(frame_t);
    while (!currFrame->isFree) {
        if (i == totalFrames) {
            return ERROR;
        }
        currFrame += sizeof(frame_t);
        i++;
    }
    jailFrame(currFrame);
    return currFrame;
}

void jailFrame(frame_t * frame, void * addr) {
    frame->isFree = 0;
};

void freeFrame(frame_t * frame) {
    frame->isFree = 1;
}
