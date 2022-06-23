/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"
/*{{{ Include Files */

#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#include <string.h>
#include <setjmp.h>
#include <stdarg.h>

#include "buffer.h"
#include "screen.h"
#include "window.h"
#include "display.h"
#include "sysdep.h"
#include "file.h"
#include "keymap.h"
#include "ledit.h"
#include "misc.h"
#include "ins.h"
#include "paste.h"

/*}}}*/

/*{{{ Global Variables */

/* Do not malloc these without changing occurances of sizeof() first */
char Error_Buffer[256];
int Exit_From_MiniBuffer;

static unsigned char Macro_Buffer[JED_KBD_MACRO_SIZE];
unsigned char *Macro_Buffer_Ptr = Macro_Buffer;
static unsigned char *Macro_Ptr_Max = Macro_Buffer;
int Defining_Keyboard_Macro = 0;
int Executing_Keyboard_Macro = 0;

MiniInfo_Type Mini_Info;

/*}}}*/

typedef struct /*{{{*/
{
   jmp_buf b;
}

/*}}}*/
jmp_buf_struct;

extern jmp_buf_struct Jump_Buffer, *Jump_Buffer_Ptr;

static struct /*{{{*/
{
   int def;
   int exe;
}

/*}}}*/
Macro_State;

static void macro_store_key(char ch) /*{{{*/
{
   /* I need to put something here to increase the size of the Macro_Buffer
      in case of overflow */

   if (Macro_Buffer_Ptr - Macro_Buffer >= JED_KBD_MACRO_SIZE) msg_error("Macro Size exceeded.");
   else *Macro_Buffer_Ptr++ = ch;
}

/*}}}*/

static void save_macro_state(void) /*{{{*/
{
   Macro_State.def = Defining_Keyboard_Macro;
   Macro_State.exe = Executing_Keyboard_Macro;
}

/*}}}*/

void jed_suspend_macro_state (void)
{
   save_macro_state ();
   Defining_Keyboard_Macro = Executing_Keyboard_Macro = 0;
}

void jed_resume_macro_state (void)
{
   Defining_Keyboard_Macro = Macro_State.def;
   Executing_Keyboard_Macro = Macro_State.exe;
}

void jed_abort_keyboard_macro (void)
{
   Defining_Keyboard_Macro = Executing_Keyboard_Macro = 0;
   save_macro_state();
}

/* if not in the mini buffer and if during keyboard macro, allow user to enter
   different text each time macro is executed */

int macro_query() /*{{{*/
{
   char *s;
   int n;

   /* macro state is already set */
   Defining_Keyboard_Macro = 0;
   Executing_Keyboard_Macro = 0;

   if (!IN_MINI_WINDOW)
     {
	if (NULL == (s = read_from_minibuffer("Enter String:", (char *) NULL, NULL, &n))) return(0);
	(void) jed_insert_nbytes ((unsigned char *) s, n);
	SLfree(s);
     }

   /* exit from mini restores the state */
   return(1);
}

/*}}}*/

int jed_check_string_key_buffer (void)
{
   int ch, ch1;

   if (Read_This_Character == NULL)
     return -1;

   ch = *Read_This_Character++;
   if (ch == 0)
     {
	Read_This_Character = NULL;
	return -1;
     }

   if ((ch1 = *Read_This_Character) == 0)
     {
	Read_This_Character = NULL;
	return ch;
     }

   if (ch == '^')
     {
	if (ch1 == '?') ch = 127;
	else ch = ch1 - '@';
	Read_This_Character++;
     }
   else if (ch == '\\')
     {
	ch = ch1;
	Read_This_Character++;
     }
   if (*Read_This_Character == 0) Read_This_Character = NULL;

   return ch;
}

