CC = gcc

CFLAGS = -O4 -ggdb -Wall -Wextra  -Wfatal-errors -fPIC
CFLAGS += -I ../../Symlib/include -DDYNLINK

TARGETS :=  main main_static
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


main:  main.o mmap_stack_test.o libextension.a libkallsyms.so
#main:  main.o libkallsyms.so
#	$(CC) $(CFLAGS) -o $@ $< mmap_stack_test.o -lc -L../../Symlib/dynam_build -lSym -L. -lkallsyms  -L. -lkallsyms -lextension 
	$(CC) $(CFLAGS) -o $@ $< mmap_stack_test.o -lc  -L. -lkallsyms -lextension ../../Symlib/build/L1/L1.a

main_static: main.o mmap_stack_test.o libkallsyms.a libextension.a
	$(CC) $(CFLAGS) -o $@ $^  libkallsyms.a libextension.a -lc -L../../Symlib/dynam_build -lSym  

clean:
	rm -rf $(wildcard $(TARGETS) *.o *.so *.a ext.kbin .ext)


