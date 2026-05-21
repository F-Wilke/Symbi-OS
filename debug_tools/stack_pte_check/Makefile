# Makefile for stack_pte_probe kernel module
#
# Build with: make
# Install with: sudo insmod stack_pte_probe.ko
# Remove with: sudo rmmod stack_pte_probe
# View logs with: dmesg -w

obj-m += stack_pte_probe.o

# Default kernel directory
# KDIR ?= /lib/modules/$(shell uname -r)/build
KDIR ?= /home/user/Symbi-OS/linux

# Build directory
PWD := $(shell pwd)

# Default target
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Install module (requires root)
install: all
	sudo insmod stack_pte_probe.ko

# Remove module (requires root)
uninstall:
	sudo rmmod stack_pte_probe

# View kernel log
log:
	dmesg -w

# Show module info
info: all
	modinfo stack_pte_probe.ko

.PHONY: all clean install uninstall log info
