#
#	Sample Makefile for Yalnix kernel and user programs.
#	
#	Prepared by Sean Smith and Adam Salem and various Yalnix developers
#	of years past...
#
#	You must modify the KERNEL_SRCS and KERNEL_OBJS definition below to be your own
#	list of .c and .o files that should be linked to build your Yalnix kernel.
#
#	You must modify the USER_SRCS and USER_OBJS definition below to be your own
#	list of .c and .o files that should be linked to build your Yalnix user programs
#
#	The Yalnix kernel built will be named "yalnix".  *ALL* kernel
#	Makefiles for this lab must have a "yalnix" rule in them, and
#	must produce a kernel executable named "yalnix" -- we will run
#	your Makefile and will grade the resulting executable
#	named "yalnix".
#

#make all will make all the kernel objects and user objects
ALL = $(KERNEL_ALL) $(USER_APPS)
KERNEL_ALL = yalnix

KERNEL_DATA_STRUCTURES_DIR = KernelDataStructures
PAGE_TABLE = $(KERNEL_DATA_STRUCTURES_DIR)/PageTable/PageTable
FRAME_LIST = $(KERNEL_DATA_STRUCTURES_DIR)/FrameList/FrameList
SCHEDULER = $(KERNEL_DATA_STRUCTURES_DIR)/Scheduler/Scheduler
TTY_BUFFER = $(KERNEL_DATA_STRUCTURES_DIR)/TtyBuffer/TtyBuffer

SYNC_OBJECTS_DIR = $(KERNEL_DATA_STRUCTURES_DIR)/SyncObjects
LOCK = $(SYNC_OBJECTS_DIR)/Lock
CVAR = $(SYNC_OBJECTS_DIR)/CVar
PIPE = $(SYNC_OBJECTS_DIR)/Pipe

GENERAL_DATA_STRUCTURES_DIR = GeneralDataStructures
QUEUE = $(GENERAL_DATA_STRUCTURES_DIR)/Queue/Queue
HASH_MAP = $(GENERAL_DATA_STRUCTURES_DIR)/HashMap/HashMap
SET_DS = $(GENERAL_DATA_STRUCTURES_DIR)/HashMap/set

KERNEL_CALLS_DIR = KernelCalls
# remeber there is only one header file, $(KERNEL_CALLS_HEADER) for this module
KERNEL_CALLS_HEADER = $(KERNEL_CALLS_DIR)/KernelCalls.h
GENERAL_KERNEL_CALLS = $(KERNEL_CALLS_DIR)/GeneralKernelCalls
TTY_KERNEL_CALLS = $(KERNEL_CALLS_DIR)/TtyKernelCalls
LOCK_KERNEL_CALLS = $(KERNEL_CALLS_DIR)/LockKernelCalls
CVAR_KERNEL_CALLS = $(KERNEL_CALLS_DIR)/CVarKernelCalls
PIPE_KERNEL_CALLS = $(KERNEL_CALLS_DIR)/PipeKernelCalls

TRAP_HANDLERS = TrapHandlers/TrapHandlers
LOAD_PROGRAM = LoadProgram


#List all kernel source files here.  
KERNEL_SRCS = Kernel.c $(PAGE_TABLE).c $(FRAME_LIST).c $(SCHEDULER).c $(TTY_BUFFER).c $(LOCK).c $(CVAR).c $(PIPE).c $(QUEUE).c $(HASH_MAP).c $(SET_DS).c $(TRAP_HANDLERS).c $(LOAD_PROGRAM).c $(GENERAL_KERNEL_CALLS).c $(TTY_KERNEL_CALLS).c $(LOCK_KERNEL_CALLS).c $(CVAR_KERNEL_CALLS).c  $(PIPE_KERNEL_CALLS).c
#List the objects to be formed form the kernel source files here.  Should be the same as the prvious list, replacing ".c" with ".o"
KERNEL_OBJS = Kernel.o $(PAGE_TABLE).o $(FRAME_LIST).o $(SCHEDULER).o $(TTY_BUFFER).o $(LOCK).o $(CVAR).o $(PIPE).o $(QUEUE).o $(HASH_MAP).o $(SET_DS).o $(TRAP_HANDLERS).o $(LOAD_PROGRAM).o $(GENERAL_KERNEL_CALLS).o $(TTY_KERNEL_CALLS).o $(LOCK_KERNEL_CALLS).o $(CVAR_KERNEL_CALLS).o $(PIPE_KERNEL_CALLS).o
#List all of the header files necessary for your kernel
KERNEL_INCS = Kernel.h $(PAGE_TABLE).h $(FRAME_LIST).h $(SCHEDULER).h $(TTY_BUFFER).h $(LOCK).h $(CVAR).h $(PIPE).h $(QUEUE).h $(HASH_MAP).h $(SET_DS).h $(TRAP_HANDLERS).h $(LOAD_PROGRAM).h $(KERNEL_CALLS_HEADER)

