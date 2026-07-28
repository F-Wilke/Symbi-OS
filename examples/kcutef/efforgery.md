EF adaptor — guard CS/SS forgery with faulting RIP sign check
===============================================================

In ef_adaptor_asm_fix_ist_err_code, after the existing CR2 sign
check, add a matching check on the faulting RIP saved in the exception
frame at 0x28(%rsp).  If RIP < 0 (kernel address), kernel code caused
the fault — skip forgery and let the exception-table fixup path handle
it (e.g. get_user returning -EFAULT).  Only forge when BOTH CR2 and RIP
are positive (user addresses), meaning the elevated task's own user code
faulted on a user address.

Why:
  The existing check "CR2 > 0" distinguishes user-address faults from
  kernel-address faults but does not distinguish whether the fault was
  caused by the elevated task's own code or by kernel code running on
  that CPU (ISR, NMI handler, or a kernel function called directly by
  the elevated task).  Adding the RIP sign check provides that
  distinction: all kernel code runs at negative canonical addresses on
  x86-64, all user code runs at positive canonical addresses.

  Without this fix, a kernel-mode get_user() fault inside a perf NMI
  handler (or any in-kernel user-memory access) bypasses the exception
  table, gets treated as a user-mode page fault, attempts signal
  delivery, and hits "BUG: scheduling while atomic" because the fault
  occurred in an atomic context (perf called pagefault_disable()).

  This also makes bad-pointer handling more correct in general: a kernel
  function called directly by the elevated task that does copy_from_user
  with an invalid pointer will now correctly get -EFAULT instead of
  causing SIGSEGV delivery.

Stack layout at the check point (after pushq %rdi/%rsi/%rcx and the
call-pushed return address):
  [rsp+0x00] = %rcx (saved)
  [rsp+0x08] = %rsi (saved)
  [rsp+0x10] = %rdi (saved)
  [rsp+0x18] = return address
  [rsp+0x20] = error code        <- orl $(ERRCODE_PF_USER), 0x20(%rsp)
  [rsp+0x28] = RIP (faulting IP) <- NEW check here
  [rsp+0x30] = CS                <- movq $(USER_CS), 0x30(%rsp)
  [rsp+0x38] = RFLAGS
  [rsp+0x40] = RSP
  [rsp+0x48] = SS                <- movq $(USER_SS), 0x48(%rsp)

Note: ef_adaptor_asm_fix_ist_no_err_code performs no CS/SS forgery so
requires no change.
