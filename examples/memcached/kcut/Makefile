# extension boiler plate
libextension.a ext.kbin: $(wildcard *.kc *.kh)
	-cp -s ../../kcuttest/*.kc .
	-cp -s ../../kcuttest/*.kh .
	-cp -s ../../kcuttest/*.kS .
	-cp -s ../../kcuttest/idtadaptor.h .
	../../tools/kcc

libkallsyms.so:
	cp /proc/$@ $@

libkallsyms.a:
	cp /proc/$@ $@

clean:
	rm -rf $(wildcard $(TARGETS) app.h *.o *.a *.kbin .ext)
