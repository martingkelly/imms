-include ../vars.mk
-include .*.d

.PHONY: first

first: configure
	 $(warning Please run the "configure" script)

include ../rules.mk

all: libimms.so

libimms.so: $(XMMS_OBJ) immsconf.h
	$(CXX) $(XMMS_OBJ) \
		$(LDFLAGS) \
		-shared -Wl,-z,defs,-soname,$@ -o $@

immstool: libimmscore.a

sqlite_speed_test-LIBS = libimmscore.a

immsremote: immsremote.o comm.o 
immsremote: -lreadline -lcurses

libimmscore.a: $(call objects,../) immsconf.h
	$(AR) $(ARFLAGS) $@ $(filter %.o,$^)

analyzer-LIBS=`pkg-config fftw3f --libs`

analyzer: $(call objects,../analyzer)
analyzer: utils.o
