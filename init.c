// #include <hardware.h>

// void doIdle() {
// 	while(1) {
// 		TracePrintf(1, "DoIdle\n");
// 		Pause();
// 	}
// }

void main(int argc, void *argv) {
	while(1) {
		TracePrintf(1, "\nI'm the init process!\n");
	}
}
