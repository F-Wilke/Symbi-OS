CC := gcc
O  := 2
CFLAGS = -O${O} -ggdb -MD -MP -Wall -Wextra  -Wfatal-errors -fPIC
CFLAGS += -I ../kcutpf -I ../kcutevac -I ../../Symlib/include -DDYNLINK
#KFLAGS = -I ../kcutidt -K ../kcutidt -k kcutidt.ko 
TARGET  := kcuttest
KTARGET := kcuttest.ko 
LIBS := -lc  ../../Symlib/build/L1/L1.a
LDFLAGS := ${LIBS}

.PHONY: all clean

all: ${KTARGET}
#$(TARGET) 

# main program
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

${KTARGET}: $(wildcard *.kc *.kh *.kS)
	 ../tools/kcc -o $@  ${KFLAGS} 

${TARGET}:  main.o mmap_stack_test.o 
	$(CC) $(CFLAGS) -o $@ $^ ${LDFLAGS}

-include *.d

clean:
	rm -rf $(wildcard $(TARGET) $(KTARGET) *.o *.so *.d .ext)


