# Building IMMS on Windows #

IMMS on Windows is a work in progress. The main options are compiling using Cygwin or MinGW. MinGW is preferred as this results in a native executable. Building IMMS under Cygwin means people have to install cygwin on their system before they can run IMMS, adding an unnecessary dependency.


This has only been tested on Windows XP (SP3).

## Building with MinGW ##

**Note:** if you have both cygwin and mingw installed on your machine this may cause problems, as mingw may pick up some paths from the Windows environment variable. When you launch the MSYS terminal, check your path ('set path'). You may want to change your path to not include anything with cygwin!

See also "MinGW related" at the MinGW FAQ - http://www.mingw.org/wiki/FAQ

**Note:** IMMS on mingw has only been tested for compilation, not running. Still working on that.


**1.** Download mingw from mingw.org. The files might be at http://sourceforge.net/projects/mingw/files_beta/Automated%20MinGW%20Installer/ . You want the Automated Installer.

When installing, install the base system with C compiler, the C++ compiler, the fortran compiler, MSYS, and MinGW Developer Toolkit. See http://www.mingw.org/wiki/Getting_Started .

**2.** Download the IMMS code to a folder inside the Mingw installation, such as ~/imms (i.e. C:\MinGW\msys\1.0\home\{yourusername}\imms , if you installed Mingw to C:\Mingw).

**3.** Open a new MSYS prompt (select 'MinGW Shell' from the Start menu) and cd into the directory with the IMMS code.

**4.** Get ready to download some requirements! :D

**a) zlib** http://www.zlib.net/
  * cd into the folder
  * If you try to run ./configure from mingw you will probably get "Please use win32/Makefile.gcc instead."
  * Do "make -f win32/Makefile.gcc"
  * You will need to add some lines to your path before you can install zlib. Try either of these options. Either way you will run the same make command: 'make install -f win32/Makefile.gcc'

option 1: prepend the path variable before you call make:
>'BINARY\_PATH=/mingw/bin INCLUDE\_PATH=/mingw/include LIBRARY\_PATH=/mingw/lib make install -f win32/Makefile.gcc'

option 2: add the path definitions inside your win32/Makefile.gcc file:

> BINARY\_PATH=/mingw/bin<br>
<blockquote>INCLUDE_PATH=/mingw/include<br>
LIBRARY_PATH=/mingw/lib<br></blockquote>

, then install zlib with 'make install -f win32/Makefile.gcc'<br>
<br>
<br>
<b>b) sqlite</b> <a href='http://www.sqlite.org/download.html'>http://www.sqlite.org/download.html</a>

<ul><li>you probably want the 'amalgamation' tar.gz, that contains the configure script and makefile<br>
</li><li>everything should compile fine in mingw with sqlite 3.7.3, but we need to specify different installation paths for the header files and libraries/dlls. Instead of just './configure', run:</li></ul>

<blockquote>./configure --prefix=/mingw<br>
make<br>
make install<br></blockquote>

<b>c) glib.</b> <a href='http://www.gtk.org/download-windows.html'>http://www.gtk.org/download-windows.html</a>

You want the latest 'Sources' for glib.<br>
<br>
Note: If you grabbed autotools as part of the msysDTK rather than using the MinGW Developer Toolkit you may need to compile libiconv and gettext before you can build glib. If so see instructions in the "Other ways to get autotools on MinGW" section below.<br>
<br>
Note: it's possible that this could be done with just the glib Dev files. Feel free to try and let the list know how it goes.<br>
<br>
<blockquote>./configure --prefix=/mingw<br>
make<br>
make install<br></blockquote>


<b>d) pkg-config.</b> <a href='http://pkgconfig.freedesktop.org/releases/'>http://pkgconfig.freedesktop.org/releases/</a>

Get the latest release; this guide used 0.25.<br>
<br>
<blockquote>./configure --prefix=/mingw<br>
make<br>
make install<br></blockquote>

<b>e) pcre</b> <a href='http://sourceforge.net/projects/pcre/files/'>http://sourceforge.net/projects/pcre/files/</a>

your favourite mantra again:<br>
<br>
<blockquote>./configure --prefix=/mingw<br>
make<br>
make install<br></blockquote>

note: if you don't have pkg-config installed first, you might see an error like<br>
<br>
<blockquote>"checking for pkg-config... no<br>
checking for pcre... configure: error: PCRE required and missing."</blockquote>

this is actually an error with pkg-config, not pcre. to fix, install pkg-config first.<br>
<br>
<b>f) fftw</b>

