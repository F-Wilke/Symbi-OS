# Makefile for pftest assembly testing

CC = gcc
AS = as
LD = ld

CFLAGS = -Wall -Wextra -g -O0 -no-pie
ASFLAGS = 
LDFLAGS = -no-pie

TARGET = pftest
OBJS = test_pfadaptor.o pftest.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

test_pfadaptor.o: test_pfadaptor.c
	$(CC) $(CFLAGS) -c -o $@ $<

pftest.o: pftest.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