int jed_getkey (void) /*{{{*/
{
   int ch, diff;
   static int mini_flag = 0;
   int *srf;

   ch = jed_check_string_key_buffer ();
   if (ch != -1)
     return ch;

   if (Executing_Keyboard_Macro)
     {
	if (Macro_Buffer_Ptr < Macro_Ptr_Max) return (*Macro_Buffer_Ptr++);
	Executing_Keyboard_Macro = 0;
#if JED_HAS_MENUS
	/* This means that the macro was stopped from a menu */
	if (Jed_Menus_Active)
	  (void) jed_exit_menu_bar ();
#endif
	update((Line *) NULL, 0, 0, 1);
     }

   diff = (int) (Key_Bufferp - Key_Buffer);
   if ((SLang_get_error () == 0)
       && SLang_Key_TimeOut_Flag
       && ((mini_flag && (diff > 0))
	   || !input_pending(&Number_Ten)))
     {
	message(Key_Buffer);
	safe_strcat(Message_Buffer, "-", sizeof(Message_Buffer));

	/* This ensures that the update is performed if we are echoing from
	 * digit argument */
	srf = Repeat_Factor; Repeat_Factor = NULL;
	update((Line *)NULL, 0, 0, 1);
	Repeat_Factor = srf;
	clear_message ();
	mini_flag = 1;
     }
   else (mini_flag = 0);

   ch = my_getkey();
   if (diff < SLANG_MAX_KEYMAP_KEY_SEQ)
     {
	*Key_Bufferp++ = (char) ch;
	*Key_Bufferp = 0;
     }
   else
     {
	/* msg_error("KeyBuffer overflow!"); */
	Key_Bufferp = Key_Buffer;
     }

   if (Defining_Keyboard_Macro)
     {
	/* starting a new keysequence to save this point */
	if (diff == 0)
	  {
	     Macro_Ptr_Max = Macro_Buffer_Ptr;
	  }

	macro_store_key(ch);
     }
   return(ch);
}

/*}}}*/

void jed_error_hook (SLFUTURE_CONST char *msg)
{
   if ((JWindow == NULL) || Batch)
     fprintf(stderr, "%s\r\n", msg);
   else
     JWindow->trashed = 1;

   if (Error_Buffer[0] == 0)
     safe_strcpy(Error_Buffer, msg, sizeof (Error_Buffer));
}

void msg_error(char *msg) /*{{{*/
{
#if 0
   Defining_Keyboard_Macro = Executing_Keyboard_Macro = 0;
   Read_This_Character = NULL;
   Repeat_Factor = NULL;
   set_macro_state();
#endif
   SLang_verror (SL_INTRINSIC_ERROR, "%s", msg);
}

/*}}}*/

/* read from minibuffer using prompt providing a default, and stuffing
   the minibuffer with init if necessary */
