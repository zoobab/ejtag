AC_INIT(ejtag.spec.in)
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE(ejtag, 0.2)
AM_CONFIG_HEADER(config.h)

dnl AC_CHECK_TOOL()

AC_PROG_MAKE_SET
AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
dnl AC_PROG_RANLIB
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_HEADER_STDC
AC_FUNC_ALLOCA
dnl AC_PROG_YACC
AC_CHECK_PROGS(YACC, 'bison135 -y' 'bison -y' byacc, yacc)
AM_PROG_LEX

AC_CHECK_PROG(RANLIB, ranlib, ranlib, :)
AC_CHECK_PROG(DLLTOOL, dlltool, dlltool, dlltool)
AC_CHECK_PROG(AS, as, as, as)
AC_CHECK_PROG(AR, ar, ar, ar)
AC_CHECK_PROG(LD, ld, ld, ld)
AC_CHECK_PROG(WINDRES, windres, windres, i686-pc-cygwin-windres)

AC_CYGWIN
AC_MINGW32
AC_EXEEXT
AC_OBJEXT

dnl check for cross compiler path
if test x$cross_compiling = xyes; then
  AC_MSG_CHECKING(for cross compiler path)
  if test -d /usr/local/cross/i686-pc-cygwin; then
    CROSSCOMPPATH=/usr/local/cross/i686-pc-cygwin
  elif test -d /usr/local/cygwin/i686-pc-cygwin; then
    CROSSCOMPPATH=/usr/local/cygwin/i686-pc-cygwin
  else
    AC_ERROR("cross compiler not found")
  fi
  AC_MSG_RESULT($CROSSCOMPPATH)
fi

AC_CHECK_FUNCS(getopt_long,,[AC_LIBOBJ([getopt]) AC_LIBOBJ([getopt1])])
AC_CHECK_FUNCS(memmem,,[AC_LIBOBJ([memmem])])
AC_CHECK_HEADERS(sys/io.h asm/io.h getopt.h syslog.h)
AC_CHECK_FUNCS(snprintf vsnprintf syslog vsyslog openlog closelog gettimeofday iopl revoke)

AC_MSG_CHECKING(for GetSystemTime)
getsystemtime=no
AC_TRY_COMPILE([#include <windows.h>], 
     [ SYSTEMTIME tm; GetSystemTime(&tm); ],
     AC_DEFINE(HAVE_GETSYSTEMTIME,1,[define if GetSystemTime is available]) getsystemtime=yes)
AC_MSG_RESULT($getsystemtime)

AC_MSG_CHECKING(for signed bittypes)
signedbittypes=no
AC_TRY_COMPILE([#include <sys/types.h>], 
     [ int8_t a; int16_t c; int32_t e; ],
     AC_DEFINE(HAVE_SIGNED_BITTYPES,1,[define if bittypes like int16_t are defined]) signedbittypes=yes)
AC_MSG_RESULT($signedbittypes)

AC_MSG_CHECKING(for unsigned bittypes)
unsignedbittypes=no
AC_TRY_COMPILE([#include <sys/types.h>], 
     [ u_int8_t b; u_int16_t d; u_int32_t f; ],
     AC_DEFINE(HAVE_UNSIGNED_BITTYPES,1,[define if bittypes like u_int16_t are defined]) unsignedbittypes=yes)
AC_MSG_RESULT($unsignedbittypes)

AC_TYPE_SIGNAL

AC_MSG_CHECKING(for M_PI)
mpi=no
AC_TRY_COMPILE([#include <math.h>], 
     [ double f = M_PI; ],
     mpi=yes,AC_DEFINE(M_PI,3.14159265358979323846,[define if M_PI is not defined by math.h]))
AC_MSG_RESULT($mpi)

AC_MSG_CHECKING(for M_LOG10E)
mlog10e=no
AC_TRY_COMPILE([#include <math.h>], 
     [ double f = M_LOG10E; ],
     mlog10e=yes,AC_DEFINE(M_LOG10E,0.43429448190325182765,[define if M_LOG10E is not defined by math.h]))
AC_MSG_RESULT($mlog10e)

AC_MSG_CHECKING(for IO functions such as inb, outb)
iofuncs=no
AC_TRY_COMPILE([#include <sys/io.h>], 
     [ inb(0); inw(0); inl(0); outb(0,0); outw(0,0); outl(0,0); ],
     AC_DEFINE(HAVE_IOFUNCS,1,[define if inb and friends are available]) iofuncs=yes,
     AC_TRY_COMPILE([#include <asm/io.h>], 
         [ inb(0); inw(0); inl(0); outb(0,0); outw(0,0); outl(0,0); ],
         AC_DEFINE(HAVE_IOFUNCS) iofuncs=yes))
AC_MSG_RESULT($iofuncs)

dnl Add the languages which your application supports here.
dnl ALL_LINGUAS=""
dnl AM_GNU_GETTEXT

dnl Set PACKAGE_LOCALE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR,"${ac_default_prefix}/${DATADIRNAME}/locale",[various directories])
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR,"${prefix}/${DATADIRNAME}/locale")
fi

dnl Set PACKAGE_DATA_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR,"${ac_default_prefix}/share/${PACKAGE}",[various directories])
  else
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR,"${prefix}/share/${PACKAGE}")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR,"${datadir}/${PACKAGE}")
fi

dnl Set PACKAGE_LIBEXEC_DIR in config.h.
if test "x${libexecdir}" = 'x${exec_prefix}/libexec'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_LIBEXEC_DIR,"${ac_default_prefix}/libexec/${PACKAGE}",[various directories])
  else
    AC_DEFINE_UNQUOTED(PACKAGE_LIBEXEC_DIR,"${prefix}/libexec/${PACKAGE}")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_LIBEXEC_DIR,"${libexecdir}/${PACKAGE}")
