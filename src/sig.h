/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef _JED_SIG_H_
#define _JED_SIG_H_

#ifndef __MSDOS_16BIT__
extern void init_signals (void);
extern void jed_reset_signals (void);
#endif
extern int Stdin_Is_TTY;

#ifdef SIGTSTP
# ifdef __unix__
extern void sig_sys_spawn_cmd(int);
extern int Signal_Sys_Spawn_Flag;      /* used if something else sends stop */
# endif
extern int Jed_Handle_SIGTSTP;
#endif

extern int jed_handle_interrupt (void);

#endif /* _JED_SIG_H_ */
