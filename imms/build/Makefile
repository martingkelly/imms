-include ../vars.mk
-include .*.d

.PHONY: first

first: ../configure
	 $(warning Please run the "configure" script)

include ../rules.mk

all: libimms.so analyzer immstool immsremote

libimms.so: $(XMMS_OBJ)
	$(CXX) $(XMMS_OBJ) \
		$(LDFLAGS) \
		-shared -Wl,-z,defs,-soname,$@ -o $@

libimmscore.a: $(call objects,..)
	$(AR) $(ARFLAGS) $@ $(filter %.o,$^)

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

dbus-server-CPPFLAGS=`pkg-config dbus-1 dbus-glib-1 --cflags` -DDBUS_API_SUBJECT_TO_CHANGE
dbus-server-LIBS=`pkg-config dbus-1 dbus-glib-1 --libs`
dbus-server: dbus-server.o

dbus-client-CPPFLAGS=`pkg-config dbus-1 dbus-glib-1 --cflags` -DDBUS_API_SUBJECT_TO_CHANGE
dbus-client-LIBS=`pkg-config dbus-1 dbus-glib-1 --libs`
dbus-client: dbus-client.o

realpath-test: libimmscore.a
