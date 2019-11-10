#include <yalnix.h>
#include <hardware.h>
#include <stdio.h>

#define DELAY_LENGTH 2
#define PARENT_DELAY_LENGTH 6
#define PARENT_EXIT_STATUS 20
#define CHILD_EXIT_STATUS 30
#define GRANDCHILD_EXIT_STATUS 40
#define MALLOC_BYTES 100
#define EXEC_FILENAME "execTest"

void main() {

	TracePrintf(1, "init calling GetPid()...\n");
	int pid = GetPid();
	TracePrintf(1, "init's pid: %d\n", pid);

	TracePrintf(1, "init calling Delay(%d)...\n", DELAY_LENGTH);
	Delay(DELAY_LENGTH);

	TracePrintf(1, "malloc'ing %d bytes to trigger Brk()...\n", MALLOC_BYTES);
	malloc(MALLOC_BYTES);

	TracePrintf(1, "init calling Fork()...\n");
	pid = Fork();

	if (0 == pid) {
		pid = GetPid();
		TracePrintf(1, "child (pid = %d) is calling Fork()...\n", pid);
		pid = Fork();
		
		if (0 == pid) {
			pid = GetPid();
			TracePrintf(1, "grandchild (pid = %d) is going to test tty's...\n", pid);
			
			int i;
			for (i = 0; i < NUM_TERMINALS; i++) {
				TtyPrintf(i, "Writing to terminal %d as the grandchild (pid = %d)\n", i, pid);
			}
			
			Exit(GRANDCHILD_EXIT_STATUS);
		}
		else {
			pid = GetPid();
			TracePrintf(1, "child (pid = %d) is calling Exec()...\n", pid);
			char *argvec[] = {EXEC_FILENAME, NULL};
			Exec(EXEC_FILENAME, argvec);
			
			/* ---- NOT REACHED ---- */
			TracePrintf(1, "In init; somehow the code past exec is getting executed; child (pid = %d) exiting with status code = %d...\n", pid, CHILD_EXIT_STATUS);
			Exit(CHILD_EXIT_STATUS);
		}
	}
	else {
		int status;
		TracePrintf(1, "parent calling Delay(%d) to jumble things up.\n", PARENT_DELAY_LENGTH);
		Delay(PARENT_DELAY_LENGTH);
		TracePrintf(1, "parent calling Wait()\n");
		Wait(&status);
		TracePrintf(1, "child status = %d\n", status);
	}	

	TracePrintf(1, "parent exiting with status code = %d...\n", PARENT_EXIT_STATUS);
	Exit(PARENT_EXIT_STATUS);
}
