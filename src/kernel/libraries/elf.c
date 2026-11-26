// elf.c
// Handles the loading of elf files, and by extension, init.
#include <elf.h> // Literally defined by this file
#include <serial.h> // For debugging

bool check_elf_validity(void* elf_ptr) {
	{
		// This code may look ugly but it is practically the same as if I put them all into one if statement when it is compiled.
		uint8_t* elf = elf_ptr;
		if (elf[0] != 0x7F) { // Magic Number
			return false;
		}
		if (elf[1] != 'E' || elf[2] != 'L' || elf[3] != 'F') { // ELF
			return false;
		}
		if (elf[4] != 2) { // 32/64 bit
			return false;	
		}
		if (elf[5] != 1) { // Endianness
			return false;
		}
		if (elf[7] != 0) { // ABI (we use Sys V)
			return false;
		}

		if ((uint16_t) elf[18] != 0x3E) { // Check the Instruction Set
			return false; // Check for x86_64
		}
		if ((uint16_t) elf[16] != 2) { // Make sure it is executable and not shared/core/relocatable	
			return false;
		}
	}

	// NOT FINISHED
	return true;
}