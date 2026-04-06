#!/bin/bash
[[ -n $GDB || -n $DBG ]] && set -x
MY_PATH=$(realpath $0)

SYMBIPATH=${MY_PATH%%/Symbi-OS/*}
SYMBIPATH=${SYMBIPATH}/Symbi-OS
SYMLIBPATH=${SYMBIPATH}/Symlib/dynam_build
VALKEYPATH=${SYMBIPATH}/examples/valkey
KALLSYMSPATH=${VALKEYPATH}/kcut

SYMLIB=${SYMLIBPATH}/libSym.so
KALLSYMLIB=${KALLSYMSPATH}/libkallsyms.so
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

[[ ! -a ${VALKEYCONFIG} ]] && {
    echo "ERROR: could not find $VALKEYCONFIG"
    exit -1
}

[[ ! -a ${VALKEYSERVER} ]] && {
     echo "ERROR: could not find $VALKEYSERVER"
     exit -1
 }

echo "RUNNING: sudo LD_DEBUG=files LD_LIBRARY_PATH=${KALLSYMSPATH} $VALKEYSERVER $VALKEYCONFIG $VALKEYARGS" > /dev/stderr

[[ -n $GDB ]] && GDB="$GDB --args"

sudo LD_DEBUG=files LD_LIBRARY_PATH=${KALLSYMSPATH} $GDB  $VALKEYSERVER $VALKEYCONFIG $VALKEYARGS
