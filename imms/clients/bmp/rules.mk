BMPCPPFLAGS=`pkg-config bmp --cflags` -I../clients/xmms/ -DBMP
BMPLDFLAGS=`pkg-config bmp dbus-glib-1 --libs`
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
	BMPDESTDIR=`beep-config --general-plugin-dir`
else
	BMPDESTDIR=${HOME}/.bmp/Plugins/General
endif

libbmpimms.so_install: libbmpimms.so
	${INSTALL_PROGRAM} -D $^ $(BMPDESTDIR)/libbmpimms.so

libbmpimms2.so_install: libbmpimms2.so
	${INSTALL_PROGRAM} -D $^ $(BMPDESTDIR)/libbmpimms.so
