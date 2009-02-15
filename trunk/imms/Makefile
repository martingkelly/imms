#
#  IMMS: Intelligent Multimedia Management System
#  Copyright (C) 2001-2009 Michael Grigoriev
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
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
	rm -f $(wildcard build/[^M]* core* build/.*.d)

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
	@$(MAKE) -C build --no-print-directory $@
