#include <generated/asm-offsets.h>
#include <asm/percpu.h>
#include <asm/asm.h>
#include <asm/nospec-branch.h>
#include <asm/thread_info.h>

#define TSS_IST7    84

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
	
	.equ RFLAGS_SWAPPED_GS, 1<<29
	.equ RFLAGS_SWAPPED_GS_MASK, ~(1<<29)

        .equ USER_CS, 0x33
	.equ USER_SS, 0x2b

	.equ CS_MASK, ~0xff
	.equ SS_MASK, ~0xff
	.equ FLAGS_MASK, 0xffffffff00000000
	.equ ERRCODE_MASK, ~0xff

	.text
	.align 16
	.global ef_adaptor_asm_fix_ist_err_code
ef_adaptor_asm_fix_ist_err_code:
	// This will be executed on an IST stack
	// we copy the exception frame from the IST stack to the regular kernel stack
	// if we faulted on a user stack we use tss.rsp0
	// if we faulted on a kernel stack we use the stack pointer from the exception frame
	cld
	testq $0x8, %rsp
	jz ef_adaptor_asm_fix_ist_no_err_code

	pushq %rdi

	xorq %rdi, %rdi
	movq %rsp, %rdi
	andq $0xfff, %rdi //mask to get the offset within the page of the faulting stack pointer. If this is small, then we likely faulted on a user stack. If it's large, then we likely faulted on a kernel stack. This is not a perfect check but should work in practice since user stacks are usually near the bottom of the user address space and kernel stacks are near the top of the kernel address space.
	cmpq $0xfc0, %rdi //compare against a threshold near the top of the page
	jne .Lerr_c_len_bad

.Lerr_c_len_normal:
	movq 0x30(%rsp), %rdi //faulting stack pointer from exception frame
	pushq %rsi
	pushq %rcx
	
	//get tss.rsp0 as destination (rdi)
	rdgsbase %rcx
	testq %rcx, %rcx
	js .Lerr_c_no_swapgs //if gsbase is negative, then we were elevated
	swapgs //swapgs was not a kernel address, so we need to swap it
	orl $RFLAGS_SWAPPED_GS, 0x38(%rsp) //set bit 40 of the saved RFLAGS in the exception frame to indicate that we swapped gs. We will check this in the common handler and swap back if needed.


.Lerr_c_no_swapgs:	
	movabs $0xcccccccccccccccc, %rcx
	cmp %rcx, %rdi
	je .Lerr_c_do_double_fault //we have 

	testq %rdi, %rdi
	jns .Lskip_double_fault_test //if positive then we came from user mode, no df check needed

	//check for df here, we know we have a valid gs
	lea cpu_tss_rw(%rip), %rsi
	movq %gs:TSS_sp0(%rsi), %rsi
	
	//is cr2 on the page below our kernel stack (in rsi)?
	subq $(THREAD_SIZE + 0x1000), %rsi
	movq %cr2, %rcx
	andw $0xf000, %cx
	testq %rcx, %rsi
	jns .Lerr_c_skip_set_dest //we are on some other page so no df

.Lerr_c_do_double_fault:
	//here we'll jump to the DF handler
	popq %rcx
	popq %rsi
	popq %rdi
	addq $8, %rsp //remove return address from stack
	jmp   *df_handler_location
	int3

	
.Lskip_double_fault_test:
	lea cpu_tss_rw(%rip), %rdi
	movq %gs:TSS_sp0(%rdi), %rdi

.Lerr_c_skip_set_dest:
	//page fault handler specific: if we're elevated, we need to modify the error code
	movq %gs:current_task(%rip), %rsi 

	testb $1, TASK_symbiote_elevated(%rsi)

	jz  .Lerr_c_skip_flag_set

	movq %cr2, %rcx
	testq %rcx, %rcx
	js .Lerr_c_skip_flag_set 
	//if cr2 is negative, it's a kernel address, so we want to give the appearance that we faulted in the kernel.
	orl   $(ERRCODE_PF_USER) , 0x20(%rsp)

	movq   $(USER_CS) , 0x30(%rsp)
	movq   $(USER_SS) , 0x48(%rsp)

