-include ../vars.mk
-include .*.d

.PHONY: first

first: ../configure
	 $(warning Please run the "configure" script)

include ../rules.mk

all: libxmmsimms.so immsd analyzer immstool

libxmmsimms.so: $(XMMS_OBJ)
libxmmsimms-LIBS = $(DBUSLDFLAGS) $(XMMSLDFLAGS)

libimmscore.a: $(call objects,..)
	$(AR) $(ARFLAGS) $@ $(filter %.o,$^)

immsd: libimmscore.a

immstool-CPPFLAGS = -I../analyzer
immstool: immstool.o libimmscore.a spectrum.o

analyzer-LIBS=`pkg-config fftw3f --libs`

analyzer: $(call objects,../analyzer)
analyzer: libimmscore.a

interface-CPPFLAGS=$(XMMSCPPFLAGS)
plugin-CPPFLAGS=$(XMMSCPPFLAGS)
plugin2-CPPFLAGS=$(XMMSCPPFLAGS)
songinfo-CPPFLAGS=$(TAGCPPFLAGS)
dbuscore-CPPFLAGS=$(DBUSCPPFLAGS)
dbusclient-CPPFLAGS=$(DBUSCPPFLAGS)
dbusserver-CPPFLAGS=$(DBUSCPPFLAGS) $(GLIB2CPPFLAGS)
glib1dbus-CPPFLAGS=$(DBUSCPPFLAGS) $(GLIB1CPPFLAGS)

immsd: $(call objects,../immsd)
immsd-CPPFLAGS=$(GLIB2CPPFLAGS)
immsd-LIBS=$(GLIB2LDFLAGS) $(DBUSLDFLAGS)

dbus-server-CPPFLAGS=$(DBUSCPPFLAGS)
dbus-server-LIBS=$(DBUSLDFLAGS) -limmscore -L.
dbus-server: dbus-server.o

dbus-client-CPPFLAGS=$(DBUSCPPFLAGS)
dbus-client-LIBS=$(DBUSLDFLAGS) -limmscore -L.
dbus-client: dbus-client.o

glib-test-CPPFLAGS=$(GLIB1CPPFLAGS)
glib-test-LIBS=-limmscore -L. $(GLIB1LDFLAGS)
glib-test: glib-test.o
