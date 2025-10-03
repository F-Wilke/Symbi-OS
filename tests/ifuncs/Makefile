CC = gcc
CFLAGS = -O0 -ggdb -Wall -Wextra -mno-red-zone -fomit-frame-pointer -Wfatal-errors 
CFLAGS += -I ../../Symlib/include -DDYNLINK

all: $(TARGET)


greeter.ko:
	mkdir -p ../../built_modules
	$(MAKE) -C ../.. build_module_full MODULE_SRC=./linux-kernel-module/greeter MODULE_DEST=/root/greeter MODULE_NAME=greeter V=1
	cp ../../built_modules/greeter.ko .

greeter.o: greeter.ko
	ld -r -b binary -o greeter.o greeter.ko

load_mod.o: load_mod.c
	$(CC) $(CFLAGS) -c -o load_mod.o load_mod.c

ifunc_elevate.o: ifunc_elevate.c
	$(CC) $(CFLAGS) -c -o ifunc_elevate.o ifunc_elevate.c

ifunc_elevate.preproc.c: ifunc_elevate.c
	$(CC) $(CFLAGS) -E ifunc_elevate.c -o ifunc_elevate.preproc.c

i_load_mod: greeter.o load_mod.o ifunc_elevate.o
	$(CC) $(CFLAGS) -o i_load_mod greeter.o load_mod.o ifunc_elevate.o -L../../Symlib/dynam_build -lSym -L/proc/ -lkallsyms 


ifunc_e_early: CFLAGS +=  -L../../Symlib/dynam_build -lSym -I ../../Symlib/include
ifunc_e_early: ifunc_elevate.c
	$(CC) $(CFLAGS) -DE_EARLY -o ifunc_e_early ifunc_elevate.c
	

ifunc_elevate: CFLAGS +=  -L../../Symlib/dynam_build -lSym -I ../../Symlib/include
ifunc_elevate: ifunc_elevate.c
	$(CC) $(CFLAGS) -o ifunc_elevate ifunc_elevate.c


clean:
	rm -f ifunc_elevate ifunc_e_early i_load_mod greeter.o greeter.ko load_mod.o ifunc_elevate.o ifunc_elevate.preproc.c
	$(MAKE) -C ../.. clean_module MODULE_SRC=./linux-kernel-module/greeter MODULE_DEST=/root/greeter MODULE_NAME=greeter

.PHONY: all clean run