# usage compile(compiler, source, output, flags) 
DEPFILE = $(if $(filter %.o,$1),$(dir $1).$(notdir $(1:.o=.d)),/dev/null)
define compile
        @$1 $4 $2 -M -E > $(DEPFILE)
        $1 $4 -c $2 -o $3
endef

# usage link(objects, output, flags) 
link = $(CXX) $3 $(filter-out %.a,$1) $(filter %.a,$1) -o $2

%.o: %.cc $(call compile, $(CXX), $<, $@, $($*-CXXFLAGS) $(CXXFLAGS) $($*-CPPFLAGS) $(CPPFLAGS))
%.o: %.c; $(call compile, $(CC), $<, $@, $($*-CFLAGS) $(CFLAGS) $($*-CPPFLAGS) $(CPPFLAGS))
%: %.o; $(call link, $^ $($*-OBJ) $($*-LIBS) $(LIBS), $@, $(LDFLAGS))

# macros that expand to the object files in the given directories
objects=$(sort $(foreach type,c cc,$(call objects_$(type),$1)))
objects_c=$(patsubst %.c,%.o,$(wildcard $(addsuffix /*.c,$1)))
objects_cc=$(patsubst %.cc,%.o,$(wildcard $(addsuffix /*.cc,$1)))

.PHONY: clean distclean dist

clean:
	rm -f $(wildcard $(XMMS_OBJ) $(CORE_OBJ) \
		libimms.so libimmscore.a immstool immsremote \
	       	imms-*.tar.* imms*.o core* .*.d)

distclean: clean
	rm -f $(wildcard .\#* config.* configure immsconf.h* vars.mk)
	rm -rf $(wildcard autom4te.cache)

configure: configure.ac
	autoheader
	autoconf

immsconf.h: configure
	$(error Please run the "configure" script)

dist: immsconf.h distclean 
	cp -r . /tmp/imms-$(VERSION)
	rm -rf /tmp/imms-$(VERSION)/.svn
	tar -C /tmp/ -cj imms-$(VERSION)/ -f imms-$(VERSION).tar.bz2
	tar -C /tmp/ -cz imms-$(VERSION)/ -f imms-$(VERSION).tar.gz
	rm -rf /tmp/imms-$(VERSION)/

.PHONY: install install-user install-system user-message system-message

ifeq ($(shell id -u), 0)
    install: system-message install-system
else
    install: user-message install-user
endif

system-message:
	$(warning Defaulting to installing for all users.)
	$(warning Use 'make install-user' to install for the current user only.)

install-system: libimms.so
	${INSTALL_PROGRAM} libimms.so ${DESTDIR}`xmms-config --visualization-plugin-dir`

user-message:
	$(warning Defaulting to installing for current user only.)
	$(warning Use 'make install-system' to install for all users.)

install-user: libimms.so
	mkdir -p ${HOME}/.xmms/Plugins/Visualization/
	rm -f ${HOME}/.xmms/Plugins/General/libimms.so
	${INSTALL_PROGRAM} libimms.so ${HOME}/.xmms/Plugins/Visualization/
