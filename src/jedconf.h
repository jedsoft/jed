#ifndef _JED_CONFIG_H_
#define _JED_CONFIG_H_
/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

/* This file handles all non-Unix Operating systems */

#ifdef __VMS
#include "vms_x_fix.h"
#endif

#if defined(__WATCOMC__) && defined(__DOS__)
# define  DOS386 1
#endif

/* Set of the various defines for pc systems.  This includes OS/2 */

#ifdef __GO32__
# ifndef __DJGPP__
#  define __DJGPP__ 1
# endif
#endif

#if defined(__MSDOS__) || defined(__DOS__) || defined(__WIN16__)
# ifndef __MSDOS__
#  define __MSDOS__
# endif
#endif

#if defined(OS2)
# ifndef __os2__
#  define __os2__ 1
# endif
#endif

#if defined(CYGWIN32) && !defined(__CYGWIN32__)
# define __CYGWIN32__
#endif

#if defined(MINGW32) && !defined(__MINGW32__)
# define __MINGW32__ 1
#endif

#if defined(WIN32) || defined(__CYGWIN32__) || defined(__MINGW32__)
# ifndef __WIN32__
#  define __WIN32__ 1
# endif
#endif

#if defined(__WIN32__) || defined(__WIN16__)
# if !defined(MSWINDOWS)
#  define MSWINDOWS	1
# endif
#endif

#ifndef VMS
# ifndef IBMPC_SYSTEM
#  define IBMPC_SYSTEM 1
# endif
#endif

#if defined(__MSDOS__) && !defined(__GO32__) && !defined(DOS386) && !defined(__WIN32__)
# ifndef __MSDOS_16BIT__
#  define __MSDOS_16BIT__	1
# endif
# if defined(__BORLANDC__) && !defined(IBMPC_USE_ASM)
#  define IBMPC_USE_ASM
# endif
#endif

#if defined(__os2_16__) || defined(__MSDOS_16BIT__)
# define SIXTEEN_BIT_SYSTEM
#endif

#ifdef IBMPC_SYSTEM
# define HAVE_STDLIB_H
# define HAVE_PUTENV
# if defined(__GO32__) || defined(__CYGWIN32__)
#  define HAVE_UNISTD_H
# endif
#endif

#if !defined(VMS) && !defined(__WATCOMC__)
# define HAVE_MEMORY_H
#endif

#if defined(__WIN32__) && defined(__BORLANDC__)
# define HAVE_UTIME 1
#endif

#define HAVE_GETCWD

#if defined(_MSC_VER) || defined(__EMX__) || defined(__WATCOMC__)
# define strcmpi stricmp
# define strncmpi strnicmp
#endif

#define HAVE_MEMCPY
#define HAVE_MEMSET
#ifndef VMS
# define HAVE_MEMCMP
# define HAVE_MEMCHR
#endif

/* OS/2 probably has gethostname */
#undef HAVE_GETHOSTNAME

#if !defined(__WATCOMC__) || defined(OS2)
# define HAS_MOUSE 1
#endif

#if defined(__DECC)
# define HAVE_STDLIB_H
# define HAVE_UNISTD_H
#else
# ifdef VMS
#  if __VMS_VER >= 60200000
#   define HAVE_UNISTD_H
#  endif
# endif
#endif

#define HAVE_LONG_LONG 1	       /* is this true?? */

/* Define this if compiler has limits.h */
#define HAVE_LIMITS_H

/* Set this to 1 if the filesystem is case-sensitive */
#define JED_FILE_PRESERVE_CASE 0

/* Define if you have the vsnprintf, snprintf functions and they return
 * EOF upon failure.
 */
#undef HAVE_VSNPRINTF
#undef HAVE_SNPRINTF

/* If you have missing typedefs for any of these, uncomment and modify
 * accordingly
 */
/* typedef int mode_t; */
/* typedef int pid_t; */
typedef int uid_t;
typedef int gid_t;
/* typedef unsigned int dev_t; */
/* typedef unsigned long ino_t; */

#ifdef _MSC_VER
# define HAVE_DLOPEN 1
#endif

/* Finally, a reality check... */
#ifdef REAL_UNIX_SYSTEM
# undef REAL_UNIX_SYSTEM
#endif

#ifndef JED
# define JED
#endif

#endif				       /* _JED_CONFIG_H_ */
