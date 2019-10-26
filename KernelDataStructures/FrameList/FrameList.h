#ifndef _FrameList_h
#define _FrameList_h

typedef struct frame {
    void * addr;
    int isFree; // 0 == not free; 1 == is free
} frame_t;

int initFrameList(frame_t **FrameListp, int numFrames, void *currKernelBrk);

int getFrame(frame_t *FrameList, int numFrames, frame_t **frame);

void freeFrame(frame_t *FrameList, int numFrames, u_long pfn);

void freeFrameList(frame_t *FrameList);


#endif /* _FrameList_h */