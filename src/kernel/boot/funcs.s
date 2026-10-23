
.section .data
gdtr:
	.word 0 # For limit storage
	.quad 0 # For base storage

.section .text
.global set_gdt
set_gdt:
	movw %di, gdtr
	movq %rsi, gdtr+2
	lgdt gdtr

reload_segments:
	pushq $0x08
	leaq .reload_CS(%rip), %rax # RIP relative addressing I believe
	pushq %rax
	lretq # Long return

.reload_CS:
	movw $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ret

.global enable_shit
# void enable_shit(void);
enable_shit:
	# enable SSE I think?
	mov %cr0, %rax
	and $0xFFFB, %ax
	or $0x2, %ax
	mov %rax, %cr0
	mov %cr4, %rax
	or $(3 << 9), %rax
	mov %rax, %cr4
	ret