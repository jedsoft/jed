dnl -*- autoconf -*-
dnl
dnl   This file requires m4 macros that I define in my acsite.m4 file.
dnl   You can obtain this file from <ftp://space.mit.edu/pub/davis/acsite.m4>.
dnl
AC_INIT(src/ledit.c)
JD_INIT

AC_CONFIG_AUX_DIR(autoconf)
AC_CANONICAL_HOST
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_INSTALL

JD_ANSI_CC
JD_ELF_COMPILER

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
openpty \
snprintf vsnprintf \
)

JD_SET_OBJ_SRC_DIR
JD_TERMCAP
JD_WITH_LIBRARY(slang)
dnl JD_FIND_SLANG
JD_GCC_WARNINGS

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

JD_GET_MODULES(src/modules.unx)
JD_CREATE_MODULE_ORULES

AC_CONFIG_HEADER(src/sysconf.h:src/config.hin)
AC_OUTPUT(Makefile:autoconf/Makefile.in src/Makefile)

dnl JD_CREATE_ORULE(jprocess)
dnl JD_CREATE_ORULE(chkslang)
JD_CREATE_ORULE(display)
JD_CREATE_ORULE(vterm)
JD_CREATE_ORULE(xterm)
JD_CREATE_ORULE(gpmmouse)
JD_CREATE_ORULE(rgrep)
JD_CREATE_ORULE(getmail)

JD_APPEND_RULES(src/Makefile)

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
