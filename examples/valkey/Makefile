KCC    := $(realpath ../tools/kcc)
KLD    := $(realpath ../tools/src/kld/kld)

KFLAGS       := -K $(realpath ../kcutidt) -K $(realpath ../kcutef) -K $(realpath ../kcutevac)  -K $(realpath ../kcuttcp) -k kcutidt.ko -k kcutef.ko -k kcutevac.ko -k kcuttcp.ko -k /proc/kallsyms
KLDFLAGS     := -t $(PWD)/kotbl.bin
KCUT_CFLAGS  := $(CFLAGS) -D KCUT_TCP -I $(realpath ../kcuttcp) -I $(realpath ../kcuttest) -I $(realpath ../kcutevac)  -I $(realpath ../../Symlib/include) -DDYNLINK
# kludge to avoid symbol conflict with libkern -- until we add support to kld to filter conflicting symbols (eg black or white list apparoch)

KCUT_CFLAGS  += -Dmodules=valkey_modules
KCUT_LDFLAGS := -lc  $(realpath ../../Symlib/build/L1/L1.a)  -L $(realpath ../kcuttcp) -L $(realpath ../kcutevac) -L$(realpath .) -lkcuttcp -l kcutevac -lkern 

.PHONY: clean distclean
TARGETS=valkey/src/valkey-server-cpcs valkey/src/valkey-server-bpcs valkey/src/valkey-server-cpbs valkey/src/valkey-server-bpbs

all: $(TARGETS)

libkern.so kotbl.bin:
	${KLD} ${KFLAGS}
	cp libkern.so ./libkern.valkey.so

valkey/src/valkey-server-cpcs: valkey/Makefile libkern.so kotbl.bin
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS="${KCUT_CFLAGS}" KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv valkey/src/valkey-server $@

valkey/src/valkey-server-bpcs: valkey/Makefile valkey/Makefile libkern.so kotbl.bin
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS="${KCUT_CFLAGS} -D BRACKET_PRIV" KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv valkey/src/valkey-server $@

valkey/src/valkey-server-cpbs: valkey/Makefile valkey/Makefile libkern.so kotbl.bin
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS="${KCUT_CFLAGS} -D BRACKET_STACK" KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv valkey/src/valkey-server $@

valkey/src/valkey-server-bpbs: valkey/Makefile valkey/Makefile libkern.so kotbl.bin
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS="${KCUT_CFLAGS} -D BRACKET_PRIV -D BRACKET_STACK" KCUT_LDFLAGS="${KCUT_LDFLAGS}" KLD="${KLD}" KLDFLAGS="${KLDFLAGS}" V=1
	mv valkey/src/valkey-server $@

valkey/Makefile:
	git clone git@github.com:valkey-io/valkey.git
	cd valkey && git checkout -b 9.0.3 9.0.3
	cd valkey && patch -p1 < ../kcut/valkey.patch

kcut/valkey.patch:valkey/src/Makefile valkey/src/socket.c valkey/src/server.c
	cd valkey && git diff > ../kcut/valkey.patch

clean:
	make -C valkey clean
	-${RM} -rf $(wildcard $(TARGETS) libkern.so kotbl.ko)

distclean: clean
	-${RM} -rf $(wildcard valkey)


valkey-vanilla/Makefile:
	git clone git@github.com:valkey-io/valkey.git valkey-vanilla
	cd valkey-vanilla && git checkout -b 9.0.3 9.0.3

valkey-vanilla/src/valkey-server: valkey-vanilla/Makefile
	make -j -C valkey-vanilla V=1
