all: libimms.so

libimms.so: $(XMMS_OBJ) immsconf.h
	$(CXX) $(XMMS_OBJ) \
		$(LDFLAGS) \
	       	-shared -Wl,-z,defs,-soname,$@ -o $@

immstool: libimmscore.a

libimmscore.a: $(CORE_OBJ) immsconf.h
	$(AR) $(ARFLAGS) $@ $(CORE_OBJ)

%.o: %.cc vars.mk
	@$(CXX) $(CPPFLAGS) -M -E $< > .$*.d
	$(CXX) $(CPPFLAGS) -c $< -o $@

%.o: %.c vars.mk
	@$(CXX) $(CFLAGS) -M -E $< > .$*.d
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean distclean

clean:
	rm -f $(wildcard $(XMMS_OBJ) $(CORE_OBJ) \
		libimms.so libimmscore.a immstool imms-*.tar.* core* .*.d)

distclean: clean
	rm -f $(wildcard .\#* config.* configure immsconf.h* vars.mk)
	rm -rf $(wildcard autom4te.cache)

.PHONY: dist

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
	${INSTALL_PROGRAM} libimms.so ${HOME}/.xmms/Plugins/Visualization/
