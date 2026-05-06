CC = gcc
CFLAGS = -O2 -ggdb -Wall -Wextra  -Wfatal-errors -fPIC
CFLAGS += -I ../../Symlib/include -DDYNLINK

TARGET := kcutidt.ko
.PHONY: clean 

# extension boiler plate
${TARGET}: $(wildcard *.kc *.kh *.kS)
	 ../tools/kcc -o $@ 

# main program
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(wildcard $(TARGET) *.o *.so *.a *.ko kotbl.bin libk .ext)


