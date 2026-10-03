CC := x86_64-elf-gcc
CAS := x86_64-elf-as
LD := x86_64-elf-ld
NASM := nasm

OUTPUT      := bin/vice.elf
ISO_IMG     := out/vice.iso
BIOS        ?= /usr/share/edk2/x64/OVMF.4m.fd
BUILD := build

CFLAGS  := -Wall -Wextra -O2 -std=c11 -ffreestanding -g \
           -fno-stack-protector -fno-stack-check -fno-lto -fno-pic -fno-pie \
           -m64 -march=x86-64 -mabi=sysv \
           -mno-red-zone \
           -mcmodel=kernel -Isrc/kernel/include

LDFLAGS := -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -T linker.lds

NASMFLAGS := -f elf64 -g

SRCS := $(shell find src/kernel -name '*.c')
SRCS_GAS := $(shell find src/kernel -name '*.s')
OBJS := $(patsubst src/kernel/%.c, $(BUILD)/%.o, $(SRCS))
OBJS += $(patsubst src/kernel/%.s, $(BUILD)/%.o, $(SRCS_GAS))

all: $(ISO_IMG)

$(BUILD)/%.o: src/kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/kernel/%.s
	@mkdir -p $(dir $@)
	$(CAS) $< -o $@

$(OUTPUT): $(OBJS)
	@mkdir -p bin
	$(LD) $(LDFLAGS) $(OBJS) -o $(OUTPUT)

$(ISO_IMG): $(OUTPUT) limine
	@mkdir -p out iso_root/boot/limine iso_root/EFI/BOOT
	@cp $(OUTPUT) iso_root/boot/
	@cp limine.conf iso_root/boot/limine/
	@cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	@cp limine/BOOTX64.EFI limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	@cp -r copy_to_iso/* iso_root/ 2>/dev/null || true
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO_IMG)
	@./limine/limine bios-install $(ISO_IMG) 2>/dev/null

limine:
	@rm -rf limine
	@if [ ! -d "limine" ]; then \
		curl -L https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz | gunzip | tar -xf -; \
		mv limine-binary/ limine; \
		$(MAKE) -C limine; \
	fi

.PHONY: run-kvm
run-kvm: $(ISO_IMG)
	@echo "[LAUNCH] Initializing ViceOS inside QEMU..."
	qemu-system-x86_64 -m 2G -cdrom $(ISO_IMG) -bios $(BIOS) -serial file:serial.log -cpu host,host-phys-bits=on -enable-kvm

.PHONY: run
run: $(ISO_IMG)
	@echo "[LAUNCH] Initializing ViceOS inside QEMU..."
	qemu-system-x86_64 -m 2G -cdrom $(ISO_IMG) -bios $(BIOS) -serial file:serial.log -cpu max -no-reboot -no-shutdown -d int,cpu_reset -D qemu.log

.PHONY: debug
debug: $(ISO_IMG)
	@echo "[DEBUG] Initializing ViceOS inside QEMU with GDB..."
	qemu-system-x86_64 -m 2G -cdrom $(ISO_IMG) -bios $(BIOS) -serial file:serial.log -s -S -cpu max -no-reboot -no-shutdown -d int,cpu_reset


.PHONY: gdb
gdb: $(ISO_IMG)
	@echo "[DEBUG] Starting up GDB..."
	gdb bin/cat_kernel.elf -ex "target remote :1234" -ex "layout asm" -ex "layout regs" 

clean:
	rm -rf bin out iso_root $(BUILD)

.PHONY: all run run-kvm debug gdb clean