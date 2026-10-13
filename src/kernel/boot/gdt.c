// gdt.c
// handles GDT setup with all 4 rings
#include <stdint.h> // Include cause I need this
#include <stddef.h> // Included just in case
#include <stdbool.h> // Included just in case
#include <shit.h> // Included for hcf()
#include <serial.h> // Included for printf()
#include <mem.h> // Included for alloc_page()

struct GDT {
	uint32_t limit; // Ignored
	uint64_t base; // Ignored
	uint8_t access_byte; // [P][DPL][DPL][S][E][DC][RW][A]
	uint8_t flags; // 0000[G][DB][L][Reserved]
};

void encode_gdt_entry(uint8_t *target, struct GDT source) {
	// Check the limit to make sure that it can be encoded
    if (source.limit > 0xFFFFF) {printf("GDT cannot encode limits larger than 0xFFFFF"); hcf();}
    
    // Encode the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] = (source.limit >> 16) & 0x0F;
    
    // Encode the base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;
    
    // Encode the access byte
    target[5] = source.access_byte;
    
    // Encode the flags
    target[6] |= (source.flags << 4);
}

extern void set_gdt(uint16_t limit, uint64_t base);
void create_gdt() {
	uint8_t* target = alloc_page();
	memset(target, 0, 0x1000); // Clear the page, and by extension, create a null segment
	struct GDT source;
    source.limit = 0;
    source.base = 0;

	source.access_byte = 0b10011011; // 0x9B Ring 0 Code Segment
	source.flags = 0x2; // Long Mode
	encode_gdt_entry(target + 0x8, source);

    source.access_byte = 0b10010011; // 0x93 Ring 0 Data Segment
    // Long Mode
    encode_gdt_entry(target + 0x10, source);

    // [P][DPL][DPL][S][E][DC][RW][A]
    source.access_byte = 0b10111011; // 0xBB Ring 1 Code Segment 
    // Long Mode Still
    encode_gdt_entry(target + 0x18, source);

    source.access_byte = 0b10110011; // 0xB3 Ring 1 Data Segment
    // Long Mode, still. Wow
    encode_gdt_entry(target + 0x20, source); 

    printf("Does it work?\n");
    // TODO: Add Rings 1, 2, and 3 here later
    set_gdt(0x27, (uint64_t) target);
}