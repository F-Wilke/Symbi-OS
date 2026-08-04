# Elevated context switching semantic and mechanisms

Elevated tasks that stay executing on a user stack (Constant Privilege and Constant 
user Stack (CPCS)) breaks invariants in the context switch logic.

The invariant:

```
The scheduling path assumes 1) RSP is a value on a unique kernel stack that is mapped into all address spaces and 2) that where to resume execution is maintained in an IRETQ frame at the top of this stack.  1 is critical since even when the page table is switched the code continues to use the unique kernel stack to seamlessly continue execution of the scheduling logic. 2 is critical since the resumption code (`__switch_to_asm`) expects to execute an IRETQ to resume the task.
```

There are two fundamental cases:
1. Voluntary (`schedule()`, `cond_resched()` called for any reason eg. elevate
   task call path results in voluntary blocking/yeilding).
2. Preemption (eg. timer interrupt fires and leads to `schedule()`, `cond_resched()`.


## Voluntary

Context switching sequence:

1. at thread of an elevated application calls into a kernel routine
   on a standard user stack backed by pagable memory and leads
   to a 
```
__schedule()
        context_switch()
              switch_mm_irqs_off(prev->active_mm, next->mm, next)
```
2. `switch_mm_irqs_off` job is to change the hardware page table base pointer 
    register (PTBR) `cr3`.  after updating the `cr3` this code path goes
	on to continue to use the stack it is running on eg. calls other functions.
	When executing on a user stack this is a problem as we have now switch
	to a diffent page table and can be attempting to write to an arbitrary 
	location in that address space.
3.  another systemic problem in the voluntary case, is that `__switch_to_asm` in `entry_64.S` will
    save the current rsp (which assumably is expected to be a kernel 
	stack address of the kernel stack associated with the currently running
	task an whose current state hold the frames leading to the context switch) to
	the current tasks saved context.
	
	

## Preemption (Involuntary)

1. On quantum expiration (eg. timer interrupt) when running a normal
   process (CPL==3) the interrupt will induce a privledge switch (CPL3->CPL0)
   as such the preemption logic will run on a kernel stack.  Thus ensuring 
   the expected invariant.
2. In the case of an elevated process (CPL0) that uses a user stack for its 
   execution both user and kernel code we have two cases to consider.  
     a. user code execution: our exception frame adapator will convert
      the interrupt with no priviledge switch (CPL0->CPL0) into a
      conforming state with respect to the invariant.  Specfically,
      using an exception frame buffer it will detect the use of user
      stack in CPL0 and transfer the frame to the task's SP0 (kernel
      stack) and continue the orginal interrupt logic on that kernel
      stack.  As such the invariant is maintained and everything
      appears as it normally would. See below for red-zone motivation.
     b. kernel code execution: In this scenario we normally would have
      been on a kernel stack and an interrupt at CPL0 would have
      pushed the exception frame to the current top the executing
      kernel stack.  However, our ef adaptor's model of elevated
      execution will detect a user stack and CPL0 execution, and like
      case a (since it does not special case the PC being in the
      kernel) -- will run the interrupt on the top of the task.sp0
      (fresh kernel stack execution).  This should cause the state of
      the elevated task's kernel execution to be saved into the task
      struct and when resumed execution kernel execution will restart
      as expected for the elevated task (register context and stack
      restored correctly).  The user RSP (and thus the entire kernel
      call chain on the user stack) is preserved via the IRETQ frame
      placed on SP0 and is restored automatically by IRETQ on
      reschedule.  While this is different from the standard behaviour
      of execution kernel code with a kernel stack and thus running
      preemption logic on that stack.  Our approach still runs the
      premeption logic on a kernel stack and thus the invariant is
      preserved and correct behaviour should be maintained
   
**The big caveat is that we have been configuring the kernel with PREEMPT_NONE. This avoids premption of any kernel-mode code, so we have not ever excercised and tested this path.  Note: CONFIG_PREEMPT_DYNAMIC=y means that this can be changed at runtime -- so PREEMPT_NONE does not on its own make us safe to preemption.**
	
## General Scheduling notes

- on a core `__schedule` turns off interrupts to make context switching idempotent
  however, remember nmi's can still happen.
  
  
## Critical Invariant details

The fundamental invariant for task resumption:

   `thread.sp` (saved by __switch_to_asm)
     - points into the task's SP0 kernel stack
     - which, after unwinding scheduler frames, has an IRETQ frame below the schedule call frames
     - IRETQ restores: RIP (where to resume) + RSP (which stack) + CS/RFLAGS/SS

  This is why preemption works for elevated CPCS tasks — the EF
  adaptor's job is precisely to manufacture that IRETQ frame on SP0
  for every interrupt, even CPL0→CPL0 ones where the hardware wouldn't
  normally do it. The EF adaptor is the bridge that keeps the
  IRETQ-on-SP0 invariant intact.

  And this is why voluntary yield is broken — there's no interrupt, no
  EF adaptor intercept, no IRETQ frame. The elevated task calls
  schedule() directly on the user stack. __switch_to_asm saves the
  user RSP as thread.sp instead of an SP0 kernel RSP, and there's no
  IRETQ frame anywhere to resume from.



## Exception Frame Adaptor

Uses the concept of an exception frame buffer and exception frame
contexts on that buffer to ensure all interrupts and exceptions do not
accidentally spill exception frame state to a stack that might have
user code running on it as such code might use red-zone optimizations
that can leave live state on the stack below the address pointed to by
the stack pointer register.

