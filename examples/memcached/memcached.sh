#!/bin/bash
#set -x
MY_PATH=$(realpath $0)

SYMBIPATH=${MY_PATH%%/Symbi-OS/*}
SYMBIPATH=${SYMBIPATH}/Symbi-OS
SYMLIBPATH=${SYMBIPATH}/Symlib/dynam_build
MEMCACHEDPATH=${SYMBIPATH}/examples/memcached
KALLSYMSPATH=${MEMCACHEDPATH}/kcut

SYMLIB=${SYMLIBPATH}/libSym.so
KALLSYMLIB=${KALLSYMSPATH}/libkallsyms.so
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


[[ ! -d $SYMBIPATH ]] && {
    echo "ERROR: you are not in a subdir of Symbi-OS"
    exit -1
}

[[ ! -a $SYMLIB ]] && {
    echo "ERROR: could not find $SYMLIB -- try make in $SYMLIBPATH"
    exit -1
}

[[ ! -a ${KALLSYMLIB} ]] && {
    if [[ -a /proc/libkallsyms.so ]]; then
	sudo cp /proc/libkallsyms.so ${KALLSYMLIB}
    else
	echo "ERROR: could find $KALLSYMLIB or /proc/libkallsyms.so"
	exit -1
    fi
}


[[ ! -a ${MEMCACHEDSERVER} ]] && {
     echo "ERROR: could not find $MEMCACHEDSERVER"
     exit -1
 }

echo "RUNNING: LD_DEBUG=files LD_LIBRARY_PATH=${SYMLIBPATH}:${KALLSYMSPATH} $MEMCACHEDSERVER $MEMCACHEDARGS" > /dev/stderr

LD_DEBUG=files LD_LIBRARY_PATH=${SYMLIBPATH}:${KALLSYMSPATH} $MEMCACHEDSERVER $MEMCACHEDARGS

