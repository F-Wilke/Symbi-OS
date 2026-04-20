SRCS       := main.c kldfs.c fs.c sigproc.c elf.c
OBJS       := $(SRCS:%.c=%.o)
O          := 2
CFLAGS     += -g -O${O} -std=gnu99 -MD -MP -Wall -Werror $(shell pkg-config fuse3 --cflags)
TARGETS    := kld readsymtbl listlibs stubstest
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
	@echo -e "****** To test our approach to symbols run *******\nLD_LIBRARY_PATH=. ./stubtest"

mklibstubs: elf.c
	${CC} -DMAIN $< -o $@ -lelf

listlibs: listlibs.c
	${CC} -DMAIN $< -o $@ -lelf

readsymtbl: readsymtbl.c
	${CC} -g $< -o $@ -lelf

##
clean:
	-${RM} -rf $(wildcard *.o *.d ${TARGETS} mklibstubs *.so libk)

distclean: clean
	-${RM} rf -rf $(wildcard ext)

-include $(wildcard *.d)



