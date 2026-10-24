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


// only thing from my old ass OS.
struct __attribute__((packed)) TSS_entry {
    uint32_t reserved0;
    uint64_t rsp0; // Kernel Stack
    uint64_t rsp1; // Don't care
    uint64_t rsp2; // Don't care
    uint64_t reserved1;
    uint64_t ist1; // Interrupt Stack Tables
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7; 
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base; 
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


void encodeTssEntry(uint8_t *target, uint64_t tss_address)
{
    uint32_t limit = 104 - 1;
    
    target[0] = limit & 0xFF;
    target[1] = (limit >> 8) & 0xFF;
    target[6] = (limit >> 16) & 0x0F;
    
    target[2] = tss_address & 0xFF;
    target[3] = (tss_address >> 8) & 0xFF;
    target[4] = (tss_address >> 16) & 0xFF;
    target[7] = (tss_address >> 24) & 0xFF;
    
    target[5] = 0x89;
    target[6] |= (0x0 << 4);
    
    target[8] = (tss_address >> 32) & 0xFF;
    target[9] = (tss_address >> 40) & 0xFF;
    target[10] = (tss_address >> 48) & 0xFF;
    target[11] = (tss_address >> 56) & 0xFF;
    
    target[12] = 0;
    target[13] = 0;
    target[14] = 0;
    target[15] = 0;
}

struct TSS_entry tss __attribute__((aligned(16))) = {0};    



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

    source.access_byte = 0b11110011; // 0xF3 Ring 3 Data Segment
    encode_gdt_entry(target + 0x18, source);

    source.access_byte = 0b11111011; // 0xFB Ring 3 Code Segment
    encode_gdt_entry(target + 0x20, source);

    printf("Attempting to write a TSS so that way we don't die.\n");
    memset(&tss, 0, sizeof(struct TSS_entry));
    uint64_t stack = (uint64_t) alloc_page(); // This will not be freed.
    memset((void*)stack, 0, 0x1000);

    tss.rsp0 = stack + 0x1000;
    tss.iomap_base = sizeof(struct TSS_entry);
    encodeTssEntry(target + 0x28, (uint64_t)&tss);


    printf("Does it work?\n");
    set_gdt(0x38, (uint64_t) target);
}