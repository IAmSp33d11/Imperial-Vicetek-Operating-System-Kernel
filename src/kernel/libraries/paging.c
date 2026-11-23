// paging.c
// Handles mapping pages, copying Page Maps, etc.

#include <stdint.h> // Included for uint8_t and the others
#include <stdbool.h>  // Included for true/false
#include <serial.h> // Included for debug printing
#include <shit.h> // Included for hcf and shit
#include <mem.h> // Included for allocating a 4KiB page for the page map.
#include <paging.h> // Included cause that's what this is for.
#include <limine.h> // Included for the setup paging function.



// We just assume that mib is a 2MiB page and split it into 512 identical 4KiB pages
void mib_2_kib(uint64_t* mib, uint64_t* kib_level, uint16_t mib_idx) {
	uint64_t mib_bs = mib[mib_idx];

	uint64_t phys_addr = ((mib_bs >> 21) & 0x7FFFFFFFULL) << 21;

	// Convert it once MOTHERFUCKERS
	uint64_t starter = 0;
	starter |= ((mib_bs >> 52) & 0xFFFULL) << 52; // Get the upper twelve bits
	starter |= (mib_bs & 0b1111111ULL); // Lower 7 bits
	starter |= ((mib_bs >> 12) & 1ULL) << 7; // PAT
	starter |= ((mib_bs >> 8) & 0xFULL) << 8; // Put the next 4 bits

	uint64_t temp = 0;
	for (int idx = 0; idx < 512; idx++) {
		temp = starter;
		temp |= (phys_addr + (0x1000 * idx));
		kib_level[idx] = temp;
	}
}

// Sized Flags
// 15:14 Reserved (0)
// 13 - XD
// 12:9 - PK
// 8 - G
// 7 - PAT
// 6 - D
// 5 - A
// 4 - PCD
// 3 - PWT
// 2 - U/S
// 1 - R/W
// 0 - P
// AVL 0:9 used
// Returns 0 if successful
// Returns 1 if the page exists already (as 2mib)
// Returns 2 if the page exists already (as 4kib)
int map_page(uint64_t* page_map, uint64_t virt_addr, uint64_t phys_addr, uint64_t hhdm, uint8_t phys_addr_size, uint16_t avl, bool overwrite, uint16_t flags, bool mib_2) {
	uint64_t mask = ((1ULL << phys_addr_size) - 1) & ~4095ULL;
	uint64_t mib2_mask = ((1ULL << phys_addr_size) - 1) & ~0x1FFFFFULL;


	if ((phys_addr & (mib_2 ? mib2_mask : mask)) != phys_addr) { // Make sure its not out of range and the CPU actually supports it.
		printf("Kernel Panic: Attempted to map out of range physical address 0x%uxl to virtual address 0x%uxl!\nCPU supports %udb bits!\nMask used to check: 0x%uxl\n", phys_addr, virt_addr, phys_addr_size, (mib_2) ? mib2_mask : mask);
		hcf();
	}
	if ((virt_addr & (mib_2 ? ~0x1FFFFFULL : ~4095ULL)) != virt_addr) {
		printf("Kernel Panic: Attempted to map physical address 0x%uxl to unaligned virtual address 0x%uxl!\nAttempted to map a %s page!\n", phys_addr, virt_addr, (mib_2) ? "2 MiB" : "4 KiB");
		hcf();
	}

	// Canonical check
	{ // Temporary stuff
		uint64_t a = virt_addr >> 47; // Check it using 4 level, since I couldn't give a shit about PML5
		if (a != 0x1FFFF && a != 0) { // Its not canonical.
			printf("Kernel Panic: Attempted to map non-canonical virtual address 0x%uxl to physical address 0x%uxl!\n", virt_addr, phys_addr);
			hcf();
		}
	}

	// Okay we can say that its not an illegal mapping that it seems.
	// I will add more checks later probably that make sure its not illegal in other ways.

	// 39 because 48 - 9, and each page map level has 0x1FF entries, aka 512 - 1 since it's 0 indexed.
	uint64_t idx = (virt_addr >> 39) & 0x1FF;

	uint64_t* pdpte;
	if (!(page_map[idx] & 1)) { // If the present bit is set, then
		// Allocate the pdpte
		pdpte = alloc_page();
		if (pdpte == NULL) {
			printf("We fucked up.\nPDPTE\n");
			hcf();
		}
		memset(pdpte, 0, 0x1000);


		uint64_t pdpte_phys = (uint64_t)pdpte - hhdm;
		uint64_t temp = 0;
		temp |= pdpte_phys; // Set the phys address of pdpte
		temp |= 0b111; // Set Present, RW, and User to make it the most permissive.
		page_map[idx] = temp;
	} else { // Otherwise, we just grab the existing one
		pdpte = (uint64_t*) ((page_map[idx] & mask) + hhdm);
	}

	idx = (virt_addr >> 30) & 0x1FF;

	uint64_t* pde;
	if (!(pdpte[idx] & 1)) { // If the present bit is set, then
		// Allocate the pde
		pde = alloc_page();
		if (pde == NULL) {
			printf("We fucked up.\nPDE\n");
			hcf();
		}
		memset(pde, 0, 0x1000);


		uint64_t pde_phys = (uint64_t)pde - hhdm;
		uint64_t temp = 0;
		temp |= pde_phys; // Set the phys address of pdpte
		temp |= 0b111; // Set Present, RW, and User to make it the most permissive.
		pdpte[idx] = temp;
	} else { // Otherwise, we just grab the existing one
		pde = (uint64_t*) ((pdpte[idx] & mask) + hhdm);
	}
	idx = (virt_addr >> 21) & 0x1FF;

	if (mib_2) {
		if (overwrite || !(pde[idx] & 1)) {
			uint64_t temp = 0;
			temp |= phys_addr; // Set the physical address of the page.
			temp |= ((flags >> 13) & 1ULL) << 63; // Set XD
			temp |= (flags & 0b1111111); // Set the lower 7 flags
			temp |= 1ULL << 7; // Tell the CPU this is a 2MiB page.
			temp |= ((flags >> 8) & 1ULL) << 8; // Set the Global Bit
			temp |= ((flags >> 9) & 0xFULL) << 59; // Set the Protection Key or whatever
			temp |= ((flags >> 7) & 1ULL) << 12; // Set the PAT Bit
			temp |= (avl & 0b111ULL) << 9; // Lower AVL
			temp |= ((avl >> 3) & 0b1111111ULL) << 52; // Higher AVL
			pde[idx] = temp;
			return 0;
		}
		return 1;
	}


	uint64_t* pte;
	if (!(pde[idx] & 1) ||  (((pde[idx] >> 7) & 1) && (pde[idx] & 1))) { // If the present bit is set, then
		// Allocate the pte

		pte = alloc_page();
		if (pte == NULL) {
			printf("We fucked up.\nPTE\n");
			hcf();
		}
		

		if ((pde[idx] >> 7) & 1) { // If its a 2MiB page
			if (overwrite) {
				mib_2_kib(pde, pte, idx);
			} else {
				free_addr(pte);
				return 1; // If the caller doesn't want us overwriting stuff, then fuck are we supposed to do?!
			}
		} else {
			memset(pte, 0, 0x1000);
		}


		uint64_t pte_phys = (uint64_t)pte - hhdm;
		uint64_t temp = 0;
		temp |= pte_phys; // Set the phys address of pde
		temp |= 0b111; // Set Present, RW, and User to make it the most permissive.
		pde[idx] = temp;
	} else { // Otherwise, we just grab the existing one
		pte = (uint64_t*) ((pde[idx] & mask) + hhdm);
	}
	idx = (virt_addr >> 12) & 0x1FF;

	if (overwrite || !(pte[idx] & 1)) {
		uint64_t temp = 0;
		temp |= phys_addr; // Set the physical address
		temp |= ((flags >> 13) & 1ULL) << 63; // Set XD
		temp |= (flags & 0b111111111); // Set the lower flags (9 of them)
		temp |= (avl & 0b111ULL) << 9; // Lower AVL
		temp |= ((avl >> 3) & 0b1111111ULL) << 52; // Higher AVL
		temp |= ((flags >> 9) & 0xFULL) << 59; // Set the Protection Key or whatever
		pte[idx] = temp;
		return 0;
	}

	return 2;
}




