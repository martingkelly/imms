AUDACIOUSCPPFLAGS=`pkg-config audclient dbus-glib-1 --cflags` -I../clients/xmms/ -DAUDACIOUS
AUDACIOUSLDFLAGS=`pkg-config audclient glib dbus-glib-1 --libs`

libaudaciousimms.so: audplugin.o audaciousinterface.o clientstubbase.o libimmscore.a 
libaudaciousimms-LIBS = $(AUDACIOUSLDFLAGS)

audaciousinterface-CPPFLAGS=$(AUDACIOUSCPPFLAGS)
audplugin-CPPFLAGS=$(AUDACIOUSCPPFLAGS)

audaciousinterface.o: bmpinterface.c
	$(call compile, $(CC), $<, $@, $($*-CFLAGS) $(CFLAGS) $($*-CPPFLAGS) $(CPPFLAGS))

AUDACIOUSDESTDIR=""
ifeq ($(shell id -u), 0)
	AUDACIOUSDESTDIR=`pkg-config --variable=general_plugin_dir audacious`
else
	AUDACIOUSDESTDIR=${HOME}/.audacious/Plugins/General
endif

libaudaciousimms.so_install: libaudaciousimms.so
	${INSTALL} -D $^ $(AUDACIOUSDESTDIR)/libaudaciousimms.so