/* I should make recursive mini buffers */
static char *read_from_minibuffer_1 (char *prompt, char *deflt, char *what, int *n) /*{{{*/
{
   char buf[JED_MAX_PATH_LEN];
   jmp_buf_struct *mini_jmp_save, mini_jmp_buf;
   unsigned char *ps, *psmax;
   unsigned char *p, ch;
   static Window_Type *current_window;
   /* may get changed if user leaves minibuffer and returns from other
    * window which means that the new window becomes target
    * of minibuffer action
    */
   Window_Type *w;
   char *ret;
   int len;
   Buffer *start_buffer;

   if (Batch)
     {
	if (prompt != NULL)
	  fputs (prompt, stdout);
	if ((deflt != NULL) && (*deflt != 0))
	  fprintf (stdout, "(default %s)", deflt);

	fflush (stdout);

	*n = 0;
	if (NULL == fgets (buf, sizeof(buf), stdin))
	  {
	     SLang_verror (SL_Read_Error, "Read from stdin failed");
	     return NULL;
	  }

	if (NULL == (ret = SLmake_string (buf)))
	  return NULL;

	len = strlen (ret);
	if (len && (ret[len-1] == '\n'))
	  {
	     len--;
	     ret[len] = 0;
	  }
	*n = (int) len;
	return ret;
     }

   if (!IN_MINI_WINDOW)
     {
	current_window = JWindow;
	start_buffer = CBuf;
     }
   else
     {
	current_window = NULL;
	start_buffer = NULL;
     }

   if (select_minibuffer()) return(NULL);   /* we should be on a new line of mini buffer */

   ps = Mini_Info.prompt;
   psmax = Mini_Info.prompt + JED_PROMPT_BUF_SIZE;
   p = (unsigned char *) prompt;
   len = 0;

   if (p != NULL)
     {
	while ((0 != (ch = *p++))
	       && (ps < psmax))
	  {
	     *ps++ = ch;
	     len++;
	  }
	if (ps < psmax)
	  {
	     *ps++ =  ' ';
	     len++;
	  }
     }

   if ((deflt != NULL) && (*deflt != 0))
     {
	SLsnprintf (buf, sizeof (buf), "(default: %s) ", deflt);
	p = (unsigned char *) buf;

	while ((0 != (ch = *p++)) && (ps < psmax))
	  {
	     *ps++ = ch;
	     len++;
	  }
     }

   if (ps == psmax)
     {
	ps -= 4;
	*ps++ = '.'; *ps++ = '.'; *ps++ = '.';
	len--;
     }
   *ps = 0;

   Mini_Info.prompt_len = len;
   Mini_Info.effective_prompt_len = jed_compute_effective_length (Mini_Info.prompt, Mini_Info.prompt + len);

   touch_window();
   if (what != NULL) jed_insert_string(what);
   update((Line *) NULL, 0, 0, 1);

   mini_jmp_save = Jump_Buffer_Ptr;
   Jump_Buffer_Ptr = &mini_jmp_buf;

   if (setjmp(mini_jmp_buf.b) != 0)
     {
	/* Do not call the restart function from here, in case this function
	 * has been called from the interpreter.
	 */
	/* SLang_restart(1); */
	update((Line *) NULL, 0, 0, 1);
     }

   while(!Exit_From_MiniBuffer)
     {
	int err = 0;
	do_jed();

	err = SLang_get_error ();
	if (((err == SL_USER_BREAK) || SLKeyBoard_Quit)
	    && IN_MINI_WINDOW)
	  break;

	update((Line *) NULL, 0, 0, 1);
	if (err)
	  {
	     SLang_restart (0);
	     SLang_set_error (0);
	     SLKeyBoard_Quit = 0;
	  }
     }

   if (Exit_From_MiniBuffer && !Executing_Keyboard_Macro
       && (Repeat_Factor == NULL) && !JWindow->trashed && !input_pending(&Number_Zero))
     {
	SLsmg_gotorc (Jed_Num_Screen_Rows - 1, 0);
	SLsmg_refresh ();
     }

   Jump_Buffer_Ptr = mini_jmp_save;

   exit_minibuffer ();		       /* be graceful */

   Exit_From_MiniBuffer = 0;
   MiniBuffer_Active = 0;
   jed_resume_macro_state ();

   if (!SLKeyBoard_Quit)
     {
	if (CLine->len == 0)
	  {
	     if (deflt != NULL) jed_insert_string(deflt);
	  }
	bob();
	jed_push_mark();
	eob();
	ret = make_buffer_substring(n);
     }
   else
     {
	SLKeyBoard_Quit = 0;
	ret = NULL;
     }

   /* we should be in minibuffer so delete marks and spots */
   while (CBuf->spots) pop_spot();
   while (CBuf->marks) jed_pop_mark(0);
   erase_buffer();

   /* Remove MiniWindow from the ring */
   w = JWindow;
   while (w->next != JWindow) w = w->next;
   other_window();
   w->next = JWindow;

   if (current_window != NULL)
     {
	/* Note that by this time, current_window might not exist. */
	while((JWindow != current_window) && (JWindow != w))
	  other_window();
     }

   if ((start_buffer != NULL)
       && (buffer_exists (start_buffer)))
     switch_to_buffer (start_buffer);

   JWindow->trashed = 1;

   /* delete_buffer(MiniBuffer); */
   MiniBuffer = NULL;

#if JED_HAS_MENUS
   jed_notify_menu_buffer_changed ();
#endif

   return(ret);
}

