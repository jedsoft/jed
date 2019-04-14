/* Copyright (c) 2007-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#ifndef _JED_LIMIT_H_
#define _JED_LIMIT_H_

#include <limits.h>

#if defined(SIXTEEN_BIT_SYSTEM)
# define JED_KBD_MACRO_SIZE		256
# define JED_MAX_ADD_COMPLETIONS	32
#else
# define JED_KBD_MACRO_SIZE		8192
# define JED_MAX_ADD_COMPLETIONS	512
#endif

#ifdef PATH_MAX
# define JED_MAX_PATH_LEN PATH_MAX
#else
# ifdef IBMPC_SYSTEM
#  define JED_MAX_PATH_LEN 256
# else
#  define JED_MAX_PATH_LEN 1024
# endif
#endif

#define JED_MAX_STATUS_LEN 256

#endif				       /* _JED_LIMIT_H_ */
