/* Copyright (c) 1992-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef __DAVIS_SYSDEP_H__
#define __DAVIS_SYSDEP_H__

#include <sys/types.h>

extern int Ignore_User_Abort;
extern char *_Jed_Backspace_Key;

#ifdef __MSDOS_16BIT__
#define MAX_INPUT_BUFFER_LEN 40
#else
#define MAX_INPUT_BUFFER_LEN 1024
#endif

extern int Input_Buffer_Len;
extern unsigned char Input_Buffer[MAX_INPUT_BUFFER_LEN];

extern unsigned char sys_getkey(void);
extern int sys_input_pending(int *, int);

extern int init_tty(void);
extern void reset_tty(void);
extern void sys_suspend(void);
extern int my_getkey(void);

extern int input_pending(int *);
extern void flush_input(void);
extern void ungetkey_string(char *, int);
extern void buffer_keystring (char *, int);
extern void ungetkey(int *);
extern void sys_resume(void);
extern int sys_delete_file(char *);
extern int sys_chmod(SLFUTURE_CONST char *, int, mode_t *, uid_t *, gid_t *);
extern unsigned long sys_file_mod_time(char *);

/* expand the file in a canonical way and return a pointer to a
   static area which should be regarded volatile */
extern char *jed_standardize_filename_static (char *);

extern char *jed_expand_filename (char *);

/* return pointer to place filename starts in path */
extern char *extract_file(SLFUTURE_CONST char *);

/* Like extract_file, except ignores the trailing slash */
char *jed_extract_file_or_dir (char *);

extern int sys_findfirst(char *);
extern int sys_findnext(char *);

extern unsigned long  sys_time(void);
extern int Meta_Char;
extern int DEC_8Bit_Hack;
extern void map_character(int *, int *);
extern int make_random_number(int *, int *);
extern int ch_dir(char *);
extern void deslash(char *);
extern char *slash2slash (char *);

#if defined(__MSDOS_16BIT__) || defined(__os2__) || defined(__WIN32__)
extern int NumLock_Is_Gold;
#endif

#ifdef __os2__
extern int IsHPFSFileSystem(char *);
#endif

#if defined(__GO32__) || (defined(__WATCOMC__) && !defined(__QNX__))
extern void i386_check_kbd(void);
extern int i386_access (char *, int);
#endif

#ifdef IBMPC_SYSTEM
# define SCREEN_HEIGHT 25
extern int PC_Alt_Char;
extern char *msdos_pinhead_fix_dir(char *);
extern int sys_System (char *);
# ifdef MSWINDOWS
extern int PC_Fn_Char;
# endif
extern int Jed_Filename_Case_Sensitive;
#else
# define SCREEN_HEIGHT 24
#endif

#ifdef sequent
# define strchr index
extern char *my_strstr();
#endif

#ifdef __unix__
extern void enable_flow_control(int *);
#endif

#ifdef VMS
void vms_cancel_exithandler(void);
extern int vms_send_mail(char *, char *);
extern int vms_get_help (char *, char *);
#endif

#ifdef __WIN32__
extern int jed_win32_rename (char *, char *);
#endif

#ifdef VMS
#endif

extern char *jed_get_cwd(void);

extern void jed_pause (int *);
extern void sys_pause (int);
extern void jed_sleep (unsigned int n);

extern char Jed_Root_Dir [JED_MAX_PATH_LEN];

extern void (*Jed_Sig_Exit_Fun) (void);

extern int jed_set_default_key_wait_time (const int *tsecs);

#endif /* __DAVIS_SYSDEP_H__ */
