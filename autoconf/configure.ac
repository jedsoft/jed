dnl -*- autoconf -*-
dnl
AC_INIT([Jed],[0.99-19],[jed-users@jedsoft.org])
AC_CONFIG_SRCDIR([src/ledit.c])
JD_INIT
AC_CONFIG_AUX_DIR(autoconf)

m4_include([./include/pkgcfg.m4])

dnl # infodir="\${JED_ROOT}/info"

AC_CANONICAL_HOST
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_INSTALL

JD_ANSI_CC
JD_ELF_COMPILER

PKG_PROG_PKG_CONFIG([0.14.0])

# determine X include and libs for XJed
AC_PATH_XTRA

#Check these header since they cause trouble
AC_CHECK_HEADERS( \
stdlib.h \
unistd.h \
memory.h \
glob.h \
malloc.h \
termios.h \
fcntl.h \
sys/fcntl.h \
sys/pty.h \
pty.h \
)

# special treatment for sys/wait.h
AC_HEADER_SYS_WAIT
AC_HEADER_DIRENT

AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_UID_T
AC_CHECK_TYPES([dev_t, ino_t])

AC_CHECK_LIB(util,openpty)

AC_CHECK_FUNCS(\
memset \
memcpy \
putenv \
setenv \
unsetenv \
getcwd \
setsid \
tcgetattr tcsetattr cfgetospeed \
grantpt \
setlocale \
utime \
symlink \
gethostname \
tcgetpgrp \
tcsetpgrp \
getpgid \
setpgid \
openpty \
fsync \
snprintf vsnprintf \
)

AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(float, 4)
AC_CHECK_SIZEOF(double, 8)
JD_CHECK_LONG_LONG
JD_LARGE_FILE_SUPPORT

JD_SET_OBJ_SRC_DIR
JD_TERMCAP
JD_WITH_LIBRARY(slang)
JD_GCC_WARNINGS

AC_ARG_ENABLE(xft,
  AS_HELP_STRING([--disable-xft],
     [disable X FreeType library support (antialiased fonts)]),
  [enable_xft=$enableval], [enable_xft=default])
if test X"$enable_xft" = Xno
then
  AC_MSG_NOTICE(checking for xft disabled)
else
  AC_CHECK_LIB(Xft, XftFontOpen,
    [XRENDERFONTLIBS="-lXft"
     AC_SUBST(XRENDERFONTLIBS)
     AC_DEFINE(XJED_HAS_XRENDERFONT, 1)
     AC_PATH_PROG(PKGCONFIG, pkg-config)
     if test -n "$PKGCONFIG"
     then
       X_CFLAGS="$X_CFLAGS $($PKGCONFIG --cflags xft)"
     fi],
    [if test X"$enable_xft" = Xyes
     then
       AC_MSG_FAILURE([libXft not found])
     fi])
fi
 
dnl #------------------------------------------------------------------------
AC_ARG_ENABLE(gpm,
  AS_HELP_STRING([--disable-gpm], [disable gpm mouse support]),
  [enable_gpm=$enableval], [enable_gpm=default])
if test X"$enable_gpm" = Xno
then
  AC_MSG_NOTICE(checking for gpm disabled)
  MOUSELIB=""
  MOUSEDEFINES=""
  MOUSE_O_FILE=""
else
  AC_CHECK_LIB(gpm, Gpm_Open,
    [MOUSELIB="-lgpm"
     MOUSEDEFINES="-DUSE_GPM_MOUSE"
     MOUSE_O_FILE="gpmmouse.o"
     MOUSE_OBJ_FILE="\${OBJDIR}/gpmmouse.o"
     ],
    [if test X"$enable_gpm" = Xyes
     then
       AC_MSG_FAILURE([libgpm not found])
     fi])
fi
AC_SUBST(MOUSELIB)
AC_SUBST(MOUSEDEFINES)
AC_SUBST(MOUSE_O_FILE)
AC_SUBST(MOUSE_OBJ_FILE)

#---------------------------------------------------------------------------
# gtk
#---------------------------------------------------------------------------
AC_DEFUN([WITH_GTK],[
  AC_ARG_WITH( [gtk],
               AC_HELP_STRING([--with-gtk=PATH], [Directory where the pkg-config file gtk+-2.0.pc can be found. This is often /usr/lib/pkgconfig, /usr/lib64/pkgconfig or in a default garnome build of gnome $HOME/garnome/lib/pkgconfig]),
	       GTK_DIR="$withval", GTK_DIR="" )
  PKG_CONFIG_PATH="$GTK_DIR"
  export PKG_CONFIG_PATH
  PKG_CHECK_MODULES( GTK, [gtk+-2.0 >= 2.8.0], [jd_with_gtk="yes"], [jd_with_gtk="no"] )
  echo "GTK dir: $GTK_DIR"
  AC_MSG_CHECKING([for gtk+-2.0 version >= 2.8.0])
  AC_MSG_RESULT([$jd_with_gtk])
  AC_MSG_RESULT([CFlags: $GTK_CFLAGS])
  AC_MSG_RESULT([Libraries: $GTK_LIBS])
  BUILD_GTKJED=$jd_with_gtk
  AC_SUBST(BUILD_GTKJED)
])

WITH_GTK

dnl #------------------------------------------------------------------------
dnl JD_CHECK_FOR_LIBRARY(pcre)
dnl if test $jd_with_pcre_library = yes
dnl then 
dnl   LINK_LIBPCRE="-lpcre"
dnl   AC_DEFINE(HAVE_PCRE_H)
dnl else
dnl   LINK_LIBPCRE=""
dnl fi
dnl AC_SUBST(LINK_LIBPCRE)
dnl #------------------------------------------------------------------------
if test "x$prefix" != xNONE; then
  default_jed_root="$prefix/jed"
else
  default_jed_root="$ac_default_prefix/jed"
fi
JED_ROOT="${JED_ROOT:-$default_jed_root}"
AC_SUBST(JED_ROOT)dnl

dnl JD_GET_MODULES(src/modules.unx)
dnl JD_CREATE_MODULE_ORULES

AC_CONFIG_HEADER(src/sysconf.h:src/config.hin)
AC_CONFIG_FILES([Makefile:autoconf/Makefile.in src/Makefile])
AC_OUTPUT

dnl JD_CREATE_ORULE(jprocess)
dnl JD_CREATE_ORULE(chkslang)
dnl JD_CREATE_ORULE(display)
dnl JD_CREATE_ORULE(vterm)
dnl JD_CREATE_ORULE(xterm)
dnl JD_CREATE_ORULE(gpmmouse)
dnl JD_CREATE_ORULE(rgrep)
dnl JD_CREATE_ORULE(getmail)

dnl JD_APPEND_RULES(src/Makefile)

echo ""
echo "You will need to edit src/Makefile if any of the following are not correct:"
echo "        CC =" "$CC"
echo "    CFLAGS =" "$CFLAGS"
echo "   LDFLAGS =" "$LDFLAGS" "$DYNAMIC_LINK_FLAGS"
echo "  JED_ROOT =" "$JED_ROOT"
echo ""
echo "Also edit src/Makefile to enable:"
echo "  GPM Mouse support (Linux)"
echo "You may also want to edit src/jed-feat.h to enable or disable some features."
echo ""
echo "Before typing 'make', type 'make clean' first."
echo ""
