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

# Run kld and open its stdout as a readable FD.  Using exec+process-substitution
# rather than coproc avoids the race where bash unsets KLDPROC[] (and closes the
# FD) between the coproc line and the capture of KLDPROC[0].
exec {kld_fd}< <("$kld_bin" "$prog")
kld_pid=$!

# Read NUL-separated fields from kld stdout
IFS= read -r -d '' interp   <&"$kld_fd" || true
IFS= read -r -d '' execpath <&"$kld_fd" || true
IFS= read -r -d '' ldpath   <&"$kld_fd" || true

# Close read FD and collect kld exit status
exec {kld_fd}<&-

if ! wait "$kld_pid"; then
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
