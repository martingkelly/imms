XMMSCPPFLAGS= `xmms-config --cflags`
XMMSLDFLAGS=`xmms-config --libs`
XMMSCOMMON=interface.o clientstubbase.o libimmscore.a

libxmmsimms.so: plugin.o $(XMMSCOMMON)
libxmmsimms-LIBS = $(DBUSLDFLAGS) $(XMMSLDFLAGS)
libxmmsimms2.so: plugin2.o $(XMMSCOMMON)
libxmmsimms2-LIBS = $(DBUSLDFLAGS) $(XMMSLDFLAGS)

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
	${INSTALL_PROGRAM} -D $^ $(DESTDIR)/libxmmsimms.so

libxmmsimms2.so_install: libxmmsimms2.so
	${INSTALL_PROGRAM} -D $^ $(DESTDIR)/libxmmsimms.so
