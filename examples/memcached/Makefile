KCC    := $(realpath ../tools/kcc)
KLD    := $(realpath ../tools/src/kld/kld)

KALLSYMS_PATH ?= /proc/kallsyms
KFLAGS       := -S $(KALLSYMS_PATH) -K $(realpath ../kcutidt) -K $(realpath ../kcutef) -K $(realpath ../kcutevac)  -K $(realpath ../kcuttcp) -k kcutidt.ko -k kcutef.ko -k kcutevac.ko -k kcuttcp.ko -k /proc/kallsyms
KLDFLAGS     := -t $(PWD)/kotbl.bin
KCUT_CFLAGS  := $(CFLAGS) -D KCUT_TCP -I $(realpath ../kcuttcp) -I $(realpath ../kcuttest) -I $(realpath ../kcutevac)  -I $(realpath ../../Symlib/include) -DDYNLINK

# KCUT_CFLAGS  += -Dmodules=valkey_modules
KCUT_LDFLAGS := -lc  $(realpath ../../Symlib/build/L1/L1.a)  -L $(realpath ../kcuttcp) -L $(realpath ../kcutevac) -L$(realpath .) -lkcuttcp -l kcutevac -lkern 

.PHONY: clean distclean
TARGETS=memcached/memcached-cpcs memcached/memcached-bpcs memcached/memcached-cpbs memcached/memcached-bpbs
TARGETS+=memcached/memcached-cpcs-evac memcached/memcached-bpcs-evac memcached/memcached-cpbs-evac memcached/memcached-bpbs-evac

KLD_TARGETS=libkern.so kotbl.bin

all: $(TARGETS)

$(KLD_TARGETS):
	${KLD} ${KFLAGS}
	cp libkern.so ./libkern.valkey.so

memcached/memcached-cpcs: memcached/Makefile $(KLD_TARGETS)
	make -C memcached clean
	make -j -C memcached KCUT_CFLAGS="${KCUT_CFLAGS}" KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv memcached/memcached $@

memcached/memcached-bpcs: memcached/Makefile $(KLD_TARGETS)
	make -C memcached clean
	make -j -C memcached KCUT_CFLAGS='${KCUT_CFLAGS} -D BRACKET_PRIV' KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv memcached/memcached $@

memcached/memcached-cpbs: memcached/Makefile $(KLD_TARGETS)
	make -C memcached clean
	make -j -C memcached KCUT_CFLAGS='${KCUT_CFLAGS} -D BRACKET_STACK' KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv memcached/memcached $@

memcached/memcached-bpbs: memcached/Makefile $(KLD_TARGETS)
	make -C memcached clean
	make -j -C memcached KCUT_CFLAGS='${KCUT_CFLAGS} -D BRACKET_PRIV -D BRACKET_STACK' KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv memcached/memcached $@



memcached/memcached-cpcs-evac: memcached/Makefile $(KLD_TARGETS)
	make -C memcached clean
	make -j -C memcached KCUT_CFLAGS='${KCUT_CFLAGS} -D EVACUATE' KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv memcached/memcached $@

memcached/memcached-bpcs-evac: memcached/Makefile $(KLD_TARGETS)
	make -C memcached clean
	make -j -C memcached KCUT_CFLAGS='${KCUT_CFLAGS} -D BRACKET_PRIV -D EVACUATE' KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv memcached/memcached $@

memcached/memcached-cpbs-evac: memcached/Makefile $(KLD_TARGETS)
	make -C memcached clean
	make -j -C memcached KCUT_CFLAGS='${KCUT_CFLAGS} -D BRACKET_STACK -D EVACUATE' KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv memcached/memcached $@

memcached/memcached-bpbs-evac: memcached/Makefile $(KLD_TARGETS)
	make -C memcached clean
	make -j -C memcached KCUT_CFLAGS='${KCUT_CFLAGS} -D BRACKET_PRIV -D BRACKET_STACK -D EVACUATE' KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv memcached/memcached $@


memcached/Makefile:
	git clone git@github.com:memcached/memcached.git
	cd memcached && git checkout -b 1.6.41 1.6.41
	cd memcached && patch -p1 < ../kcut/memcached.patch
	cd memcached && ./autogen.sh
	cd memcached && ./configure

kcut/memcached.patch:memcached/Makefile.am memcached/memcached.c memcached/thread.c
	cd memcached && git diff > ../kcut/memcached.patch


clean:
	make -C memcached clean
	-${RM} -rf $(wildcard $(TARGETS) libkern.so kotbl.ko)

distclean: 
	-${RM} -rf $(wildcard memcached)

memcached-vanilla/Makefile:
	git clone git@github.com:memcached/memcached.git memcached-vanilla
	cd memcached-vanilla && git checkout -b 1.6.41 1.6.41
	cd memcached-vanilla && ./autogen.sh
	cd memcached-vanilla && ./configure

memcached-vanilla/memcached: memcached-vanilla/Makefile
	make -C memcached-vanilla memcached
