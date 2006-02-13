/* Copyright (c) 1992, 1998, 2000, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef _JD_MACROS_H_
#define _JD_MACROS_H_

/* This file defines some macros that I use with programs that link to 
 * the slang library.
 */

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif

#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif 

#ifndef SLMEMSET
# ifdef HAVE_MEMSET
#  define SLMEMSET memset
# else
#  define SLMEMSET SLmemset
# endif
#endif

#ifndef SLMEMCHR
# ifdef HAVE_MEMCHR
#  define SLMEMCHR memchr
# else
#  define SLMEMCHR SLmemchr
# endif
#endif

#ifndef SLMEMCPY
# ifdef HAVE_MEMCPY
#  define SLMEMCPY memcpy
# else
#  define SLMEMCPY SLmemcpy
# endif
#endif

/* Note:  HAVE_MEMCMP requires an unsigned memory comparison!!!  */
#ifndef SLMEMCMP
# ifdef HAVE_MEMCMP
#  define SLMEMCMP memcmp
# else
#  define SLMEMCMP SLmemcmp
# endif
#endif

#if defined(__QNX__) && defined(__WATCOMC__)
# include <unix.h>
#endif

#endif				       /* _JD_MACROS_H_ */
