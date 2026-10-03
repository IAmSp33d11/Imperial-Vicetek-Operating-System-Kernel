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
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 1
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




extern void enable_shit(void);

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

    if (module_request.response == NULL) {
        fill(framebuffer, 0x00FF0000);
        hcf();
    }

    // Get the HHDM
    uint64_t hhdm = hhdm_request.response->offset;
    if (hhdm_request.response == NULL) {
        fill(framebuffer, 0x00FF0000);
        hcf();
    }

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
    init_serial();
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
    // We're done, just hang...
    hcf();
}