# Elevated Task Address Space Access Semantics

## Background

Symbi-OS introduces the concept of an *elevated task*: a Linux user-space process that
can request to run at CPL0 (ring 0 / supervisor privilege) while continuing to operate
primarily within its own user-space address space.  This hybrid execution model allows
user code to invoke kernel functionality directly — including existing kernel code — without
a traditional syscall boundary, while still sharing data structures and stack space that
live at user-space virtual addresses.

The fundamental tension this creates is that the x86 architecture (and others) enforce
a hard separation between supervisor-mode code and user-space memory.  An elevated task
running at CPL0 must be able to read and write its own user-space pages, yet the hardware
and kernel software are both designed to prevent exactly that by default.

This document describes the prototype approach currently implemented and a more principled
alternative for future consideration.

---

## The Problem in Detail

On x86, two distinct mechanisms enforce the supervisor/user memory separation:

### Hardware enforcement: SMEP and SMAP

- **SMEP** (Supervisor Mode Execution Prevention, CR4.SMEP): prevents CPL0 code from
  *fetching instructions* from pages marked user-accessible (U/S=1 in the PTE).
- **SMAP** (Supervisor Mode Access Prevention, CR4.SMAP): prevents CPL0 code from
  *reading or writing* user-accessible pages unless the AC flag in EFLAGS is set.

