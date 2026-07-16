# kld Runtime Approaches: Tradeoffs and Option-2 Implementation Plan

## Context and goals

Current priorities (ranked):

1. **Keep current interpreter approach (`kld.so`)** as default.
2. **Explore in-process transparency** by removing `kld.so` mappings before final handoff.
3. **Consider `LD_AUDIT`** as an alternative path.

Primary goals:

- Preserve runtime correctness across supported workloads.
- Preserve debugger/tool usability (including GDB).
- Improve transparency only when it does not compromise reliability.

---

## Option 1: Keep current `kld.so` interpreter flow (default)

### Summary

`kld.so` remains PT_INTERP, performs prep + runtime map/handoff to real `ld.so`.

### Pros

- Already implemented and validated on current test targets.
- High observability and easier debugging of `kld.so` behavior.
- Works without depending on `LD_AUDIT` support.
- Preserves current development/debug workflow.

### Cons

- `kld.so` remains visible in process mappings (less transparent).
- Requires maintaining startup/handoff correctness and GDB rendezvous compatibility.

### Current status

- Functional baseline is complete.
- Keep this as primary/production path.

---

## Option 2: In-process trampoline self-unmap/remap (transparency mode)

### Summary

Use a small trampoline (outside `kld.so` mappings) to unmap `kld.so` and map final `ld.so`, then jump to `ld.so` entry. No extra exec boundary.

### Pros

- Highest transparency: final mapping can look like `kld.so` was never present.
- No re-exec discontinuity (better than exec-based restart for process continuity).

### Cons

- Highest implementation risk and complexity.
- Easy to break if any state/code/data remains in unmapped regions.
- Harder failure analysis than option 1.

---

## Option 3: `LD_AUDIT` runtime approach

### Summary

Move behavior into an audit module loaded by `ld.so` (glibc-style audit hooks).

### Pros

- Potentially cleaner process image and strong native debugger behavior on glibc.
- Less custom interpreter control-flow machinery.

### Cons

- Loader-dependent portability constraints.
- Not a universal replacement for PT_INTERP-level control.
- Policy/security modes can restrict or alter audit behavior.

---

# Detailed implementation plan for Option 2

## Design principle

Split work into two phases:

1. **In `kld.so` C phase**: compute everything, map required artifacts, prepare immutable handoff metadata.
2. **In trampoline phase**: perform minimal unsafe operations only (unmap/remap/jump) with no external dependencies.

## Required components

### 1) Trampoline image and metadata block

Create an executable scratch mapping (`mmap(PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS)` then `mprotect(PROT_READ|PROT_EXEC)`) containing:

- tiny trampoline code
- compact metadata block:
  - new startup stack pointer (`final_rsp`)
  - final `ld.so` entry point
  - list of `kld.so` PT_LOAD ranges to unmap
  - list of final mapped `ld.so` ranges (if remapping in trampoline)
  - auxv patch targets/values (at least `AT_BASE`)

No trampoline references to symbols or data in `kld.so` after jump.

### 2) Enumerate `kld.so` own mappings to remove

Derive from `kld.so` program headers and runtime base:

- collect exact page-aligned PT_LOAD intervals
- include any auxiliary `kld.so`-owned scratch mappings intended to disappear
- exclude stack, vdso, vvar, main executable mappings, and target `ld.so` mappings

### 3) Prepare final `ld.so` state before teardown

Two safe variants:

- **Preferred**: map final `ld.so` fully in `kld.so` phase, validate entry/base, then trampoline only unmaps `kld.so` and jumps.
- **Alternative**: trampoline does both unmap + map (harder; more syscall/data surface in trampoline).

### 4) Startup frame and auxv integrity

Before trampoline jump:

- ensure chosen `final_rsp` is valid/aligned
- ensure auxv points at stable PHDR tuple expected by loader
- patch `AT_BASE` to final interpreter base
- preserve any required debug/rendezvous state policy (warn/continue if unavailable)

### 5) Trampoline control flow

Strict sequence:

1. enter trampoline (running from scratch mapping)
2. unmap all `kld.so` ranges
3. (if needed) apply final map/protect steps
4. set `%rsp = final_rsp`
5. clear `%rbp`
6. `jmp` to final `ld.so` entry

No returns.

## Safety checks (must-have)

1. **Range overlap validator**  
   Verify `kld.so` unmap ranges do not include:
   - main executable PT_LOAD ranges
   - current stack pages
   - trampoline mapping itself

2. **Pre-jump invariant checks**
   - all planned addresses page-aligned where required
   - target entry within mapped executable range
   - auxv mandatory fields present (`AT_PHDR/AT_PHNUM/AT_PHENT`)

3. **Failure policy**
   - if any invariant fails, abort option 2 path and fall back to option 1 behavior.

## Incremental rollout strategy

1. **Phase A: dry-run mode**
   - build trampoline and metadata
   - log computed ranges/actions
   - do not unmap/jump

2. **Phase B: unmap-only experiment**
   - enter trampoline
   - perform candidate unmap validation only (or no-op guard)
   - return failure intentionally to prove control-path correctness (in dedicated dev branch)

3. **Phase C: full jump behind explicit flag**
   - enabled only with opt-in env/flag (e.g., `KLD_TRANSPARENT=1`)
   - keep option 1 as default

4. **Phase D: broaden test matrix**
   - PIE + non-PIE executables
   - gdb launch/attach flows
   - module-heavy workloads
   - repeated runs to catch address/layout drift

## Recommended mode architecture

- **Default mode**: Option 1 (visible, robust, debug-friendly).
- **Transparency mode**: Option 2, explicit opt-in, strict guardrails, automatic fallback to option 1 on invariant failure.

This preserves a stable baseline while enabling transparent operation where safe.

