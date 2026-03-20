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
	
	//build source operand for rep movs
	movq %rsp, %rsi 

	testq %rdi, %rdi
	js .Lskip_set_dest //if negative, then we already have the right destination 

	//get tss.rsp0 as destination (rdi)


.Lskip_set_dest:
	subq $0x40, %rdi //point to start of exception frame on IST stack
.Lcopy_frame_to_dest:
	movq $9, %rcx //6 quads for exception frame + 3 for registers we push at the start of this function
	rep movsq

	//rdi now points to the bottom of the copied exception frame
	lea -0x48(%rdi), %rsp //switch to new stack before we pop registers and jump to common handler

.Lfinish_ist_fixup:
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
	
	// swapgs //swapgs was not a kernel address, so we need to swap it
    // movq %gs:cpu_current_top_of_stack(%rip), %rdi
	// swapgs //restore original gsbase

	jmp .Lno_err_c_skip_set_dest
.Lno_err_c_no_swapgs:
	// movq %gs:cpu_current_top_of_stack(%rip), %rdi

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