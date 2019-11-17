#include <yalnix.h>
#include <hardware.h>
#include <stdio.h>

#define DELAY_LENGTH 2
#define PARENT_DELAY_LENGTH 1
#define PARENT_EXIT_STATUS 20
#define CHILD_DELAY_LENGTH 1
#define CHILD_EXIT_STATUS 30
#define GRANDCHILD_EXIT_STATUS 40
#define MALLOC_SIZE 1024
#define READ_SIZE 10
#define EXEC_FILENAME "execTest"

void main() {
	TtyPrintf(0, "init calling GetPid()...\n");
	int pid = GetPid();
	TtyPrintf(0, "init's pid: %d\n", pid);

	TtyPrintf(0, "init calling Delay(%d)...\n", DELAY_LENGTH);
	Delay(DELAY_LENGTH);

	TtyPrintf(0, "malloc'ing %d bytes to trigger Brk()...\n", MALLOC_SIZE);
	void *allocated_memory = (void *) malloc(MALLOC_SIZE);
	
	TtyPrintf(0, "free'ing the %d bytes just allocated... \n", MALLOC_SIZE);
	free(allocated_memory);

	TtyPrintf(0, "init initializing lock...\n");
	int lock_id;
	LockInit(&lock_id);
	TtyPrintf(0, "lock id = %d...\n", lock_id);

	TtyPrintf(0, "init initializing cvar...\n");
	int cvar_id;
	CvarInit(&cvar_id);
	TtyPrintf(0, "cvar id = %d...\n", cvar_id);

	TtyPrintf(0, "init calling Fork()...\n");
	pid = Fork();
	
	int status;
	if (0 == pid) {
		pid = GetPid();

		TtyPrintf(0, "child (pid = %d) is trying to acquire lock %d...\n", pid, lock_id);
		Acquire(lock_id);
		TtyPrintf(0, "child (pid = %d) has acquired lock %d...\n", pid, lock_id);

		TtyPrintf(0, "child (pid = %d) calling Delay(%d) after acquiring lock.\n", pid, CHILD_DELAY_LENGTH);
		Delay(CHILD_DELAY_LENGTH);

		// NOTE: because these are processes and not threads, we cannot share data between them
		// The condition will have to involve pipes once those are finished
		int condition = 0;
		while (condition == 0) {
			TtyPrintf(0, "condition == %d, so child (pid = %d) calling CvarWait(%d, %d)...\n", condition, pid, cvar_id, lock_id);
			CvarWait(cvar_id, lock_id);

			// imagine the following is being done by another process
			// but we can't do this yet, because we don't have pipes
			condition++;
		}

		TtyPrintf(0, "condition != 0, so child (pid = %d) is proceeding...\n", pid);

		TtyPrintf(0, "child (pid = %d) is releasing lock %d...\n", pid, lock_id);
		Release(lock_id);

		TtyPrintf(0, "child (pid = %d) is calling Fork()...\n", pid);
		pid = Fork();
		
		if (0 == pid) {
			pid = GetPid();
			TtyPrintf(0, "grandchild (pid = %d) is going to test other tty's...\n", pid);
			
			int i;
			int num_read;
			char *buffer = (char *) malloc(TERMINAL_MAX_LINE);
			for (i = 1; i < NUM_TERMINALS; i++) {
				TtyPrintf(i, "Terminal %d, give me (pid = %d) something to read (%d characters):\n", i, pid, READ_SIZE);
				
				num_read = TtyRead(i, buffer, READ_SIZE);
				
				buffer[num_read] = '\0';
				
				TtyPrintf(i, "TtyRead said I read %d bytes, and I read the following:\n%s\n", num_read, buffer);
			}
			
			free(buffer);
			
			Exit(GRANDCHILD_EXIT_STATUS);
		}
		else {
			int grandchildPid = pid;
			pid = GetPid();
			TtyPrintf(0, "child (pid = %d) is waiting on grandchild (pid = %d)...\n", pid, grandchildPid);
			Wait(&status);
			TtyPrintf(0, "grandchild (pid = %d) status = %d\n", grandchildPid, status);
			
			TtyPrintf(0, "child (pid = %d) is calling Exec()...\n", pid);
			char *argvec[] = {EXEC_FILENAME, NULL};
			Exec(EXEC_FILENAME, argvec);
			
			/* ---- NOT REACHED ---- */
			TtyPrintf(0, "In init; somehow the code past exec is getting executed; child (pid = %d) exiting with status code = %d...\n", pid, CHILD_EXIT_STATUS);
			Exit(CHILD_EXIT_STATUS);
		}
	}
	else {
		TtyPrintf(0, "parent is trying to acquire lock %d...\n", lock_id);
		Acquire(lock_id);
		TtyPrintf(0, "parent has acquired lock %d...\n", lock_id);

		TtyPrintf(0, "parent calling Delay(%d) after forking & acquiring lock.\n", PARENT_DELAY_LENGTH);
		Delay(PARENT_DELAY_LENGTH);

		TtyPrintf(0, "parent is releasing lock %d...\n", lock_id);
		Release(lock_id);

		TtyPrintf(0, "parent signaling cvar %d...\n", cvar_id);
		CvarSignal(cvar_id);

		TtyPrintf(0, "parent calling Wait()\n");
		Wait(&status);
		TtyPrintf(0, "child (pid = %d) status = %d\n", pid, status);
	}

	TtyPrintf(0, "parent exiting with status code = %d...\n", PARENT_EXIT_STATUS);
	Exit(PARENT_EXIT_STATUS);
}
