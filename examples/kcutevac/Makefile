CC = gcc
KFLAGS = 
TARGET := kcutevac.ko
.PHONY: clean 

# extension boiler plate
${TARGET}: $(wildcard *.kc *.kh *.kS)
	 ../tools/kcc -o $@  ${KFLAGS} 

# main program
%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(wildcard $(TARGET) *.o *.so *.a *.ko kotbl.bin Module.symvers .ext)