TEST_DIR = test

#List all user programs here.
USER_APPS = init $(TEST_DIR)/exectest $(TEST_DIR)/forktest $(TEST_DIR)/bigstack $(TEST_DIR)/torture $(TEST_DIR)/zero
#List all user program source files here.  SHould be the same as the previous list, with ".c" added to each file
USER_SRCS = init.c $(TEST_DIR)/exectest.c $(TEST_DIR)/forktest.c $(TEST_DIR)/bigstack.c $(TEST_DIR)/torture.c $(TEST_DIR)/zero.c
#List the objects to be formed form the user  source files here.  Should be the same as the prvious list, replacing ".c" with ".o"
USER_OBJS = init.o $(TEST_DIR)/exectest.o $(TEST_DIR)/forktest.o $(TEST_DIR)/bigstack.o $(TEST_DIR)/torture.o $(TEST_DIR)/zero.o
#List all of the header files necessary for your user programs
USER_INCS = 

#write to output program yalnix
YALNIX_OUTPUT = yalnix

#
#	These definitions affect how your kernel is compiled and linked.
#       The kernel requires -DLINUX, to 
#	to add something like -g here, that's OK.
#

#Set additional parameters.  Students generally should not have to change this next section

#Use the gcc compiler for compiling and linking
CC = gcc

DDIR58 = /yalnix
LIBDIR = $(DDIR58)/lib
INCDIR = $(DDIR58)/include
ETCDIR = $(DDIR58)/etc

# any extra loading flags...
LD_EXTRA = 

KERNEL_LIBS = $(LIBDIR)/libkernel.a $(LIBDIR)/libhardware.so

# the "kernel.x" argument tells the loader to use the memory layout
# in the kernel.x file..
KERNEL_LDFLAGS = $(LD_EXTRA) -L$(LIBDIR) -lkernel -lelf -Wl,-T,$(ETCDIR)/kernel.x -Wl,-R$(LIBDIR) -lhardware
LINK_KERNEL = $(LINK.c)

#
#	These definitions affect how your Yalnix user programs are
#	compiled and linked.  Use these flags *only* when linking a
#	Yalnix user program.
#

USER_LIBS = $(LIBDIR)/libuser.a
ASFLAGS = -D__ASM__
CPPFLAGS= -m32 -fno-builtin -I. -I$(INCDIR) -g -DLINUX 


##########################
#Targets for different makes
# all: make all changed components (default)
# clean: remove all output (.o files, temp files, LOG files, TRACE, and yalnix)
# count: count and give info on source files
# list: list all c files and header files in current directory
# kill: close tty windows.  Useful if program crashes without closing tty windows.
# test: start yalnix with init and trace level 1.
# $(KERNEL_ALL): compile and link kernel files
# $(USER_ALL): compile and link user files
# %.o: %.c: rules for setting up dependencies.  Don't use this directly
# %: %.o: rules for setting up dependencies.  Don't use this directly

all: $(ALL)

clean:
	rm -f *~ TTYLOG* TRACE.txt DISK $(YALNIX_OUTPUT) $(USER_APPS) core.*
	find . -name '*.o' -delete

count:
	wc $(KERNEL_SRCS) $(USER_SRCS)

list:
	ls -l *.c *.h

kill:
	killall yalnixtty yalnixnet yalnix
	
TEST_FLAGS = -t TRACE.txt -lk 1 -lh 2 -lu 1
	
test: all
	$(YALNIX_OUTPUT) $(TEST_FLAGS)
	
gdb: all
	gdb --args $(YALNIX_OUTPUT) $(TEST_FLAGS)

no-core:
	rm -f core.*

$(KERNEL_ALL): $(KERNEL_OBJS) $(KERNEL_LIBS) $(KERNEL_INCS)
	$(LINK_KERNEL) -o $@ $(KERNEL_OBJS) $(KERNEL_LDFLAGS)

$(USER_APPS): $(USER_OBJS) $(USER_INCS)
	$(ETCDIR)/yuserbuild.sh $@ $(DDIR58) $@.o
	
