#!/bin/bash
[[ -n $GDB || -n $DBG ]] && set -x
MY_PATH=$(realpath $0)

VALKEYPATH=$(dirname ${MY_PATH})
VALKEYCONFIG=${VALKEYPATH}/valkey.conf
VALKEYSERVER=${VALKEYPATH}/valkey/src/valkey-server

#check first arguments, append to VALKEYSERVER if exists
[[ -n $1 ]] && VALKEYSERVER="$VALKEYSERVER-$1"

#define variable that holds all other arguments, except the first one
VALKEYARGS=""
for arg in "$@"; do
    if [[ "$arg" != "$1" ]]; then
        VALKEYARGS="$VALKEYARGS $arg"
    fi
done

[[ ! -a ${VALKEYCONFIG} ]] && {
    echo "ERROR: could not find $VALKEYCONFIG"
    exit -1
}

[[ ! -a ${VALKEYSERVER} ]] && {
     echo "ERROR: could not find $VALKEYSERVER"
     exit -1
 }

[[ -n $GDB ]] && GDB="$GDB --args"
[[ -n $LD_DEBUG ]] && LD_DEBUG="LD_DEBUG=${LD_DEBUG}"
[[ -n $KLD_DEBUG ]] && KLD_DEBUG="KLD_DEBUG=${KLD_DEBUG}"

echo "RUNNING: sudo $GDB $LD_DEBUG $KLD_DEBUG $VALKEYSERVER $VALKEYCONFIG $VALKEYARGS" > /dev/stderr

sudo $GDB $LD_DEBUG $KLD_DEBUG $VALKEYSERVER $VALKEYCONFIG $VALKEYARGS
