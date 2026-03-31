CC = gcc

# lets go through these and make sure we understand what we really need -- if dpld logic
# requires mitigations then let's stack switch extra as needed
CFLAGS = -O0 -ggdb -Wall -Wextra -fno-omit-frame-pointer -Wfatal-errors \
	-fPIC
CFLAGS += -I ../../Symlib/include -DDYNLINK

TARGETS :=  esrv eclnt
MODNAME := idt_adaptor.kbin
.PHONY: all clean libkallsyms.so libkallsyms.a

all: $(TARGETS)

# extension boiler plate
libextension.a $(MODNAME): $(wildcard *.kc *.kh)
	target=$(MODNAME) ../tools/kcc

libkallsyms.so:
	cp /proc/$@ $@

libkallsyms.a:
	cp /proc/$@ $@

# main program
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

esrv: esrv.o
	$(CC) $(CFLAGS) -o $@ $< 

eclnt: eclnt.o
	$(CC) $(CFLAGS) -o $@ $< -lm 

clean:
	-${RM} -rf $(wildcard $(TARGETS) *.o *.so *.a ext.kbin .ext)