fi

dnl Set PACKAGE_SOURCE_DIR in config.h.
packagesrcdir=`cd $srcdir && pwd`
AC_DEFINE_UNQUOTED(PACKAGE_SOURCE_DIR,"${packagesrcdir}",[various directories])

if test x$CYGWIN = xyes -o x$MINGW32 = xyes; then
  AC_DEFINE(WIN32,1,[define if compiling under Windows32])
  LIBS="$LIBS -ldsound -lgdi32"
fi

dnl Only use -Wall if we have gcc
if test "x$GCC" = "xyes"; then
  if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
    CFLAGS="$CFLAGS -Wall -O2"
    CXXFLAGS="$CXXFLAGS -Wall -O2"
  fi
fi

xlibs=$LIBS
LIBS=
AC_CHECK_FUNCS(vsnprintf,,[AC_LIBOBJ([vsnprintf])])
AC_CHECK_FUNCS(random,,[AC_LIBOBJ([random])])
LIBS=$xlibs

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

AC_ARG_ENABLE(littleendian,
[  --enable-littleendian   MIPS CPU is in little endian mode],
[case "${enableval}" in
  yes) littleendian=true ;;
  no)  littleendian=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-littleendian) ;;
esac],[littleendian=false])

CROSS=mips-linux-
if test x$littleendian = xtrue; then
    CROSS=mipsle-linux-
fi

AC_CHECK_PROGS(TARGETRANLIB, [${CROSS}ranlib ${CROSS}uclibc-ranlib], :)
AC_CHECK_PROGS(TARGETAS, [${CROSS}as ${CROSS}uclibc-as])
AC_CHECK_PROGS(TARGETAR, [${CROSS}ar ${CROSS}uclibc-ar])
AC_CHECK_PROGS(TARGETLD, [${CROSS}ld ${CROSS}uclibc-ld])
AC_CHECK_PROGS(TARGETCC, [${CROSS}gcc ${CROSS}uclibc-gcc])

AC_ARG_ENABLE(mmx,
[  --enable-mmx            Utilize MMX(tm) instructions if available (x86 only)],
[case "${enableval}" in
  yes) usemmx=true ;;
  no)  usemmx=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-mmx) ;;
esac],[usemmx=true])

AC_ARG_ENABLE(vis,
[  --enable-vis            Utilize VIS(tm) instructions if available (ultrasparc only)],
[case "${enableval}" in
  yes) usevis=true ;;
  no)  usevis=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-vis) ;;
esac],[usevis=false])

if test x$usemmx != xfalse; then
    AC_DEFINE(USEMMX,1,[define if MMX is to be used])
fi

if test x$usevis != xfalse; then
    AC_DEFINE(USEVIS,1,[define if VIS is to be used])
    dnl CFLAGS="$CFLAGS -mv8plus -mvis -Wa,-xarch=v8plusa"
    CFLAGS="$CFLAGS -Wa,-xarch=v8plusa"
fi

AC_ARG_ENABLE(ppuser,
[  --enable-ppuser         Enable PPUSER parport access method],
[case "${enableval}" in
  yes) ppuser=true ;;
  no)  ppuser=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-ppuser) ;;
esac],[ppuser=true])

if test x$ppuser != xfalse; then
    AC_DEFINE(HAVE_PPUSER,1,[define if we want ppuser support])
fi

AC_ARG_ENABLE(ppkdrv,
[  --enable-ppkdrv         Enable Linux Kernel driver parport access method],
[case "${enableval}" in
  yes) ppkdrv=true ;;
  no)  ppkdrv=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-ppkdrv) ;;
esac],[ppkdrv=true])

if test x$ppkdrv != xfalse; then
    AC_DEFINE(HAVE_PPKDRV,1,[define if we want kernel driver parport access support])
fi

AM_CONDITIONAL(USEMMX, test x$usemmx != xfalse)
AM_CONDITIONAL(USEVIS, test x$usevis != xfalse)
AM_CONDITIONAL(CROSSCOMP, test x$cross_compiling = xyes)
AM_CONDITIONAL(DIRECTX, test x$directx = xyes)
AM_CONDITIONAL(WIN32, test x$CYGWIN = xyes -o x$MINGW32 = xyes)
AM_CONDITIONAL(PPUSER, test x$ppuser != xfalse)
AM_CONDITIONAL(PPKDRV, test x$ppkdrv != xfalse)

echo 
AC_DEFINE_UNQUOTED(EXEC_EXT,"${ac_exeext}",[define to the executable extension of the host system])

AC_SUBST(HAVE_PPUSER)
AC_SUBST(HAVE_PPKDRV)
AC_SUBST(HAVE_BITTYPES)
AC_SUBST(HAVE_DIRECTX)
AC_SUBST(HAVE_MKISS)
AC_SUBST(HAVE_IFRNEWNAME)
AC_SUBST(HAVE_IOFUNCS)
AC_SUBST(HAVE_LIBZ)
AC_SUBST(M_PI)
AC_SUBST(LIBMKISS)
AC_SUBST(LIBZ)
AC_SUBST(LIBCURSES)
AC_SUBST(USEMMX)
AC_SUBST(USEVIS)
AC_SUBST(WIN32)
AC_SUBST(AR)
AC_SUBST(AS)
AC_SUBST(LD)
AC_SUBST(RANLIB)

AC_CONFIG_FILES([Makefile
libmisc/Makefile
ntdrv/Makefile
w9xdrv/Makefile
parport/Makefile
target/Makefile
doc/Makefile
ejtag.spec])
AC_OUTPUT
