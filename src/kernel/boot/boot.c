// I should probably put stuff up here
// boot.c
// Handles the booting process
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <shit.h>
#include <mem.h>
#include <serial.h>
#include <stdarg.h>
#include <asm.h>

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 4
};



__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;



extern void ring_1_call_gate_stub(void); // src/kernel/driver-handler/rings.s
extern void enable_shit(void); // src/kernel/boot/funcs.s
void create_gdt(); // src/kernel/boot/gdt.c
extern void idt_init(void); // src/kernel/boot/idt.c
extern void drop_to_ring1(uint64_t address, uint64_t target_stack); // src/test/rings.s
extern void syscall_stub(void);

__attribute__((noreturn))
void bootup(void);
__attribute__((noreturn))
void bootup_cont(void); 
__attribute__((noreturn))
void ring_1_test(void); 


uint8_t cpl = 0x0; // Global CURRENT PRIVILEGE LEVEL

void bootup(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    init_serial();

    // Get the HHDM
    if (hhdm_request.response == NULL) {
        fill(framebuffer, 0x00FF0000);
        hcf();
    }
    uint64_t hhdm = hhdm_request.response->offset;


    // Get the memmap
    struct limine_memmap_response *memmap_response = memmap_request.response;
    if (memmap_response == NULL) {
        fill(framebuffer, 0x00FF0000);
        hcf();
    }

    // TODO : Load init later
    /*
    for (uint64_t i = 0; i < module_request.response->module_count; i++) {
        struct limine_file *file = module_request.response->modules[i];


    }
    */
    
    uint64_t ram = get_total_ram(memmap_response);
    enable_shit();
    double mib = bytes_to_mib(ram);
    printf("Total ram: %f MiB\n", mib);
    ram = get_usable_ram(memmap_response);
    mib = bytes_to_mib(ram);
    printf("Usable ram: %f MiB\n", mib);
    void *bitmap;
    { // Force gcc to make temp go out of scope
        uint16_t temp = 0;
        bitmap = create_bitmap(memmap_response, hhdm, &temp);
        finish_bitmap(memmap_response, bitmap, temp);
    }


    // Attempt to allocate a single page.
    printf("Attempting to allocate a page!\nHHDM: %ux\n", hhdm);
    void* page1 = alloc(100, false);
    printf("We attempted to allocate a page! Got a structure located at 0x%p!\n", page1);

    struct allocated_pages *out = page1;
    
    printf("Amount of entries: %udl\n", out->entry_count);
    free(out);

    // prepare a second stack (this will not be freed)
    struct allocated_pages *stack = alloc(4, true);
    if (stack == NULL) {
        printf("Well we were unable to allocate 16KiB of consecutive memory for a stack\n");
        hcf();
    }
    uint64_t rsp = stack->pages[0] + hhdm + 0x4000;
    printf("%uxl\n", rsp);
    memset((uint8_t*) stack, 0, 0x4000);
    
    
    printf("Finished testing physical memory allocator!\nCreating Global Descriptor Table!\n");
    create_gdt();
    printf("Successfully created GDT!\nAttempting to create IDT!\n");
    idt_init();
    printf("Successfully created IDT!\n");

    char* string = get_cpu_name();
    printf("%s\n", string);
    free_addr(string);

    // Before we drop into ring 0, we must do this
    uint64_t efer = rdmsr(0xC0000080);
    efer |= 0b1; // Set bit 0 for System Call Extensions
    wrmsr(0xC0000080, efer);


    // Prepare a syscall function so that way ring 1 drivers can get back here.
    
    { // Make temp go out of scope so GCC reuses its memory
        uint64_t temp = 0x10LL << 48; // Set to usermode data segment - 0x8;
        wrmsr(0xC0000081, temp); // Set STAR msr.
    }

    wrmsr(0xC0000082, (uint64_t) syscall_stub);
    wrmsr(0xC0000084, 1 << 9);

    // Setup GS and point it to a kernel data page.
    uint64_t* kernel_data = alloc_page(); // This will not be freed.
    kernel_data[0] = rsp; // the stack
    kernel_data[1] = hhdm; // the higher half direct map
    kernel_data[2] = (uint64_t) &cpl; // The cpl. Used for syscall.


    // write to the GS MSR. I forgot its technical name. I'll check it when I get home.
    wrmsr(0xC0000102, (uint64_t) kernel_data);

    struct allocated_pages *ring1_stack = alloc(2, true);
    if (ring1_stack == NULL) {
        printf("Well we were unable to allocate 8KiB of consecutive memory for a ring 1 stack\n");
        hcf();
    }
    uint64_t ring1_rsp = (uint64_t) ring1_stack->pages[0] + 0x2000 + hhdm;
    memset((uint8_t*) ring1_stack, 0, 0x2000);

    printf("CPL is : %udb\n", get_cpl());
    drop_to_ring1((uint64_t) ring_1_test, ring1_rsp);
    
    bootup_cont();
}

void syscall(const char* string) {
    __asm__ volatile (
        "movb %b1, (%2);"
        "syscall;"
        : "+a"(string)
        : "r" (get_cpl()), "r" (&cpl)
        : "rcx", "r11", "memory"
    );
}


void ring_1_test() {
    write_serial("We are in ring 1 successfully!\n");
    syscall("Syscall test!\n");
    printf("CPL is : %udb\n", get_cpl());
    while (true); // Halt it since we can't use hcf() here
}

void bootup_cont(void) {
    
    hcf();
}