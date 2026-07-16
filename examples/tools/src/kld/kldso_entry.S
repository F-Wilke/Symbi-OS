.section .bss
.align 8
.global kld_initial_rsp
kld_initial_rsp:
    .quad 0

.section .text
.global _start
	.equ MAX_EXTRA_STACK_SPACE, 4096
_start:
    xor %rbp, %rbp      # Clear frame pointer for stack traces
    mov %rsp, kld_initial_rsp(%rip)  # Save kernel-provided initial stack	
    mov (%rsp), %rdi    # First argument for C: argc (at top of stack)
    lea 8(%rsp), %rsi   # Second argument for C: argv (just below argc)

    # Create extra space on stack incase c code needs to modify envp
    sub $MAX_EXTRA_STACK_SPACE, %rsp
    mov %rsp, %rdx      # Third argument
    mov $MAX_EXTRA_STACK_SPACE, %rcx # Fourth argument sizeo of extra
	
    # We assume MAX_EXTRA_STACK_SPACE is a 16 byte aligned
    # so 16 byte alignment of stack is preserved
	
    call c_entry        # Jump to your C code

    # If C returns, exit safely via syscall
    mov $60, %rax       # sys_exit
    xor %rdi, %rdi
    syscall
