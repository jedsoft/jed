/* Copyright (c) 1992-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#define MAX_PROCESSES 10

#ifdef __WIN32__
# ifdef VOID
#  undef VOID
# endif
# include <windows.h>
# include <stdio.h>
/* # define WIN95BUG	1 */
/* extern CRITICAL_SECTION Critical_Section; */
extern HANDLE Input_Events[];
#else
extern int Subprocess_Read_fds [MAX_PROCESSES][3];
extern int Max_Subprocess_FD;
#endif

extern int Num_Subprocesses;
extern volatile int Child_Status_Changed_Flag;

extern void read_process_input (int);
extern int jed_close_process (int *);
extern int jed_send_process (int *, char *);
extern int jed_open_process (int *);
extern int jed_open_process_pipe (int *);

extern void jed_get_child_status (void);
extern void jed_kill_process (int);
extern void jed_get_process_mark (int *);
extern void jed_set_process (void);
extern void jed_send_process_eof (int *);
extern void get_process_input (int *);
extern int jed_signal_process (int *, int *);
extern int jed_signal_fg_process (int *, int *);
extern int jed_processes_ok_to_exit (void);
extern void jed_query_process_at_exit (int *, int *);

extern void jed_set_process_flags (int *, int *);
extern int jed_get_process_flags (int *);

#ifdef REAL_UNIX_SYSTEM
extern void jed_block_child_signal (int);
extern int jed_fork_monitor (void);
#endif

extern FILE *jed_popen (char *, char *);
extern int jed_pclose (FILE *);

