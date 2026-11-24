/* Tell the linker that we want an x86_64 ELF64 output file */
OUTPUT_FORMAT(elf64-x86-64)

/* The entry point */
ENTRY(bootup)

/* Define the program headers we want so the bootloader gives us the right */
/* MMU permissions; this also allows us to exert more control over the linking */
/* process. */
PHDRS
{
    limine_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    /* We want to be placed in the topmost 2GiB of the address space, for optimisations */
    /* and because that is what the Limine spec mandates. */
    /* Any address in this region will do, but often 0xffffffff80000000 is chosen as */
    /* that is the beginning of the region. */
    . = 0xffffffff80000000;
    _kernel_start = .;

    /* Define a section to contain the Limine requests and assign it to its own PHDR */
    . = ALIGN(4096);
    _limine_requests = .;
    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests

    . = ALIGN(4096);
    _text = .;
    .text : {
        *(.text .text.*)
    } :text


    . = ALIGN(4096);
    _rodata = .;
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    /* Add a .note.gnu.build-id output section in case a build ID flag is added to the */
    /* linker command. */
    . = ALIGN(4096);
    _note_gnu_build_id = .;
    .note.gnu.build-id : {
        *(.note.gnu.build-id)
    } :rodata

    /* I'm aligning everything to page bounardies since I do not fucking like having things cross page boundaries if its possible to avoid that. */
    . = ALIGN(4096);
    _data = .;
    .data : {
        *(.data .data.*)
    } :data

    /* NOTE: .bss needs to be the last thing mapped to :data, otherwise lots of */
    /* unnecessary zeros will be written to the binary. */
    /* If you need, for example, .init_array and .fini_array, those should be placed */
    /* above this. */
    . = ALIGN(4096);
    _bss = .;
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    . = ALIGN(4096);
    _kernel_end = .;

    /* Discard .note.* and .eh_frame* since they may cause issues on some hosts. */
    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}