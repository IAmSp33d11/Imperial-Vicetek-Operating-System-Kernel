.section .data # We put any data we need here.

.section .text
.global drop_to_ring1
# void drop_to_ring1(uint64_t address, uint64_t rsp);
drop_to_ring1:
	pushfq # Push current RFLAGS
	popq %r10 # pop into r10
	orq $0x1000, %r10 # Allow Ring 1 to use ports
	orq $0x202, %r10 # Set the IF flag and bit 1.
	pushq %r10
	popfq # put it into RFLAGs

	pushq $0x31
	pushq %rsi # Push the stack pointer
	pushq $0x29 # Push the GDT offset of ring 1 code
	pushq %rdi # Push the address to jump to
	lretq