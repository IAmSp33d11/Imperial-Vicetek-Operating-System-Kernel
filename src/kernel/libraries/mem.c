// mem.c
// Handles physical memory management
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <mem.h>

// Might be slow, but at least it works. Since I do not believe there is a better
// way to do this
uint64_t get_total_ram(struct limine_memmap_response *memmap) {
	uint64_t mem_counted = 0;
	
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		uint64_t type = entry->type;
		if (type != LIMINE_MEMMAP_RESERVED && type != LIMINE_MEMMAP_BAD_MEMORY 
			&& type != LIMINE_MEMMAP_FRAMEBUFFER && type != LIMINE_MEMMAP_ACPI_NVS) {
				mem_counted += entry->length;
		}
	}
	return mem_counted;
}

uint64_t* create_bitmap() {

}