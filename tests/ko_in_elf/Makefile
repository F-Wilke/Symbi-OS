PATCH_DEFINES := 
CFLAGS := -O0 -ggdb -Wall -Wextra -mno-red-zone -fno-omit-frame-pointer -Wfatal-errors


# dynam_build: CFLAGS += -DDYNLINK -lkallsyms
dynam_build: CFLAGS += -lkallsyms
dynam_build: PATCH_DEFINES += LIBRARY_PATH=/proc/ 
dynam_build: basic


greeter.ko:
	mkdir -p ../../built_modules
	$(MAKE) -C ../.. build_module_full MODULE_SRC=./linux-kernel-module/greeter MODULE_DEST=/root/greeter MODULE_NAME=greeter V=1
	cp ../../built_modules/greeter.ko .

greeter.o: greeter.ko
	ld -r -b binary -o greeter.o greeter.ko

basic: greeter.o
	$(PATCH_DEFINES) gcc main.c greeter.o -Wl,-T,default.ld -o mymodloader -L../../Symlib/dynam_build -lSym -I ../../Symlib/include $(CFLAGS)

mymodloader.so: greeter.o
	$(PATCH_DEFINES) gcc -shared -fPIC main.c greeter.o -o mymodloader.so -L../../Symlib/dynam_build -lSym -I ../../Symlib/include $(CFLAGS)

clean:
	rm -f mymodloader greeter.o greeter.ko mymodloader.so
	$(MAKE) -C ../.. clean_module MODULE_SRC=./linux-kernel-module/greeter MODULE_DEST=/root/greeter MODULE_NAME=greeter