-include vars.mk

.PHONY: first

first: configure
	$(error Please run the "configure" script)
 
configure: configure.ac
	autoheader
	autoconf

immsconf.h: configure
	$(error Please run the "configure" script)

vars.mk:;

%:
	@make -C build --no-print-directory $@