Both are enabled in CR4 at boot on any modern x86 system and are normally permanent
(protected by the kernel's "pinned CR4 bits" mechanism).

### Software enforcement: fault.c SMAP check

Even with SMAP disabled in CR4, the kernel's page fault handler (`arch/x86/mm/fault.c`)
contains an additional software check at the point where a supervisor-mode fault is
being classified:

```c
if (cpu_feature_enabled(X86_FEATURE_SMAP) &&
    !(error_code & X86_PF_USER) &&
    !(regs->flags & X86_EFLAGS_AC))
    page_fault_oops(...);   // kills the process
```

This fires whenever:
1. The CPU supports SMAP (feature bit — not the current CR4 state), AND
2. The faulting access was from supervisor mode (`X86_PF_USER` not set in error code), AND
3. AC is not set in the saved EFLAGS on the exception frame.

For an elevated task, condition 2 is *always* true (CPL0 access), so this check fires
unless AC=1 is present in the saved exception frame.  Critically, the kernel's
`CLAC` instruction (used throughout `copy_to/from_user` and related paths) can clear the
live AC flag between elevation and the moment a page fault occurs, leaving AC=0 on the
exception frame even if the elevated task originally had AC=1.

---

## Prototype Approach (Current Implementation)

### Strategy

The prototype takes a *hardware-protection bypass* approach: disable the hardware
SMEP/SMAP enforcement mechanisms at the CR4 level when a task elevates, and ensure AC=1
is present in the saved EFLAGS whenever a page fault is handled on behalf of an elevated
task.

### Implementation

#### 1. SMEP/SMAP removal from CR4 pinned bits (`arch/x86/kernel/cpu/common.c`)

The kernel's `cr4_pinned_mask` normally includes `X86_CR4_SMEP` and `X86_CR4_SMAP`,
causing `cr4_clear_bits()` to silently ignore attempts to clear them.  Under
`CONFIG_SYMBIOTE`, these two bits are removed from the pinned mask:

```c
static const unsigned long cr4_pinned_mask =
    (X86_CR4_SMEP | X86_CR4_SMAP | X86_CR4_UMIP | ...)
#ifdef CONFIG_SYMBIOTE
    /* Elevated tasks run at CPL0 but access user-space addresses, so SMEP and SMAP
     * must be dynamically togglable via cr4_clear/set_bits(). Removing them from
     * the pinned set prevents cr4_clear/set_bits() from silently ignoring those changes. */
    & ~(X86_CR4_SMEP | X86_CR4_SMAP)
#endif
    ;
```

#### 2. SMEP/SMAP toggle on elevation/lower (`arch/x86/kernel/sys_x86_64.c`)

`symbi_toggle_nosmap()` and `symbi_toggle_nosmep()` call `cr4_clear_bits()` /
`cr4_set_bits()` (keeping the kernel CR4 shadow in sync) when a task elevates or lowers.
The `SymbiReg` passed to the elevation syscall carries `no_smep` and `no_smap` bits that
control whether toggling is requested.

#### 3. AC flag management on elevation/lower

When the caller requests `enable_ac` (bit 11 of `SymbiReg`):

- **On elevate** (`arch_elevate()`): the task's original AC state is saved in
  `task_struct::symbiote_orig_ac` (normalised to 0/1 with `!!`), `symbiote_enable_ac`
  is set to 1, and `regs->flags |= X86_EFLAGS_AC` forces AC=1 into the saved EFLAGS
  so the process returns from the elevation syscall with AC set.

- **On lower** (`arch_elevate()` lower path): `regs->flags` has AC restored to its
  original value from `symbiote_orig_ac`, and `symbiote_enable_ac` is cleared.

- **Fast lower** (`symbi_fast_lower_iret`, `symbi_fast_lower_sysret`): the `eflags`
  value pushed for IRET or loaded into R11 for SYSRET is synthesised as
  `X86_EFLAGS_IF | X86_EFLAGS_FIXED`, with AC conditionally OR'd in from `symbiote_orig_ac`
  before clearing `symbiote_enable_ac`.

#### 4. AC recovery in the EF adaptor (`examples/kcutef/efasm.kS`)

Because `CLAC` instructions scattered through kernel code (e.g., inside
`copy_to/from_user`) can clear AC between the elevation point and the moment a page
fault is taken, the EF adaptor (which intercepts IST exception entries before
`exc_page_fault` runs) unconditionally restores AC=1 in the saved EFLAGS frame for
any elevated task with `symbiote_enable_ac=1`:

```asm
testb $1, TASK_symbiote_enable_ac(%rsi)
jz    .Lerr_c_ac_done
orl   $X86_EFLAGS_AC, 0x38(%rsp)    /* RFLAGS slot in exception frame */
.Lerr_c_ac_done:
```

This ensures the `fault.c` software SMAP check sees AC=1 and does not kill the process.
The remainder of the page fault handler then resolves the fault normally (demand paging,
CoW, etc.) without knowing or caring that the faulting access was from an elevated task.

### Prototype Limitations

> **Critical limitation — SMEP and SMAP are disabled globally on the CPU.**
>
> `cr4_clear_bits(X86_CR4_SMEP | X86_CR4_SMAP)` modifies the CR4 register of the
> *current CPU*.  While an elevated task is running, **all** kernel code executing on
> that CPU — including code running on behalf of entirely unrelated processes — loses the
> protection of SMEP and SMAP.  A bug in any kernel code path that runs while SMEP/SMAP
> are disabled could silently corrupt or be corrupted by user-space memory without the
> hardware raising a fault.  This fundamentally undermines the security guarantees those
> features provide.

Additional limitations:

- **Architecture-specific**: relies on x86 CR4 bits (SMEP, SMAP) and the EFLAGS AC flag.
  Porting to ARM (PAN/PXN) or RISC-V requires entirely different mechanisms.

- **AC flag semantics are fragile**: `CLAC`/`STAC` are used throughout the kernel in
  contexts that assume AC=0 is safe.  Forcing AC=1 on behalf of an elevated task bypasses
  the *intent* of those instructions even if the EF adaptor successfully patches the
  exception frame.  Any kernel path that relies on AC=0 to enforce its own access
  discipline may behave incorrectly for an elevated task.

- **Per-CPU, not per-task**: SMEP/SMAP state is a CPU-wide setting.  If the elevated
  task migrates between CPUs, or if the scheduler runs other tasks on the same CPU, the
  CR4 state must be carefully managed at context-switch time (not yet fully implemented
  in the prototype).

- **No per-page granularity**: SMEP/SMAP are all-or-nothing.  There is no way with this
  approach to allow the elevated task to access only *its own* user pages while still
  protecting other user pages.

---

## Alternative Approach: Page Table Promotion

### Strategy

Rather than bypassing hardware enforcement mechanisms, this approach modifies the
*page tables* of the elevated task so that its user-space pages are marked as supervisor
pages (U/S=0 in the PTE) for the duration of elevation.  An elevated task truly becomes
a CPL0 task with supervisor-accessible memory; SMEP, SMAP, and AC require no special
handling.

### How It Works

On elevation, the kernel walks the task's page tables (or does so lazily, on demand)
and clears the User bit (bit 2) in each PTE covering the task's user address space.
From this point:

- CPL0 code can read and write those pages freely — they are supervisor pages.
- SMEP does not block instruction fetches from them (supervisor pages are exempt).
- SMAP does not block data accesses to them (SMAP only restricts access to *user* pages).
- The `fault.c` software SMAP check is irrelevant — the fault error code will not
  indicate a supervisor-to-user access because the pages are no longer user pages.

On lower, the kernel restores the User bit in the PTEs (aggressively or lazily), and
the process returns to normal CPL3 execution.

### Promotion Strategies

**Aggressive (at elevation/lower time):**
Walk the entire user virtual address space, flip U/S bits, flush the TLB.  Simple to
reason about but expensive — both the walk and the required TLB shootdowns (inter-CPU
IPIs) scale with address space size and CPU count.

**Lazy (fault-driven promotion):**
On elevation, only update task state.  The first CPL0 access to any not-yet-promoted
user page takes a page fault.  The EF adaptor (or a dedicated fault handler) promotes
the PTE (U→S), invalidates the TLB entry (`INVLPG`), and resumes.  Pages accumulate as
supervisor pages over time.  On lower, either walk promoted pages (tracked via a custom
PTE available bit or a per-task list) and demote them, or use lazy demotion (CPL3
access to a still-supervisor page faults; the handler demotes and resumes).

### Advantages Over the Prototype

- **Architecture independent**: the User/Supervisor PTE bit is a universal concept
  present in x86, ARM (AP bits), RISC-V (U bit), and virtually all MMU-bearing
  architectures.  The same semantic applies everywhere.

- **No global CPU state modification**: CR4 is untouched.  SMEP and SMAP remain fully
  active for all other processes and all other kernel code on the same CPU — the
  security guarantee is preserved.

- **Per-page granularity**: only the elevated task's own pages are promoted.  Other
  processes' pages remain user-accessible and retain full hardware protection.

- **No AC flag manipulation**: `CLAC`/`STAC` semantics are entirely undisturbed.  The
  kernel's existing access-discipline machinery works correctly for all code paths.

- **Clean semantics**: the elevated task is genuinely a supervisor-mode task with
  supervisor memory.  The hardware model and the software model agree.

### Challenges and Open Questions

- **TLB coherency cost**: each PTE change requires TLB invalidation.  For lazy promotion
  this is one `INVLPG` per newly-touched page; for aggressive promotion it requires a
  full TLB flush and cross-CPU TLB shootdown IPIs — expensive at scale.

- **Shared and CoW pages**: pages shared with other processes (mapped libraries, CoW
  pages) each have their own PTE in the elevated task's page table, so promotion only
  affects this task's mapping.  However, care is needed around in-flight CoW: promoting
  a page that is about to be copied could race with the CoW path.  Proper
  synchronisation with the MM subsystem is required.

- **Tracking promoted pages**: the lazy lower path needs to know which pages were
  promoted.  Using available PTE bits (x86 PTEs have several software-available bits)
  to mark promoted pages avoids a separate data structure at the cost of a page table
  walk on lower.  Alternatively, a per-task xarray or bitmap could track promoted
  ranges.

- **Stack and VDSO pages**: the task's stack and the VDSO are user pages that the
  elevated task uses immediately.  Under lazy promotion these will fault and be promoted
  early; under aggressive promotion they are handled in the initial walk.

- **Interaction with kernel mm paths**: `get_user_pages()`, `follow_pte()`, and similar
  paths used by DMA, `ptrace`, and other subsystems inspect the U/S bit to determine
  whether a page is user-accessible.  Temporarily making user pages appear as supervisor
  pages could confuse these paths; they would need to be aware of the elevated-task
  promotion state.

### Relationship to the Prototype

The lazy promotion approach still requires a fault-handling path for the first access
to each page — conceptually similar to what the EF adaptor already provides.  However,
instead of patching AC in the exception frame, the handler performs a concrete PTE
modification.  The EF adaptor's existing infrastructure (IST interception, current-task
detection) is a natural foundation for this.

---

## Summary Comparison

| Property | Prototype (CR4/AC) | PTE Promotion |
|---|---|---|
| SMEP/SMAP on CPL0 access | Disabled in CR4 (globally) | Not needed (pages are supervisor) |
| Impact on other processes | **SMEP/SMAP disabled system-wide on CPU** | None — CR4 untouched |
| AC flag manipulation | Required; fragile w.r.t. CLAC/STAC | Not needed |
| Architecture portability | x86-only (CR4 bits, EFLAGS.AC) | Universal (U/S PTE bit) |
| Elevation cost | O(1) — 2 CR4 writes | O(pages) aggressive; O(1) lazy |
| Per-page fault cost | None | PTE write + INVLPG (lazy only) |
| Lower cost | O(1) — 2 CR4 writes | O(promoted pages) or lazy faults |
| Granularity | All-or-nothing (entire CPU) | Per-page, per-task |
| Implementation complexity | Moderate (done) | High (MM integration needed) |
| Production suitability | No (global security regression) | Potentially yes |

The prototype is sufficient for research experimentation and performance measurement but
should not be considered a production-viable design.  The PTE promotion approach is the
recommended direction for a production implementation, with the lazy variant being the
most practical starting point given its lower elevation latency.