.Lerr_c_skip_flag_set:
.Lerr_c_copy_frame_to_dest:

	subq $0x50, %rdi //point to start of exception frame on IST stack
	movq %rsp, %rsi //source is current stack pointer
	movq $10, %rcx //6 quads for exception frame + 1 ret addr + 3 for registers we push at the start of this function
	rep movsq

	//rdi now points to the bottom of the copied exception frame
	lea -0x50(%rdi), %rsp //switch to new stack before we pop registers and jump to common handler
	
	//here we can restore IST7 to the base of the stack we entered on. 
	//rsi already points to the top of the stack.
	//read address of tss.IST7
	//write rsi to that location
	movq %rsi, %gs:cpu_tss_rw+TSS_IST7(%rip)
	
	testl $RFLAGS_SWAPPED_GS, 0x38(%rsp)
	jz .Lerr_c_no_swap_back
	swapgs //swap back gs if we swapped it in the beginning of this function
	andl $RFLAGS_SWAPPED_GS_MASK, 0x38(%rsp) //clear the swapped gs bit in the saved RFLAGS in the exception frame so that the common handler doesn't try to swapgs back again

.Lerr_c_no_swap_back:
	popq %rcx
	popq %rsi
	popq %rdi
	cmpq $0, (%rsp)
	je .Lerr_c_ret_state_bad
	testl $ERRCODE_MASK, 0x8(%rsp)
	jnz .Lerr_c_ret_state_bad
	
	testl $CS_MASK, 0x18(%rsp)
	jnz .Lerr_c_ret_state_bad

	testl $FLAGS_MASK, 0x20(%rsp)
	jnz .Lerr_c_ret_state_bad

	testl $SS_MASK, 0x30(%rsp)
	jnz .Lerr_c_ret_state_bad

.Lerr_c_ret_normal:
	retq
	// int3 to satifiy objtool desire for a trap after a speculative jump to try and improve security
	int3



.Lerr_c_len_bad:
	jmp .Lerr_c_len_normal
	int3

.Lerr_c_ret_state_bad:
	jmp .Lerr_c_ret_normal
	int3


	.text
	.align 16
	.global ef_adaptor_asm_fix_ist_no_err_code
ef_adaptor_asm_fix_ist_no_err_code:
	// This will be executed on an IST stack
	// we copy the exception frame from the IST stack to the regular kernel stack
	// if we faulted on a user stack we use tss.rsp0
	// if we faulted on a kernel stack we use the stack pointer from the exception frame

	pushq %rdi

	xorq %rdi, %rdi
	movq %rsp, %rdi
	andq $0xfff, %rdi //mask to get the offset within the page of the faulting stack pointer. If this is small, then we likely faulted on a user stack. If it's large, then we likely faulted on a kernel stack. This is not a perfect check but should work in practice since user stacks are usually near the bottom of the user address space and kernel stacks are near the top of the kernel address space.
	cmpq $0xfc8, %rdi //compare against a threshold near the top of the page
	jne .Lno_err_c_len_bad

.Lno_err_c_len_normal:
	movq 0x28(%rsp), %rdi //faulting stack pointer from exception frame // validated
	pushq %rsi
	pushq %rcx
	
	rdgsbase %rcx
	testq %rcx, %rcx
	js .Lno_err_c_no_swapgs //if gsbase is negative, then we were elevated
	
	swapgs //swapgs was not a kernel address, so we need to swap it
	orl $RFLAGS_SWAPPED_GS, 0x30(%rsp) //if negative, then we already have the right destination 
	
.Lno_err_c_no_swapgs:
	testq %rdi, %rdi
	js .Lno_err_c_skip_set_dest

	lea cpu_tss_rw(%rip), %rdi
	movq %gs:TSS_sp0(%rdi), %rdi

.Lno_err_c_skip_set_dest:
.Lno_err_c_copy_frame_to_dest:
	subq $0x48, %rdi //point to start of exception frame on IST stack

	movq %rsp, %rsi //source is current stack pointer
	movq $9, %rcx //6 quads for exception frame + 3 for registers we push at the start of this function
	rep movsq

	//rdi now points to the bottom of the copied exception frame
	lea -0x48(%rdi), %rsp //switch to new stack before we pop registers and jump to common handler
	//here we can restore IST7 to the base of the stack we entered on. 
	//rsi already points to the top of the stack.

	//read address of tss.IST7
	//write rsi to that location
	movq %rsi, %gs:cpu_tss_rw+TSS_IST7(%rip)

	testl $RFLAGS_SWAPPED_GS, 0x30(%rsp)
	jz .Lno_err_c_no_swap_back
	swapgs //swap back gs if we swapped it in the beginning of this function
	andl $RFLAGS_SWAPPED_GS_MASK, 0x30(%rsp) //clear the swapped gs bit in the saved RFLAGS in the exception frame so that the common handler doesn't try to swapgs back again

