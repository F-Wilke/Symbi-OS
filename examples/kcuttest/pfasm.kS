#include <generated/asm-offsets.h>

	// HW ERROR CODE is last item pushed on to stack by exception logic
	// bits are defined as follows for pagefault exception-- from intel manual
	.equ ERRCODE_PF_PROT,  1<<0
	.equ ERRCODE_PF_WRITE, 1<<1
	.equ ERRCODE_PF_USER,  1<<2
	.equ ERRCODE_PF_RSVD,  1<<3
	.equ ERRCODE_PF_INSTR, 1<<4	
	.equ ERRCODE_PF_PK,    1<<5
	.equ ERRCODE_PF_SHSTK, 1<<6
	.equ ERRCODE_PF_HLAT,  1<<7
	.equ ERRCODE_PF_SGX,   1<<15
	
	.text
	.align 16
	.global pf_adaptor_asm_fix_err_code_u
pf_adaptor_asm_fix_err_code_u:
	// NYI: Add tests to only fix error code if faulting process is elevated and
	//      address and fault type are sensible.  
	// x86 allows immediate and memory operands without using a register
	incq  pf_adaptor_pf_cnt
	//check current->symbiote_elevated and only set user bit if elevated
	
	testq %rsp, %rsp
	js .Lskip_stack_touch
	sti //if we page fault again, we must not have interrupts disabled, otherwise do_user_addr_fault will issue a warning.
	movb $1, -0x1000(%rsp) //touch stack so that 
	cli

.Lskip_stack_touch:
	pushq %rax
	rdgsbase %rax
	test %rax, %rax
	jns  .Lskip_user_bit //gsbase is positve or zero (no sign flag) => user mode => definitely not elevated. 

	movq %gs:current_task(%rip), %rax
	// movq $0x1, %rax
	testb $1, TASK_symbiote_elevated(%rax)
	// testb $1, %al
	jz  .Lskip_user_bit

	orl   $ERRCODE_PF_USER, 8(%rsp)



.Lskip_user_bit:
	popq %rax

	jmp   *pf_adaptor_asm_exc_page_fault
	// int3 to satifiy objtool desire for a trap after a speculative jump to try and improve security
	int3




	.text
	.align 16
	.global pf_adaptor_asm_fix_err_code_uw
pf_adaptor_asm_fix_err_code_uw:
	// This will be executed on the kernel IST stack
	// fix double fault 
	// x86 allows immediate and memory operands without using a register
	incq  pf_adaptor_df_cnt
	pushq %rax
	//which assumptions can I make about gs base after a double fault? 
	//paranoid entry reads from $0xc0000101 to get a value for gsbase. So I can clearly not 
/*
	It swaps gs if the value read from 0xc0000101 not a kernel address. It does nothing if the value is a kernel address. 
	So I can also just check that.
   0xffffffff81001c46 <+134>:   mov    $0x1,%ebx
   0xffffffff81001c4b <+139>:   mov    $0xc0000101,%ecx
   0xffffffff81001c50 <+144>:   rdmsr
   0xffffffff81001c52 <+146>:   test   %edx,%edx
   0xffffffff81001c54 <+148>:   js     0xffffffff81001c5b <paranoid_entry+155>
   0xffffffff81001c56 <+150>:   xor    %ebx,%ebx
   0xffffffff81001c58 <+152>:   swapgs
*/
	//the same check as in paranoid_entry
	rdgsbase %rax
	test %rax, %rax
	jns  .Lskip_df_set //if gsbase is not a kernel address, we're not elevated

	//if gsbase is garbage but looks like a kernel address exc_double_fault would also blow up, 
	// so we might as well blow up here
	movq %gs:current_task(%rip), %rax 

	testb $1, TASK_symbiote_elevated(%rax)

	jz  .Lskip_df_set

	orl   $(ERRCODE_PF_USER|ERRCODE_PF_WRITE), 8(%rsp)
	popq %rax
	jmp   *pf_adaptor_asm_exc_page_fault 


.Lskip_df_set:
	popq %rax

	jmp   *pf_adaptor_asm_exc_double_fault	
	// int3 to satifiy objtool desire for a trap after a speculative jump to try and improve security
	int3
	


	.text
	.align 16
	.global pf_adaptor_asm_fix_err_code_gp
pf_adaptor_asm_fix_err_code_gp:
	// This will be executed on the kernel IST stack
	// fix double fault 
	// x86 allows immediate and memory operands without using a register
	incq  pf_adaptor_gp_cnt
	
	testq %rsp, %rsp
	js .Lgp_skip_stack_touch
	sti //if we page fault again, we must not have interrupts disabled, otherwise do_user_addr_fault will issue a warning.
	movb $1, -0x1000(%rsp) //touch stack so that 
	cli
	
