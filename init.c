#include <yalnix.h>
#include <hardware.h>
#include <stdio.h>

#define DELAY_LENGTH 2
#define PARENT_DELAY_LENGTH 6
#define PARENT_EXIT_STATUS 20
#define CHILD_EXIT_STATUS 30
#define GRANDCHILD_EXIT_STATUS 40
#define MALLOC_SIZE 1024
#define READ_SIZE 10
#define EXEC_FILENAME "execTest"

void main() {
	TracePrintf(1, "init calling GetPid()...\n");
	int pid = GetPid();
	TracePrintf(1, "init's pid: %d\n", pid);

	TracePrintf(1, "init calling Delay(%d)...\n", DELAY_LENGTH);
	Delay(DELAY_LENGTH);

	TracePrintf(1, "malloc'ing %d bytes to trigger Brk()...\n", MALLOC_SIZE);
	void *allocated_memory = (void *) malloc(MALLOC_SIZE);
	
	TracePrintf(1, "free'ing the %d bytes just allocated... \n", MALLOC_SIZE);
	free(allocated_memory);

	TracePrintf(1, "init calling Fork()...\n");
	pid = Fork();
	
	int status;
	if (0 == pid) {
		pid = GetPid();
		TracePrintf(1, "child (pid = %d) is calling Fork()...\n", pid);
		pid = Fork();
		
		if (0 == pid) {
			pid = GetPid();
			TracePrintf(1, "grandchild (pid = %d) is going to test tty's...\n", pid);
			
			int i;
			int num_read;
			char *buffer = (char *) malloc(TERMINAL_MAX_LINE);
			for (i = 0; i < NUM_TERMINALS; i++) {
				TtyPrintf(i, "Terminal %d, give me (pid = %d) something to read (%d characters):\n", i, pid, READ_SIZE);
				
				num_read = TtyRead(i, buffer, READ_SIZE);
				
				buffer[num_read] = '\n';
				
				TtyPrintf(i, "TtyRead said I read %d bytes, and I read the following:\n%s\n", num_read, buffer);
			}
			
			free(buffer);
			
			Exit(GRANDCHILD_EXIT_STATUS);
		}
		else {
			int grandchildPid = pid;
			pid = GetPid();
			TracePrintf(1, "child (pid = %d) is waiting on grandchild (pid = %d)...\n", pid, grandchildPid);
			Wait(&status);
			TracePrintf(1, "grandchild (pid = %d) status = %d\n", grandchildPid, status);
			
			TracePrintf(1, "child (pid = %d) is calling Exec()...\n", pid);
			char *argvec[] = {EXEC_FILENAME, NULL};
			Exec(EXEC_FILENAME, argvec);
			
			/* ---- NOT REACHED ---- */
			TracePrintf(1, "In init; somehow the code past exec is getting executed; child (pid = %d) exiting with status code = %d...\n", pid, CHILD_EXIT_STATUS);
			Exit(CHILD_EXIT_STATUS);
		}
	}
	else {
		TracePrintf(1, "parent calling Delay(%d) to jumble things up.\n", PARENT_DELAY_LENGTH);
		Delay(PARENT_DELAY_LENGTH);
		TracePrintf(1, "parent calling Wait()\n");
		Wait(&status);
		TracePrintf(1, "child (pid = %d) status = %d\n", pid, status);
	}

	TracePrintf(1, "parent exiting with status code = %d...\n", PARENT_EXIT_STATUS);
	Exit(PARENT_EXIT_STATUS);
}
