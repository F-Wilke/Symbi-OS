# extension boiler plate
MODNAME := kcuttest.kbin

libextension.a ext.kbin: $(wildcard *.kc *.kh)
	-cp -s ../../kcuttest/*.kc .
	-cp -s ../../kcuttest/*.kh .
	-cp -s ../../kcuttest/*.kS .
	-cp -s ../../kcuttest/kcut.h .
	-cp -s ../../kcuttest/ktos.h .
	-cp -s ../../kcuttest/evacuate.h .
	-cp -s ../../kcuttest/idtadaptor.h .
	-cp -s ../../kcuttest/kcut_tcpmsg.h .
	target=$(MODNAME) ../../tools/kcc

libkallsyms.so:
	cp /proc/$@ $@

libkallsyms.a:
	cp /proc/$@ $@

clean:
	-${RM} -rf $(wildcard $(TARGETS) app.h *.o *.a *.kbin *.kc *.kh *.kS *.kS kcut.h ktos.h evacuate.h idtadaptor.h kcut_tcpmsg.h .ext *.so *.a)
