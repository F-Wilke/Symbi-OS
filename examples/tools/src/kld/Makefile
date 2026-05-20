SRCS       := main.c kldfs.c fs.c sigproc.c kldelf.c modinfo.c loadbo.c
OBJS       := $(SRCS:%.c=%.o)
O          := 0
CFLAGS     += -g -O${O} -std=gnu99 -MD -MP -Wall -Werror $(shell pkg-config fuse3 --cflags) 
TARGETS    := kld readsymtbl listlibs stubstest kerntest
UTHASHREPO := git@github.com:troydhanson/uthash.git
UTHASHDIR  := ext/uthash
UTHASHINCS  = ${UTHASHDIR}/include

CFLAGS     += -I${UTHASHINCS} 
LDFLAGS    += $(shell pkg-config fuse3 --cflags --libs) -lelf
EXTFILES    = ${UTHASHINCS}/uthash.h

.PHONY: clean distclean

all: ${EXTFILES} ${TARGETS} 

kld: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS} 

${UTHASHINCS}/uthash.h:
	-mkdir -p ext && git clone ${UTHASHREPO} ${UTHASHDIR}

# elf test and experimentation code
stubstest: stubtest.c mklibstubs
	./mklibstubs
	${CC} -fPIC $< -o $@ -L. -lstubs
	@echo -e "****** To test our approach to symbols run *******\nLD_LIBRARY_PATH=." ./$@

mklibstubs: kldelf.c
	${CC} -DMAIN $< -o $@ -lelf

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



