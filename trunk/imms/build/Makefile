-include ../vars.mk
-include .*.d

.PHONY: first

first: configure
	 $(warning Please run the "configure" script)

include ../rules.mk

all: libimms.so analyzer immstool immsremote

libimms.so: $(XMMS_OBJ) immsconf.h
	$(CXX) $(XMMS_OBJ) \
		$(LDFLAGS) \
		-shared -Wl,-z,defs,-soname,$@ -o $@

libimmscore.a: $(call objects,..) immsconf.h
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