.Lno_err_c_no_swap_back:

	popq %rcx
	popq %rsi
	popq %rdi
	cmpq $0, (%rsp)
	je .Lerr_c_ret_state_bad

	testl $CS_MASK, 0x10(%rsp)
	jnz .Lerr_c_ret_state_bad

	testl $FLAGS_MASK, 0x18(%rsp)
	jnz .Lerr_c_ret_state_bad

	testl $SS_MASK, 0x28(%rsp)
	jnz .Lerr_c_ret_state_bad

	retq
	// int3 to satifiy objtool desire for a trap after a speculative jump to try and improve security
	int3

.Lno_err_c_len_bad:
	jmp .Lno_err_c_len_normal
	int3


// #define DEBUG_OHSHIT
/*
	.glob ef_isr_x
	.glob ef_isr_tgt_x
	.align 8
ef_isr_x:	
	subq $8, %rsp
	push %rdi
	movq ef_isr_tgt_x(%rip), %rdi
	movq %rdi, 8(%rsp)
	pop  %rdi
	jmp  common_ef_hdlr
	.align 8
ef_isr_tgt_x:
	.quad 0
	*/

#ifdef DEBUG_OHSHIT
	.text
	.align 16
	.global ef_common_hdlr
ef_common_hdlr:
	retq
	int3
	
	// switched to using a table to avoid code patching
	// NOTE: not executing cld!!!! be careful
	//       assume real entry points will take are of the real work
	//       so we must be careful
#define ef_isr(i)	 			\
	.global ef_adaptor_isr_##i	;	\
	.align 32			;	\