/* All the bs that's in the linker.lds file*/
extern uint64_t _limine_requests; /* The Limine Requests, Duh*/
extern uint64_t _text; /* Our Code, Comrade. */
extern uint64_t _rodata; /* Hey you can only read this, no touchy. */
extern uint64_t _note_gnu_build_id; /* What? Its read only tho */
extern uint64_t _data; /* Read and Write I think? */
extern uint64_t _bss; /* UNITIAILIZED :3 */
extern uint64_t _kernel_end; /* The end of the kernel */
/* Nothing else */


// Returns the physical address of said page map.
// Pass less stuff so we don't have to use the stack.
uint64_t make_page_map(uint64_t* kernel_data) {
	void* page_map = alloc_page(); // Allocate the PML4
	memset(page_map, 0, 0x1000); // Zero it out
	printf("Preparing to make the page map\n");
	
	// Fun fact, the framebuffer is in the HHDM!
	struct limine_memmap_response *memmap = (void*) kernel_data[3];

	uint64_t hhdm_offset = kernel_data[1]; // The offset to the HHDM

	printf("Mapping the kernel elf segments\n");
	printf("Mapping Limine Requests segments\n");
	for (uint64_t i = _limine_requests; i < _text; i += 4096) {
		map_page(page_map, i, i - kernel_data[4], hhdm, kernel_data[5], 0, true, 0b0010000000000001); // XD and P set (Read only No Execute)
	}
	printf("Mapping Text segments\n");
	for (uint64_t i = _text; i < _rodata; i += 4096) {
		map_page(page_map, i, i - kernel_data[4], hhdm, kernel_data[5], 0, true, 0b1); // We literally just need P set. (Read Only Execute)
	}
	printf("Mapping read only data segments.\n");
	for (uint64_t i = _rodata; i< _note_gnu_build_id; i+= 4096) {
		map_page(page_map, i, i - kernel_data[4], hhdm, kernel_data[5], 0, true, 0b0010000000000001); // XD and P set (Read Only No Execute)
	}
	printf("Mapping whatever the fuck the \".note.gnu.build.id\" is in our ELF.\n");
	for (uint64_t i = _note_gnu_build_id; i < _data; i+= 4096) {
		map_page(page_map, i, i - kernel_data[4], hhdm, kernel_data[5], 0, true, 0b0010000000000001); // XD and P set (Read Only No Execute)
	}
	printf("Mapping data segments\n");
	for (uint64_t i = _data; i < _bss; i += 4096) {
		map_page(page_map, i, i - kernel_data[4], hhdm, kernel_data[5], 0, true, 0b0010000000000011); // XD, P and RW set (RW No Execute)
	}
	printf("Mapping the bss\n");
	for (uint64_t i = _bss; i < _kernel_end; i += 4096) {
		map_page(page_map, i, i - kernel_data[4], hhdm, kernel_data[5], 0, true, 0b0010000000000011); // XD, P and RW set (RW No Execute)
	}

	printf("Mapping the stuff we find in the memmap!\n");
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		
	}
}