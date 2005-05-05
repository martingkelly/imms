-include vars.mk

.PHONY: first

first: configure
	$(error Please run the "configure" script)
 
configure: configure.ac
	autoheader
	aclocal
	autoconf

immsconf.h: configure
	$(error Please run the "configure" script)

.PHONY: clean distclean dist

clean:
	rm -f $(wildcard build/*.o)
	rm -f $(wildcard build/lib*imms*.* build/analyzer build/immstool build/immsd build/imms-*.tar.* build/imms*.o core* build/.*.d)

distclean: clean
	rm -f $(wildcard .\#* config.* configure immsconf.h* aclocal.m4* vars.mk)
	rm -rf $(wildcard autom4te.cache)

dist: immsconf.h distclean
	mv autogen.sh configure
	cp -r . /tmp/imms-$(VERSION)
	rm -rf `find /tmp/imms-$(VERSION)/ -name .svn`
	tar -C /tmp/ -cj imms-$(VERSION)/ -f build/imms-$(VERSION).tar.bz2
	tar -C /tmp/ -cz imms-$(VERSION)/ -f build/imms-$(VERSION).tar.gz
	rm -rf /tmp/imms-$(VERSION)/
	mv configure autogen.sh

vars.mk:;

%:
	@make -C build --no-print-directory $@
