#include <generated/asm-offsets.h>
#include <asm/percpu.h>
#include <asm/asm.h>
#include <asm/nospec-branch.h>
#include <asm/thread_info.h>


#define TSS_IST7    84

    .equ FRAME_BUF_MASK, ~(0x7ff) //0x80 B = 128 B per frame, enough for 16 quad words, we use 10
    .equ FRAME_BUF_SIZE, 0x80

	.text
	.align 16
	.global nmi_asm_check_ist7
nmi_asm_check_ist7:
    push %rdi
 	rdgsbase %rdi
    testq %rdi, %rdi
    jns nmi_asm_check_ist7_user_gs
	
    lea cpu_tss_rw(%rip), %rdi
	movq %gs:TSS_IST7(%rdi), %rdi

    sub 0x20(%rsp), %rdi // rdi = rdi - rsp
    //we're on ist7 if 0 <= rdi <= 0x1000 
    cmp $0, %rdi
    jl nmi_asm_check_ist7_exit_now
    cmp $0x1000, %rdi
    jg nmi_asm_check_ist7_exit_now
    //we're one IST7
#ifdef DEBUG    
    movq $1, %rax 
#else 
    //here we'll push IST7 further down
    subq $FRAME_BUF_SIZE, %gs:cpu_tss_rw+TSS_IST7(%rip)
#endif



nmi_asm_check_ist7_exit_now:
    pop %rdi
    ANNOTATE_RETPOLINE_SAFE 
    jmp *nmi_adaptor_asm_exc_nmi
    int3

nmi_asm_check_ist7_user_gs:
    swapgs
	
    lea cpu_tss_rw(%rip), %rdi
	movq %gs:TSS_IST7(%rdi), %rdi

    sub 0x20(%rsp), %rdi // rdi = rdi - rsp
    //we're on ist7 if 0 <= rdi <= 0x1000 
    cmp $0, %rdi
    jl nmi_asm_check_ist7_user_exit_now
    cmp $0x1000, %rdi
    jg nmi_asm_check_ist7_user_exit_now
    //we're one IST7
#ifdef DEBUG
    movq $1, %rax //here we'll push IST7 further down
#else
    //here we'll push IST7 further down
    // andq $FRAME_BUF_MASK, %rdi
    subq $FRAME_BUF_SIZE, %gs:cpu_tss_rw+TSS_IST7(%rip)
#endif


nmi_asm_check_ist7_user_exit_now:
    swapgs
    pop %rdi
    ANNOTATE_RETPOLINE_SAFE	
    jmp *nmi_adaptor_asm_exc_nmi
    int3


#ifdef DEBUG
	.text
	.align 16
	.global nmi_asm_debug
nmi_asm_debug:
    push %rbx
    sub $0x28, %rsp
    mov $0, %rax

    movq %rsp, %rbx //any kernel stack
    testl $1, %edi //arg1: normalNMI(non IST7 stack)
    jnz .Lnmi_debug_main
    
    //set exception frame stack to IST7
    lea cpu_tss_rw(%rip), %rbx
    movq %gs:TSS_IST7(%rbx), %rbx
    jmp .Lnmi_debug_main

.Lnmi_debug_main:
    movq $0x18, 0x20(%rsp)
    movq %rbx, 0x18(%rsp)
    movq $0xf00, 0x10(%rsp)
    movq $0x10, 0x8(%rsp)
    

    lea nmi_asm_ret_stub(%rip), %rbx
    movq %rbx, (%rsp) //rip on exception frame
    movq %rbx, nmi_adaptor_asm_exc_nmi(%rip)

    jmp nmi_asm_check_ist7

nmi_asm_ret_stub:

    add $0x28, %rsp
    pop %rbx

    RET
    int3

#endif

