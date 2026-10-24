.section .data # We put any data we need here.

.section .text
.global syscall_stub
# void syscall(uint64_t rax, uint64_t rbx)
# This just is a syscall stub, duh.
syscall_stub:
	swapgs



	movq %rsp, %r10
	movq %gs:0, %rsp # Get the stack pointer saved in our pointer
	

	

	pushq %rcx # Save our registers
	pushq %r10
	pushq %r11
	


	movq %rax, %rdi # Move the arguments so GCC will recognize them
	movq %rbx, %rsi

	call printf

	

	popq %r11 # Restore our registers
	popq %r10
	popq %rcx



	

	movq %r10, %rsp # Restore the stack
	swapgs
	sysretq # No.


