CC := gcc
O  := 2
CFLAGS = -O${O} -ggdb -MD -MP -Wall -Wextra  -Wfatal-errors -fPIC
CFLAGS += -I ../kcutpf -I ../kcutevac -I ../../Symlib/include -DDYNLINK
#KFLAGS = -I ../kcutidt -K ../kcutidt -k kcutidt.ko
KFLAGS  = -K ../kcutidt -K ../kcutpf -K ../kcutevac -k kcutidt.ko -k kcutpf.ko -k kcutevac.ko -k /proc/kallsyms
TARGET  := kcuttest
KTARGET := kcuttest.ko 
LIBS := -lc  ../../Symlib/build/L1/L1.a -L. -L ../kcutidt -L ../kcutpf -L ../kcutevac -l kcutidt -l kcutpf -l kcutevac -lkcuttest -lkern
LDFLAGS := ${LIBS}

.PHONY: all clean

all: ${KTARGET} $(TARGET) 

# main program
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

${KTARGET}: $(wildcard *.kc *.kh *.kS)
	 ../tools/kcc -o $@  ${KFLAGS}
#	 sudo ../tools/src/kld/kld 

${TARGET}:  main.o mmap_stack_test.o 
	$(CC) $(CFLAGS) -o $@ $^ ${LDFLAGS}
	../tools/src/kldso/kldinst kcuttest

-include *.d

clean:
	rm -rf $(wildcard $(TARGET) $(KTARGET) *.o *.so *.d .ext)


