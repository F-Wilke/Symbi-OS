# Back-linkage: Kernel PLT/GOT Design

**Status:** Future consideration — not yet implemented.  
**Supersedes:** The `__user_fn__` / `__user_data__` / `app.h` approach currently in use.

---

## Problem

Kernel extension modules (`.ko`) need to call functions and access data owned
by the user-space application that loaded them.  The kernel's module linker
resolves `SHN_UNDEF` symbols by walking `__ksymtab` — a kernel-only table.
User-space addresses are structurally absent from this table and cannot be
injected into the normal resolution path.

---

## Current approach (implemented — `__user_sym__` GOT slots)

`app.kh` annotates user symbols with custom macros:

```c
__user_fn__   int user_add(int a, int b);
__user_data__ int user_counter;
```

`genappheader` (Python) and `kcc` (bash) each independently parse `app.kh` and
generate:

- `app.h` — `extern void *__user_sym__X` slots + typed inline wrappers with
  `void *` casts; included by every `.kc` file that references a user symbol
- `app_init.h` — slot definitions (`void *__user_sym__X = NULL`); included
  exactly once by the generated `init.c`
- Resolver function `__kld_resolve_app_syms_<name>(void **addrs, int count)` in
  `init.c` — positional assignment: `if (count > i) __user_sym__X = addrs[i]`
- `applink.c` — user-space `.init_array` constructor; resolves user symbols via
  `dlsym(RTLD_DEFAULT, name)` then calls the resolver under `sym_elevate()`

**Call path at runtime:**
```
kcuttest.ko:  mov rax, [rip + __user_sym__user_add]   ; load void* from BSS
              call __x86_indirect_thunk_rax             ; Spectre indirect call
```

### Known brittleness

| Problem | Impact |
|---|---|
| Two independent parsers (Python + bash) must agree on symbol **order** | Positional index mismatch silently writes the wrong address to the wrong slot |
| `app.h` wrappers embed type casts derived by regex | A mis-parsed signature compiles silently with wrong types |
| `app_init.h` single-include contract | Easy to violate as the project grows |
| `__user_fn__` / `__user_data__` custom macros | Non-standard syntax; regex can miss complex signatures |

---

## Proposed approach: kernel PLT/GOT merged into the extension module

### Core idea

Mirror user-space dynamic linking exactly, in kernel address space.  Generate
PLT stubs and GOT slots directly inside the extension module (e.g.
`kcuttest.ko`) — no separate module required.  The kernel module linker patches
intra-module call sites to the PLT stubs at load time.  The `applink.o`
constructor fills the GOT slots with `dlsym`-resolved user addresses using the
kernel's own `kallsyms_lookup_name()` once elevated.

### Developer experience

`app.kh` becomes a plain C declaration file — **no macro decorations**:

```c
/* app.kh — standard C forward declarations only */
int user_add(int a, int b);
int user_counter;
```

The tool infers intent from declaration syntax:
- Has parameter list → function → generate PLT stub + GOT slot
- No parameter list → data variable → generate GOT slot + accessor

Extension module source (`.kc`) uses a normal `extern` declaration — no
generated `app.h` required for function symbols:

```c
/* greeter.kc */
extern int user_add(int a, int b);

int kernel_user_add(int a, int b) { return user_add(a, b); }
EXPORT_SYMBOL(kernel_user_add);
```

### Generated code (`app_plt.c` — compiled into the extension module)

For each **function** symbol:

```c
/* GOT slot — .data section, writable, filled by applink constructor */
void *user_add_ptr = NULL;

/* PLT stub — .text section, never modified after load */
__naked int user_add(int a, int b)
{
    asm volatile ("jmp *user_add_ptr(%rip)\n");
}
```

For each **data** symbol (pointer slot + thin accessor):

```c
void *user_counter_ptr = NULL;
```

A generated `app_accessors.h` (included by `.kc` files that reference data
symbols) provides typed access:

```c
/* app_accessors.h — generated, do not edit */
#define user_counter  (*(int *)user_counter_ptr)
```

### `applink.o` constructor (updated)

All `dlsym` lookups happen at ring 3 before elevation.  Once elevated,
`kallsyms_lookup_name()` resolves GOT slot addresses directly inside the
kernel — no `/proc/kallsyms` file parsing needed.

```c
/* Phase 1 — ring 3 */
void *user_add_addr     = dlsym(RTLD_DEFAULT, "user_add");
void *user_counter_addr = dlsym(RTLD_DEFAULT, "user_counter");

/* Phase 2 — ring 0 */
sym_elevate();

void **slot;
slot  = (void **)kallsyms_lookup_name("user_add_ptr");
*slot = user_add_addr;

slot  = (void **)kallsyms_lookup_name("user_counter_ptr");
*slot = user_counter_addr;

symbi_fast_lower();
```

