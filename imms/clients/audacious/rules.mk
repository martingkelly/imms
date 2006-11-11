AUDACIOUSCPPFLAGS=`pkg-config audacious --cflags` -I../clients/xmms/ -DAUDACIOUS
AUDACIOUSLDFLAGS=`pkg-config audacious glib --libs`
AUDACIOUSCOMMON=audaciousinterface.o clientstubbase.o libimmscore.a

libaudaciousimms.so: audaciousplugin.o $(AUDACIOUSCOMMON)
libaudaciousimms-LIBS = $(AUDACIOUSLDFLAGS)
libaudaciousimms2.so: audaciousplugin2.o $(AUDACIOUSCOMMON)
libaudaciousimms2-LIBS = $(AUDACIOUSLDFLAGS)

audaciousinterface-CPPFLAGS=$(AUDACIOUSCPPFLAGS)

audaciousinterface.o: bmpinterface.c
	$(call compile, $(CC), $<, $@, $($*-CFLAGS) $(CFLAGS) $($*-CPPFLAGS) $(CPPFLAGS))

audaciousplugin.o: plugin.cc
	$(call compile, $(CXX), $<, $@, $(CXXFLAGS) $(AUDACIOUSCPPFLAGS) $(CPPFLAGS))

audaciousplugin2.o: plugin2.cc
	$(call compile, $(CXX), $<, $@, $(CXXFLAGS) $(AUDACIOUSCPPFLAGS) $(CPPFLAGS))

AUDACIOUSDESTDIR=""
ifeq ($(shell id -u), 0)
	AUDACIOUSDESTDIR=`pkg-config --variable=general_plugin_dir audacious`
else
	AUDACIOUSDESTDIR=${HOME}/.audacious/Plugins/General
endif

libaudaciousimms.so_install: libaudaciousimms.so
	${INSTALL} -D $^ $(AUDACIOUSDESTDIR)/libaudaciousimms.so

libaudaciousimms2.so_install: libaudaciousimms2.so
	${INSTALL} -D $^ $(AUDACIOUSDESTDIR)/libaudaciousimms.so
