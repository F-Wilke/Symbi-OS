#!/bin/bash
# To ease prototyping we using a shell script rather than
# writing this in C -- optimize by rewriting into kldd
[[ -n $KLD_DEBUG ]] && echo "[KLDSO.SH]: $@" >&2

exec /lib64/ld-linux-x86-64.so.2 $@
