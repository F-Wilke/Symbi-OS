CC := gcc
KCC := ../tools/kcc
KLD := ../tools/src/kld/kld
O  := 2
CFLAGS = -O${O} -ggdb -MD -MP -Wall -Wextra  -Wfatal-errors -fPIC
CFLAGS += -I ../kcutef -I ../kcutevac -I ../kcuttcp -I ../kcuttest -I ../../Symlib/include -DDYNLINK
KFLAGS  = -K ../kcutidt -K ../kcutnmi -K ../kcutef -K ../kcutevac -K ../kcuttcp -k kcutidt.ko -k kcutnmi.ko -k kcutef.ko -k kcutevac.ko -k kcuttcp.ko -k /proc/kallsyms
TARGET  := kcuttest
KTARGET := kcuttest.ko 
LIBS := -lc  ../../Symlib/build/L1/L1.a -L. -L ../kcutevac -L ../kcuttcp -l kcutevac -l kcuttcp -lkcuttest -lkern
# -rdynamic exports executable symbols so dlsym(RTLD_DEFAULT, "user_add") works.
# applink.o is linked directly (not via -lapp) to guarantee the constructor runs.
LDFLAGS := -pie -rdynamic ${LIBS}

.PHONY: all clean

all: ${KTARGET} $(TARGET) 

# main program
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

${KTARGET} kotbl.bin app.h applink.o: $(wildcard *.kc *.kh *.kS)
	 ${KCC} -o ${KTARGET} ${KFLAGS}

${TARGET}:  main.o mmap_stack_test.o applink.o | kotbl.bin
	$(CC) $(CFLAGS) -o $@ $^ ${LDFLAGS}
	${KLD} -o $@

-include *.d

clean:
	rm -rf $(wildcard $(TARGET) $(KTARGET) *.o *.so *.d kotbl.bin Module.symvers .ext applink.o app.h)


