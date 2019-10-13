#ifndef _pcb_h
#define _pcb_h

#include <hardware.h>

typedef struct PCB_t {
    int pid;
    int uspace;
    UserContext uctxt;
    KernelContext kctxt;
    int kstack;
};

#endif /*!_pcb_h*/