No positional index.  No resolver function in the module.  No inter-tool
ordering contract.  Each symbol is matched by name at both ring 3 (`dlsym`)
and ring 0 (`kallsyms_lookup_name`).

### Runtime call path

```
kcuttest.ko:  call user_add             ; direct intra-module PC-relative call
              -- user_add PLT stub --
              jmp [rip + user_add_ptr]  ; indirect branch through GOT slot
              -- user space --
              user_add(a, b)            ; application function
```

Same instruction count as the current approach.  The difference is at the
source level: the call site in `.kc` is a normal function call with full
compiler type checking against the `extern` declaration.

### Why text-patching the PLT stub is not done

`CONFIG_STRICT_KERNEL_RWX` maps `.text` read-only after load.  A `movabs rax,
<user_addr>; jmp rax` stub written directly into `.text` would fault even from
ring 0.  The only kernel text-patching mechanism (`text_poke`) is not exported
to modules.  The GOT slot in `.data` is writable — this is the same reason
user-space PLT/GOT separates the stub (`.text`) from the pointer (`.data`).

### Module load ordering

No constraint.  The PLT stubs and GOT slots are part of `kcuttest.ko` itself.
The call from `kernel_user_add` → `user_add` is an intra-module reference,
patched by the kernel linker at load time to a PC-relative offset within the
same module.  No external module dependency is introduced.

### Module unloading

Identical to current behavior.  `KLDOPT_RELOAD` in kld unloads and reloads
stale modules on the next application launch.  Crash cleanup remains the same
unresolved problem as today — no regression.

---

## Comparison

| Property | Current (`__user_sym__` slots) | Proposed (PLT/GOT in module) |
|---|---|---|
| `app.kh` syntax | Custom macros `__user_fn__` / `__user_data__` | Plain C declarations |
| Extension `.kc` syntax | `#include "app.h"`, custom wrappers | Normal `extern` decl |
| Type checking at call sites | `void *` cast (generated, unverified) | Full C type checking |
| Symbol resolution method | Positional index (order-sensitive) | Name-matched by kernel linker + `kallsyms_lookup_name` |
| Independent parsers | Two (Python + bash, must agree on order) | One (single source drives generated file) |
| Generated files | `app.h`, `app_init.h`, `applink.c`, resolver in `init.c` | `app_plt.c`, `app_accessors.h` (data only), `applink.c` |
| GOT slot location | Module BSS (`void *__user_sym__X`) | Module `.data` (`void *X_ptr`) |
| Slot address discovery | Positional array passed to resolver | `kallsyms_lookup_name()` from ring 0 |
| `/proc/kallsyms` parsing | Required (by kld at build+runtime) | Not required for back-linkage |
| Separate back-linkage module | No | No (merged into extension module) |
| Load ordering constraint | None | None |
| Indirection at call | load BSS ptr + indirect call | direct call + indirect jmp via GOT |
| Conceptual model | Custom | Standard PLT/GOT |

---

## Implementation outline

1. **`app.kh` parser** (single, replaces both genappheader and kcc parsers):
   parse standard C declarations; classify as function or data by presence of
   parameter list; extract name and full type signature.

2. **`app_plt.c` generator** (new kcc phase):
   emit one `void *X_ptr = NULL` GOT slot and one `__naked` PLT stub per
   function symbol; emit one `void *X_ptr = NULL` per data symbol; add
   `app_plt.c` to `obj-m` in the `.ext/Makefile`.

3. **`app_accessors.h` generator** (replaces `app.h` for data symbols):
   emit one `#define X (*(type *)X_ptr)` per data symbol; included only by
   `.kc` files that reference data symbols.

4. **`applink.c` generator** (simplified):
   emit `dlsym` calls for each symbol name; under `sym_elevate()`, emit
   `kallsyms_lookup_name("X_ptr")` + dereference-assign for each slot.
   No resolver function.  No positional array.

5. **`app.h` / `app_init.h` / resolver function** — eliminated entirely.

6. **`app.kh`** — remove `__user_fn__` and `__user_data__` macros; plain
   C declarations only.

---

## Decision pending

Deferred in favour of first cleaning up brittleness in the current
implementation (positional index, dual parsers, `void *` cast warnings).
Revisit once the current approach is stable and the kcuttest runtime test
passes.
