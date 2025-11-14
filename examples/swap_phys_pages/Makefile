CC = gcc
CFLAGS := -O0 -ggdb -Wall -Wextra -mno-red-zone -fno-omit-frame-pointer -Wfatal-errors \
	-fPIC
CFLAGS += -I ../../Symlib/include -DDYNLINK
SRC_DIR := $(PWD)

MOD_ARGS := MODULE_SRC=$(SRC_DIR)/kmod MODULE_DEST=/root/greeter MODULE_NAME=greeter V=1 FEDORA_RELEASE=43

all: main main.readelf.txt main_static main_static.readelf.txt main.preproc.c

# kernel module build
greeter.ko: kmod/greeter.c
	mkdir -p ../../built_modules
	$(MAKE) -C ../.. build_module_full $(MOD_ARGS)
	cp ../../built_modules/greeter.ko .

greeter.o: greeter.ko
	ld -r -b binary -o greeter.o greeter.ko


# kmod load logic
load_mod.o: load_mod.c
	$(CC) $(CFLAGS) -c -o load_mod.o load_mod.c


# main program
main.o: main.c
	$(CC) $(CFLAGS) -c -o main.o main.c

main: greeter.o load_mod.o main.o libkallsyms.so
	$(CC) $(CFLAGS) -o main greeter.o load_mod.o main.o -lc -L../../Symlib/dynam_build -lSym -L. -lkallsyms 

main_static: greeter.o load_mod.o main.o libkallsyms.a
	$(CC) $(CFLAGS) -o main_static greeter.o load_mod.o main.o libkallsyms.a -lc -L../../Symlib/dynam_build -lSym 



main.readelf.txt:
	readelf -Wa main > main.readelf.txt

main_static.readelf.txt:
	readelf -Wa main_static > main_static.readelf.txt

main.preproc.c: main.c
	$(CC) $(CFLAGS) -E main.c -o main.preproc.c


libkallsyms.so:
	sudo cp /proc/libkallsyms.so .

libkallsyms.a:
	sudo cp /proc/libkallsyms.a .



clean:
	rm -f ifunc_elevate ifunc_e_early i_load_mod i_load_mod_static greeter.o greeter.ko load_mod.o ifunc_elevate.o ifunc_elevate.preproc.c libmylib.so libmylib.nodebug.so libmylib.nosymtab.so mylib.o libkallsyms.* *.readelf.txt
	$(MAKE) -C ../.. clean_module $(MOD_ARGS)

.PHONY: all clean