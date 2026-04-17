O             := 2
DBGFLG        := -g
OFLG          := -O${O}
KLDSO_SH_PATH ?= $(realpath ./kldso.sh) 

TARGETS       := kld.so hello

CFLAGS        := ${OFLG} ${DBGFLG} -DKLDD_SH_PATH=$(KLDSO_SH_PATH) -fno-builtin -fno-stack-protector -nostdlib -MD
LDFLAGS       := ${DBGFLG}
ASMFLAGS      := ${DBGFLG}

.PHONEY: clean

all: ${TARGETS}

kld.so: kldso_entry.o kldso.o
	${LD} ${LDFLAGS} -o $@ $^

kldso_entry.o: kldso_entry.S
	${AS} ${ASMFLAGS} -o $@ $<

kldso.o: main.c
	${CC} ${CFLAGS} -c -o $@ $<

# A simple test: Will go through our trampoline
# $ ./hello  a b c  
# Following will provided some evidence
# $ KLD_DEBUG=all ./hello a b c 
hello: hello.c
	${CC} -o $@ $<
	./kldinst hello
	@echo -e "******** Try ************\n./hello a b c\n and\n KLD_DEBUG=all ./hello a b c\n**** To see the magic ***\n"

# here to document my experimentation with bpf module status monitoring 
modlogger.bpf.o: modlogger.bpf.c 
	clang -g -O2 -target bpf -D__TARGET_ARCH_x86 -c modlogger.bpf.c -o $@

clean:
	-${RM} -rf $(wildcard *.o *.d ${TARGETS})
