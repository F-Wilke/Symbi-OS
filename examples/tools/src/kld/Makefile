SRCS       := main.c kldelf.c modinfo.c loadbo.c runtime_kotbl.c module_patch.c
OBJS       := $(SRCS:%.c=%.o)
O          := 0
# Find .c files not overridden locally from the parent directory
vpath %.c ..
vpath %.h ..
CFLAGS     += -g -O${O} -std=gnu99 -MD -MP -Wall -Werror
CFLAGS     += -I. -I..
KLDSO_PATH ?= $(realpath .)/kld.so
KLDSO_CFLAGS := -O2 -g -fno-builtin -fno-stack-protector -nostdlib -MD
TARGETS    := kld kldd kld.so readsymtbl listlibs stubstest kerntest
UTHASHREPO := git@github.com:troydhanson/uthash.git
UTHASHDIR  := ext/uthash
UTHASHINCS  = ${UTHASHDIR}/include

CFLAGS     += -I${UTHASHINCS} -DKLDSO_PATH_DFLT=$(KLDSO_PATH)
LDFLAGS    += -lelf
EXTFILES    = ${UTHASHINCS}/uthash.h

.PHONY: clean distclean

all: ${EXTFILES} ${TARGETS} 

kld: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS} 

kldd: kld
	ln -sf kld kldd

kld.so: kldso_entry.o kldso.o runtime_kotbl_nostd.o module_patch_nostd.o runtime_prepare_nostd.o
	${LD} -g -o $@ $^

kldso_entry.o: kldso_entry.S
	${AS} -g -o $@ $<

kldso.o: kldso_main.c
	${CC} ${KLDSO_CFLAGS} -c -o $@ $<

runtime_kotbl_nostd.o: runtime_kotbl.c runtime_kotbl.h
	${CC} ${KLDSO_CFLAGS} -c -o $@ $<

module_patch_nostd.o: module_patch.c module_patch.h
	${CC} ${KLDSO_CFLAGS} -c -o $@ $<

runtime_prepare_nostd.o: runtime_prepare.c runtime_prepare.h
	${CC} ${KLDSO_CFLAGS} -c -o $@ $<

${UTHASHINCS}/uthash.h:
	-mkdir -p ext && git clone ${UTHASHREPO} ${UTHASHDIR}

# elf test and experimentation code
stubstest: stubtest.c mklibstubs
	./mklibstubs
	${CC} -fPIC $< -o $@ -L. -lstubs
	@echo -e "****** To test our approach to symbols run *******\nLD_LIBRARY_PATH=." ./$@

mklibstubs: kldelf.c
	${CC} ${CFLAGS}  -DMAIN $< -o $@ -lelf 

libkern.so: kld
	sudo ./kld -k /proc/kallsyms

kerntest: kerntest.c libkern.so
	${CC} -fPIC $< -o $@ -L. -lkern
	@echo -e "****** To test our libkern generation run *******\nLD_LIBRARY_PATH=." ./$@

libkallsyms.so:
	cp /proc/libkallsyms.so .

kallsymtest: kerntest.c libkallsyms.so
	${CC} -fPIC $< -o $@ -lc -L. -lkallsyms
	@echo -e "****** To test against original libkallsyms run *******\nLD_LIBRARY_PATH=." ./$@

listlibs: listlibs.c
	${CC} -DMAIN $< -o $@ -lelf

modinfo: modinfo.c modinfo.h incs.h kldelf.o
	 gcc ${CFLAGS} -DMAIN -o $@ $< kldelf.o -lelf

readsymtbl: readsymtbl.c
	${CC} -g $< -o $@ -lelf

##
clean:
	-${RM} -rf $(wildcard *.o *.d *.bin ${TARGETS} mklibstubs *.so libk)

distclean: clean
	-${RM} rf -rf $(wildcard ext)

-include $(wildcard *.d)
