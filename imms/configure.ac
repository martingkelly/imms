AC_INIT([IMMS],[1.0-pre1],[mag@luminal.org])

AC_PREREQ([2.52g])

AC_REVISION($Id: configure.ac,v 1.30 2003/10/23 23:53:20 mag Exp $)

AC_CONFIG_SRCDIR(imms.cc)

AC_PREFIX_DEFAULT("/")

AC_ARG_WITH(id3lib,
            AC_HELP_STRING([--with-id3lib],
                           [Native MP3 tag support]))

AC_ARG_WITH(vorbis,
            AC_HELP_STRING([--with-vorbis],
                           [Native OGG/Vorbis tag support]))

AC_ARG_WITH(screensaver,
            AC_HELP_STRING([--with-screensaver],
                           [XScreenSaver extension]))

AC_ARG_ENABLE(debug,
            AC_HELP_STRING([--enable-debug],
                           [Extra debug output]),
            [enable_debug='yes' DEBUG_CFLAGS="-ggdb -O0 -DDEBUG"],
            [enable_debug='no' DEBUG_CFLAGS="-O3"])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

AC_LANG(C++)

AC_SUBST(enable_debug)
AC_MSG_CHECKING([for --enable-debug])
AC_MSG_RESULT([$enable_debug])

AC_CHECK_PROG(have_xmms_config, xmms-config, "yes", "no")
if test "$have_xmms_config" = "no"; then
    AC_MSG_ERROR([xmms-config required and missing.])
else
    CPPFLAGS=`xmms-config --cflags`
fi

AC_CHECK_HEADERS(xmms/plugin.h,, [with_xmms=no])
AC_CHECK_HEADERS(xmms/xmmsctrl.h,, [with_xmms=no])
if test "$with_xmms" = "no"; then
    AC_MSG_ERROR([xmms required and missing.])
fi

AC_CHECK_LIB(z, compress,, [with_zlib=no])
AC_CHECK_HEADERS(zlib.h,, [with_zlib=no])
if test "$with_zlib" = "no"; then
    AC_MSG_ERROR([zlib required and missing.])
fi

AC_CHECK_LIB(sqlite, sqlite_open,, [with_sqlite=no])
AC_CHECK_HEADERS(sqlite.h,, [with_sqlite=no])
if test "$with_sqlite" = "no"; then
    AC_MSG_ERROR([sqlite required and missing.])
fi

AC_CHECK_LIB(pcre, pcre_compile,, [with_pcre=no])
AC_CHECK_HEADERS(pcre.h,, [with_pcre_1=no])
if test "$with_pcre_1" = "no"; then
    AC_CHECK_HEADERS(pcre/pcre.h,, [with_pcre=no])
fi
if test "$with_pcre" = "no"; then
    AC_MSG_ERROR([PCRE required and missing.])
fi

if test "$with_id3lib" != "no"; then
    AC_CHECK_LIB(id3, ID3Tag_New,, [with_zlib=no])
    AC_CHECK_MEMBER(class ID3_Frame.GetField(ID3FN_EMAIL),,
        [with_id3lib=no], [#include <id3/tag.h>])
fi
if test "$with_id3lib" = "no"; then
    AC_MSG_WARN([id3lib is missing.])
else
    AC_DEFINE(WITH_ID3LIB,, [Native mp3 tag support])
fi

if test "$with_vorbis" != "no"; then
    AC_CHECK_LIB(vorbis, vorbis_comment_query,, [with_vorbis=no])
    AC_CHECK_LIB(vorbisfile, ov_comment,, [with_vorbis=no])
    AC_CHECK_HEADERS(vorbis/codec.h vorbis/vorbisfile.h,, [with_vorbis=no])
fi
if test "$with_vorbis" = "no"; then
    AC_MSG_WARN([Vorbis is missing.])
else
    AC_DEFINE(WITH_VORBIS,, [Native Vorbis tag support])
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
fi

AC_SUBST(CXX)
AC_SUBST(CC)
AC_SUBST(INSTALL)
AC_SUBST(DEBUG_CFLAGS)
AC_SUBST(LIBS)

AC_CONFIG_FILES(vars.mk)
AC_CONFIG_HEADERS(immsconf.h)
AC_OUTPUT

touch immsconf.h
