.section .data # We put any data we need here.

.section .text
.global ring_1_call_gate_stub
ring_1_call_gate_stub:
	pushq %rbx
	pushq %rbp
	pushq %r12
	pushq %r13
	pushq %r14

	pop %r14
	pop %r13
	pop %r12
	pop %rbp
	pop %rbx
	lretq

.global drop_to_ring1
# void drop_to_ring1(uint64_t address);
drop_to_ring1:
	pushq $0x21
	pushq %rsi
	pushq $0x19 # Push the GDT offset of ring 1 code reload_segments
	pushq %rdi
	lretq

.jump_to_address:
	jmp %rdi