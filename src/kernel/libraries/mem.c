// mem.c
// Handles physical memory management
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <mem.h>
#include <shit.h>
#include <serial.h>

static void *bitmap; // Bitmap so that way the caller does not always need to have it on hand.
static uint64_t bitmap_size; // Bitmap size so the caller doesn't need to have it on hand also.
static uint64_t hhdm; // USEFUL :3

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
void* create_bitmap(struct limine_memmap_response *memmap, uint64_t high, uint16_t *best_fit_used) {
	bitmap_size = ((get_highest_usable(memmap) + 4095) / 4096);
	uint64_t size = ((((bitmap_size + 7) / 8) + 4095) / 4096); // Gets the amount of pages we will need for the bitmap
	printf("size = %dl\nHighest usable = %dl\nHighest usable in MiB = %f\n", bitmap_size, get_highest_usable(memmap), bytes_to_mib(get_highest_usable(memmap)));
	// Find a viable place to put the bitmap
	uint16_t best_fit = UINT16_MAX; // The entry with the best fit. Set to largest value by default
	uint64_t best_fit_size = UINT64_MAX; // Set the best fit size to the largest value, so that the first one we find will always be better
	for (uint16_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i]; // Buffer it so my code looks nicer lol
		if (entry->type == LIMINE_MEMMAP_USABLE || entry->length < size * 4096) { // Make sure it works
			if (entry->length < best_fit_size && entry->length >= size * 4096) { // Its better, save it
				best_fit = i; // Best fit so far
				best_fit_size = entry->length;
			}
		} else {continue;}
	}
	if (best_fit == UINT16_MAX) {
		// Fuck
		printf("PANIC: UNABLE TO ALLOCATE BITMAP!\nBitmap size in pages: %d\n", size);
		hcf();
	}
	printf("Found a viable place to put the bitmap!\nEntry: %udl\nEntry_Size (bytes): %udl\nRequired_Size (bytes): %udl\nEntry_Size (pages): %udl\nRequired_Size (pages): %dl\n", best_fit, best_fit_size, size * 4096, (best_fit_size / 4096) + 1, size);

	// Place the bitmap and fill it with "allocated" statuses
	uint64_t *address = (uint64_t*) (memmap->entries[best_fit]->base + high);
	printf("Entry Address: 0x%uxl\n",((uint64_t) address) - high);
	for (uint64_t i = 0; i < (size * 512); i++) { // Multiply by 512 to get the amount of uint64s in it

		address[i] = UINT64_MAX;
	}

	printf("Placed the bitmap!\n");
	*best_fit_used = best_fit;
	hhdm = high; // Also not addressing why I called it high lol.
	return address; // Return the bitmap in the hhdm
}

// Sets the status of a specified page to "free" by clearing the bit
void clear_page(uint8_t *bitmap_saved, uint64_t page) {
	uint64_t byte_offset = page >> 3;
	uint8_t bit = page & 0b111;
	bitmap_saved[byte_offset] &= ~(1 << bit);
}
// Just a stupid wrapper, no one likes it lol.
void free_page(uint64_t page) {
	clear_page(bitmap, page);
}
// Sets the status of a specified page to "allocated" by setting the bit to 1
void set_page(uint8_t *bitmap_saved, uint64_t page) {
	uint64_t byte_offset = page >> 3;
	uint8_t bit = page & 0b111;
	bitmap_saved[byte_offset] |= (1 << bit);
}

bool get_page(uint8_t *bitmap_saved, uint64_t page) {
	uint64_t byte_offset = page >> 3;
	uint8_t bit = page & 0b111;
	return (bitmap_saved[byte_offset] >> bit) & 0b1;
}

