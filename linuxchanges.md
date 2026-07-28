Perf related changes:

Fix 1: perf callchain dispatch — RSP-based routing for elevated tasks
=====================================================================

File: linux/kernel/events/callchain.c
Function: get_perf_callchain() (around line 220)

Change: Before the existing kernel/user callchain dispatch, add a branch
for symbiote_elevated tasks that routes based on the sign of regs->sp
(the actual NMI-time stack pointer) rather than user_mode(regs) (which
checks CS and is always "kernel" for elevated tasks).

Key differences from the standard path:
  1. RSP sign, not CS, determines whether we are on a kernel or user stack.
     This is the correct discriminator because the frame chain lives on
     the stack, not at the instruction pointer.
       RSP < 0 (kernel stack) -> kernel callchain  (CPBS/BPBS kernel phase)
       RSP >= 0 (user stack)  -> user callchain    (CPCS/BPCS always;
                                                     CPBS/BPBS user phase)

  2. For the user-stack case the actual NMI regs are used directly —
     NOT task_pt_regs(current).  task_pt_regs() holds the register state
     saved at the last kernel-entry point (the sym_elevate syscall) which
     has stale values (e.g. rbp=0x3 from the calling convention at
     elevation time).  The NMI regs hold the live user-space RSP/RBP at
     the moment of the NMI, which are the correct starting point for
     frame-pointer unwinding.

  3. For CPCS mode (constant privilege, constant user stack): even when
     the elevated task is executing kernel code at ring 0, the stack is
     the user stack, so all frames (including kernel function frames) are
     on the user stack.  The user callchain walker reads them correctly
     via get_user() since SMAP/SMEP are disabled for elevated tasks.

Mode coverage:
  CPCS: RSP always user  -> always user callchain with NMI regs        OK
  BPCS: RSP always user  -> always user callchain with NMI regs        OK
  CPBS: RSP user/kernel  -> user or kernel callchain by RSP sign       OK
  BPBS: RSP always kernel when elevated -> kernel callchain            OK
  Non-elevated (BPCS/BPBS lowered): symbiote_elevated=0 -> unchanged   OK

Dependency: Fix 1 (efasm RIP guard) is required for the user-callchain
path to be safe when frame pointers are absent and get_user() may fault.

Fix 2: perf_arch_misc_flags — correct user/kernel sample attribution
====================================================================

File: linux/arch/x86/events/core.c
Function: host_misc_flags() (around line 3058), called by
           perf_arch_misc_flags() (line 3075)

Change: In host_misc_flags(), before the standard user_mode(regs) check,
add a branch for symbiote_elevated tasks that uses the sign of regs->ip
(the NMI-time instruction pointer) to classify the sample.

Why RIP sign here (not RSP sign as in Fix 2):
  perf_misc_flags determines WHERE THE CODE IS — which binary/module
  should be used for symbolication (user vmaps vs kernel kallsyms).
  The correct discriminator is the instruction pointer, not the stack
  pointer.  A kernel-space RIP means kernel code was executing; a
  user-space RIP means user code was executing.  Using RSP here would
  misclassify CPCS-kernel-executing-on-user-stack samples as user
  (correct for the stack, wrong for code attribution).

Effect without this fix:
  Elevated tasks always produce PERF_RECORD_MISC_KERNEL even when
  executing user-space code (CS=0x10 -> user_mode()=false).
  perf report then tries to symbolicate user-space IPs against kernel
  kallsyms, producing garbage output or failed lookups.

Mode coverage:
  CPCS user code:    RIP > 0 -> MISC_USER  -> symbolicated vs user binary OK
  CPCS kernel code:  RIP < 0 -> MISC_KERNEL -> symbolicated vs kallsyms  OK
  BPCS (elevated):   RIP < 0 (calling kernel fn) -> MISC_KERNEL          OK
  BPCS (lowered):    symbiote_elevated=0 -> standard path                 OK
  CPBS user phase:   RIP > 0 -> MISC_USER                                 OK
  CPBS kernel phase: RIP < 0 -> MISC_KERNEL                               OK
  BPBS (elevated):   RIP < 0 -> MISC_KERNEL                               OK
  BPBS (lowered):    symbiote_elevated=0 -> standard path                 OK

Fix 3: perf_get_regs_user — correct user register snapshot for elevated tasks
=============================================================================

File: linux/arch/x86/kernel/perf_regs.c
Function: perf_get_regs_user() (line 133, x86-64 path)

Change: When the current task is symbiote_elevated and the NMI-time stack
pointer is in user space, use the actual NMI regs instead of
task_pt_regs(current) as the source of user register values.

This fix affects:
  - perf record --call-graph dwarf  (PERF_SAMPLE_STACK_USER uses these
    regs to locate and copy the user stack)
  - perf record --sample-regs-user  (reports user register state at
    sample time)

Why task_pt_regs(current) is wrong for elevated tasks:
  task_pt_regs(current) returns the pt_regs saved at the LAST kernel-
  entry point for the task.  For an elevated task (CPCS), this is the
  sym_elevate() syscall — a snapshot potentially billions of instructions
  old.  In particular:
    regs->sp = user RSP at elevation time (stale)
    regs->bp = user RBP at elevation time (stale, e.g. 0x3 for valkey)
    regs->ip = user RIP at elevation time (stale)
  Passing these to perf_sample_stack_user() causes it to copy the wrong
  region of the user stack (wrong RSP base); passing bp=0x3 to the frame-
  pointer walker causes a fault that the EF adaptor mishandles.

  The actual NMI regs hold the live user-space state at the moment of the
  sample:
    regs->sp = actual user RSP -> correct stack base for DWARF copy
    regs->bp = actual user RBP -> correct frame pointer start
    regs->ip = actual user RIP -> correct instruction pointer

RSP-sign guard (same rationale as Fix 2):
  If the NMI-time RSP is on a kernel stack (CPBS/BPBS kernel phase), the
  elevated task was executing kernel code on a real kernel stack at sample
  time.  In that case task_pt_regs() does not reflect any useful user
  state and we fall through to the normal non-elevated path which returns
  NULL for a kernel thread or the stale task_pt_regs for a user thread.
  For the kernel-stack case we clear regs_user to signal "no user regs"
  since the task was genuinely in kernel mode at sample time.

Note on the NMI task_pt_regs-in-setup guard (lines 151-156):
  The existing guard checks whether regs->sp falls inside the task_pt_regs
  region (meaning the NMI fired during syscall entry before pt_regs was
  fully saved).  For elevated tasks with a user-space RSP this check
  naturally fails (user RSP is nowhere near the kernel stack), so it does
  not interfere.
