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
BMPCPPFLAGS=`pkg-config bmp --cflags` -I../clients/xmms/ -DBMP
BMPLDFLAGS=`pkg-config bmp glib --libs`
BMPCOMMON=bmpinterface.o clientstubbase.o libimmscore.a

libbmpimms.so: bmpplugin.o $(BMPCOMMON)
libbmpimms-LIBS = $(BMPLDFLAGS)
libbmpimms2.so: bmpplugin2.o $(BMPCOMMON)
libbmpimms2-LIBS = $(BMPLDFLAGS)

bmpinterface-CPPFLAGS=$(BMPCPPFLAGS)

bmpplugin.o: plugin.cc
	$(call compile, $(CXX), $<, $@, $(CXXFLAGS) $(BMPCPPFLAGS) $(CPPFLAGS))

bmpplugin2.o: plugin2.cc
	$(call compile, $(CXX), $<, $@, $(CXXFLAGS) $(BMPCPPFLAGS) $(CPPFLAGS))

BMPDESTDIR=""
ifeq ($(shell id -u), 0)
	BMPDESTDIR=`pkg-config --variable=general_plugin_dir bmp`
else
	BMPDESTDIR=${HOME}/.bmp/Plugins/General
endif

libbmpimms.so_install: libbmpimms.so
	${INSTALL} -D $^ $(BMPDESTDIR)/libbmpimms.so

libbmpimms2.so_install: libbmpimms2.so
	${INSTALL} -D $^ $(BMPDESTDIR)/libbmpimms.so
