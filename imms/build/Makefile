-include ../vars.mk
-include .*.d

.PHONY: first

first: ../configure
	 $(warning Please run the "configure" script)

include ../rules.mk

all: immsd immstool $(OPTIONAL) $(PLUGINS)

libimmscore.a: $(call objects,../immscore)
	$(AR) $(ARFLAGS) $@ $(filter %.o,$^)

immstool: immstool.o distance.o emd.o libimmscore.a 

analyzer: $(call objects,../analyzer)
analyzer: libimmscore.a
analyzer-LIBS=`pkg-config fftw3 --libs` -ltorch

classifier: $(call objects,../classifier)
classifier: beatkeeper.o libimmscore.a
classifier-LIBS=-ltorch

songinfo-CPPFLAGS=$(TAGCPPFLAGS)
socketserver-CPPFLAGS=$(GLIB2CPPFLAGS)

immsd: libimmscore.a
immsd: $(call objects,../immsd)
immsd-CPPFLAGS=$(GLIB2CPPFLAGS)
immsd-LIBS=$(GLIB2LDFLAGS)

PLUGINS_INSTALL=$(patsubst %,%_install,$(PLUGINS))
plugins_install: $(PLUGINS_INSTALL)
