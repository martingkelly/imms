-include ../vars.mk
-include .*.d

.PHONY: first

first: ../configure
	 $(warning Please run the "configure" script)

include ../rules.mk

all: immsd analyzer immstool $(PLUGINS)

libimmscore.a: $(call objects,../immscore)
	$(AR) $(ARFLAGS) $@ $(filter %.o,$^)

immstool-CPPFLAGS = -I../analyzer
immstool: immstool.o libimmscore.a spectrum.o

analyzer: $(call objects,../analyzer)
analyzer: libimmscore.a
analyzer-LIBS=`pkg-config fftw3f --libs`

songinfo-CPPFLAGS=$(TAGCPPFLAGS)
dbuscore-CPPFLAGS=$(DBUSCPPFLAGS)
dbusclient-CPPFLAGS=$(DBUSCPPFLAGS)
dbusserver-CPPFLAGS=$(DBUSCPPFLAGS) $(GLIB2CPPFLAGS)
socketserver-CPPFLAGS=$(GLIB2CPPFLAGS)
glib1dbus-CPPFLAGS=$(DBUSCPPFLAGS) $(GLIB1CPPFLAGS)
glib2dbus-CPPFLAGS=$(DBUSCPPFLAGS) $(GLIB2CPPFLAGS)

immsd: libimmscore.a
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

socket-client-CPPFLAGS=$(GLIB1CPPFLAGS)
socket-client-LIBS=-limmscore -L. $(GLIB1LDFLAGS)
socket-client: socket-client.o

PLUGINS_INSTALL=$(patsubst %,%_install,$(PLUGINS))
plugins_install: $(PLUGINS_INSTALL)
