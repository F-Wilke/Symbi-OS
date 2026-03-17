.PHONY: clean distclean

memcached/memcached: kcut/extension.a memcached/Makefile
	make -C memcached memcached

kcut/extension.a:
	make -C kcut

memcached/Makefile:
	git clone git@github.com:memcached/memcached.git
	cd memcached && git checkout -b 1.6.41 1.6.41
	cd memcached && patch -p1 < ../kcut/memcached.patch
	cd memcached && ./autogen.sh
	cd memcached && ./configure

kcut/memcached.patch:memcached/Makefile.am memcached/memcached.c
	cd memcached && git diff > ../kcut/memcached.patch

clean:
	make -C kcut clean
	make -C memcached clean

distclean: clean
	-${RM} -rf $(wildcard memcached)
