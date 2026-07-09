CC = gcc
KFLAGS = -I ../kcutidt -I ../kcutnmi -K ../kcutidt -k kcutidt.ko -K ../kcutnmi -k kcutnmi.ko
TARGET := kcutef.ko
.PHONY: clean 

# extension boiler plate
${TARGET}: $(wildcard *.kc *.kh *.kS)
	 ../tools/kcc -o $@  ${KFLAGS} 

# main program
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(wildcard $(TARGET) *.o *.so *.a *.ko kotbl.bin Module.symvers .ext)