It's no problem! Wonderful fftw users have already practised compiling under mingw :)<br>
<br>
<ul><li>Download from <a href='http://www.fftw.org/download.html'>http://www.fftw.org/download.html</a>
</li><li>View the mingw steps on <a href='http://www.fftw.org/install/windows.html'>http://www.fftw.org/install/windows.html</a></li></ul>

Basically we use their recommended switches and add our prefix<br>
<br>
<blockquote>./configure --with-our-malloc16 --with-windows-f77-mangling --enable-shared --disable-static --enable-threads --with-combined-threads --enable-portable-binary --enable-sse2 --prefix=/mingw<br>
make<br>
make install<br></blockquote>

making fftw may take a while.<br>
<br>
<b>g) torch</b>

We're using torch3 - download 'Torch3 src' (Torch3src.tgz) from <a href='http://www.torch.ch/downloads.php'>http://www.torch.ch/downloads.php</a>

<b>Note:</b> getting torch compiled is a bit tricky. This uses the source code and gnu make options. It may be easier to try xmake, but that involves installing python; it may be easier to build with visual c++ as well, but that requires visual c++. Feel free to experiment and ping the list with your results.<br>
<br>
<br>
Torch doesn't compile on mingw by default; we get to edit the makefile. To fix the makefiles you have two choices:<br>
<ol><li>Edit the makefiles and change "OS := $(shell uname -s)" to "OS := Linux". This will need to be done in each subdirectory where you want to build Torch components (in our case: 'core', 'distributions', and 'gradients').<br>
</li><li>Copy the "Makefile_options_Linux" makefile and rename it to "Makefile_options_MINGW32_NT-5.1" (where "MINGW32_NT-5.1" is the name reported by 'uname -s' in your MSYS terminal). This will need to be done in each subdirectory where you want to build Torch components (in our case: 'core', 'distributions', and 'gradients').</li></ol>

(I have contacted the Torch owners to see if they will accept a patch in the form of a mingw makefile. We'll see)<br>
<br>
<br>
For this guide we'll be doing the second option.<br>
<br>
<ul><li>Next, change all of the lines in the Makefiles that read "@\rm -Rf to "@rm -Rf". These are mostly in the 'clean:' directive. If you leave in the '\', make will give you errors like "<code>make[1]</code>: \rm: Command not found". This will need to be done in each subdirectory where you want to build Torch components ('core', 'distributions', and 'gradients').<br>
</li><li>Also change all lines that read "@\mkdir" to "@mkdir", or you'll get the same error. These are mostly in the "depend:" section. This also has to be done in each file.</li></ul>

<ul><li>Next, edit the Makefile_options_g++ file(s):<br>
<ul><li>Remove the "-mcpu=i686" flag from CFLAGS_OPT_DOUBLE and CFLAGS_OPT_FLOAT. otherwise gcc will tell you "-mcpu is deprecated".<br>
</li><li>Add the proper include directory to MYINCS; in this case "MYINCS = -I/mingw/include". TODO: may change if our directories change. try from scratch.</li></ul></li></ul>

<ul><li>Edit the Makefile_options_MINGW32_NT-5.1 file (or Makefile_options_Linux file):<br>
<ul><li>Remove the "-mcpu=i686" flag from CFLAGS_OPT_DOUBLE and CFLAGS_OPT_FLOAT.<br>
</li><li>Add the proper include directory to MYINCS; in this case "MYINCS = -I/mingw/include". TODO: may change if our directories change.<br>
</li><li>Change the PACKAGES line to include the extra packages we need for IMMS: "PACKAGES = distributions gradients".</li></ul></li></ul>


<ul><li>HACK: currently Torch3/core/Timer.cc doesn't compile because it needs some functions in sys/time.h, which are not provided by MinGW. Fortunately, we don't use any Timer stuff inside IMMS (TODO: right?), so we can work around this rather than fixing it. Because of the way the Torch makefiles work, if you remove the Timer.cc file or rename it to something other than '.cc', Torch will ignore it. So do that now.<br><br>Long term: we could try using gnulib to patch in support for the needed functions. That's a lot for something we don't use though.</li></ul>


<ul><li>We should now be ready. Build torch by running 'make depend' and then 'make'. They may take a while.<br>
</li><li>Torch3 doesn't have a 'make install' option, so we get to install the files ourselves :P (Note: I have contacted the owners to see if they will accept a patch for this too).<br>
<ul><li>Copy the header files from the 'core' (except Timer.h :P), 'distributions', and 'gradients' folders to /mingw/include/torch/ (C:\MinGW\include\torch on Windows).<br>
</li><li>Copy the compiled 'libtorch.a' library file from Torch3/libl/MINGW32_NT-5.1_OPT_FLOAT (or Torch3/liblLinux_OPT_FLOAT, if you used the Linux makefile) to /mingw/lib (C:\MinGW\lib in Windows).</li></ul></li></ul>


<b>h) sox</b> <a href='http://sox.sourceforge.net/'>http://sox.sourceforge.net/</a>

