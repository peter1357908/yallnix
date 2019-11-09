#include <yalnix.h>
#include <stdio.h>

#define DELAY_LENGTH 2
#define CHILD_EXIT_STATUS 20
#define PARENT_EXIT_STATUS 21
#define MALLOC_BYTES 100

void main() {

	TracePrintf(1, "init calling GetPid()...\n");
	int pid = GetPid();
	TracePrintf(1, "init's pid: %d\n", pid);

	TracePrintf(1, "init calling Delay(%d)...\n", DELAY_LENGTH);
	Delay(DELAY_LENGTH);

	TracePrintf(1, "malloc'ing a bunch to call Brk()...\n");
	malloc(100);

	TracePrintf(1, "forking process...\n");
	pid = Fork();

	if (0 == pid) {
		// TODO: pass in the child exit status to test argument passing
		TracePrintf(1, "child is calling Exec()...\n");
		char *argvec[] = {"execTest", NULL};
		Exec("execTest", argvec);
		
		/* ---- NOT REACHED ---- */
		TracePrintf(1, "In init; somehow the code past exec is getting executed; child exiting with status code = %d...\n", CHILD_EXIT_STATUS);
		Exit(CHILD_EXIT_STATUS);
	}
	else {
		int status;
		TracePrintf(1, "parent calling Wait()\n");
		Wait(&status);
		TracePrintf(1, "child status = %d\n", status);
	}	

	TracePrintf(1, "parent exiting with status code = %d...\n", PARENT_EXIT_STATUS);
	Exit(PARENT_EXIT_STATUS);
}
