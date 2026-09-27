
.section .text
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