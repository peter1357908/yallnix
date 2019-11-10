#include <yalnix.h>

#define ITERATIONS 2
#define DELAY_LENGTH 3
#define EXIT_STATUS 50

void main() {
	int i;
	for (i = 0; i < ITERATIONS; i++) {
		TracePrintf(1, "In execTest; iteration = %d, about to delay with ticks = %d\n", i, DELAY_LENGTH);
		Delay(DELAY_LENGTH);
	}
	
	Exit(EXIT_STATUS);
}
