BMPCPPFLAGS=`pkg-config bmp --cflags` -I../clients/xmms/ -DBMP
BMPLDFLAGS=`pkg-config bmp dbus-glib-1 --libs`

libbmpimms.so: bmpplugin.o bmpinterface.o clientstubbase.o libimmscore.a
libbmpimms-LIBS = $(DBUSLDFLAGS) $(BMPLDFLAGS)

bmpinterface-CPPFLAGS=$(BMPCPPFLAGS)

bmpplugin.o: plugin.cc
	$(call compile, $(CXX), $<, $@, $(CXXFLAGS) $(BMPCPPFLAGS) $(CPPFLAGS))

BMPDESTDIR=""
ifeq ($(shell id -u), 0)
	BMPDESTDIR=`beep-config --general-plugin-dir`
else
	BMPDESTDIR=${HOME}/.bmp/Plugins/General
endif

libbmpimms.so_install: libbmpimms.so
	${INSTALL_PROGRAM} -D $^ $(BMPDESTDIR)/libbmpimms.so
