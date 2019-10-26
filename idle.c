#include <hardware.h>

main() {
	while(1) {
		TracePrintf(1, "DoIdle\n");
		Pause();
	}
}

