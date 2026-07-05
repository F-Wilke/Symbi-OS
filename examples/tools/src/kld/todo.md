# kld / kld.so TODO

- Add optional runtime fallback in `kld.so` for missing/unupdatable `lib*.so` artifacts by invoking `kld` helper (`fork` + `execve`) in a guarded one-shot path.
  - Keep current standalone fast path as default.
  - Guard against recursion/re-entry.
  - Re-run update phase after helper success.
