# kld / kld.so TODO

- Add optional runtime fallback in `kld.so` for missing/unupdatable `lib*.so` artifacts by invoking `kld` helper (`fork` + `execve`) in a guarded one-shot path.
  - Keep current standalone fast path as default.
  - Guard against recursion/re-entry.
  - Re-run update phase after helper success.

- Add a fast pre-parse gate to avoid reading `/proc/kallsyms` on every run.
  - Use `boot_id` + per-module runtime anchor checks (`/sys/module/<mod>/sections/.text` or equivalent) against stamp metadata.
  - If all anchors match and stamps cover current `.so` files, skip full kallsyms parse/update.

- Add a separate libkern fast-skip path independent of module-symbol parsing.
  - If `boot_id` unchanged and `libkern.so` stamp is valid/covering, skip libkern update without parsing all of kallsyms.

- Add optional persistent parsed-symbol cache under `/run/kld`.
  - Key by `(boot_id + module-state fingerprint)` and version the cache format.
  - Reuse cache on key match; invalidate/rebuild on mismatch.
