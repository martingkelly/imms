#
# IMMS: Intelligent Multimedia Management System
# Copyright (C) 2001-2009 Michael Grigoriev
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
AC_INIT([IMMS],[3.1.0-rc10],[mag@luminal.org])

AC_PREREQ([2.52g])

AC_REVISION($Id: configure.ac,v 1.30 2003/10/23 23:53:20 mag Exp $)

AC_CONFIG_SRCDIR(immscore/imms.cc)

AC_PREFIX_DEFAULT("/usr")

AC_DEFUN([AC_APPEND],
    [if test -z "$$1"; then
        $1="$2"
    else
        $1="$$1 $2"
    fi
])

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
            [enable_debug=$enableval])

AC_ARG_ENABLE(queuecontrol,
            AC_HELP_STRING([--disable-queuecontrol],
                           [Don't use XMMS queue control functions]),
            [enable_queuecontrol=$enableval])

AC_ARG_ENABLE(analyzer,
            AC_HELP_STRING([--disable-analyzer],
                           [Disable acoustic song similarity analyzer]),
            [enable_analyzer=$enableval])

AC_ARG_ENABLE(immsremote,
            AC_HELP_STRING([--disable-immsremote],
                           [Disable building of immsremote]),
            [enable_immsremote=$enableval])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

AC_LANG(C++)

AC_MSG_CHECKING([for --enable-debug])

XCPPFLAGS="-ggdb -O0 -DDEBUG"
if test "$enable_debug" != "yes"; then
    XCPPFLAGS="-O3"
    AC_MSG_RESULT([no])
else
    AC_MSG_RESULT([yes])
fi

AC_MSG_CHECKING([for usability of initstate_r])

AC_TRY_COMPILE([#include <stdlib.h>],[
  static struct random_data rand_data;
  static char rand_state[256];
  initstate_r(0, rand_state, sizeof(rand_state), &rand_data);
  exit(0);
], initstate_r_works=yes,)

if test "$initstate_r_works" != "yes"; then
    AC_MSG_RESULT([no])
else
    AC_MSG_RESULT([yes])
    AC_DEFINE(INITSTATE_USABLE,, [initstate_r is usable])
fi

AC_CHECK_LIB(z, compress,, [with_zlib=no])
AC_CHECK_HEADERS(zlib.h,, [with_zlib=no])
if test "$with_zlib" = "no"; then
    AC_MSG_ERROR([zlib required and missing.])
fi

AC_CHECK_LIB(sqlite3, sqlite3_get_autocommit,, [with_sqlite=no])
AC_CHECK_HEADERS(sqlite3.h,, [with_sqlite=no])
if test "$with_sqlite" = "no"; then
    AC_MSG_ERROR([sqlite >= 3.2.2 required and missing.])
fi

PKG_CHECK_MODULES([pcre], [libpcre], [], [with_pcre=no])
if test "$with_pcre" = "no"; then
    AC_MSG_ERROR([PCRE required and missing.])
else
    LIBS="$LIBS $pcre_LIBS"
    CPPFLAGS="$CPPFLAGS $pcre_CFLAGS"
fi

if test "$enable_analyzer" != "no"; then
    PKG_CHECK_MODULES([FFTW], [fftw3 >= 3.0],
                      [], [have_fftw=no])
    if test "$have_fftw" = "no"; then
        AC_MSG_WARN([******************************************************]);
        AC_MSG_WARN("fftw3 >= 3.0 required for analyzer and missing");
        AC_MSG_WARN("Acoustic analyzer will not be built");
        AC_MSG_WARN([******************************************************]);
        enable_analyzer=no
    fi
fi

if test "$enable_analyzer" != "no"; then
    AC_CHECK_HEADERS(torch/KMeans.h,, [have_torch=no])

    saved_libs="$LIBS"
    LIBS="$LIBS -ltorch"
    AC_MSG_CHECKING([libtorch usability])
    AC_TRY_LINK([#include <torch/KMeans.h> ],
                [Torch::KMeans kmeans(10, 5)],
        [AC_MSG_RESULT(yes)],
        [have_torch=no; AC_MSG_RESULT(no)])
    
    if test "$have_torch" = "no"; then
        AC_MSG_WARN([******************************************************]);
        AC_MSG_WARN("torch >= 3.0 required for analyzer and missing");
        AC_MSG_WARN("Acoustic analysis engine will not be built");
        AC_MSG_WARN([******************************************************]);
        enable_analyzer=no
        LIBS=$saved_libs
    else
        AC_DEFINE(WITH_TORCH,, [libtorch support])
    fi
fi

AC_CHECK_TOOL(OBJCOPY, objcopy)
if test "x$OBJCOPY" = "x"; then
    AC_MSG_ERROR("objcopy from GNU binutils >= 2.11.90 not found")
fi
AC_CHECK_TOOL(OBJDUMP, objdump)
dnl extract target and architecture if objdump was found
if test "x$OBJDUMP" = "x"; then :; else
    AC_MSG_CHECKING(for object target)
    octarget=`$OBJDUMP --info | sed -ne '2p'` # extract first target
    AC_MSG_RESULT($octarget)
    AC_MSG_CHECKING(for object architecture)
    ocarch=`$OBJDUMP --info | sed -ne '4p'` # extract corresponding arch
    AC_MSG_RESULT($ocarch)
fi
AC_SUBST(OBJCOPYTARGET, $octarget)
AC_SUBST(OBJCOPYARCH, $ocarch)

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.0],
                  [], [AC_MSG_ERROR([Required packages missing.])])

saved_cppflags="$CPPFLAGS"
saved_libs="$LIBS"

if test "$enable_analyzer" != "no"; then
    CPPFLAGS="`pkg-config fftw3 --cflags`"
    LIBS="`pkg-config fftw3 --libs`"
    AC_CHECK_HEADERS(fftw3.h,, [with_fftw=no])
    AC_CHECK_LIB(fftw3, fftw_plan_dft_r2c_1d,, [with_fftw=no])
    if test "$with_fftw" = "no"; then
        AC_MSG_WARN([******************************************************]);
        AC_MSG_WARN("fftw3 >= 3.0 required for analyzer and missing");
        AC_MSG_WARN("Acoustic analyzer will not be built");
        AC_MSG_WARN([******************************************************]);
        enable_analyzer=no
    fi
fi

CPPFLAGS="`pkg-config glib-2.0 --cflags`"
LIBS="`pkg-config glib-2.0 --libs`"
AC_CHECK_HEADERS(glib.h,, [with_glib=no])
AC_CHECK_LIB(glib-2.0, g_timeout_source_new,, [with_glib=no])
if test "$with_glib" = "no"; then
    AC_MSG_ERROR([glib 2.0 required and missing.])
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
        [LIBS=$saved_libs; AC_MSG_RESULT(no)])
