-include vars.mk
-include .*.d

.PHONY: first

first: configure
	 $(warning Please run the "configure" script)

include rules.mk

all: libimms.so

libimms.so: $(XMMS_OBJ) immsconf.h
	$(CXX) $(XMMS_OBJ) \
		$(LDFLAGS) \
		-shared -Wl,-z,defs,-soname,$@ -o $@

immstool-LIBS = libimmscore.a

sqlite_speed_test-LIBS = libimmscore.a

immsremote-OBJ = comm.o
immsremote-LIBS = -lreadline -lcurses

libimmscore.a: $(CORE_OBJ) immsconf.h
	$(AR) $(ARFLAGS) $@ $(CORE_OBJ)

analyzer: analyzer/analyzer

analyzer/analyzer-LIBS=`pkg-config fftw3f --libs`

analyzer/analyzer: $(call objects,analyzer)
analyzer/analyzer: utils.o