/*}}}*/

char *read_from_minibuffer (char *prompt, char *deflt, char *what, int *n)
{
   int *rf = Repeat_Factor;
   int repeat = 0;
   char *s;

   if (rf != NULL)
     repeat = *rf;

   s = read_from_minibuffer_1 (prompt, deflt, what, n);

   if (rf != NULL)
     {
	Repeat_Factor = rf;
	*rf = repeat;
     }

   return s;
}

static int Macro_Illegal_Now = 0;
static int check_macro_legality(void) /*{{{*/
{
   if (Macro_Illegal_Now)
     {
	msg_error("Illegal context for Macro definition.");
	return (0);
     }
   return (1);
}

/*}}}*/

int begin_keyboard_macro() /*{{{*/
{
   if (!check_macro_legality()) return (0);

   Macro_Buffer_Ptr = Macro_Buffer;
   message("Defining Macro.");
   Defining_Keyboard_Macro = 1;
   save_macro_state ();
   return(1);
}

/*}}}*/

int end_keyboard_macro() /*{{{*/
{

   if (Defining_Keyboard_Macro) message("Macro Defined.");
   else
     {
	if (!Executing_Keyboard_Macro) msg_error("Not defining Macro!");
	Executing_Keyboard_Macro = 0;
	return(1);
     }

   /* Macro_Ptr_Max = Macro_Buffer_Ptr; */
   Defining_Keyboard_Macro = 0;
   save_macro_state();
   return(1);
}

/*}}}*/

int execute_keyboard_macro() /*{{{*/
{
   int repeat = 0, *repeat_ptr;

   if (!check_macro_legality()) return (0);

   if (Defining_Keyboard_Macro)
     {
	msg_error("Can't execute a macro while defining one.");
	return(0);
     }

   Executing_Keyboard_Macro = 1;
   save_macro_state();
   Macro_Buffer_Ptr = Macro_Buffer;

   /* save the repeat context */
   repeat_ptr = Repeat_Factor;
   if (repeat_ptr != NULL) repeat = *repeat_ptr;

   JWindow->trashed = 1;
   Macro_Illegal_Now = 1;
   while ((Macro_Buffer_Ptr < Macro_Ptr_Max)
	  && Executing_Keyboard_Macro) /* since we might enter minibuffer
					and stop macro before returning
					here */
     {
	Repeat_Factor = NULL;
	/* ch = *Macro_Buffer_Ptr++; */
	(void) jed_do_key();
	if (SLKeyBoard_Quit || (*Error_Buffer)) break;
     }
   Macro_Illegal_Now = 0;

   /* restore context */
   Repeat_Factor = repeat_ptr;
   if (repeat_ptr != NULL) *repeat_ptr = repeat;
   Executing_Keyboard_Macro = 0;
#if JED_HAS_MENUS
   /* This means that the macro was stopped from a menu */
   if (Jed_Menus_Active)
     (void) jed_exit_menu_bar ();
#endif
   save_macro_state();
   return(1);
}

/*}}}*/

void get_last_macro () /*{{{*/
{
   unsigned char *m, *s;
   unsigned char buf[2 * JED_KBD_MACRO_SIZE + 1];

   if (Defining_Keyboard_Macro)
     {
	msg_error("Complete Macro first!");
	return;
     }

   m = Macro_Buffer;
   if (m == Macro_Ptr_Max)
     {
	msg_error("Macro not defined.");
	return;
     }
   s = buf;
   while (m < Macro_Ptr_Max)
     {
	unsigned char ch = *m++;
	if ((ch < ' ') || (ch == 127))
	  {
	     *s++ = '^';
	     if (ch == 127) ch = '?'; else ch = '@' + ch;
	  }
	else if ((ch == '^') || (ch == '\\'))
	  {
	     *s++ = '\\';
	  }

	*s++ = (char)ch;
     }
   *s = 0;
   SLang_push_string ((char *)buf);
}

