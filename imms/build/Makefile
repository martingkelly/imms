-include ../vars.mk
-include .*.d

.PHONY: first

first: ../configure
	 $(warning Please run the "configure" script)

include ../rules.mk

all: libxmmsimms.so immsd analyzer immstool immsremote

libxmmsimms.so: $(XMMS_OBJ)
libxmmsimms-LIBS = $(DBUSLDFLAGS) $(XMMSLDFLAGS)

libimmscore.a: $(call objects,..)
	$(AR) $(ARFLAGS) $@ $(filter %.o,$^)

immsd: libimmscore.a

immstool-CPPFLAGS = -I../analyzer
immstool: immstool.o libimmscore.a spectrum.o

sqlite_speed_test: libimmscore.a
sqlite_bork_test: libimmscore.a

immsremote: immsremote.o comm.o 
immsremote: -lreadline -lcurses

analyzer-LIBS=`pkg-config fftw3f --libs`

analyzer: $(call objects,../analyzer)
analyzer: libimmscore.a

test: test.o
test: libimmscore.a

interface-CPPFLAGS=$(XMMSCPPFLAGS)
plugin-CPPFLAGS=$(XMMSCPPFLAGS)
plugin2-CPPFLAGS=$(XMMSCPPFLAGS)
songinfo-CPPFLAGS=$(TAGCPPFLAGS)
dbuscore-CPPFLAGS=$(DBUSCPPFLAGS)
dbusclient-CPPFLAGS=$(DBUSCPPFLAGS)
glib1dbus-CPPFLAGS=$(DBUSCPPFLAGS) `pkg-config glib --cflags`

immsd: $(call objects,../immsd)
immsd-CPPFLAGS=$(GLIBCPPFLAGS)
immsd-LIBS=$(GLIBLDFLAGS) $(DBUSLDFLAGS)

dbusserver-CPPFLAGS=$(DBUSCPPFLAGS) $(GLIBCPPFLAGS)

dbus-server-CPPFLAGS=$(DBUSCPPFLAGS)
dbus-server-LIBS=$(DBUSLDFLAGS) -limmscore -L.
dbus-server: dbus-server.o

dbus-client-CPPFLAGS=$(DBUSCPPFLAGS)
dbus-client-LIBS=$(DBUSLDFLAGS) -limmscore -L.
dbus-client: dbus-client.o

glib-test-CPPFLAGS=`pkg-config glib --cflags`
glib-test-LIBS=-limmscore -L. `pkg-config glib --libs`
glib-test: glib-test.o
