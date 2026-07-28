#!/bin/bash

# adaptors in required build order (eg. provides symbols potential to one another)
ADAPTORS=${ADAPTORS=kcutidt kcutnmi kcutef kcutevac kcuttcp}
APPS=${APPS=kcuttest kcuttcptest valkey memcached}

for a in ${ADAPTORS}; do
    echo "ADAPTOR: building $a"
    make -C $a clean
    if ! make -C $a; then
	echo "ADAPTOR: $a failed to build."
	exit -1
    fi
done


for a in ${APPS}; do
    echo "APP: building $a"
    make -C $a clean
    if ! make -C $a; then
	echo "APP: $a failed to build."
	exit -1
    fi
done
	
