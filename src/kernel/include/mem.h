#ifndef MEM_H
#define MEM_H

// Stuff for ram usage and shit
uint64_t get_total_ram(struct limine_memmap_response *memmap);
uint64_t get_usable_ram(struct limine_memmap_response *memmap);

// Stuff for the bitmaps
void* create_bitmap(struct limine_memmap_response *memmap, uint64_t hhdm, uint16_t *best_fit_used);
void finish_bitmap(struct limine_memmap_response *memmap, uint8_t *bitmap_saved, uint16_t used_bitmap_page);

// Allocate/free
void free_page(uint64_t page);
struct allocated_pages* alloc(uint8_t page_num, bool consecutive);	

// Structure that returns a list of pages allocated
struct allocated_pages {
	uint64_t entry_count;
	uint64_t *pages; // A pointer to a list of addresses
};

#endif