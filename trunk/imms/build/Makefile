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

autotag: $(call objects,../autotag)
autotag: libimmscore.a
autotag-LIBS=-lmusicbrainz

immsremote: $(call objects,../immsremote)
immsremote: libimmscore.a
immsremote-CPPFLAGS=`pkg-config --cflags gtk+-2.0 libglade-2.0`
immsremote-LIBS=`pkg-config --libs gtk+-2.0 libglade-2.0`

classifier: $(call objects,../classifier)
classifier: beatkeeper.o libimmscore.a
classifier-LIBS=-ltorch

songinfo-CPPFLAGS=$(TAGCPPFLAGS)
socketserver-CPPFLAGS=$(GLIB2CPPFLAGS)

immsd: libimmscore.a
immsd: $(call objects,../immsd)
immsd: distance.o emd.o
immsd-CPPFLAGS=$(GLIB2CPPFLAGS)
immsd-LIBS=$(GLIB2LDFLAGS)

PLUGINS_INSTALL=$(patsubst %,%_install,$(PLUGINS))
plugins_install: $(PLUGINS_INSTALL)
