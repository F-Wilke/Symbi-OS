obj-m += kcutpf.o
kcutpf-y := $(patsubst %.c,%.o,pfadaptor.c) $(patsubst %.S,%.o,pfasm.S) init.o
ccflags-y := -I /home/user/Symbi-OS/examples/kcutpf -I /home/user/Symbi-OS/examples/kcutidt

KBUILD_EXTRA_SYMBOLS := $(PWD)/../../kcutidt/Module.symvers

KO_FILES := kcutpf.ko 

.PHONY: all clean kcutpf.ko

all: app.h kcutpf.ko

%.kbin: %.ko
	ld -r -b binary -z noexecstack -o $@ $<

kcutpf.ko: pfadaptor.c
	make -C /home/user/Symbi-OS/linux M=/home/user/Symbi-OS/examples/kcutpf/.ext modules

app.h: app_orig.h
	/home/user/Symbi-OS/examples/tools/genappheader app.h app_got.h app_orig.h

modules.h: kcutpf.ko
	/home/user/Symbi-OS/examples/tools/genmodsheader -o modules.h $(wildcard *.ko)

clean:
	make -C /home/user/Symbi-OS/linux M=/home/user/Symbi-OS/examples/kcutpf/.ext clean
	-rm -rf $(wildcard *.o kcutpf.ko *.kbin)

