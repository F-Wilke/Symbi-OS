CC = gcc
KFLAGS = -I ../kcutidt -K ../kcutidt -k kcutidt.ko 
TARGET := kcutpf.ko
.PHONY: clean 

# extension boiler plate
${TARGET}: $(wildcard *.kc *.kh *.kS)
	 ../tools/kcc -o $@  ${KFLAGS} 

# main program
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(wildcard $(TARGET) *.o *.so *.a *.ko kotbl.bin libk .ext)


