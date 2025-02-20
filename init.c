#include <yalnix.h>
#include <hardware.h>
#include <stdio.h>

#define DELAY_LENGTH 1
#define MALLOC_SIZE 1024
#define PIPE_READ_SIZE 20
#define TTY_READ_SIZE 10

#define PARENT_DELAY_LENGTH 2
#define CHILD_DELAY_LENGTH 1
#define GRANDCHILD_DELAY_LENGTH 2

#define PARENT_EXIT_STATUS 20
#define CHILD_EXIT_STATUS (PARENT_EXIT_STATUS + 10)
#define GRANDCHILD_EXIT_STATUS (CHILD_EXIT_STATUS + 10)

#define ORPHAN_TEST_GRANDCHILD_DELAY_LENGTH 3
#define ORPHAN_TEST_PARENT_DELAY_LENGTH (ORPHAN_TEST_GRANDCHILD_DELAY_LENGTH + 2)
#define ORPHAN_TEST_CHILD_EXIT_STATUS (CHILD_EXIT_STATUS + 5)
#define ORPHAN_TEST_GRANDCHILD_EXIT_STATUS (GRANDCHILD_EXIT_STATUS + 5)

#define EXEC_TEST "./test/exectest"
#define FORK_TEST "./test/forktest"
#define TORTURE_TEST "./test/torture"
#define ZERO_TEST "./test/zero"
#define BIGSTACK_TEST "./test/bigstack"

