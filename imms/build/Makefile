-include ../vars.mk
-include .*.d

.PHONY: first

first: ../configure
	 $(warning Please run the "configure" script)

include ../rules.mk

all: immsd immstool $(OPTIONAL) $(PLUGINS)

training: training_data train_model

libimmscore.a: $(call objects,../immscore)
	$(AR) $(ARFLAGS) $@ $(filter %.o,$^)

libmodel.a: $(call objects,../model) svm-similarity-data.o
	$(AR) $(ARFLAGS) $@ $(filter %.o,$^)

immstool: immstool.o libmodel.a libimmscore.a mfcckeeper.o
training_data: training_data.o libmodel.a libimmscore.a 
train_model: train_model.o libmodel.a libimmscore.a 

analyzer: $(call objects,../analyzer)
analyzer: libimmscore.a libmodel.a
analyzer-LIBS=`pkg-config fftw3 --libs`

autotag: $(call objects,../autotag)
autotag: libimmscore.a
autotag-CPPFLAGS=$(TAGCPPFLAGS)
autotag-LIBS=-lmusicbrainz -ltag

immsremote: $(call objects,../immsremote)
immsremote: libimmscore.a
immsremote-CPPFLAGS=`pkg-config --cflags gtk+-2.0 libglade-2.0` -DDATADIR=\"$(datadir)\"
immsremote-LIBS=`pkg-config --libs gtk+-2.0 libglade-2.0` -rdynamic

songinfo-CPPFLAGS=$(TAGCPPFLAGS)
socketserver-CPPFLAGS=$(GLIB2CPPFLAGS)

immsd: libimmscore.a libmodel.a
immsd: $(call objects,../immsd)
immsd-CPPFLAGS=$(GLIB2CPPFLAGS)
immsd-LIBS=$(GLIB2LDFLAGS)

PLUGINS_INSTALL=$(patsubst %,%_install,$(PLUGINS))
plugins_install: $(PLUGINS_INSTALL)

analyzer_install: analyzer
	${INSTALL} -D $< $(bindir)/$<

autotag_install: autotag
	${INSTALL} -D $< $(bindir)/$<

immsremote_install: immsremote glade_install
	${INSTALL} -D $< $(bindir)/$<

glade_install: ../immsremote/immsremote.glade
	${INSTALL} -m 644 -D $< $(datadir)/imms/immsremote.glade