Easy. Grab the source code, then your favourite mantra<br>
<br>
<blockquote>./configure<br>
make -s (-s is specified in the INSTALL doc)<br>
make install<br></blockquote>


<b>5.</b> Other compilation issues<br>
<br>
IMMS will now be able to pass and run the entire configure script. However, there are numerous problems with building IMMS natively on Windows.<br>
<br>
a) IMMS has a signal for SIGPIPE, but Windows as an OS doesn't support or implement SIGPIPE. Possible solution: We can wrap that one definition in an #ifdef. In immsd.cc change<br>
<br>
<pre><code>    // SIGPIPE is not supported on Windows, so ignore it if we're building under MinGW<br>
    // TODO: find reference<br>
    // TODO: is this the right #ifdef to use?<br>
    // TODO: this requires adding a unit test - what happens if we try to open a closed file socket on Windows?<br>
    #ifndef __MINGW32__<br>
    signal(SIGPIPE, quit);<br>
    #endif<br>
</code></pre>

b) mkdir. on POSIX systems this function takes 2 arguments (directory, permissions); on Windows it takes 1 (directory). We could possibly get around this by redefining the function for windows:<br>
<br>
<pre><code>#ifdef _WIN32<br>
# define  mkdir( D, M )   _mkdir( D )<br>
#endif<br>
</code></pre>

This would have to be tested.<br>
<br>
c) The constant 'ERROR' is already defined in MinGW, in include/wingdi.h. To get around this we could redefine 'ERROR' and 'INFO' to 'IMMSERROR' and 'IMMSINFO'. This would affect immsd/immsd.cc and immscore/fetcher.cc.<br>
<br>
d) srandom. This is not supplied by minGW or gnulib. We could potentially add<br>
<br>
<pre><code>#ifdef WIN#@<br>
#define random rand<br>
#define srandom srand<br>
#endif<br>
</code></pre>

to immscore/immsutil.cc. This would need to be tested.<br>
<br>
e) realpath. This is supposedly supplied by gnulib, but I haven't gotten it to work yet. Note that IMMS currently builds without using 'automake', so we may have to add some automake parts to the IMMS build system for gnulibe to be able to import realpath. This would need to be tested so it doesn't break the Linux version of IMMS.<br>
<br>
<ul><li>Import gnulib realpath code with<br>
<blockquote><code>../prereqs/gnulib-HEAD-b392a22/gnulib-tool --source-base=gnulib --m4-base=gnulib/m4 --import canonicalize-lgpl</code>
</blockquote></li><li>gnulib requires a newer version of autoconf, so we'd have to update IMMS <code> </code> to {{{}}}<br>
</li><li>add "gnulib/Makefile" to AC_CONFIG_FILES in ./configure.ac,<br>
</li><li>invoke gl_EARLY in ./configure.ac, right after AC_PROG_CC,<br>
</li><li>invoke gl_INIT in ./configure.ac.</li></ul>

<ul><li>Then set up the IMMS build system so it can use automake too<br>
<ul><li>Create a 'Makefile.am' file in the main IMMS directory<br>
</li><li>mention "gnulib" in SUBDIRS in Makefile.am,<br>
</li><li>mention "-I gnulib/m4" in ACLOCAL_AMFLAGS in Makefile.am,<br>
</li><li>add AM_INIT_AUTOMAKE to configure.ac<br>
</li><li>copy in the 'missing', depcomp, config.guess, and config.sub files from automake 1.11 TODO: or use 'automake --add-missing'?<br>
</li><li>add AC_PROG_RANLIB to configure.ac, right after AC_PROG_CC.<br>
</li><li>either call aclocal with the new -I flag ('aclocal -I gnulib/m4') TODO: or add it to INCLUDES in vars.mk? TODO: how can we make missing gl_EARLY and gl_INIT macros an error rather than a warning?<br>
</li></ul></li><li>Add the gnulib dir to the -I include flags for the compiler? And add some flags to the linker?</li></ul>

