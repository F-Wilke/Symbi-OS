CC = gcc

# lets go through these and make sure we understand what we really need -- if dpld logic
# requires mitigations then let's stack switch extra as needed
CFLAGS = -O2 -ggdb -Wall -Wextra -Wfatal-errors -fPIC

KCUT_CFLAGS = $(CFLAGS) -D KCUT_TCP -I ../../Symlib/include -DDYNLINK
KCUT_LDFLAGS = -lc  -Lkcut -lkallsyms -lextension ../../Symlib/build/L1/L1.a
TARGETS :=  esrv eclnt kcut_bpbs_esrv kcut_cpbs_esrv kcut_bpcs_esrv kcut_cpcs_esrv

.PHONY: all clean 

all: $(TARGETS)

esrv.o: esrv.c
	$(CC) $(CFLAGS) -c -o $@ $<
esrv: esrv.o
	$(CC) $(CFLAGS) -o $@ $< 

kcut/libextension.a kcut/libkallsyms.so kcut/kcut_tcpmsg.h:
	make -C kcut

kcut_bpbs_esrv.o: esrv.c kcut/kcut_tcpmsg.h kcut/libextension.a kcut/libkallsyms.so 
	$(CC) $(KCUT_CFLAGS) -D BRACKET_PRIV -D BRACKET_STACK  -c -o $@ $<
kcut_bpbs_esrv: kcut_bpbs_esrv.o
	$(CC) $(KCUT_CFLAGS) -D BRACKET_PRIV -D BRACKET_STACK  -o $@ $< $(KCUT_LDFLAGS)

kcut_cpbs_esrv.o: esrv.c kcut/kcut_tcpmsg.h kcut/libextension.a kcut/libkallsyms.so  
	$(CC) $(KCUT_CFLAGS) -D BRACKET_STACK -c -o $@ $<
kcut_cpbs_esrv: kcut_cpbs_esrv.o
	$(CC) $(KCUT_CFLAGS) -D BRACKET_STACK -o $@ $< $(KCUT_LDFLAGS)

kcut_bpcs_esrv.o: esrv.c kcut/kcut_tcpmsg.h kcut/libextension.a kcut/libkallsyms.so 
	$(CC) $(KCUT_CFLAGS) -D BRACKET_PRIV -c -o $@ $<
kcut_bpcs_esrv: kcut_bpcs_esrv.o
	$(CC) $(KCUT_CFLAGS) -D BRACKET_PRIV -o $@ $< $(KCUT_LDFLAGS)

kcut_cpcs_esrv.o: esrv.c kcut/kcut_tcpmsg.h kcut/libextension.a kcut/libkallsyms.so 
	$(CC) $(KCUT_CFLAGS) -c -o $@ $<
kcut_cpcs_esrv: kcut_cpcs_esrv.o
	$(CC) $(KCUT_CFLAGS) -o $@ $< $(KCUT_LDFLAGS)

eclnt.o: esrv.c
	$(CC) $(CFLAGS) -c -o $@ $<
eclnt: eclnt.o
	$(CC) $(CFLAGS) -o $@ $< -lm 

clean:
	-make -C kcut clean
	-${RM} -rf $(wildcard $(TARGETS) *.o) 


