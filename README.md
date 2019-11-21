# Zac and Shengsong's Yalnix Operating System

Our project lives in master

## Usage
To make all changed components: `make all`
To remove all output (.o files, temp files, LOG files, TRACE, and yalnix): `make clean`
To count: count and give info on source files: `make count`
To list: list all c files and header files in current directory: `make list`
To kill: close tty windows.  Useful if program crashes without closing tty windows: `make kill`
To test: start yalnix with init and trace level 1: `make test`

To run other tests (e.g. bigstack, forktest, torture, zero), go into `init.c` and uncomment the corresponding test file.

## General Design

### Kernel.h
This is where `KernelStart` and `SetKernelData` are located. We initialize the interrupt vector array, page tables, ttyBuffers, LockMap, CvarMap, and PipeMap. We also load the 'init' and 'idle' processes. Useful global constants like KERNEL_STACK_BASE_VPN, KERNEL_BASE_VPN, and USER_BASE_VPN are declared here.

### KernelDataStructures/

#### FrameList.h

The FrameList keeps track of all the physical frames on the machine and whether they are free or not. We implemented this with an array, but in the future, this could be optimized with a queue.

#### PageTable.h
 
The PageTable module handles all things involved with the PageTable: initializing a page table, setting PTEs, invalidating PTEs, getting an address's region, and getting an address's protection bits.

#### Scheduler.h

The Scheduler module contains a lot of the core functionality of this operating system. It initializes processes and handles all context switching. The scheduler keeps track of processes using several queues and hashmaps. `blockingMap` is a map containing all processes that are currently blocked for general reasons. `sleepingMap` is similar; it contains all processes that are currently sleeping (they called Delay). `readyQ` contains all proceseses that are ready to run. Whichever process is currently running lives at `currPCB`. 

Our scheduler also has special arrays and queues for processes attempting to transmit/read to/from the terminals. `currTransmitters` is an array containing pointers to the processes that are currently transmitting. Each slot 'i' in the array corresponds to a different terminal 'i'. `transmittingQs` is an array of queues where each slot 'i' in the array corresponds to a queue of processes waiting to transmit to terminal 'i'. `readingQs` is an array of queues where each slot 'i' in the array corresponds to a queue of processes waiting to read from terminal 'i'.

##### PCB_t

The PCB_t data structure lives inside Scheduler. The struct has a few notable members. `parent` is  PCB_T pointer to a process's parent (if it was created during fork());`childrenMap` is a map containing the process's children (for when it calls fork). This is useful if the parent exits before the child. The parent can "orphan" its child by setting its childs' `parent` to NULL. `numChildren` is included for similar bookkeeping reasons. `zombieQ` is used to keep track of child processes after they've exited. After a child exits, it adds itself to its parent's `zombieQ`, so its parent can access its status.  `numRemainingDelayTicks` pertains to the Delay() function. It tells the OS how many more clock ticks the process must live inside the `sleepingMap`. Finally, `stackPfns` is an array containing the process's physical frames for its KernelStack. Because the KernelStack's size and address is static, when context switching, we can simply point the KernelStack's PTEs at these frames.

#### SyncObjects/

##### Cvar.h
Each Cvar is a queue of PCBs. When a process waits for a Cvar, it is pushed onto its queue. When the cvar is signaled, the Cvar (queue) is popped and the popped process is unblocked. When the cvar is broadcast, every process is removed from the process and unblocked.

#### Lock.h
Each lock keeps track of (1) which process currently owns it (if no one owns it, then it is free); (2) a queue of processes waiting to acquire the lock. When a process tries to acquire a lock, it first checks if it is free. If the lock is free, the process is set to the lock's owner and proceeds unblocked. If the lock is not free, the process is pushed onto the lock's queue of waiting processes and blocked. When a process releases a lock, the lock pops a process off its waiting queue and unblocks it.

#### Pipe.h

#### TtyBuffer.h
The TtyBuffer is used for `TtyRead()`. The buffer lives inside Kernelland (or region 0). When the user gives input through the terminal, the data is copied into this buffer. As mentioned above, `Scheduler` keeps track of processes waiting to read. After writing to the buffer, if there are X bytes available and there is a process waiting for X+ bytes, that process is unblocked. (It will only wake up if the line read is not just EOF) If a process attempts to read X bytes from the buffer and there are fewer than X bytes, this process is blocked. When there are X+ bytes available, the process will read as much as possible (either the length wanted or the available length). It is also important to note that `writeBuffer()` will overwrite any remaining un-read bytes.

### KernelCalls/
This is the directory where all KernelCalls live. `KernelCalls.h` contains the signatures for all the functions. They are implemented in separate files: `CvarKernelCalls.c`, `GeneralKernelCalls.c`, `LockKernelCalls.c`, `PipeKernelCalls.c`, and `TtyKernelCalls.c`.

### GeneralDataStructures/

We utilized the following general data structures in our Yalnix implementation: Hashmap and Queue.

## Potential Improvements

Currently, our TtyBuffer overwrites unread bytes. Our Pipe, on the other hand, appends to unread bytes if possible. In the future, our TtyBuffer's behavior should be consistent with Pipe.

We could potentially represent processes with a PCB_t pointer instead of a pid.

We could be more consistent in abstraction level (the sync objects are less abstracted than TtyBuffer)

We could create a Halt() wrapper, so that when kernel Halts, all processes are deleted and resources are freed. 