This code has a unique relationship with the NMI logic as NMI's can
occur while the exception frame logic is running thus it has to take
care to be restartable and support recursion.


## Elevated blocking

More generally, by supporting voluntary context switching/scheduling 
we make it possible for an elevated task to call blocking functions
as they will restart correctly when they are unblocked -- scheduling 
will be triggered.

Note to provide finer grain control of elevated impacts and costs
we could make these features independently kernel compile configurations.

Eg. if you don't need to call block or support volountary scheduling you 
don't have to turn the extra code on in the scheduling paths.


## Elevated Voluntary Scheduling support

`arch/x86/kernel/symbi_sched.S`

`symbi_voluntary_cs_asm(sched_mode, resume_rip, resume_rsp, rflags)`:
1. stash rsi/rdx/rcx into r12/r13/r14 (callee-saved, survive call schedule)
2. movq PER_CPU_VAR(cpu_current_top_of_stack), %rsp   ← switch to kernel SP0
3. push 5-word IRETQ frame: [SS, resume_rsp, rflags, __KERNEL_CS, resume_rip]
4. call schedule : runs full scheduler from kernel stack; CPCS check is false (kernel RSP is negative)
5. either use `iretq` or 6 instruction sequence and `ret` to  restores RIP=symbi_resume, RSP=__schedule()'s frame 
   (we have decided to use 6 + `ret` and not `iretq`  to better exploit the advantages of being elevated and not
   needing to disturb the processor architecural  state)

`kernel/sched/core.c`: 
Two additions:
1. Before `__schedule()`: `symbi_elevated_cs_trampoline()` — noinline, forced frame-pointer, computes RBP+16 = __schedule()'s RSP, captures RFLAGS.
2. Top of `__schedule()`:
```
   if (unlikely(current->symbiote_elevated &&
                (long)current_stack_pointer >= 0)) {
       symbi_elevated_cs_trampoline(sched_mode, &&symbi_resume);
       __builtin_unreachable();
   }
   if (0) { symbi_resume: return; }
```

`arch/x86/kernel/Makefile`: add building of new symbi sched.o
```
   obj-$(CONFIG_SYMBIOTE) += symbi_sched.o
```
  
 ### Discussion 

The whole mechanism is: 
1. one check, 
2. one 5-word frame, and
3. a 6 instruction seqeuence to allow `ret` to continue execution. 

No duplicate state saving. The user stack's intact C return chain does the rest naturally.

The precise mental model -- What happens to the original call chain:

```
[elevated task]
       - cond_resched() / schedule() / mutex_lock() / ...
           - __schedule_loop() or preempt_schedule_common()
               - __schedule()   --  WE INTERCEPT HERE
                   - [trampoline switches to kernel SP0 stack]
                       - schedule()     -- NEW call from kernel stack
                           - __schedule()  -- RUNS NORMALLY (kernel RSP, check=false)
                               - context_switch() / switch_mm_irqs_off() etc. 
```

When rescheduled, the unwind:

```
iretq - symbi_resume: return; in __schedule()
       -  returns to __schedule_loop() or preempt_schedule_common()
           - need_resched() == false → loop exits
               - returns to cond_resched() / schedule() / mutex_lock() / ...
                   - returns to [elevated task on user stack]  
```

The original `__schedule()` call never completes — it is replaced by the new `schedule()` call on the 
kernel stack. The `ret` makes it appear to the original caller that `__schedule()` returned normally, 
so every caller's post-`__schedule()` logic (preemption re-enable, `need_resched()` re-check, 
lock acquisition, etc.) runs correctly.

The elevated task ultimately sees exactly what it expects: its blocking/yielding call returned.

**The invariant that makes this safe: the check must fire before `__schedule()` has established any state that its caller depends on having been set up. That's why the placement at the very top is critical — not just "before context_switch()" but before the very first real statement.**

If the check were even one line later (after `local_irq_disable()`,
say), the original invocation would have disabled IRQs and the
ret/iretq-return would leave them disabled — broken. The "zero state
established" property is what makes the whole thing clean. The
scheduling infrastructure is designed so that all serialization is
internal to `__schedule()` — the runqueue lock, IRQ disabling, task
state reads with proper barriers, etc. are all acquired and released
within `__schedule()` itself.

The callers (`__schedule_loop`, `preempt_schedule_common`,
`cond_resched`) hold no scheduling state when they call
`__schedule()`. They're just making a request. The atomicity and
mutual exclusion are entirely owned inside `__schedule()`.

So the reason the new `schedule()` call cannot conflict:

1. The original __schedule() invocation never entered its critical section — it holds no runqueue lock, no IRQ-disabled region, nothing
2. The new `schedule()`: ` __schedule()` invocation acquires those locks from scratch, operates on the runqueue, and releases them — exactly as a normal `schedule()` call
  would
3. The two invocations don't actually run concurrently — the original is frozen on the user stack while the new one runs on the kernel stack

**The deeper point: this approach works because of how `__schedule()` was designed, not despite it. If callers held partial scheduling state across the call (like a lock grabbed before entering `__schedule()`), our intercept would be unsafe. The fact that `__schedule()` is a clean transaction — acquire everything, do the work, release everything — is what makes the "intercept at the top, start a fresh transaction" approach sound.**

