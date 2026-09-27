// serial.c
// handles various functions related to the serial port
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <stdbool.h>
#include <stdarg.h>

#include <serial.h>
#include <shit.h>

// CAUTION: Does not do bounds checking, requires the string to be null terminated!
void write_serial(char *string) {
	int i = 0;
	while (string[i] != '\0') {
		outb(PORT, string[i++]);
	}
}

void write_char_serial(char n) {
	outb(PORT, n);
}

void printf(char *string, ...) {
	va_list args;
	va_start(args, string);
	int i = 0;
	char current_char = string[i++]; // So we don't read from RAM so often
	while (current_char != '\0') {
		if (current_char != '%') {
			write_char_serial(current_char); // its a normal character so we shall print it!
		} else {
			// Okay its a formatted one
			current_char = string[i++];
			if (current_char == 'x') {
				int64_t val = va_arg(args, int64_t);
				char buffer[128];
				sitoa_hex(val, buffer);
				write_serial(buffer);
			} else if (current_char == 'd') {
				int64_t val = va_arg(args, int64_t);
				char buffer[128];
				sitoa(val, buffer);
				write_serial(buffer);
			} else if (current_char == 'f') {
				double val = va_arg(args, double);
				char buffer[128];
				dtoa(val, 8, buffer);
				write_serial(buffer);
			} else if (current_char == 'u') { // Unsigned ones!
				current_char = string[i++];
				if (current_char == 'x') {
					uint64_t val = va_arg(args, uint64_t);
					char buffer[128];
					itoa_hex(val, buffer);
					write_serial(buffer);
				} else if (current_char == 'd') {
					uint64_t val = va_arg(args, uint64_t);
					char buffer[128];
					itoa(val, buffer);
					write_serial(buffer);
				}
			} else if (current_char == 's') { // Ooh its a string
				char* string = va_arg(args, char*);
				write_serial(string); // We trust the string
			} else if (current_char == 'p') { // Okay so this is a pointer
				void* val = va_arg(args, void*);
				char buffer[128];
				itoa_hex((uint64_t) val, buffer);
				write_serial(buffer);
			} else if (current_char == '%') {
				write_char_serial('%'); // okay its just litterally '%'
			}
		}
		current_char = string[i++];
	}
}