/*}}}*/

char *safe_strcpy (char *a, SLFUTURE_CONST char *b, unsigned int n) /*{{{*/
{
   if (n != 0)
     {
	n--;
	strncpy (a, b, n);
	a[n] = 0;
     }
   return a;
}

/*}}}*/

char *safe_strcat (char *a, SLFUTURE_CONST char *b, unsigned int n) /*{{{*/
{
   unsigned int len = strlen (a);
   unsigned int max;

   n--;

   if (n > len)
     {
	max = n - len;
	strncpy (a + len, b, max);
	a[n] = 0;
     }
   return a;
}

/*}}}*/

void jed_vmessage (int now, char *fmt, ...)
{
   va_list ap;
   char buf[2048];

   va_start (ap, fmt);
   SLvsnprintf (buf, sizeof (buf), fmt, ap);
   va_end (ap);

   if (now) flush_message (buf);
   else message (buf);
}

void jed_verror (char *fmt, ...)
{
   va_list ap;
   char buf[2048];

   va_start (ap, fmt);
   SLvsnprintf (buf, sizeof (buf), fmt, ap);
   va_end (ap);

   msg_error (buf);
}

/* Since the interpreter deals mainly with hashed strings, and there is
 * the possibility that the strings to be compared are the same, we
 * will exploit this fact.
 */

int jed_case_strncmp (char *a, char *b, unsigned int n)
{
   register unsigned char cha, chb;
   char *bmax;

   if (a == b) return 0;
   bmax = b + n;
   while (b < bmax)
     {
	cha = UPPER_CASE(*a);
	chb = UPPER_CASE(*b);

	if (cha != chb)
	  return (int) cha - (int) chb;
	if (chb == 0) return 0;
	a++;
	b++;
     }
   return 0;
}

int jed_case_strcmp (char *a, char *b)
{
   register unsigned char cha, chb;

   if (a == b) return 0;
   while (1)
     {
	cha = UPPER_CASE(*a);
	chb = UPPER_CASE(*b);

	if (cha != chb)
	  return (int) cha - (int) chb;
	if (chb == 0) return 0;
	a++;
	b++;
     }
}

char *jed_malloc0 (unsigned int len)
{
   char *s = SLmalloc (len);
   if (s != NULL)
     memset ((char *) s, 0, len);
   return s;
}

Buffer *jed_get_mini_action_buffer (void)
{
   Window_Type *w;

   if (IN_MINI_WINDOW == 0)
     return NULL;

   w = Mini_Info.action_window;

   if (w != NULL)
     {
	Buffer *b = w->buffer;
	if (buffer_exists (b))
	  return b;
     }
   return NULL;
}

/* Returns 0, if the keysequence is valid, otherwise returns -1. */
int jed_getkey_wchar (SLwchar_Type *wchp)
{
   int ch;
   SLuchar_Type buf[SLUTF8_MAX_MBLEN+1];
   unsigned int i;

   ch = jed_getkey ();
   *wchp = (SLwchar_Type) ch;

   if ((ch < 128) || (Jed_UTF8_Mode == 0))
     return 0;

   buf[0] = (SLuchar_Type) ch;
   i = 1;
   while (1)
     {
	unsigned int n;
	if (NULL != SLutf8_decode (buf, buf+i, wchp, &n))
	  break;
	if ((i == SLUTF8_MAX_MBLEN) || (ch < 128))
	  {
	     ungetkey_string ((char *)buf + 1, i-1);
	     *wchp = buf[0];
	     return -1;
	  }
	ch = jed_getkey ();
	buf[i] = (SLuchar_Type) ch;
	i++;
     }
   return 0;
}

void jed_ungetkey_wchar (SLwchar_Type wc)
{
   SLuchar_Type *b, buf[JED_MAX_MULTIBYTE_SIZE];

   if (NULL == (b = jed_wchar_to_multibyte (wc, buf)))
     return;

   ungetkey_string ((char *)buf, (int)(b-buf));
}
