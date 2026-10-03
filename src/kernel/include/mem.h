#ifndef MEM_H
#define MEM_H

// Stuff for ram usage and shit
uint64_t get_total_ram(struct limine_memmap_response *memmap);
uint64_t get_usable_ram(struct limine_memmap_response *memmap);

// Stuff for the bitmaps
void* create_bitmap(struct limine_memmap_response *memmap, uint64_t hhdm, uint16_t *best_fit_used);
void finish_bitmap(struct limine_memmap_response *memmap, uint8_t *bitmap, uint16_t used_bitmap_page);

// Allocate/free
void free_page(uint8_t *bitmap, uint64_t page);


#endif