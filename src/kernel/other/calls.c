// calls.c
// Handles the syscalls
#include <serial.h> // Temporary, rempove after replacing rax case 1.

void syscall(uint64_t rax, uint64_t rbx, uint64_t r12, uint64_t rdx) {
	switch (rax) {
		case 1: // temporary printf for debugging
			write_serial((char*) rbx);
			break;
	}
}