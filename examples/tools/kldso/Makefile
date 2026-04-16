KLDSO_SH_PATH ?= $(realpath ./kldso.sh)

all: kld.so hello

kld.so: kld_entry.o kld.o
	ld -g -o $@ kld_entry.o kld.o

kld_entry.o: kld_entry.S
	as -g -o $@ $<

kld.o: kld.c
	gcc -g -DKLDD_SH_PATH=$(KLDSO_SH_PATH) -c -fno-builtin -fno-stack-protector -nostdlib -o $@ $<

hello: hello.c
	gcc -o $@ $<
	./kldinst hello

clean:
	-${RM} -rf $(wildcard *.o kld.so hello)