.Lgp_skip_stack_touch:

	jmp   *pf_adaptor_asm_exc_general_protection	
	// int3 to satifiy objtool desire for a trap after a speculative jump to try and improve security
	int3




	.text
	.align 16
	.global pf_adaptor_asm_fix_ist_err_code
pf_adaptor_asm_fix_ist_err_code:
	// This will be executed on an IST stack
	// we copy the exception frame from the IST stack to the regular kernel stack
	// if we faulted on a user stack we use tss.rsp0
	// if we faulted on a kernel stack we use the stack pointer from the exception frame

	pushq %rdi

	movq 0x28(%rsp), %rdi //faulting stack pointer from exception frame // validated
	pushq %rsi
	pushq %rcx
	

	testq %rdi, %rdi
	js .Lerr_c_skip_set_dest //if negative, then we already have the right destination 

	//get tss.rsp0 as destination (rdi)
	rdgsbase %rdi
	testq %rdi, %rdi
	js .Lerr_c_no_swapgs //if gsbase is negative, then we were elevated
	
	swapgs //swapgs was not a kernel address, so we need to swap it
	movq %gs:cpu_current_top_of_stack(%rip), %rdi
	swapgs //restore original gsbase

	jmp .Lerr_c_skip_set_dest

.Lerr_c_no_swapgs:
	movq %gs:cpu_current_top_of_stack(%rip), %rdi

	//page fault handler specific: if we're elevated, we need to modify the error code
	movq %gs:current_task(%rip), %rsi 

	testb $1, TASK_symbiote_elevated(%rsi)

	jz  .Lerr_c_skip_flag_set

	orl   $(ERRCODE_PF_USER|ERRCODE_PF_WRITE), 18(%rsp)

.Lerr_c_skip_flag_set:
.Lerr_c_copy_frame_to_dest:

	subq $0x40, %rdi //point to start of exception frame on IST stack
	movq %rsp, %rsi //source is current stack pointer
	movq $9, %rcx //6 quads for exception frame + 3 for registers we push at the start of this function
	rep movsq

	//rdi now points to the bottom of the copied exception frame
	lea -0x48(%rdi), %rsp //switch to new stack before we pop registers and jump to common handler

	popq %rdi
	popq %rsi
	popq %rcx
	jmp   *pf_adaptor_asm_exc_page_fault
	// int3 to satifiy objtool desire for a trap after a speculative jump to try and improve security
	int3






	.text
	.align 16
	.global pf_adaptor_asm_fix_ist_no_err_code
pf_adaptor_asm_fix_ist_no_err_code:
	// This will be executed on an IST stack
	// we copy the exception frame from the IST stack to the regular kernel stack
	// if we faulted on a user stack we use tss.rsp0
	// if we faulted on a kernel stack we use the stack pointer from the exception frame

	pushq %rdi

	movq 0x20(%rsp), %rdi //faulting stack pointer from exception frame // validated
	pushq %rsi
	pushq %rcx
	

	testq %rdi, %rdi
	js .Lno_err_c_skip_set_dest //if negative, then we already have the right destination 

	//get tss.rsp0 as destination (rdi)
	rdgsbase %rdi
	testq %rdi, %rdi
	js .Lno_err_c_no_swapgs //if gsbase is negative, then we were elevated
	
	swapgs //swapgs was not a kernel address, so we need to swap it
	movq %gs:cpu_current_top_of_stack(%rip), %rdi
	swapgs //restore original gsbase

	jmp .Lno_err_c_skip_set_dest
.Lno_err_c_no_swapgs:
	movq %gs:cpu_current_top_of_stack(%rip), %rdi

.Lno_err_c_skip_set_dest:
.Lno_err_c_copy_frame_to_dest:
	subq $0x38, %rdi //point to start of exception frame on IST stack

	movq %rsp, %rsi //source is current stack pointer
	movq $8, %rcx //6 quads for exception frame + 3 for registers we push at the start of this function
	rep movsq

	//rdi now points to the bottom of the copied exception frame
	lea -0x40(%rdi), %rsp //switch to new stack before we pop registers and jump to common handler

	popq %rdi
	popq %rsi
	popq %rcx
	jmp   *pf_adaptor_asm_exc_page_fault
	// int3 to satifiy objtool desire for a trap after a speculative jump to try and improve security
	int3