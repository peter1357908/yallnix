

typedef struct frame {
    void * addr;
    int isFree : 1; // 0 == not free; 1 == is free
} frame_t;

int getFrame(frame_t * FrameList);

void jailFrame(frame_t * frame, void * addr) ;

void freeFrame(frame_t * frame);