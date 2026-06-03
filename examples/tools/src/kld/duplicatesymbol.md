# Duplicate symbol collisions between `libkern.so` and user binaries

## Problem summary

When `kld` generates `libkern.so` from `/proc/kallsyms`, it exports a very large symbol set. User projects can define symbols with the same names (example: Valkey defines `modules`).

At final user link, this can produce warnings or conflicts like:

- `ld: warning: type of symbol 'modules' changed from 2 to 1 ...`

In this case:

- `libkern.so` exports `modules`
- Valkey object code also defines `modules`
- Linker reports symbol type mismatch while resolving globals

## Current validated workaround (Valkey)

Valkey was rebuilt with:

- `-Dmodules=valkey_modules`

Verification:

- `nm valkey/src/module.o | grep -E ' valkey_modules$| modules$'`
- expected: `valkey_modules` only

This avoids the immediate collision without changing `kld` behavior.

---

## Approach A: Blacklist filtering in `kld` (targeted, minimal change)

### Intent

Exclude known collision-prone symbol names from `libkern.so` generation.

### `kld` changes

1. Add a blacklist predicate in `kld` symbol intake path (where `/proc/kallsyms` entries are accepted; currently through `kld_sym_init_from_kallsyms`).
2. If symbol name is blacklisted, skip it (do not add to generated `libkern.so`).
3. Optional: support CLI/file-based blacklist so projects can add names without recompiling `kld`.

### Build strategy

- Keep existing single-pass build flow.
- Generate `libkern.so` as usual, but with blacklist applied during generation.

### Pros

- Smallest change.
- Low build-time overhead.
- Easy to deploy quickly.

### Cons

- Reactive maintenance (new collisions require blacklist updates).
- Risk of blacklisting a symbol some other project actually needs.

---

## Approach B: Whitelist filtering in `kld` (generic, robust)

### Intent

Generate `libkern.so` exporting only symbols actually needed by the current user link.

### `kld` changes

1. Add input support for a required-symbol list (plain text names).
2. During `/proc/kallsyms` processing, include symbols only if present in required-symbol set.
3. Keep optional "always include" list for required runtime bridge symbols (`sym_elevate`, `symbi_fast_lower`, etc., if needed).

### Build strategy (two-pass)

1. **Compile-only pass** of user code (`.o` files), no final link yet.
2. Extract unresolved symbols from user objects/libs (`nm -u` / linker dry-run).
3. Intersect unresolved set with `/proc/kallsyms` names.
4. Invoke `kld` to build a **whitelisted** `libkern.so` from that set.
5. Final link against this filtered `libkern.so`.

### Pros

- Most generic and scalable.
- Minimizes symbol namespace pollution.
- Greatly reduces future duplicate-symbol collisions.

### Cons

- More complex build graph.
- Requires a pre-link discovery stage and orchestration changes.

---

## Recommendation

- **Near term:** keep Valkey workaround (`-Dmodules=valkey_modules`) and implement **Approach A** (blacklist) for fast relief.
- **Long term:** move to **Approach B** (whitelist + two-pass build), which is the durable solution for large user projects.

