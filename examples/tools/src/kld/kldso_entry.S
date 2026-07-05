.section .bss
.align 8
.global kld_initial_rsp
kld_initial_rsp:
    .quad 0

.section .text
.global _start

_start:
    xor %rbp, %rbp      # Clear frame pointer for stack traces
    mov %rsp, kld_initial_rsp(%rip)  # Save kernel-provided initial stack
    mov (%rsp), %rdi    # First argument for C: argc (at top of stack)
    lea 8(%rsp), %rsi   # Second argument for C: argv (just below argc)
    
    # Align stack to 16 bytes before calling C
    and $-16, %rsp
    
    call c_entry        # Jump to your C code

    # If C returns, exit safely via syscall
    mov $60, %rax       # sys_exit
    xor %rdi, %rdi
    syscall
