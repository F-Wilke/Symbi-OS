.PHONY: clean distclean

valkey/valkey-server: kcut/extension.a valkey/Makefile
	make -j -C valkey V=1

kcut/extension.a:
	make -C kcut

valkey/Makefile:
	git clone git@github.com:valkey-io/valkey.git
	cd valkey && git checkout -b 9.0.3 9.0.3
	cd valkey && patch -p1 < ../kcut/valkey.patch

kcut/valkey.patch:valkey/src/Makefile valkey/src/socket.c
	cd valkey && git diff > ../kcut/valkey.patch

clean:
	make -C kcut clean
	make -C valkey clean

distclean: clean
	-${RM} -rf $(wildcard valkey)


valkey-vanilla/Makefile:
	git clone git@github.com:valkey-io/valkey.git valkey-vanilla
	cd valkey-vanilla && git checkout -b 9.0.3 9.0.3

valkey-vanilla/valkey-server: valkey-vanilla/Makefile
	make -C valkey-vanilla V=1
