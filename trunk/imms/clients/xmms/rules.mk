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
XMMSCPPFLAGS= `xmms-config --cflags`
XMMSLDFLAGS=`xmms-config --libs`
XMMSCOMMON=interface.o clientstubbase.o libimmscore.a

libxmmsimms.so: plugin.o $(XMMSCOMMON)
libxmmsimms-LIBS = $(XMMSLDFLAGS)
libxmmsimms2.so: plugin2.o $(XMMSCOMMON)
libxmmsimms2-LIBS = $(XMMSLDFLAGS)

interface-CPPFLAGS=$(XMMSCPPFLAGS)
plugin-CPPFLAGS=$(XMMSCPPFLAGS)
plugin2-CPPFLAGS=$(XMMSCPPFLAGS)

DESTDIR=""
ifeq ($(shell id -u), 0)
	DESTDIR=`xmms-config --general-plugin-dir`
else
	DESTDIR=${HOME}/.xmms/Plugins/General
endif

libxmmsimms.so_install: libxmmsimms.so
	${INSTALL} -D $^ $(DESTDIR)/libxmmsimms.so

libxmmsimms2.so_install: libxmmsimms2.so
	${INSTALL} -D $^ $(DESTDIR)/libxmmsimms.so