fi

CPPFLAGS=$saved_cppflags

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

if test "$enable_analyzer" != "no"; then
    AC_CHECK_PROG(have_sox, sox, "yes", "no")
    if test "$have_sox" = "no"; then
        AC_MSG_WARN([******************************************************]);
        AC_MSG_WARN([To benefit from acoustic analysis features of IMMS],);
        AC_MSG_WARN([you need to install sox [http://sox.sourceforge.net/]]);
        AC_MSG_WARN([******************************************************]);
    fi
fi

if test "$with_screensaver" != "no"; then
    LDFLAGS="-L/usr/X11R6/lib"
    AC_CHECK_LIB(X11, XFlush,, [with_screensaver=no])
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

if test "$enable_immsremote" != "no"; then
    PKG_CHECK_MODULES([IMMSREMOTE], [libglade-2.0 >= 2.0 gtk+-2.0 >= 2.4],
                      [], [enable_immsremote=no])
    if test "$enable_immsremote" = "no"; then
        AC_MSG_WARN([******************************************************]);
        AC_MSG_WARN("immsremote will not be built");
        AC_MSG_WARN([******************************************************]);
        immsremote=no
    else
        AC_APPEND(OPTIONAL, immsremote)
    fi
fi

saved_cppflags="$CPPFLAGS"
saved_libs="$LIBS"

PLUGINS=""
AC_CHECK_PROG(with_xmms, xmms-config, "yes", "no")
if test "$with_xmms" != "no"; then
    CPPFLAGS=`xmms-config --cflags`
    AC_CHECK_HEADERS(glib.h,, [with_xmms=no]) # how to disable caching for this?
    AC_CHECK_HEADERS(gtk/gtk.h,, [with_xmms=no])
    AC_CHECK_HEADERS(xmms/plugin.h,, [with_xmms=no])
    AC_CHECK_HEADERS(xmms/xmmsctrl.h,, [with_xmms=no])
fi

xmms_enable_playqueue=$enable_queuecontrol
if test "$with_xmms" = "no"; then
    AC_MSG_WARN([Not building XMMS plugin])
else 
    if test "$xmms_enable_playqueue" != "no"; then
        AC_MSG_CHECKING([for xmms_remote_playqueue_add in -lxmms])
        saved_libs="$LIBS"
        LIBS="$LIBS -lxmms"
        AC_TRY_LINK([#include <xmms/xmmsctrl.h>],
                    [xmms_remote_playqueue_add(0, 0)],
                    [AC_MSG_RESULT(yes)],
                    [xmms_enable_playqueue="no"
                         AC_MSG_RESULT(no)]);
        LIBS="$saved_libs"
    fi
    if test "$xmms_enable_playqueue" = "no"; then
        AC_APPEND(PLUGINS, libxmmsimms.so)
    else
        AC_DEFINE(XMMS_HAVE_QUEUE_CONTROL,,
                  [XMMS provides functions to control the play queue])
        AC_APPEND(PLUGINS, libxmmsimms2.so)
    fi
fi

PKG_CHECK_MODULES(BMP, [bmp >= 0.9.7], [], [with_bmp=no])

LIBS=$BMP_LIBS
CPPFLAGS=$BMP_CFLAGS

AC_CHECK_HEADERS(bmp/plugin.h,, [with_bmp=no])
AC_CHECK_HEADERS(bmp/beepctrl.h,, [with_bmp=no])

bmp_enable_playqueue=$enable_queuecontrol
if test "$with_bmp" = "no"; then
    AC_MSG_WARN([Not building BMP plugin])
else 
    if test "$bmp_enable_playqueue" != "no"; then
        AC_MSG_CHECKING([for xmms_remote_playqueue_add in -lbeep])
        AC_TRY_LINK([#include <bmp/beepctrl.h>],
                    [xmms_remote_playqueue_add(0, 0)],
                    [AC_MSG_RESULT(yes)],
                    [bmp_enable_playqueue="no"
                         AC_MSG_RESULT(no)]);
    fi
    if test "$bmp_enable_playqueue" = "no"; then
        AC_APPEND(PLUGINS, libbmpimms.so)
    else
        AC_DEFINE(BMP_HAVE_QUEUE_CONTROL,,
                  [BMP provides functions to control the play queue])
        AC_APPEND(PLUGINS, libbmpimms2.so)
    fi
fi

PKG_CHECK_MODULES(audacious, [audacious >= 1.4.0], [], [with_audacious=no])
PKG_CHECK_MODULES(dbus_glib, [dbus-glib-1 >= 0.70], [], [with_audacious=no])

LIBS="$audacious_LIBS $dbus_glib_LIBS"
CPPFLAGS="$audacious_CFLAGS $dbus_glib_CFLAGS"

AC_CHECK_HEADERS(audacious/plugin.h,, [with_audacious=no])
AC_CHECK_HEADERS(audacious/auddrct.h,, [with_audacious=no])

if test "$with_audacious" != "no"; then
    AC_MSG_CHECKING([for audacious_drct_playqueue_add in -laudacious])
    AC_TRY_LINK([#include <audacious/auddrct.h>
                 #include <audacious/plugin.h>
                 SIMPLE_GENERAL_PLUGIN(0, 0);],
                [audacious_drct_playqueue_add(0)],
                [AC_MSG_RESULT(yes)],
                [with_audacious="no"
                 AC_MSG_RESULT(no)]);
fi

if test "$with_audacious" = "no"; then
    AC_MSG_WARN([Not building audacious plugin])
else 
    AC_APPEND(PLUGINS, libaudaciousimms.so)
fi

if test "$enable_analyzer" != "no"; then
    AC_APPEND(OPTIONAL, analyzer)
    AC_DEFINE(ANALYZER_ENABLED,, [Analyzer enabled])
fi

LIBS=$saved_libs
CPPFLAGS=$saved_cppflags

AC_MSG_CHECKING([which plugins to build])
AC_MSG_RESULT($PLUGINS)

AC_MSG_CHECKING([which optional packages to build])
AC_MSG_RESULT($OPTIONAL)

AC_SUBST(CXX)
AC_SUBST(CC)
AC_SUBST(INSTALL)
AC_SUBST(XCPPFLAGS)
AC_SUBST(TAGCPPFLAGS)
AC_SUBST(LIBS)
AC_SUBST(PLUGINS)
AC_SUBST(OPTIONAL)

AC_CONFIG_FILES(vars.mk)
AC_CONFIG_HEADERS(immsconf.h)
AC_OUTPUT

touch immsconf.h