still doesn't work even with all that though :-/<br>
<br>
f) nanosleep is another linux command not present on mingw. TODO supplied by gnulib?<br>
<br>
z) sockets. We may have to change over to winsock, which involved some #ifdefs?<br>
<br>
To compile we need to change at least:<br>
<br>
in immscore/immsutil.cc<br>
<br>
<pre><code>#ifdef WIN32<br>
   #include &lt;winsock.h&gt;<br>
#else<br>
   #include &lt;sys/socket.h&gt;<br>
   #include &lt;sys/un.h&gt;<br>
#endif<br>
</code></pre>

<ul><li>and adding an init call on windows<br>
</li><li>and add a linker flag on windows</li></ul>

it may require a bunch of other stuff though (including fcntl, below)<br>
<br>
zz) fcntl. This doesn't exist/work on MinGW with F_SETFD. We could possibly substitute something like<br>
<br>
<pre><code>#ifndef __MINGW32__<br>
<br>
        fcntl(fd, F_SETFD, O_NONBLOCK);<br>
<br>
        #else<br>
<br>
        u_long *argp = 1; // TODO: only needed on Windows?<br>
        // set socket to non-zero value so that nonblocking mode<br>
        // is enabled<br>
        // http://msdn.microsoft.com/en-us/library/ms738573%28VS.85%29.aspx<br>
<br>
        int sret = ioctlsocket(fd, FIONBIO, argp);  //TODO: this requires using Winsocks.<br>
        if (sret != 0)<br>
        {<br>
            // TODO: check for errors with WSAGetLastError().<br>
        }<br>
<br>
        #endif<br>
</code></pre>

