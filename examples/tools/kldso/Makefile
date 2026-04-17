KLDSO_SH_PATH ?= $(realpath ./kldso.sh)
all: kld.so hello 

kld.so: kld_entry.o kldso.o
	ld -g -o $@ kld_entry.o kldso.o

kld_entry.o: kld_entry.S
	as -g -o $@ $<

kldso.o: kldso.c
	gcc -g -DKLDD_SH_PATH=$(KLDSO_SH_PATH) -c -fno-builtin -fno-stack-protector -nostdlib -o $@ $<

hello: hello.c
	gcc -o $@ $<
	./kldinst hello

# here to document my experimentation with bpf module status monitoring 
modlogger.bpf.o: modlogger.bpf.c 
	clang -g -O2 -target bpf -D__TARGET_ARCH_x86 -c modlogger.bpf.c -o $@

clean:
	-${RM} -rf $(wildcard *.o kld.so hello)
