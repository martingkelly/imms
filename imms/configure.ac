AC_INIT([IMMS],[1.99-rc1],[mag@luminal.org])

AC_PREREQ([2.52g])

AC_REVISION($Id: configure.ac,v 1.30 2003/10/23 23:53:20 mag Exp $)

AC_CONFIG_SRCDIR(immscore/imms.cc)

AC_PREFIX_DEFAULT("/usr")

AC_DEFUN(AC_APPEND,
    if test -z "$$1"; then
        $1="$2"
    else
        $1="$$1 $2"
    fi
)

AC_ARG_WITH(taglib,
            AC_HELP_STRING([--with-taglib],
                           [Tag support using TagLib]))

AC_ARG_WITH(id3lib,
            AC_HELP_STRING([--with-id3lib],
                           [Native MP3 tag support]))

AC_ARG_WITH(vorbis,
            AC_HELP_STRING([--with-vorbis],
                           [Native OGG/Vorbis tag support]))

AC_ARG_WITH(screensaver,
            AC_HELP_STRING([--with-screensaver],
                           [Use MIT ScreenSaver extension]))

AC_ARG_ENABLE(debug,
            AC_HELP_STRING([--enable-debug],
                           [Extra debug output]),
            [enable_debug='yes' XCPPFLAGS="-ggdb -O0 -DDEBUG"],
            [enable_debug='no' XCPPFLAGS="-O3"])

