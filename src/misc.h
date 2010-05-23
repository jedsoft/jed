/* Copyright (c) 1992, 1998, 2000, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef JED_MISC_H_
#define JED_MISC_H_
#include "window.h"

extern void exit_error(char *, int);
extern void jed_error_hook (SLFUTURE_CONST char *);
extern void msg_error(char *);
extern void jed_verror (char *, ...);
extern void jed_vmessage (int, char *, ...);
extern void read_string(char *, int *);
extern void clear_error(void);
extern int jed_getkey(void);
extern int jed_check_string_key_buffer (void);
extern int jed_getkey_wchar (SLwchar_Type *);
extern void jed_ungetkey_wchar (SLwchar_Type);

extern char *jed_malloc0 (unsigned int);

extern int begin_keyboard_macro(void);
extern int macro_query(void);
extern int end_keyboard_macro(void);
extern void jed_abort_keyboard_macro (void);
extern int execute_keyboard_macro(void);
extern void get_last_macro(void);
extern int Defining_Keyboard_Macro;
extern int Executing_Keyboard_Macro;

extern char Error_Buffer[256];

/* information needed by minibuffer and display routines */
typedef struct MiniInfo_Type
  {
     Window_Type *action_window;   /* buffer for minibuffer action */
     unsigned char prompt[132];
     int prompt_len;
     int effective_prompt_len;	       /* length when tabs/etc are expanded */
  }
MiniInfo_Type;

extern MiniInfo_Type Mini_Info;

extern Buffer *jed_get_mini_action_buffer (void);

extern int get_macro(void);
extern int Exit_From_MiniBuffer;
extern unsigned char *Macro_Buffer_Ptr;
extern char *read_from_minibuffer(char *, char *, char *, int *);
extern char *safe_strcat (char *, SLFUTURE_CONST char *, unsigned int);
extern char *safe_strcpy (char *, SLFUTURE_CONST char *, unsigned int);

extern int jed_case_strncmp (char *, char *, unsigned int);
extern int jed_case_strcmp (char *, char *);

extern char **_Jed_Startup_Argv;
extern int _Jed_Startup_Argc;

#endif				       /* JED_MISC_H_ */

