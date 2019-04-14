/* Copyright (c) 2007-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#ifndef _JED_WIN32_H_
#define _JED_WIN32_H_
extern FILE *w32_popen(char *, char *);
extern int w32_pclose(FILE *);
extern char *w32_build_command (char **, unsigned int);
extern int w32_init_subprocess_support (int);
extern int jed_init_w32_support (void);
#endif