ef_adaptor_isr_##i:			;	\
        ENDBR                           ;       \
	subq $8, %rsp			;	\
	push %rdi			;	\
	movq ef_adaptor_orig_isrs + (##i * 8)(%rip), %rdi ;	\
       testq %rdi, %rdi                ;      \
ohshit_##i:  jz    ohshit_##i           ;      \
	movq %rdi, 8(%rsp)		;	\
	pop  %rdi			;	\
	jmp ef_adaptor_asm_fix_ist_err_code
#else
	.text
	.align 16
	.global ef_common_hdlr
ef_common_hdlr:
	retq
	int3
	
	// switched to using a table to avoid code patching
	// NOTE: not executing cld!!!! be careful
	//       assume real entry points will take are of the real work
	//       so we must be careful
#define ef_isr(i)	 			\
	.global ef_adaptor_isr_##i	;	\
	.align 32			;	\
ef_adaptor_isr_##i:			;	\
        ENDBR                           ;       \
	subq $8, %rsp			;	\
	push %rdi			;	\
	movq ef_adaptor_orig_isrs + (##i * 8)(%rip), %rdi ;	\
	movq %rdi, 8(%rsp)		;	\
	pop  %rdi			;	\
	jmp ef_adaptor_asm_fix_ist_err_code
#endif
	.text
	.global ef_adaptor_isrs
	.align 32
ef_adaptor_isrs:
	ef_isr(0)
	ef_isr(1)
	ef_isr(2)
	ef_isr(3)
	ef_isr(4)
	ef_isr(5)
	ef_isr(6)
	ef_isr(7)
	ef_isr(8)
	ef_isr(9)
	ef_isr(10)
	ef_isr(11)
	ef_isr(12)
	ef_isr(13)
	ef_isr(14)
	ef_isr(15)
	ef_isr(16)
	ef_isr(17)
	ef_isr(18)
	ef_isr(19)
	ef_isr(20)
	ef_isr(21)
	ef_isr(22)
	ef_isr(23)
	ef_isr(24)
	ef_isr(25)
	ef_isr(26)
	ef_isr(27)
	ef_isr(28)
	ef_isr(29)
	ef_isr(30)
	ef_isr(31)
	ef_isr(32)
	ef_isr(33)
	ef_isr(34)
	ef_isr(35)
	ef_isr(36)
	ef_isr(37)
	ef_isr(38)
	ef_isr(39)
	ef_isr(40)
	ef_isr(41)
	ef_isr(42)
	ef_isr(43)
	ef_isr(44)
	ef_isr(45)
	ef_isr(46)
	ef_isr(47)
	ef_isr(48)
	ef_isr(49)
	ef_isr(50)
	ef_isr(51)
	ef_isr(52)
	ef_isr(53)
	ef_isr(54)
	ef_isr(55)
	ef_isr(56)
	ef_isr(57)
	ef_isr(58)
	ef_isr(59)
	ef_isr(60)
	ef_isr(61)
	ef_isr(62)
	ef_isr(63)
	ef_isr(64)
	ef_isr(65)
	ef_isr(66)
	ef_isr(67)
	ef_isr(68)
	ef_isr(69)
	ef_isr(70)
	ef_isr(71)
	ef_isr(72)
	ef_isr(73)
	ef_isr(74)
	ef_isr(75)
	ef_isr(76)
	ef_isr(77)
	ef_isr(78)
	ef_isr(79)
	ef_isr(80)
	ef_isr(81)
	ef_isr(82)
	ef_isr(83)
	ef_isr(84)
	ef_isr(85)
	ef_isr(86)
	ef_isr(87)
	ef_isr(88)
	ef_isr(89)
	ef_isr(90)
	ef_isr(91)
	ef_isr(92)
	ef_isr(93)
	ef_isr(94)
	ef_isr(95)
	ef_isr(96)
	ef_isr(97)
	ef_isr(98)
	ef_isr(99)
	ef_isr(100)
	ef_isr(101)
	ef_isr(102)
	ef_isr(103)
	ef_isr(104)
	ef_isr(105)
	ef_isr(106)
	ef_isr(107)
	ef_isr(108)
	ef_isr(109)
	ef_isr(110)
	ef_isr(111)
	ef_isr(112)
	ef_isr(113)
	ef_isr(114)
	ef_isr(115)
	ef_isr(116)
	ef_isr(117)
	ef_isr(118)
	ef_isr(119)
	ef_isr(120)
	ef_isr(121)
	ef_isr(122)
	ef_isr(123)
	ef_isr(124)
	ef_isr(125)
	ef_isr(126)
	ef_isr(127)
	ef_isr(128)
	ef_isr(129)
	ef_isr(130)
	ef_isr(131)
	ef_isr(132)
	ef_isr(133)
	ef_isr(134)
	ef_isr(135)
	ef_isr(136)
	ef_isr(137)
	ef_isr(138)
	ef_isr(139)
	ef_isr(140)
	ef_isr(141)
	ef_isr(142)
	ef_isr(143)
	ef_isr(144)
	ef_isr(145)
	ef_isr(146)
	ef_isr(147)
	ef_isr(148)
	ef_isr(149)
	ef_isr(150)
	ef_isr(151)
	ef_isr(152)
	ef_isr(153)
	ef_isr(154)
	ef_isr(155)
	ef_isr(156)
	ef_isr(157)
	ef_isr(158)
	ef_isr(159)
	ef_isr(160)
	ef_isr(161)
	ef_isr(162)
	ef_isr(163)
	ef_isr(164)
	ef_isr(165)
	ef_isr(166)
	ef_isr(167)
	ef_isr(168)
	ef_isr(169)
	ef_isr(170)
	ef_isr(171)
	ef_isr(172)
	ef_isr(173)
	ef_isr(174)
	ef_isr(175)
	ef_isr(176)
	ef_isr(177)
	ef_isr(178)
	ef_isr(179)
	ef_isr(180)
	ef_isr(181)
	ef_isr(182)
	ef_isr(183)
	ef_isr(184)
	ef_isr(185)
	ef_isr(186)
	ef_isr(187)
	ef_isr(188)
	ef_isr(189)
	ef_isr(190)
	ef_isr(191)
	ef_isr(192)
	ef_isr(193)
	ef_isr(194)
	ef_isr(195)
	ef_isr(196)
	ef_isr(197)
	ef_isr(198)
	ef_isr(199)
	ef_isr(200)
	ef_isr(201)
	ef_isr(202)
	ef_isr(203)
	ef_isr(204)
	ef_isr(205)
	ef_isr(206)
	ef_isr(207)
	ef_isr(208)
	ef_isr(209)
	ef_isr(210)
	ef_isr(211)
	ef_isr(212)
	ef_isr(213)
	ef_isr(214)
	ef_isr(215)
	ef_isr(216)
	ef_isr(217)
	ef_isr(218)
	ef_isr(219)
	ef_isr(220)
	ef_isr(221)
	ef_isr(222)
	ef_isr(223)
	ef_isr(224)
	ef_isr(225)
	ef_isr(226)
	ef_isr(227)
	ef_isr(228)
	ef_isr(229)
	ef_isr(230)
	ef_isr(231)
	ef_isr(232)
	ef_isr(233)
	ef_isr(234)
	ef_isr(235)
	ef_isr(236)
	ef_isr(237)
	ef_isr(238)
	ef_isr(239)
	ef_isr(240)
	ef_isr(241)
	ef_isr(242)
	ef_isr(243)
	ef_isr(244)
	ef_isr(245)
	ef_isr(246)
	ef_isr(247)
	ef_isr(248)
	ef_isr(249)
	ef_isr(250)
	ef_isr(251)
	ef_isr(252)
	ef_isr(253)
	ef_isr(254)
	ef_isr(255)

	.global ef_adaptor_isr_len
	.equ ef_adaptor_isr_len, ef_adaptor_isr_1 - ef_adaptor_isr_0

