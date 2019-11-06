#include <yalnix.h>

void main() {

	TracePrintf(1, "init calling GetPid()...\n");
	int pid = GetPid();
	TracePrintf(1, "init's pid: %d\n", pid);

	TracePrintf(1, "init calling Delay(1)...\n");
	Delay(1);

	TracePrintf(1, "\nmalloc'ing a bunch to call Brk()...\n");
	malloc(100);

	TracePrintf(1, "\nforking process...\n");
	pid = Fork();

	if (0 == pid) {
		// NOTE: execTest doesn't use args, we'll want to test this later
		TracePrintf(1, "\nchild is calling Exec()...\n");
		// Exec("execTest", (void *) 0X0); 
		TracePrintf(1, "\nchild is exiting w/ status code == 1...\n");
		Exit(1);
	}
	// else {
	// 	int status;
	// 	// TracePrintf(1, "\nparent calling Wait()\n");
	// 	// Wait(&status);
	// 	// TracePrintf(1, "\nstatus = %d\n", status);
	// }	

	// TracePrintf(1, "\nparent exiting w/ status code == 0...\n");
	Exit(0);
}
