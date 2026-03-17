# extension boiler plate
libextension.a ext.kbin: $(wildcard *.kc *.kh)
	../../tools/kcc

libkallsyms.so:
	cp /proc/$@ $@

libkallsyms.a:
	cp /proc/$@ $@

clean:
	rm -rf $(wildcard $(TARGETS) *.o *.so *.a *.kbin .ext)
