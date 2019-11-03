#ifndef _FrameList_h
#define _FrameList_h

// if we could assume PMEM_BASE = 0, then the FrameList can be just a bool array...
typedef struct frame {
    u_long        pfn     :24;
    unsigned int  isFree  :1; // 0 == not free; 1 == is free
} frame_t;


int initFrameList(frame_t **FrameListp, int numFrames, void *currKernelBrk);


// returns ERROR/0; writes the pfn to pfnp.
int getFrame(frame_t *FrameList, int numFrames, u_long *pfnp);

// assumes that all input parameters are meaningful.
void freeFrame(frame_t *FrameList, int numFrames, u_long pfn);


void freeFrameList(frame_t *FrameList);


#endif /* _FrameList_h */