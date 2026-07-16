#!/bin/bash
[[ -n $GDB || -n $DBG ]] && set -x
MY_PATH=$(realpath $0)

MEMCACHEDPATH=$(dirname ${MY_PATH})
MEMCACHEDSERVER=${MEMCACHEDPATH}/memcached/memcached

#check first arguments, append to MEMCACHEDSERVER if exists
[[ -n $1 ]] && MEMCACHEDSERVER="$MEMCACHEDSERVER-$1"

#define variable that holds all other arguments, except the first one
MEMCACHEDARGS=" -u root"
for arg in "$@"; do
    if [[ "$arg" != "$1" ]]; then
        MEMCACHEDARGS="$MEMCACHEDARGS $arg"
    fi
done


[[ ! -a ${MEMCACHEDSERVER} ]] && {
     echo "ERROR: could not find $MEMCACHEDSERVER"
     exit -1
 }

[[ -n $GDB ]] && GDB="$GDB --args"
[[ -n $LD_DEBUG ]] && LD_DEBUG="LD_DEBUG=${LD_DEBUG}"
[[ -n $KLD_DEBUG ]] && KLD_DEBUG="KLD_DEBUG=${KLD_DEBUG}"

echo "RUNNING:sudo $GDB $LD_DEBUG $KLD_DEBUG $MEMCACHEDSERVER $MEMCACHEDARGS" > /dev/stderr

sudo $GDB $LD_DEBUG $KLD_DEBUG $MEMCACHEDSERVER $MEMCACHEDARGS

