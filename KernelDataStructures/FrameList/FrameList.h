
typedef struct frame {
    void * addr;
    int isFree : 1; // 0 == not free; 1 == is free
} frame_t;

int initFrameList(frame_t *FrameList, int pmem_size, void * currKernelBrk);

int getFrame(frame_t * FrameList, frame_t * frame);

void freeFrame(frame_t * frame);

void freeFrameList(frame_t *FrameList);