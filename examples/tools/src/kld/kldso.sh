#!/bin/bash
# To ease prototyping we using a shell script rather than
# writing this in C -- optimize by rewriting into kldd

set -euo pipefail


[[ ${KLD_DEBUG:-} == bash ]] && set -x

[[ -n ${KLD_DEBUG:-} ]] && echo "[KLDSO.SH]: $@" >&2

prog="$1"
shift

script_dir=$(dirname "$(realpath "$0")")
kld_bin="${script_dir}/kld"

# Start kld as a coprocess so that we can reliably get its
# exist status along with its output
coproc KLDPROC { "$kld_bin" "$prog"; }

# Read NUL-separated fields from coprocess stdout
IFS= read -r -d '' interp   <&"${KLDPROC[0]}" || true
IFS= read -r -d '' execpath <&"${KLDPROC[0]}" || true
IFS= read -r -d '' ldpath   <&"${KLDPROC[0]}" || true

# Close read FD and wait for producer exit status
exec {KLDPROC[0]}<&-

if ! wait "${KLDPROC_PID}"; then
    kld_rc=$?
    echo "ERROR: kld ($kld_bin) failed rc=$kld_rc" >&2
    exit "$kld_rc"
fi


[[ -n ${KLD_DEBUG:-} ]] && {
    echo "[KLDSO.SH]: interp:$interp execpath:$execpath ldpath:$ldpath" >&2
}

if [[ -z $interp || -z $execpath ]]; then
    echo "ERROR: malformed output from kld" >&2
    exit 1
fi

[[ ! -x $interp ]] && {
    echo "ERROR: bad interpreter: $interp" >&2
    exit 1
}

if [[ -n $ldpath ]]; then
  if [[ -n ${LD_LIBRARY_PATH:-} ]]; then
    export LD_LIBRARY_PATH="${ldpath}:${LD_LIBRARY_PATH}"
  else
    export LD_LIBRARY_PATH="${ldpath}"
  fi
fi


[[ -n ${KLD_DEBUG:-} ]] && {
    echo "[KLDSO.SH]: LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >&2
    echo "[KLDSO.SH]: exec $interp $execpath $*" >&2
}
exec "$interp" "$execpath" "$@"