// Finishes the bitmap and makes it accurately depict the memory of the system
void finish_bitmap(struct limine_memmap_response *memmap, uint8_t *bitmap_saved, uint16_t used_bitmap_page) {
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		if (entry->type != LIMINE_MEMMAP_USABLE) continue; // If its not usable, skip it. It's already labeled as "allocated"

		// We have to free the usable portions
		for (uint64_t j = 0; j < (entry->length / 4096); j++) {
			clear_page(bitmap_saved, j + (entry->base / 4096));
		}
	}
	// Re-allocate the bitmap
	uint64_t size = ((((bitmap_size + 7) / 8) + 4095) / 4096);
	struct limine_memmap_entry *entry = memmap->entries[used_bitmap_page];
	for (uint64_t i = 0; i < size; i++) { // Accidentally did +-+ and that looked like a cute face so +-+
		set_page(bitmap_saved, i + (entry->base / 4096));
	}

	for (uint64_t i = 0; i < 5; i++) {
		set_page(bitmap_saved, i);
	}
	// We should be done I think???
	printf("Finished the bitmap!\n");
	bitmap = bitmap_saved;

}

/*
Returns a pointer to a structure containing the physical address of the pages allocated, 
the pages will be consecutive if *consecutive* var is true, and if not they may still be 
consecutive cause its the easiest pages to find. Will allocate page_num pages.
*/
struct allocated_pages* alloc(uint8_t page_num, bool consecutive) {
	static uint64_t last_page_idx = 0; // Stores the last page idx, to get bits, & 0b111, to get bytes, >> 3
	uint64_t i = last_page_idx;
	bool gone_around = false;
	void* first_page = NULL; // a pointer in the hhdm to the first page.
	if (page_num == 0) return NULL;
	while (true) {
		if (i >= bitmap_size) {
			if (gone_around) {
				printf("I ate the ram.\nI'm kidding. We're out of fucking ram.\n");
				hcf();
			} else {
				printf("Went around once. i = %dl\nbitmap_size = %dl\n", i, bitmap_size);
				gone_around = true;
				i = 0;
			}
		} else if ((get_page(bitmap, i) == false) && consecutive) {
			bool good = true;
			for (int j = 0; j < page_num; j++) {
				if (get_page(bitmap, j+i) == true) {
					good = false;
					break;
				}
			}
			
			if (good) {
				set_page(bitmap, i); // Take the first page of the thing
				uint64_t physical_address = i * 4096;
				first_page = (void*) physical_address + hhdm;
				break;
			}
		} else if (get_page(bitmap, i) == false) {
			set_page(bitmap, i); // Take the first page of the thing
			uint64_t physical_address = i * 4096;
			first_page = (void*) (physical_address + hhdm);
			break;
		}
		i++;
	}
	last_page_idx = i;
	printf("We found a page\n");
	struct allocated_pages* out = first_page;
	out->entry_count = page_num;
	out->pages = (uint64_t*) (((uint64_t*) first_page) + 8); // Set it as the bytes right after entry_count
	out->pages[0] = (((uint64_t) first_page) - hhdm);
	// We got the first page
	printf("allocated 1 page of ram\n0x%p\n", out);

	// get the other pages
	if (page_num == 1) return out; // if they just wanted that page then just return.
	// If it is consecutive, then we can just use a simple trick.
	if (consecutive) {
		for (int j = 1; j < page_num; j++) {
			out->pages[j] = (((uint64_t) first_page) - hhdm) + (0x1000 * (j)); // Add one page to the last one
			set_page(bitmap, i + j);
		}
		i += page_num;
	} else { // We have to do it the tedious way.
		int j = 1;
		while (true) {
			if (j >= page_num) break;
			if (i >= bitmap_size) {
				if (gone_around) {
					printf("I ate the ram.\nI'm kidding. We're out of fucking ram.\n");
					hcf();
				} else {
					printf("Went around once. i = %dl\nbitmap_size = %dl\n", i, bitmap_size);
					gone_around = true;
					i = 0;
				}
			} else if (get_page(bitmap, i) == false) {
				set_page(bitmap, i); // Take the oage
				uint64_t physical_address = i * 4096;
				out->pages[j++] = physical_address;
				continue;
			}
			i++;
		}
	}

	last_page_idx = i;
	return out;
}

// A wrapper that allocates a singular page
void* alloc_page() {
	struct allocated_pages* returned = alloc(1, false);
	return (uint64_t*) (returned->pages[0] + hhdm);
}

// Frees a series of pages, basically a stupid wrapper
void free(struct allocated_pages* in) {
	for (int i = 0; i < in->entry_count; i++) {
		free_page(in->pages[i] / 0x1000); // Free each individual page 
	}
}