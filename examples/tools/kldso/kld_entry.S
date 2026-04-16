.section .text
.global _start

_start:
    xor %rbp, %rbp      # Clear frame pointer for stack traces
    mov (%rsp), %rdi    # First argument for C: argc (at top of stack)
    lea 8(%rsp), %rsi   # Second argument for C: argv (just below argc)
    
    # Align stack to 16 bytes before calling C
    and $-16, %rsp
    
    call c_entry        # Jump to your C code

    # If C returns, exit safely via syscall
    mov $60, %rax       # sys_exit
    xor %rdi, %rdi
    syscall
