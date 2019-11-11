#include <yalnix.h>
#include <hardware.h>
#include <stdio.h>

#define ITERATIONS 2
#define DELAY_LENGTH 3
#define EXIT_STATUS 50

#define READ_SIZE 15

void main() {
	int i;
	int pid = GetPid();
	
	for (i = 0; i < ITERATIONS; i++) {
		TtyPrintf(0, "In execTest; iteration = %d, about to delay with ticks = %d\n", i, DELAY_LENGTH);
		Delay(DELAY_LENGTH);
	}
	
	int num_read;
	char *buffer = (char *) malloc(TERMINAL_MAX_LINE);
	for (i = 1; i < NUM_TERMINALS; i++) {
		TtyPrintf(i, "In execTest; Terminal %d, give me (pid = %d) something to read (%d characters):\n", i, pid, READ_SIZE);
		
		num_read = TtyRead(i, buffer, READ_SIZE);
		
		buffer[num_read] = '\n';
		
		TtyPrintf(i, "TtyRead said I read %d bytes, and I read the following:\n%s\n", num_read, buffer);
	}
	free(buffer);
	
	Exit(EXIT_STATUS);
}
