#!/bin/bash
#set -x
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

echo "RUNNING: LD_LIBRARY_PATH=${SYMLIBPATH}:${KALLSYMSPATH} $VALKEYSERVER $VALKEYCONFIG $@" > /dev/stderr

LD_LIBRARY_PATH=${SYMLIBPATH}:${KALLSYMSPATH} $VALKEYSERVER $VALKEYCONFIG $@

