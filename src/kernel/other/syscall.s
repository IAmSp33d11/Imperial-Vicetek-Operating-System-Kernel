.section .data # We put any data we need here.

.section .text
# How to call a syscall
.global syscall_stub
syscall_stub:
	pushq %rcx
	


	popq %rcx
	sysret # No.
