CC := gcc
KCC := ../tools/kcc
KLD := ../tools/src/kld/kld

O  := 2

# lets go through these and make sure we understand what we really need -- if dpld logic
# requires mitigations then let's stack switch extra as needed
CFLAGS := -O${O} -ggdb -MD -MP -Wall -Wextra -Wfatal-errors -fPIC
KFLAGS := -K ../kcutidt -K ../kcutef -K ../kcutevac  -K ../kcuttcp -k kcutidt.ko -k kcutef.ko -k kcutevac.ko -k kcuttcp.ko -k /proc/kallsyms

KCUT_CFLAGS = $(CFLAGS) -D KCUT_TCP -I ../kcuttcp -I ../kcuttest -I ../kcutevac  -I ../../Symlib/include -DDYNLINK
KCUT_LDFLAGS = -lc   ../../Symlib/build/L1/L1.a -L ../kcuttcp -L ../kcutevac -L. -lkcuttcp -l kcutevac -lkern 
TARGETS :=  esrv eclnt kcut_bpbs_esrv kcut_cpbs_esrv kcut_bpcs_esrv kcut_cpcs_esrv

.PHONY: all clean 

all: $(TARGETS)

esrv.o: esrv.c
	$(CC) $(CFLAGS) -c -o $@ $<
esrv: esrv.o
	$(CC) $(CFLAGS) -o $@ $< 

libkern.so kotbl.bin: 
	${KLD} ${KFLAGS} 

kcut_bpbs_esrv.o: esrv.c libkern.so kotbl.bin 
	$(CC) $(KCUT_CFLAGS) -D BRACKET_PRIV -D BRACKET_STACK  -c -o $@ $<
kcut_bpbs_esrv: kcut_bpbs_esrv.o
	$(CC) $(KCUT_CFLAGS) -D BRACKET_PRIV -D BRACKET_STACK  -o $@ $< $(KCUT_LDFLAGS)
	${KLD} -o $@

kcut_cpbs_esrv.o: esrv.c libkern.so kotbl.bin 
	$(CC) $(KCUT_CFLAGS) -D BRACKET_STACK -c -o $@ $<
kcut_cpbs_esrv: kcut_cpbs_esrv.o
	$(CC) $(KCUT_CFLAGS) -D BRACKET_STACK -o $@ $< $(KCUT_LDFLAGS)
	${KLD} -o $@
kcut_bpcs_esrv.o: esrv.c libkern.so kotbl.bin 
	$(CC) $(KCUT_CFLAGS) -D BRACKET_PRIV -c -o $@ $<
kcut_bpcs_esrv: kcut_bpcs_esrv.o
	$(CC) $(KCUT_CFLAGS) -D BRACKET_PRIV -o $@ $< $(KCUT_LDFLAGS)
	${KLD} -o $@

kcut_cpcs_esrv.o: esrv.c libkern.so kotbl.bin 
	$(CC) $(KCUT_CFLAGS) -c -o $@ $<
kcut_cpcs_esrv: kcut_cpcs_esrv.o
	$(CC) $(KCUT_CFLAGS) -o $@ $< $(KCUT_LDFLAGS)
	${KLD} -o $@

eclnt.o: esrv.c
	$(CC) $(CFLAGS) -c -o $@ $<
eclnt: eclnt.o
	$(CC) $(CFLAGS) -o $@ $< -lm 

-include *.d

clean:
	-${RM} -rf $(wildcard $(TARGETS) *.o *.so *.d *.bin) 


