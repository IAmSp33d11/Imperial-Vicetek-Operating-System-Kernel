.section .data # We put any data we need here.

.section .text
.global syscall_stub
# void syscall(uint64_t rax, uint64_t rbx)
# This just is a syscall stub, duh.
syscall_stub:
	swapgs



	movq %rsp, %r10
	movq %gs:0, %rsp # Get the stack pointer saved in our pointer
	movq %gs:16, %r9 # get the pointer to the CPL if its not ring 3
	

	

	pushq %rcx # Save our registers
	pushq %r10
	pushq %r11
	pushq %r9
	


	movq %rax, %rdi # Move the arguments so GCC will recognize them
	movq %rbx, %rsi

	call printf

	

	popq %r9 # Restore our registers
	popq %r11
	popq %r10
	popq %rcx


	# Cause I'm a fucking idiot and have no clue how else to do this
	pushq %r8

	movq %r9, %r8
	movb (%r9), %r9b
	movb $0, (%r8)

	popq %r8
	
	
	
	cmp $0x1, %r9b # Check if its ring 1
	je .ring1_ret

	cmp $0x2, %r9b # Check if its ring 2 (currently unsupported. TODO)
	je .ring2_ret

	jmp .ring3_ret # If its not either of those, its gotta be ring 3.

.ring1_ret:
	pushq $0x31
	pushq %r10 # The stack pointer before syscall
	pushq %r11 # RFLAGS
	pushq $0x29
	pushq %rcx # The RIP to jump to
	swapgs
	iretq


.ring2_ret: # TBD
	hlt
	jmp .ring2_ret

.ring3_ret: #Simple and easy
	movq %r10, %rsp # Restore the stack
	swapgs
	sysretq # No.