however, this may really be an issue with file sockets. The real fix may require changing to an IP socket on windows.<br>
<br>
zzz) kill. This function doesn't exist in mingw. If we were actually trying to terminate a process we could perhaps substitute for <code>raise()</code> on win32 systems, but the code looks like it's just trying to see if a file exists. This may have to deal with file sockets and might have to be part of the system rewrite for windows.<br>
<br>
zzzz) {{{../immscore/immsutil.cc:205:24: error: aggregate 'socket_connect(const std::stri<br>
ng&)::sockaddr_un sun' has incomplete type and cannot be defined.}}} This is because struct sockaddr_un sun isn't defined on windows? This may necessitate switching to winsock, along with other socket things? Or is there something else we can do?<br>
<br>
<br>
These problems are still being addressed. More to come soon...<br>
<br>
<br>
<b>6.</b> Finally! Configure and make imms<br>
<br>
<blockquote>./configure<br>
make</blockquote>




<h3>Troubleshooting on MinGW</h3>

MinGW Troubleshooting<br>
<br>
<b>Q: SQLite won't compile! All it says is "make:</b> <code>[sqlite3.lo]</code> Error 1"<br>
<br>
<b>A:</b> Did anything weird happen when configuring? Check your configure output, and/or your configure.log. In particular you may see messages like "Bad file number".<br>
<br>
Note: if you have both cygwin and mingw installed on your machine this may cause problems, as mingw may pick up some paths from the Windows environment variable. When you launch the MSYS terminal, check your path ('set path'). You may want to change your path to not include anything with cygwin!<br>
<br>
<br>
<b>Q: When compiling autoconf I get weird messages like "The system cannot find the path specified. autom4te: need GNU m4 1.4 or later". What gives?</b>

<b>A:</b> Autoconf is not buildable from source under msys or mingw. This is the official statement from the maintainer. Don't try to compile autoconf or m4 yourself - instead, download the msysDTK ("Developer Tool Kit") from  <a href='http://sourceforge.net/projects/mingw/files/'>http://sourceforge.net/projects/mingw/files/</a> and install it to get a copy of autoconf (see instructions above).<br>
<br>
<b>Q: When running ./configure for IMMS I get messages like "possibly undefined macro: PKG_CONFIG_LIBDIR", "possibly undefined macro: AC_MSG_ERROR", or "syntax error near unexpected token <code>pcre,' </code>PKG_CHECK_MODULES(pcre, libpcre, , with_pcre=no)'".</b>

<b>A:</b> This is an issue with autotools not finding the right m4 macros, similar to the entry in the FAQ <a href='http://imms.luminal.org/faq#TOC-I-get-an-error-about-undefined-macr'>http://imms.luminal.org/faq#TOC-I-get-an-error-about-undefined-macr</a> . Because we have been installing things to /mingw and the autotools only look in /usr/local by default, we need to tell them about it (TODO: may be easier to just build everything in /usr/local?) . Unfortunately each autotool has a different syntax, but you can fix this by calling them with<br>
<br>
autoheader --include=/mingw<br>
aclocal -I /mingw<br>
autoconf --include=/mingw<br>
<br>
TODO: either avoid this or patch the -I into the makefile if uname matches mingw?<br>
<br>
<br>
<h3>Other ways to get autotools on MinGW</h3>

To build IMMS we need autoconf and several other autotools to create out 'configure' file, but MSYS itself does not ship with autotools. In fact, autoconf is not buildable under the current version of msys - this is the official statement by the maintainer; you will get weird error messages about "The system cannot find the path specified. autom4te: need GNU m4 1.4 or later". Thankfully, one of the mingw authors distributes pre-built versions of the autotools and perl as part of the MSYS Developer Toolkit (or msysDTK).<br>
<br>
So there are several ways to get these autotools:<br>
<ol><li>Install the MinGW Developer Toolkit as part of your initial MinGW installation, as described above<br>
</li><li>Download the latest msysDTK (MSYS Developer Toolkit) installer exe from <a href='http://sourceforge.net/projects/mingw/files/'>http://sourceforge.net/projects/mingw/files/</a> .<br>
<ul><li>Run the exe to install the latest msysDTK package to get a copy of autotools. Note: you may need to run the installer as an Administrator for it to successfully modify C:\WINDOWS\MSYS.INI<br>
</li><li>When the installer asks you for a path, install to C:\MinGW\msys\1.0 so the tools get added to the same directory as the other msys tools.<br>
</li></ul></li><li>Grab the msysDTK with mingw-get (unconfirmed; let the list know if you try this)</li></ol>

If you use the msysDTK you will need to build libiconv and gettext yourself in order to compile glib.<br>
<br>
<br>
<b>1) libiconv</b> (for gettext) <a href='http://www.gnu.org/software/libiconv/#downloading'>http://www.gnu.org/software/libiconv/#downloading</a>

We follow some of the instructions in the 'README.woe32' file, but some are out of date. Remove the '-mno-mingw' flag, since it is no longer supported by cygwin.<br>
<br>
<blockquote>PATH=/mingw/bin:$PATH<br>
export PATH<br>
./configure --host=i586-pc-mingw32 --prefix=/mingw \<br>
<blockquote>CPPFLAGS="-Wall -I/mingw/include" \<br>
CFLAGS="-O2 -g" \<br>
CXXFLAGS="-O2 -g" \<br>
LDFLAGS="-L/mingw/lib"</blockquote></blockquote>


<b>2) gettext</b> (for glib) <a href='http://www.gnu.org/software/gettext/'>http://www.gnu.org/software/gettext/</a>


Again, we follow only some of the instructions in the 'README.woe32' file. Definitely do <b>NOT</b> specify the old GCC version numbers.<br>
<br>
<blockquote>PATH=/mingw/bin:$PATH<br>
export PATH<br>
./configure --prefix=/mingw \<br>
<blockquote>CPPFLAGS="-Wall -I/mingw/include" \<br>
LDFLAGS="-L/mingw/lib"</blockquote></blockquote>

configuring and making may both take a long time.<br>
<br>
<br>
PROBLEM: When building gettext I get an error like<br>
<br>
<blockquote>./libxml/encoding.h:29:19: fatal error: iconv.h: No such file or directory</blockquote>

A: Have you installed libiconv? Build and install that before installing gettext.<br>
<br>
PROBLEM: When building gettext I get an error like<br>
<br>
<blockquote>undefined reference to `str_cd_iconveh'</blockquote>

A: If you read the gnu README.woe32 files you will notice that it says gettext and all dependencies have to be built with the same flags.<br>
<br>
<br>
<br>
<br>
<br>
<h2>Building with Cygwin</h2>

If you like you can also compile IMMS under Cygwin. This is easier than MinGW because cygwin supplies many POSIX-expected functions, files, and libraries. However, the resulting IMMS programs depend on the Cygwin .dlls to run; for this reason Cygwin builds are unofficial and will not be distributed through the IMMS website.<br>
<br>
TODO: transfer cygwin stuff here from <a href='http://imms.luminal.org/download/immswindowscygwin'>http://imms.luminal.org/download/immswindowscygwin</a> .