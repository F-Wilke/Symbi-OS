CC := gcc
O  := 2
CFLAGS = -O${O} -ggdb -MD -MP -Wall -Wextra  -Wfatal-errors -fPIC
CFLAGS += -I ../kcutef -I ../kcutevac -I ../../Symlib/include -DDYNLINK
#KFLAGS = -I ../kcutidt -K ../kcutidt -k kcutidt.ko
KFLAGS  = -K ../kcutidt -K ../kcutef -K ../kcutevac -k kcutidt.ko -k kcutef.ko -k kcutevac.ko -k /proc/kallsyms
TARGET  := kcuttest
KTARGET := kcuttest.ko 
LIBS := -lc  ../../Symlib/build/L1/L1.a -L. -L ../kcutevac -l kcutevac -lkcuttest -lkern
LDFLAGS := -pie ${LIBS}

.PHONY: all clean

all: ${KTARGET} $(TARGET) 

# main program
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

${KTARGET}: $(wildcard *.kc *.kh *.kS)
	 ../tools/kcc -o $@  ${KFLAGS}

${TARGET}:  main.o mmap_stack_test.o 
	$(CC) $(CFLAGS) -o $@ $^ ${LDFLAGS}
	../tools/src/kld/kld -o $@ ${KFLAGS}
	KLD_NOINTERP=1 ../tools/src/kldso/kldinst kcuttest

-include *.d

clean:
	rm -rf $(wildcard $(TARGET) $(KTARGET) *.o *.so *.d kotbl.bin Module.symvers .ext)


