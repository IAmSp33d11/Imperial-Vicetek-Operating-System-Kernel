// mem.c
// Handles physical memory management
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <mem.h>
#include <shit.h>
#include <serial.h>

// Might be slow, but at least it works. Since I do not believe there is a better way to do this
uint64_t get_total_ram(struct limine_memmap_response *memmap) {
	uint64_t mem_counted = 0;
	
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		uint64_t type = entry->type;
		if (type != LIMINE_MEMMAP_RESERVED && type != LIMINE_MEMMAP_RESERVED_MAPPED
			&& type != LIMINE_MEMMAP_FRAMEBUFFER && type != LIMINE_MEMMAP_ACPI_NVS) {
				mem_counted += entry->length;
		}
	}
	return mem_counted;
}

// Using this for bitmap now ig
uint64_t get_highest_usable(struct limine_memmap_response *memmap) {
	uint64_t highest_mem = 0;
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE && entry->length + entry->base > highest_mem) {
			highest_mem = entry->length + entry->base;
		}
	}
	return highest_mem;
}

// Counts the amount of ram we can ACTUALLY USE
uint64_t get_usable_ram(struct limine_memmap_response *memmap) {
	uint64_t mem_counted = 0;
	
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		uint64_t type = entry->type;
		if (type == LIMINE_MEMMAP_USABLE) {
			mem_counted += entry->length;
		}
	}
	return mem_counted;
} 

// Creates a bitmap of reuqired size and fills it all in as if it is used, returns the bitmap in the hhdm, also places the used page into &best_fit
void* create_bitmap(struct limine_memmap_response *memmap, uint64_t hhdm, uint16_t *best_fit_used) {
	uint64_t bitmap_size = ((((((get_highest_usable(memmap) + 4095) / 4096) + 7) / 8) + 4095) / 4096); // Gets the amount of pages we will need for the bitmap
	// Find a viable place to put the bitmap
	uint16_t best_fit = UINT16_MAX; // The entry with the best fit. Set to largest value by default
	uint64_t best_fit_size = UINT64_MAX; // Set the best fit size to the largest value, so that the first one we find will always be better
	for (uint16_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i]; // Buffer it so my code looks nicer lol
		if (entry->type == LIMINE_MEMMAP_USABLE || entry->length < bitmap_size * 4096) { // Make sure it works
			if (entry->length < best_fit_size && entry->length >= bitmap_size * 4096) { // Its better, save it
				best_fit = i; // Best fit so far
				best_fit_size = entry->length;
			}
		} else {continue;}
	}
	if (best_fit == UINT16_MAX) {
		// Fuck
		printf("PANIC: UNABLE TO ALLOCATE BITMAP!\nBitmap size in pages: %d\n", bitmap_size);
		hcf();
	}
	printf("Found a viable place to put the bitmap!\nEntry: %d\nEntry_Size (bytes): %d\nRequired_Size (bytes): %d\nEntry_Size (pages): %d\nRequired_Size (pages): %d\n", best_fit, best_fit_size, bitmap_size * 4096, (best_fit_size / 4096) + 1, bitmap_size);

	// Place the bitmap and fill it with "allocated" statuses
	uint64_t *address = (uint64_t*) (memmap->entries[best_fit]->base + hhdm);
	printf("Entry Address: 0x%p\n", address);
	for (uint64_t i = 0; i < (bitmap_size * 512); i++) { // Multiply by 512 to get the amount of uint64s in it
		address[i] = UINT64_MAX;
	}

	printf("Placed the bitmap!\n");
	*best_fit_used = best_fit;
	return address; // Return the bitmap in the hhdm
}

// Sets the status of a specified page to "free" by clearing the bit
void clear_page(uint8_t *bitmap, uint64_t page) {
	uint64_t byte_offset = page >> 3;
	uint8_t bit = page & 0b111;
	bitmap[byte_offset] &= ~(1 << bit);
}
// Just a stupid wrapper, no one likes it lol.
void free_page(uint8_t *bitmap, uint64_t page) {
	clear_page(bitmap, page);
}
// Sets the status of a specified page to "allocated" by setting the bit to 1
void set_page(uint8_t *bitmap, uint64_t page) {
	uint64_t byte_offset = page >> 3;
	uint8_t bit = page & 0b111;
	bitmap[byte_offset] |= (1 << bit);
}

// Finishes the bitmap and makes it accurately depict the memory of the system
void finish_bitmap(struct limine_memmap_response *memmap, uint8_t *bitmap, uint16_t used_bitmap_page) {
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		if (entry->type != LIMINE_MEMMAP_USABLE) continue; // If its not usable, skip it. It's already labeled as "allocated"

		// We have to free the usable portions
		for (uint64_t j = 0; j < (entry->length / 4096); j++) {
			clear_page(bitmap, j + (entry->base / 4096));
		}
	}
	// Re-allocate the bitmap
	struct limine_memmap_entry *entry = memmap->entries[used_bitmap_page];
	uint64_t bitmap_size = ((((((get_highest_usable(memmap) + 4095) / 4096) + 7) / 8) + 4095) / 4096); // Get the original bitmap size.
	for (uint64_t i = 0; i < bitmap_size; i++) { // Accidentally did +-+ and that looked like a cute face so +-+
		set_page(bitmap, i + (entry->base / 4096));
	}
	// We should be done I think???
	printf("Finished the bitmap!\n");
}