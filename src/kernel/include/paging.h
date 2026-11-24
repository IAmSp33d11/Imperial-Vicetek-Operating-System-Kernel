#ifndef PAGING_H
#define PAGING_H


void set_cr3(uint64_t* val);
int map_page(uint64_t* page_map, uint64_t virt_addr, uint64_t phys_addr, uint64_t hhdm, uint8_t phys_addr_size, uint16_t avl, bool overwrite, uint16_t flags, bool mib_2);
void switch_page_map(uint64_t* new_map, uint64_t hhdm_offset);
void* make_page_map(uint64_t* kernel_data);

#endif