void main() {

	/* --- UNCOMMENT TO RUN CORRESPONDING TEST FILE --- */
	// char *fileName = FORK_TEST;
	// char *fileName = TORTURE_TEST;
	// char *fileName = ZERO_TEST;
	// char *fileName = BIGSTACK_TEST;
	
	// char *args[] = {fileName, NULL};
	// TtyPrintf(0, "init Exec()'ing into %s", fileName);
	// Exec(fileName, args);
	
	
	/* Run the following with EXEC_TEST if running none of the tests above */

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
	
	TtyPrintf(0, "init initializing pipe...\n");
	int pipe_id;
	PipeInit(&pipe_id);
	TtyPrintf(0, "pipe id = %d...\n", pipe_id);

	TtyPrintf(0, "init calling Fork()...\n");
	pid = Fork();
	
	int status;
	if (0 == pid) {
		pid = GetPid();

		TtyPrintf(0, "child (pid = %d) is trying to acquire lock %d...\n", pid, lock_id);
		Acquire(lock_id);
		TtyPrintf(0, "child (pid = %d) has acquired lock %d...\n", pid, lock_id);

		TtyPrintf(0, "child (pid = %d) calling Delay(%d) after acquiring lock.\n", pid,  CHILD_DELAY_LENGTH);
		Delay(CHILD_DELAY_LENGTH);

		// NOTE: because these are processes and not threads, we cannot share data between them
		// The condition will have to involve pipes once those are finished
		int condition = 0;
		while (condition == 0) {
			TtyPrintf(0, "condition == %d, so child (pid = %d) calling CvarWait(%d, %d)...\n", condition, pid, cvar_id, lock_id);
			CvarWait(cvar_id, lock_id);

			// imagine the following is being done by another process
			// how do we test a shared condition if processes share nothing in userland...?
			condition++;
		}

		TtyPrintf(0, "condition != 0, so child (pid = %d) is proceeding...\n", pid);
		
		TtyPrintf(0, "child (pid = %d) is releasing lock %d...\n", pid, lock_id);
		Release(lock_id);
		
		char *first_string = "1234567890";  // size without NULL: 10
		char *second_string = "abcdefghijklmnopqrstuvwxyz";  // size without NULL: 26
		TtyPrintf(0, "child writing 10 bytes, \"%s\", fewer than expected bytes (%d) to pipe %d\n", first_string, PIPE_READ_SIZE);
		PipeWrite(pipe_id, (void *) first_string, 10);
		TtyPrintf(0, "child writing 26 more bytes, \"%s\"; now pipe %d has 36 bytes, more than the expected length (%d bytes)\n", second_string, pipe_id, PIPE_READ_SIZE);
		PipeWrite(pipe_id, (void *) second_string, 26);
		TtyPrintf(0, "child is delaying for another %d ticks so the parent can print the read bytes\n", CHILD_DELAY_LENGTH);
		Delay(CHILD_DELAY_LENGTH);

		TtyPrintf(0, "child (pid = %d) is calling Fork()...\n", pid);
		pid = Fork();
		
		if (0 == pid) {
			pid = GetPid();
			TtyPrintf(0, "grandchild (pid = %d) is going to test other tty's...\n", pid);
			
			int i;
			int num_read;
			char *buffer = (char *) malloc(TERMINAL_MAX_LINE);
			for (i = 1; i < NUM_TERMINALS; i++) {
				TtyPrintf(i, "Terminal %d, give me (pid = %d) something to read (%d characters):\n", i, pid, TTY_READ_SIZE);
				
				num_read = TtyRead(i, buffer, TTY_READ_SIZE);
				
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
			TtyPrintf(0, "child (pid = %d) resumed from waiting, grandchild (pid = %d) status = %d\n", pid, grandchildPid, status);
			
			TtyPrintf(0, "child (pid = %d) is calling Exec()...\n", pid);
			char *argvec[] = {EXEC_TEST, NULL};
			Exec(EXEC_TEST, argvec);
			
			/* ---- NOT REACHED ---- */
			TtyPrintf(0, "In init; somehow the code past exec is getting executed; child (pid = %d) exiting with status code = %d...\n", pid, CHILD_EXIT_STATUS);
			Exit(CHILD_EXIT_STATUS);
		}
	}
	
	TtyPrintf(0, "parent is trying to acquire lock %d...\n", lock_id);
	Acquire(lock_id);
	TtyPrintf(0, "parent has acquired lock %d...\n", lock_id);

	TtyPrintf(0, "parent calling Delay(%d) after forking & acquiring lock.\n", PARENT_DELAY_LENGTH);
	Delay(PARENT_DELAY_LENGTH);

	TtyPrintf(0, "parent is releasing lock %d...\n", lock_id);
	Release(lock_id);

	TtyPrintf(0, "parent signaling cvar %d...\n", cvar_id);
	CvarSignal(cvar_id);
	
	TtyPrintf(0, "parent trying to read %d bytes from pipe %d... (currently there should be nothing in the pipe, and the next time the child wakes up, it will write something into it)\n", PIPE_READ_SIZE, pipe_id);
	char *buf = (char *) malloc(PIPE_READ_SIZE + 1);
	int lenRead = PipeRead(pipe_id, buf, PIPE_READ_SIZE);
	buf[lenRead] = '\0';
	TtyPrintf(0, "parent just read %d characters:\n%s\n", lenRead, buf);
	free(buf);
	
	TtyPrintf(0, "parent calling Wait(), rc = %d\n", Wait(&status));
	TtyPrintf(0, "parent resumed from Wait(), child (pid = %d) status = %d\n", pid, status);
	
	TtyPrintf(0, "parent calling Reclaim on the lock (%d, rc = %d), cvar (%d, rc = %d), and pipe (%d, rc = %d)\n", lock_id, Reclaim(lock_id), cvar_id, Reclaim(cvar_id), pipe_id, Reclaim(pipe_id));
		
	TtyPrintf(0, "now parent tries to acquire the lock %d again, and the return code should be %d\n", lock_id, ERROR);
	TtyPrintf(0, "the return code is %d\n", Acquire(lock_id));

	/* -------- ORPHAN TEST -------- */
	
	TtyPrintf(0, "init now testing what happens when a parent exits before child does (orphans should keep running). Since init exiting will halt Yalnix, we test this on its child and grandchild.\n");
	TtyPrintf(0, "parent calling Fork()...\n", pid);
	pid = Fork();
	if (0 == pid) {
		pid = GetPid();
		TtyPrintf(0, "Orphan-test child (pid = %d) calling Fork()...\n", pid);
		pid = Fork();
		if (0 == pid) {
			pid = GetPid();
			TtyPrintf(0, "Orphan-test grandchild (pid = %d) calling Delay(%d) so orphan-test child can exit first\n", pid, ORPHAN_TEST_GRANDCHILD_DELAY_LENGTH);
			Delay(ORPHAN_TEST_GRANDCHILD_DELAY_LENGTH);
			TtyPrintf(0, "Orphan-test grandchild (pid = %d) exiting (if this line is printed, grandchild was not orphan-collected)\n", pid);
			Exit(ORPHAN_TEST_GRANDCHILD_EXIT_STATUS);
		}
		pid = GetPid();
		TtyPrintf(0, "Orphan-test child (pid = %d) exiting immediately after Fork()\n", pid);
		Exit(ORPHAN_TEST_CHILD_EXIT_STATUS);
	}
	pid = GetPid();
	TtyPrintf(0, "parent (pid = %d) calling Wait()\n", pid);
	Wait(&status);
	TtyPrintf(0, "parent (pid = %d) resumed from Wait(); calling Delay(%d) so Yalnix doesn't halt until grandchild has a chance to resume.\n", pid, ORPHAN_TEST_PARENT_DELAY_LENGTH);
	Delay(ORPHAN_TEST_PARENT_DELAY_LENGTH);
	TtyPrintf(0, "parent (pid = %d) exiting with status = %d\n", pid, PARENT_EXIT_STATUS);
	Exit(PARENT_EXIT_STATUS);
}
