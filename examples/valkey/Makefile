.PHONY: clean distclean
TARGETS=valkey/src/valkey-server-cpcs valkey/src/valkey-server-bpcs valkey/src/valkey-server-cpbs valkey/src/valkey-server-bpbs
TARGETS+=valkey/src/valkey-server-cpcs-evac valkey/src/valkey-server-bpcs-evac valkey/src/valkey-server-cpbs-evac valkey/src/valkey-server-bpbs-evac

all: $(TARGETS)

valkey/src/valkey-server-cpcs: kcut/extension.a valkey/Makefile
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS='-DSHORTCUT=0'  V=1
	mv valkey/src/valkey-server $@

valkey/src/valkey-server-bpcs: kcut/extension.a valkey/Makefile
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS='-DSHORTCUT=0 -D BRACKET_PRIV' V=1
	mv valkey/src/valkey-server $@

valkey/src/valkey-server-cpbs: kcut/extension.a valkey/Makefile
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS='-DSHORTCUT=0 -D BRACKET_STACK' V=1
	mv valkey/src/valkey-server $@

valkey/src/valkey-server-bpbs: kcut/extension.a valkey/Makefile
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS='-DSHORTCUT=0 -D BRACKET_PRIV -D BRACKET_STACK'  V=1
	mv valkey/src/valkey-server $@



valkey/src/valkey-server-cpcs-evac: kcut/extension.a valkey/Makefile
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS='-DSHORTCUT=0 -D EVACUATE' V=1
	mv valkey/src/valkey-server $@

valkey/src/valkey-server-bpcs-evac: kcut/extension.a valkey/Makefile
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS='-DSHORTCUT=0 -D BRACKET_PRIV -D EVACUATE' V=1
	mv valkey/src/valkey-server $@

valkey/src/valkey-server-cpbs-evac: kcut/extension.a valkey/Makefile
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS='-DSHORTCUT=0 -D BRACKET_STACK -D EVACUATE' V=1
	mv valkey/src/valkey-server $@

valkey/src/valkey-server-bpbs-evac: kcut/extension.a valkey/Makefile
	make -C valkey clean
	make -j -C valkey KCUT_CFLAGS='-DSHORTCUT=0 -D BRACKET_PRIV -D BRACKET_STACK -D EVACUATE' V=1
	mv valkey/src/valkey-server $@

kcut/extension.a:
	make -C kcut

valkey/Makefile:
	git clone git@github.com:valkey-io/valkey.git
	cd valkey && git checkout -b 9.0.3 9.0.3
	cd valkey && patch -p1 < ../kcut/valkey.patch

kcut/valkey.patch:valkey/src/Makefile valkey/src/socket.c valkey/src/server.c
	cd valkey && git diff > ../kcut/valkey.patch

clean:
	make -C kcut clean
	make -C valkey clean
	-${RM} -rf $(wildcard $(TARGETS))

distclean: clean
	-${RM} -rf $(wildcard valkey)


valkey-vanilla/Makefile:
	git clone git@github.com:valkey-io/valkey.git valkey-vanilla
	cd valkey-vanilla && git checkout -b 9.0.3 9.0.3

valkey-vanilla/src/valkey-server: valkey-vanilla/Makefile
	make -j -C valkey-vanilla V=1
