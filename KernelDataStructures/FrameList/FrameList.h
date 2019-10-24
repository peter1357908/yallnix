
typedef struct frame {
    void * addr;
    int isFree; // 0 == not free; 1 == is free
} frame_t;

int initFrameList(frame_t **FrameListp, int numFrames, void * currKernelBrk);

int getFrame(frame_t * FrameList, int numFrames, frame_t ** frame);

void freeFrame(frame_t * frame);

void freeFrameList(frame_t *FrameList);