AC_ARG_ENABLE(queuecontrol,
            AC_HELP_STRING([--disable-queuecontrol],
                           [Don't use XMMS queue control functions]),
            [enable_queuecontrol='no'],
            [enable_queuecontrol='yes'])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

AC_LANG(C++)

AC_MSG_CHECKING([for --enable-debug])
AC_MSG_RESULT([$enable_debug])

AC_CHECK_LIB(z, compress,, [with_zlib=no])
AC_CHECK_HEADERS(zlib.h,, [with_zlib=no])
if test "$with_zlib" = "no"; then
    AC_MSG_ERROR([zlib required and missing.])
fi

AC_CHECK_PROG(have_sqlites, sqlite, "yes", "no")
if test "$have_sqlites" = "yes"; then
    AC_CHECK_PROG(have_sqlites, sqlite3, "yes", "no")
fi
if test "$have_sqlites" = "no"; then
    AC_MSG_WARN([******************************************************]);
    AC_MSG_WARN([If you are upgrading from IMMS version <= 1.1]);
    AC_MSG_WARN([You need both sqlite and sqlite3 executables installed]);
    AC_MSG_WARN([******************************************************]);
fi

AC_CHECK_LIB(sqlite3, sqlite3_open,, [with_sqlite=no])
AC_CHECK_HEADERS(sqlite3.h,, [with_sqlite=no])
if test "$with_sqlite" = "no"; then
    AC_MSG_ERROR([sqlite >= 3.0 required and missing.])
fi

AC_CHECK_LIB(pcre, pcre_compile,, [with_pcre=no])
AC_CHECK_HEADERS(pcre.h,, [with_pcre_1=no])
if test "$with_pcre_1" = "no"; then
    AC_CHECK_HEADERS(pcre/pcre.h,, [with_pcre=no])
fi
if test "$with_pcre" = "no"; then
    AC_MSG_ERROR([PCRE required and missing.])
fi

AC_CHECK_PROG(have_pkgconfig, pkg-config, "yes", "no")
if test "$have_packageconfig" = "no"; then
    AC_MSG_ERROR([pkg-config required and missing])
fi

saved_cppflags="$CPPFLAGS"
saved_libs="$LIBS"

CPPFLAGS="`pkg-config dbus-1 --cflags` -DDBUS_API_SUBJECT_TO_CHANGE"
AC_CHECK_HEADERS(dbus/dbus.h,, [have_dbus=no])
if test "$have_dbus" != "no"; then
    LIBS="`pkg-config dbus-1 --libs`"
    AC_CHECK_LIB(dbus-1, dbus_connection_send,, [have_dbus=no])
fi
if test "$have_dbus" = "no"; then
    AC_MSG_ERROR([D-BUS 0.22 required and missing.])
fi

CPPFLAGS="`pkg-config dbus-glib-1 --cflags`"
AC_CHECK_HEADERS(dbus/dbus-glib.h,, [have_dbus_glib=no])
if test "$have_dbus_glib" != "no"; then
    LIBS="`pkg-config dbus-glib-1 --libs`"
    AC_CHECK_LIB(dbus-glib-1, dbus_connection_setup_with_g_main,,
                 [have_dbus_glib=no])
fi
if test "$have_dbus_glib" = "no"; then
    AC_MSG_ERROR([D-BUS GLib bindings required and missing.])
fi

AC_CHECK_HEADERS(fftw3.h,, [with_fftw=no])
if test "$with_fftw" != "no"; then
    LIBS="`pkg-config fftw3f --libs`"
    AC_CHECK_LIB(fftw3f, fftwf_plan_dft_r2c_1d,, [with_fftw=no])
fi
if test "$with_fftw" = "no"; then
    AC_MSG_ERROR([FFTW 3.0 required and missing.])
fi

LIBS=$saved_libs
CPPFLAGS=$saved_cppflags

if test "$with_taglib" != "no"; then
    AC_CHECK_PROG(with_taglib, taglib-config, "yes", "no")
fi
if test "$with_taglib" != "no"; then
    AC_MSG_CHECKING([taglib usability])

    saved_libs="$LIBS"
    LIBS="$LIBS -ltag"
    CPPFLAGS=`taglib-config --cflags`

    AC_TRY_LINK([#include <tag.h>
                 #include <fileref.h>],
                [TagLib::FileRef f((TagLib::File*)0); f.tag()->title()],
        [TAGCPPFLAGS="$TAGCPPFLAGS `taglib-config --cflags`" 
                                            AC_MSG_RESULT(yes)],
        [LIBS=$saved_libs AC_MSG_RESULT(no)])
fi

if test "$with_taglib" = "yes"; then
    AC_DEFINE(WITH_TAGLIB,, [Tag support using TagLib])
else
    if test "$with_id3lib" != "no"; then
        AC_CHECK_LIB(id3, ID3Tag_New,, [with_id3lib=no])
        AC_CHECK_HEADERS(id3/tag.h,, [with_id3lib=no])
    fi
    if test "$with_id3lib" = "no"; then
        AC_MSG_WARN([id3lib is missing.])
    else
        AC_DEFINE(WITH_ID3LIB,, [Native mp3 tag support])
    fi

    if test "$with_vorbis" != "no"; then
        AC_CHECK_LIB(vorbis, vorbis_comment_query,, [with_vorbis=no])
        AC_CHECK_LIB(vorbisfile, ov_comment,, [with_vorbis=no])
        AC_CHECK_HEADERS(vorbis/codec.h vorbis/vorbisfile.h,,
                         [with_vorbis=no])
    fi
    if test "$with_vorbis" = "no"; then
        AC_MSG_WARN([Vorbis is missing.])
    else
        AC_DEFINE(WITH_VORBIS,, [Native Vorbis tag support])
    fi
fi

AC_CHECK_PROG(have_sox, sox, "yes", "no")
if test "$have_sox" = "no"; then
    AC_MSG_WARN([******************************************************]);
    AC_MSG_WARN([To benefit from acoustic analysis features of IMMS],);
    AC_MSG_WARN([you need to install sox [http://sox.sourceforge.net/]]);
    AC_MSG_WARN([******************************************************]);
fi

if test "$with_screensaver" != "no"; then
    LDFLAGS="-shared -L/usr/X11R6/lib"
    AC_CHECK_LIB(Xext, XextFindDisplay,, [with_screensaver=no])
    AC_CHECK_LIB(Xss, XScreenSaverQueryInfo,, [with_screensaver=no])
    AC_CHECK_HEADERS(X11/extensions/scrnsaver.h,,
                     [with_screensaver=no], [ #include <X11/Xlib.h>] )
fi
if test "$with_screensaver" = "no"; then
    AC_MSG_WARN([XScreenSaver is missing.])
else
    AC_DEFINE(WITH_XSCREENSAVER,, [XScreenSaver extension])
    AC_APPEND(LIBS, -L/usr/X11R6/lib)
fi

PLUGINS=""
AC_CHECK_PROG(with_xmms, xmms-config, "yes", "no")
if test "$xmms_config" != "no"; then
    CPPFLAGS=`xmms-config --cflags`
    AC_CHECK_HEADERS(xmms/plugin.h,, [with_xmms=no])
    AC_CHECK_HEADERS(xmms/xmmsctrl.h,, [with_xmms=no])
fi

if test "$with_xmms" = "no"; then
    AC_MSG_WARN([Not building XMMS plugin])
else 
    if test "$enable_queuecontrol" != "no"; then
        AC_CHECK_LIB(xmms, xmms_remote_playqueue_add,,
                     [enable_queuecontrol=no])
    fi
    if test "$enable_queuecontrol" = "no"; then
        AC_APPEND(PLUGINS, libxmmsimms.so)
    else
        AC_DEFINE(HAVE_QUEUE_CONTROL,,
                  [XMMS provides functions to control the play queue])
        AC_APPEND(PLUGINS, libxmmsimms2.so)
    fi
fi

AC_MSG_CHECKING([which plugins to build])
AC_MSG_RESULT($PLUGINS)

AC_SUBST(CXX)
AC_SUBST(CC)
AC_SUBST(INSTALL)
AC_SUBST(XCPPFLAGS)
AC_SUBST(TAGCPPFLAGS)
AC_SUBST(LIBS)
AC_SUBST(PLUGINS)

AC_CONFIG_FILES(vars.mk)
AC_CONFIG_HEADERS(immsconf.h)
AC_OUTPUT

touch immsconf.h
