/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
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

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <string.h>

#include "buffer.h"
#include "keymap.h"
#include "file.h"
#include "ins.h"
#include "ledit.h"
#include "screen.h"
#include "window.h"
#include "display.h"
#include "search.h"
#include "misc.h"
#include "replace.h"
#include "paste.h"
#include "sysdep.h"
#include "cmds.h"
#include "text.h"
#include "abbrev.h"
#include "indent.h"
#include "colors.h"

#if JED_HAS_LINE_ATTRIBUTES
# include "lineattr.h"
#endif

#if JED_HAS_SUBPROCESSES
# include "jprocess.h"
#endif

#if defined(__DECC) && defined(VMS)
# include <unixlib.h>
#endif

#if defined(__MSDOS__) || defined(__os2__) || defined(__WIN32__)
# include <process.h>
#endif

/*}}}*/

extern char Jed_Root_Dir[JED_MAX_PATH_LEN];

static void do_buffer_keystring (char *s) /*{{{*/
{
   buffer_keystring (s, strlen(s));
}

/*}}}*/

static void do_tt_write_string (char *s) /*{{{*/
{
   if (Batch)
     {
	fputs (s, stdout);
     }
   else tt_write_string (s);
}

/*}}}*/

#ifndef IBMPC_SYSTEM
static void do_tt_set_term_vtxxx (int *i) /*{{{*/
{
   tt_set_term_vtxxx (i);
}

/*}}}*/
#endif

static void autoload_n (void) /*{{{*/
{
   int n;
   char *file, *fun;

   if (SLang_pop_integer (&n)) return;
   while ((n > 0) && (0 == SLang_get_error ()))
     {
	n--;
	if (SLang_pop_slstring (&file))
	  return;
	if (SLang_pop_slstring (&fun))
	  {
	     SLang_free_slstring (file);
	     return;
	  }
	(void) SLang_autoload (fun, file);
	SLang_free_slstring (file);
	SLang_free_slstring (fun);
     }
}

/*}}}*/

static void add_to_completion_n (void) /*{{{*/
{
   int n;
   char *fun;

   if (SLang_pop_integer (&n)) return;
   while (n > 0)
     {
	n--;
	if (SLang_pop_slstring (&fun))
	  return;

	if (-1 == add_to_completion (fun))
	  {
	     SLang_free_slstring (fun);
	     return;
	  }
	SLang_free_slstring (fun);
     }
}

/*}}}*/

static void translate_region (void) /*{{{*/
{
   SLang_Array_Type *a;
   char **strings;
   int zero = 0;
   Mark *m;

   if (0 == check_region (&zero))
     return;

   exchange_point_mark ();
   m = CBuf->marks;

   if (-1 == SLang_pop_array_of_type (&a, SLANG_STRING_TYPE))
     return;

   if (a->num_elements != 256)
     {
	jed_verror ("translate_region: String_Type[256] required");
	SLang_free_array (a);
	return;
     }

   strings = (char **) a->data;

   Suspend_Screen_Update = 1;
   while ((CLine != m->line) || (Point != m->point))
     {
	char *s = strings [*(CLine->data + Point)];
	if (s == NULL)
	  {
	     (void) jed_right (1);
	     continue;
	  }
	if (-1 == jed_insert_nbytes ((unsigned char *)s, strlen (s)))
	  break;

	if (-1 == jed_generic_del_nbytes (1))
	  break;
     }

   /* Leave point at end of region */
   jed_pop_mark (0);
   SLang_free_array (a);
}

/*}}}*/

static int write_region_cmd (char *file) /*{{{*/
{
   int n = write_region (file);
   if (n == -1)
     {
	jed_verror ("Error writing region to %s", file);
     }
   return n;
}

/*}}}*/

static void enable_menu_bar (int *what) /*{{{*/
{
   Window_Type *w;

   /* find the top window */
   w = JWindow;

   while (w->sy != Top_Window_SY) w = w->next;
   if (*what)
     {
	if (Top_Window_SY != 0) return;
	if (w->rows < 3)
	  {
	     /* window is too small --- fix it. */
	     one_window ();  w = JWindow;
	     if (w->rows < 3) return;
	  }
	Top_Window_SY = 1;
	w->sy = Top_Window_SY;
	w->rows -= 1;
     }
   else
     {
	if (Top_Window_SY == 0) return;
	Top_Window_SY = 0;
	w->sy = Top_Window_SY;
	w->rows += 1;
     }
   touch_screen ();
}

/*}}}*/

static int rename_file (char *f1, char *f2) /*{{{*/
{
   return rename (f1, f2);
}

/*}}}*/

static void exit_error_cmd (char *msg, int *severity) /*{{{*/
{
   exit_error (msg, *severity);
}

/*}}}*/

static void do_prefix_argument (void) /*{{{*/
{
   int n;

   if (SLang_Num_Function_Args == 1)
     {
	/* old semantics */
	if (-1 == SLang_pop_integer (&n))
	  return;

	if (Repeat_Factor != NULL)
	  n = *Repeat_Factor;
     }
   else if (Repeat_Factor == NULL)
     {
	SLang_push_null ();
	return;
     }
   else n = *Repeat_Factor;

   (void) SLang_push_integer (n);
   Repeat_Factor = NULL;
}

/*}}}*/

static void set_prefix_argument (int *n)
{
   static int rf;

   rf = *n;
   if (*n < 0)
     Repeat_Factor = NULL;
   else
     Repeat_Factor = &rf;
}

static void bury_buffer(char *name) /*{{{*/
{
   Buffer *b, *cp, *bp, *bn;

   if ((NULL == (b = find_buffer(name)))
       || (b == CBuf)
       || (CBuf == (bn = b->next))) return;

   cp = CBuf->prev;
   bp = b->prev;

   CBuf->prev = b;		       /* my definition of bury buffer */
   b->next = CBuf;
   b->flags |= BURIED_BUFFER;
   bp->next = bn;
   bn->prev = bp;
   cp->next = b;
   b->prev = cp;
}

/*}}}*/

static void set_buffer_hook (void) /*{{{*/
{
   SLang_Name_Type *f;
   char *s;

   if (NULL == (f = SLang_pop_function ()))
     return;

   if (-1 == SLang_pop_slstring (&s))
     {
	SLang_free_function (f);
	return;
     }

   if (-1 == jed_set_buffer_hook (CBuf, s, f))
     SLang_free_function (f);

   SLang_free_slstring (s);
}

/*}}}*/

static void get_buffer_hook (char *name)
{
   SLang_Name_Type *nt = jed_get_buffer_hook (CBuf, name);
   (void) SLang_push_function (nt);
   SLang_free_function (nt);
}

static void unset_buffer_hook (char *h) /*{{{*/
{
   (void) jed_unset_buffer_hook (CBuf, h);
}

/*}}}*/

static Buffer *pop_buffer (int nargs)
{
   Buffer *b;
   char *s;

   if (nargs != SLang_Num_Function_Args)
     return CBuf;

   if (-1 == SLang_pop_slstring (&s))
     return NULL;

   if (NULL == (b = find_buffer (s)))
     jed_verror ("Unable to find a buffer named %s", s);

   SLang_free_slstring (s);
   return b;
}

static void get_buffer_info(void) /*{{{*/
{
   Buffer *b;

   if (NULL == (b = pop_buffer (1)))
     return;

   check_buffer (b);

   (void) SLang_push_string (b->file);
   (void) SLang_push_string (b->dir);
   (void) SLang_push_string (b->name);
   (void) SLang_push_integer (b->flags);
}

/*}}}*/

static void set_buffer_info(char *file, char *dir, char *name, int *flagsp) /*{{{*/
{
   char *tmp;
   Buffer *b;

   if (NULL == (b = pop_buffer (5)))
     return;

   tmp = SLmalloc (strlen (dir) + 2);
   if (tmp == NULL)
     return;
   strcpy (tmp, dir);
   dir = tmp;
   fixup_dir (dir);

   tmp = jed_standardize_filename (dir);
   SLfree (dir);
   if (tmp == NULL)
     return;

   if ((file != b->file) || (tmp != b->dir))
     buffer_filename (b, tmp, file);

   SLfree (tmp);

   if (name != b->name)
     uniquely_name_buffer (b, name);

   jed_set_buffer_flags (b, (unsigned int) *flagsp);
}

/*}}}*/

static void intrin_load_buffer (void)
{
   char *ns = NULL;

   if ((SLang_Num_Function_Args == 1)
       && (-1 == SLang_pop_slstring (&ns)))
     return;
	
   jed_load_buffer (ns);
   SLang_free_slstring (ns);	       /* NULL ok */
}

static int intrin_what_mode(void) /*{{{*/
{
   Buffer *b;
   char *m;

   if (NULL == (b = pop_buffer (1)))
     return -1;

   if (NULL == (m = b->mode_string))
     m = "";
   SLang_push_string (m);
   return b->modes;
}

/*}}}*/

static void intrin_set_mode (int *flags)
{
   char *mode;

   if (-1 == SLang_pop_slstring (&mode))
     return;

   SLang_free_slstring (CBuf->mode_string);
   CBuf->mode_string = mode;
   CBuf->modes = *flags;
#if JED_HAS_LINE_ATTRIBUTES
   CBuf->min_unparsed_line_num = 1;
   CBuf->max_unparsed_line_num = Max_LineNum + CBuf->nup;
#endif
}

#ifdef REAL_UNIX_SYSTEM
static char *get_termcap_string (char *cap) /*{{{*/
{
   char *s;

   if (Batch) s = NULL;
   else s = SLtt_tgetstr (cap);
   if (s == NULL) s = "";
   return s;
}
/*}}}*/
#endif

static void intrin_insert_char (unsigned long *chp)
{
   (void) jed_insert_wchar ((SLwchar_Type) *chp);
}

static void intrin_insert_byte (unsigned char *chp)
{
   (void) jed_insert_byte ((unsigned char)*chp);
}

static void del_intrinsic (void)
{
   (void) jed_del_wchar ();
}

static int file_changed_on_disk_cmd (char *file)
{
   return file_changed_on_disk (CBuf, file);
}

static int get_point_cmd (void)
{
   return Point;
}
static void set_point_cmd (int *p)
{
   jed_position_point (CLine->data + *p);
}

static void expand_filename_cmd (char *s)
{
   if (NULL != (s = jed_expand_filename (s)))
     (void) SLang_push_malloced_string (s);
}

static void insert_string (void)
{
   char *s;
   SLang_BString_Type *bs;
   unsigned int len;
   int ret = 0;

   switch (SLang_peek_at_stack ())
     {
      case SLANG_STRING_TYPE:
	if (-1 == SLang_pop_slstring (&s))
	  return;
	ret = jed_insert_string (s);
	SLang_free_slstring (s);
	break;

      default:
	if (-1 == SLang_pop_bstring (&bs))
	  return;
	if (NULL != (s = (char *)SLbstring_get_pointer (bs, &len)))
	  ret = jed_insert_nbytes ((unsigned char *)s, len);
	SLbstring_free (bs);
     }

   if (ret == -1)
     jed_verror ("insert: insertion failed");
}

static int get_top_window_row (void)
{
   return Top_Window_SY + 1;
}

static void exit_jed_intrin (void)
{
   int status = 0;
   if ((SLang_Num_Function_Args == 1)
       && (-1 == SLang_pop_integer (&status)))
     return;

   (void) jed_exit_jed (status);
}

static void quit_jed_intrin (void)
{
   int status = 0;
   if ((SLang_Num_Function_Args == 1)
       && (-1 == SLang_pop_integer (&status)))
     return;

   (void) jed_quit_jed (status);
}

static int jed_system (char *s) /*{{{*/
{
   if (Jed_Secure_Mode)
     {
	msg_error ("Access to shell denied.");
	return -1;
     }

#ifdef IBMPC_SYSTEM
   return sys_System (s);
#else
   return SLsystem (s);
#endif
}

/*}}}*/

static int run_program (char *s) /*{{{*/
{
   int status;

   if (Jed_Secure_Mode)
     {
	msg_error ("Access to shell denied.");
	return -1;
     }

   status = SLang_run_hooks ("_jed_run_program_hook", 1, s);
   if (status == 0)
     return jed_spawn_fg_process ((int (*)(VOID_STAR))jed_system, (VOID_STAR) s);
   if (status == -1)
     return -1;
   if (-1 == SLang_pop_integer (&status))
     return -1;
   return status;
}

/*}}}*/

static void expand_link_intrin (char *s)
{
   if (NULL != (s = jed_expand_link (s)))
     (void) SLang_push_malloced_string (s);
}

static int right_intrinsic (int *np)
{
   if (*np < 0)
     return 0;

   return jed_right (*np);
}

static int left_intrinsic (int *np)
{
   if (*np < 0)
     return 0;

   return jed_left (*np);
}

static int up_intrinsic (int *np)
{
   if (*np < 0) return 0;
   return jed_up ((unsigned int) *np);
}

static int down_intrinsic (int *np)
{
   if (*np < 0) return 0;
   return jed_down ((unsigned int) *np);
}

static void pop_mark_intrin (int *go)
{
   jed_pop_mark (*go);
}

static void trim_intrinsic (void)
{
   (void) jed_trim_whitespace ();
}

static void skip_white_intrin (void)
{
   (void) jed_skip_whitespace ();
}

static void set_line_number_mode (int *statusp)
{
#if JED_HAS_DISPLAY_LINE_NUMBERS
   int status = *statusp;
   if (status < 0)
     status = (CBuf->line_num_display_size == 0);

   CBuf->line_num_display_size = (status > 0);
#else
   (void) statusp;
#endif
}

static int do_what_char(void)
{
   SLwchar_Type ch;

   if (eobp ())
     ch = 0;
   else if (-1 == jed_what_char (&ch))
     return SLang_push_integer (-(int)ch);

   return SLclass_push_long_obj (SLANG_ULONG_TYPE, ch);
}

static void what_char_intrin (void)
{
   int n, m;

   if (SLang_Num_Function_Args != 1)
     {
	(void) do_what_char ();
	return;
     }

   if (-1 == SLang_pop_integer (&n))
     return;
	
   push_spot ();
   if (n < 0)
     {
	n = -n;
	m = jed_left (n);
     }
   else
     m = jed_right (n);
   if (m == n)
     (void) do_what_char ();
   else
     (void) SLang_push_integer (0);
   pop_spot ();
}

static int intrin_strwidth (char *s)
{
   char *smax = s + strlen (s);
   return (int) SLsmg_strwidth ((SLuchar_Type *)s, (SLuchar_Type *)smax);
}

#if 0
static int get_utf8_mode (void)
{
   return CBuf->local_vars.is_utf8;
}

static void set_utf8_mode (int *yes_no)
{
   CBuf->local_vars.is_utf8 = (*yes_no != 0);
}
#endif

/* These pointers are necessary because the things they are assigned to
 * are character arrays.  Character arrays are not valid in MAKE_VARIABLE.
 */
static char *Default_Status_Line_Ptr = Default_Status_Line;
static char *Message_Buffer_Ptr = Message_Buffer;
static char *Jed_Root_Dir_Ptr = Jed_Root_Dir;

static SLang_Intrin_Fun_Type Jed_Intrinsics [] = /*{{{*/
{
#if 0
   MAKE_INTRINSIC_0("get_utf8_mode", get_utf8_mode, INT_TYPE),
   MAKE_INTRINSIC_I("set_utf8_mode", set_utf8_mode, VOID_TYPE),
#endif
   MAKE_INTRINSIC_S("expand_symlink", expand_link_intrin, VOID_TYPE),
   MAKE_INTRINSIC_I("_set_point", set_point_cmd, VOID_TYPE),
   MAKE_INTRINSIC_0("_get_point", get_point_cmd, INT_TYPE),
   MAKE_INTRINSIC("_autoload", autoload_n, VOID_TYPE, 0),
   MAKE_INTRINSIC("push_mark", jed_push_mark,VOID_TYPE, 0),
   MAKE_INTRINSIC("bol", bol,VOID_TYPE, 0),
   MAKE_INTRINSIC_0("insert", insert_string,VOID_TYPE),
   MAKE_INTRINSIC_1("insert_char", intrin_insert_char, VOID_TYPE, SLANG_ULONG_TYPE),
   MAKE_INTRINSIC_1("insert_byte", intrin_insert_byte, VOID_TYPE, SLANG_UCHAR_TYPE),
   MAKE_INTRINSIC("eol", eol,VOID_TYPE, 0),
   MAKE_INTRINSIC_S("setbuf", set_buffer, VOID_TYPE),
   MAKE_INTRINSIC("_add_completion", add_to_completion_n, VOID_TYPE, 0),
   MAKE_INTRINSIC("del_region", delete_region, VOID_TYPE, 0),
   MAKE_INTRINSIC("bufsubstr", buffer_substring, VOID_TYPE, 0),
   MAKE_INTRINSIC_I("right", right_intrinsic, INT_TYPE),
   MAKE_INTRINSIC_I("left", left_intrinsic, INT_TYPE),
   MAKE_INTRINSIC("whatbuf", what_buffer, STRING_TYPE, 0),
   MAKE_INTRINSIC("getbuf_info", get_buffer_info,  VOID_TYPE, 0),
   MAKE_INTRINSIC_S("is_internal", is_internal, INT_TYPE),
   MAKE_INTRINSIC_4("setbuf_info", set_buffer_info,  VOID_TYPE, STRING_TYPE, STRING_TYPE, STRING_TYPE, INT_TYPE),
   MAKE_INTRINSIC_I("up", up_intrinsic, INT_TYPE),
   MAKE_INTRINSIC_I("down", down_intrinsic, INT_TYPE),
   MAKE_INTRINSIC_S("call", jed_call_cmd, VOID_TYPE),
   MAKE_INTRINSIC("eob", eob,VOID_TYPE, 0),
   MAKE_INTRINSIC("bob", bob,VOID_TYPE, 0),
   MAKE_INTRINSIC_S("looking_at", jed_looking_at, INT_TYPE),
   MAKE_INTRINSIC("del", del_intrinsic, VOID_TYPE, 0),
   MAKE_INTRINSIC("markp", markp, INT_TYPE, 0),
   MAKE_INTRINSIC_S("add_completion", add_to_completion, VOID_TYPE),
   MAKE_INTRINSIC("what_column", calculate_column,INT_TYPE, 0),
   MAKE_INTRINSIC("eobp", eobp,INT_TYPE, 0),
   MAKE_INTRINSIC_I("set_mode", intrin_set_mode, VOID_TYPE),
   MAKE_INTRINSIC_S("buffer_visible", jed_buffer_visible, INT_TYPE),
   MAKE_INTRINSIC_S("extract_filename", extract_file, STRING_TYPE),
   MAKE_INTRINSIC_0("trim", trim_intrinsic, VOID_TYPE),
   MAKE_INTRINSIC_S("pop2buf", pop_to_buffer, VOID_TYPE),
   MAKE_INTRINSIC_S("pop2buf_whatbuf", pop_to_buffer, STRING_TYPE),
#ifndef VMS
   MAKE_INTRINSIC_SS("copy_file", jed_copy_file, INT_TYPE),
#endif
   MAKE_INTRINSIC_S("copy_region", copy_region_cmd, VOID_TYPE),
   MAKE_INTRINSIC_S("insbuf", insert_buffer_name, VOID_TYPE),
   MAKE_INTRINSIC("bolp", bolp,INT_TYPE, 0),
   MAKE_INTRINSIC("beep", jed_beep, VOID_TYPE, 0),
   MAKE_INTRINSIC("pop_spot", pop_spot,VOID_TYPE, 0),
   MAKE_INTRINSIC("push_spot", push_spot,VOID_TYPE, 0),
   MAKE_INTRINSIC_S("sw2buf", switch_to_buffer_cmd, VOID_TYPE),
   MAKE_INTRINSIC_S("tt_send", do_tt_write_string, VOID_TYPE),
   MAKE_INTRINSIC("eolp", eolp,INT_TYPE, 0),
   MAKE_INTRINSIC_S("find_file", find_file_in_window, INT_TYPE),
   MAKE_INTRINSIC_SI("set_status_line", set_status_format, VOID_TYPE),
   MAKE_INTRINSIC_S("bury_buffer", bury_buffer, VOID_TYPE),
   MAKE_INTRINSIC("dupmark", dup_mark, INT_TYPE, 0),
   MAKE_INTRINSIC("erase_buffer", erase_buffer, VOID_TYPE, 0),
#ifndef SIXTEEN_BIT_SYSTEM
   MAKE_INTRINSIC_I("copy_region_to_kill_array", copy_region_to_kill_array, VOID_TYPE),
   MAKE_INTRINSIC_I("append_region_to_kill_array", append_region_to_kill_array, VOID_TYPE),
   MAKE_INTRINSIC_I("prepend_region_to_kill_array", prepend_region_to_kill_array, VOID_TYPE),
   MAKE_INTRINSIC_I("insert_from_kill_array", insert_from_kill_array, VOID_TYPE),
#endif     			       /* SIXTEEN_BIT_SYSTEM */
   MAKE_INTRINSIC_I("goto_column", goto_column, VOID_TYPE),
   MAKE_INTRINSIC_I("goto_column_best_try", goto_column1, INT_TYPE),
   MAKE_INTRINSIC_I("goto_line", goto_line,VOID_TYPE),
   MAKE_INTRINSIC_I("pop_mark", pop_mark_intrin, VOID_TYPE),
   MAKE_INTRINSIC_SSS("read_mini", mini_read, VOID_TYPE),
   MAKE_INTRINSIC_S("file_status", file_status, INT_TYPE),
   MAKE_INTRINSIC_0("skip_white", skip_white_intrin, VOID_TYPE),
   MAKE_INTRINSIC("bobp", bobp,INT_TYPE, 0),
   MAKE_INTRINSIC_S("flush", flush_message, VOID_TYPE),
   MAKE_INTRINSIC_I("input_pending", input_pending, INT_TYPE),
   MAKE_INTRINSIC_I("usleep", jed_pause, VOID_TYPE),
   MAKE_INTRINSIC_S("insert_file",  insert_file, INT_TYPE),
   MAKE_INTRINSIC_0("what_char", what_char_intrin, VOID_TYPE),
   MAKE_INTRINSIC_I("recenter", recenter, VOID_TYPE),
   MAKE_INTRINSIC_S("bufferp", bufferp, INT_TYPE),
   MAKE_INTRINSIC_I("update", update_cmd, VOID_TYPE),
   MAKE_INTRINSIC_I("update_sans_update_hook", update_sans_update_hook_cmd, VOID_TYPE),
   MAKE_INTRINSIC("skip_word_chars", skip_word_chars, VOID_TYPE, 0),
   MAKE_INTRINSIC("skip_non_word_chars", skip_non_word_chars, VOID_TYPE, 0),
   MAKE_INTRINSIC("bskip_word_chars", bskip_word_chars, VOID_TYPE, 0),
   MAKE_INTRINSIC("bskip_non_word_chars", bskip_non_word_chars, VOID_TYPE, 0),
   MAKE_INTRINSIC_I("whitespace", insert_whitespace,VOID_TYPE),
   MAKE_INTRINSIC_SS("file_time_compare", file_time_cmp, INT_TYPE),
   /* MAKE_INTRINSIC_I("xform_region", transform_region, VOID_TYPE), */
   MAKE_INTRINSIC_S("skip_chars", jed_skip_chars, VOID_TYPE),
   MAKE_INTRINSIC_I("set_file_translation", set_file_trans, VOID_TYPE),
#if defined (__unix__) || defined(__WIN32__) || (defined (__os2__) && !defined(__WATCOMC__))
# if (!defined(__GO32__) && !defined(__WATCOMC__)) || defined(__QNX__)
   MAKE_INTRINSIC_S("pipe_region", pipe_region, INT_TYPE),
   MAKE_INTRINSIC_S("run_shell_cmd", shell_command, INT_TYPE),
# endif
#endif
   MAKE_INTRINSIC_S("append_region_to_file", append_to_file, INT_TYPE),
   MAKE_INTRINSIC("autosave", auto_save, VOID_TYPE, 0),
   MAKE_INTRINSIC("autosaveall", auto_save_all, VOID_TYPE, 0),
   MAKE_INTRINSIC("backward_paragraph", backward_paragraph, VOID_TYPE, 0),
   MAKE_INTRINSIC("blank_rect", blank_rectangle, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("bskip_chars", jed_bskip_chars, VOID_TYPE),
   MAKE_INTRINSIC("buffer_list", make_buffer_list,  VOID_TYPE, 0),
   MAKE_INTRINSIC_I("check_region", check_region, VOID_TYPE),
   MAKE_INTRINSIC("copy_rect", copy_rectangle, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("define_word", define_word, VOID_TYPE),
   MAKE_INTRINSIC_S("delbuf", kill_buffer_cmd, VOID_TYPE),
   MAKE_INTRINSIC_S("delete_file",  sys_delete_file, INT_TYPE),
   /* MAKE_INTRINSIC_S("directory", expand_wildcards, INT_TYPE), */
   MAKE_INTRINSIC("evalbuffer", intrin_load_buffer, VOID_TYPE ,0),
   MAKE_INTRINSIC_S("expand_filename", expand_filename_cmd, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("check_buffers", check_buffers, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_S("file_changed_on_disk", file_changed_on_disk_cmd, INT_TYPE),
   MAKE_INTRINSIC("forward_paragraph", forward_paragraph, VOID_TYPE, 0),
   MAKE_INTRINSIC("indent_line", indent_line, VOID_TYPE, 0),
   MAKE_INTRINSIC("insert_rect", insert_rectangle, VOID_TYPE, 0),
   MAKE_INTRINSIC("kill_rect", kill_rectangle, VOID_TYPE, 0),
   MAKE_INTRINSIC_II("map_input", map_character, VOID_TYPE),
   MAKE_INTRINSIC("narrow_to_region", narrow_to_region, VOID_TYPE, 0),
   MAKE_INTRINSIC("narrow", narrow_to_lines, VOID_TYPE, 0),
   MAKE_INTRINSIC("open_rect", open_rectangle, VOID_TYPE, 0),
   MAKE_INTRINSIC("quit_jed", quit_jed_intrin, VOID_TYPE, 0),
   MAKE_INTRINSIC("exit_jed", exit_jed_intrin, VOID_TYPE, 0),
   MAKE_INTRINSIC_0("exit", exit_jed_intrin, VOID_TYPE),
   MAKE_INTRINSIC_S("read_file", find_file_cmd, INT_TYPE),
   MAKE_INTRINSIC_4("read_with_completion", read_object_with_completion, VOID_TYPE, STRING_TYPE, STRING_TYPE, STRING_TYPE, INT_TYPE),
   MAKE_INTRINSIC_I("set_abort_char", jed_set_abort_char, VOID_TYPE),
   MAKE_INTRINSIC("suspend", sys_spawn_cmd, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("buffer_keystring", do_buffer_keystring, VOID_TYPE),
#ifndef IBMPC_SYSTEM
   MAKE_INTRINSIC("w132", screen_w132, VOID_TYPE, 0),
   MAKE_INTRINSIC("w80", screen_w80, VOID_TYPE, 0),
#endif
   MAKE_INTRINSIC("what_mode", intrin_what_mode, INT_TYPE, 0),
   MAKE_INTRINSIC("widen", widen, VOID_TYPE, 0),
   MAKE_INTRINSIC("widen_region", widen_region, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("write_buffer", jed_save_buffer_as, INT_TYPE),
   MAKE_INTRINSIC_S("write_region_to_file", write_region_cmd, INT_TYPE),
   MAKE_INTRINSIC("count_chars", jed_count_chars, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("get_yes_no", jed_get_yes_no, INT_TYPE),
   MAKE_INTRINSIC_S("get_y_or_n", jed_get_y_n, INT_TYPE),
   MAKE_INTRINSIC_S("get_mini_response", jed_get_mini_response, INT_TYPE),
   MAKE_INTRINSIC_SS("rename_file", rename_file, INT_TYPE),
   MAKE_INTRINSIC_S("change_default_dir", ch_dir, INT_TYPE),
   MAKE_INTRINSIC_0("prefix_argument",  do_prefix_argument, VOID_TYPE),
   MAKE_INTRINSIC_I("set_prefix_argument", set_prefix_argument, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("set_buffer_hook", set_buffer_hook, VOID_TYPE),
   MAKE_INTRINSIC_S("get_buffer_hook", get_buffer_hook, VOID_TYPE),
   MAKE_INTRINSIC_S("unset_buffer_hook", unset_buffer_hook, VOID_TYPE),
   MAKE_INTRINSIC_SSS("insert_file_region", insert_file_region, INT_TYPE),
   MAKE_INTRINSIC_SSI("search_file", search_file, INT_TYPE),
   MAKE_INTRINSIC_II("random", make_random_number, INT_TYPE),
   MAKE_INTRINSIC("translate_region", translate_region, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("set_current_kbd_command", set_current_kbd_command, VOID_TYPE),
   MAKE_INTRINSIC("blink_match", blink_match, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("set_expansion_hook", set_expansion_hook, VOID_TYPE),
   MAKE_INTRINSIC("clear_message", clear_message, VOID_TYPE, 0),
   MAKE_INTRINSIC("flush_input", flush_input, VOID_TYPE, 0),
   MAKE_INTRINSIC_0("get_word_chars", jed_get_word_chars, STRING_TYPE),
   MAKE_INTRINSIC_SI("core_dump", exit_error_cmd, VOID_TYPE),
   MAKE_INTRINSIC("get_last_macro", get_last_macro, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("set_top_status_line", define_top_screen_line, VOID_TYPE),
   MAKE_INTRINSIC_I("enable_top_status_line", enable_menu_bar, VOID_TYPE),
   MAKE_INTRINSIC("create_user_mark", create_user_mark, VOID_TYPE, 0),
   MAKE_INTRINSIC("goto_user_mark", goto_user_mark, VOID_TYPE, 0),
   MAKE_INTRINSIC("move_user_mark", move_user_mark, VOID_TYPE, 0),
   MAKE_INTRINSIC("user_mark_buffer", user_mark_buffer, STRING_TYPE, 0),
   MAKE_INTRINSIC("is_user_mark_in_narrow", jed_is_user_mark_in_narrow, INT_TYPE, 0),

#if JED_HAS_LINE_MARKS
   MAKE_INTRINSIC_I("create_line_mark", jed_create_line_mark, VOID_TYPE),
#endif

#if JED_HAS_ABBREVS
   MAKE_INTRINSIC("list_abbrev_tables", list_abbrev_tables, INT_TYPE, 0),
   MAKE_INTRINSIC_S("use_abbrev_table", use_abbrev_table, VOID_TYPE),
   MAKE_INTRINSIC_SS("create_abbrev_table", create_abbrev_table, VOID_TYPE),
   MAKE_INTRINSIC_SSS("define_abbrev", define_abbrev, VOID_TYPE),
   MAKE_INTRINSIC_S("abbrev_table_p", abbrev_table_p, INT_TYPE),
   MAKE_INTRINSIC_S("dump_abbrev_table", dump_abbrev_table, VOID_TYPE),
   MAKE_INTRINSIC("what_abbrev_table", what_abbrev_table, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("delete_abbrev_table", delete_abbrev_table, VOID_TYPE),
#endif				       /* JED_HAS_ABBREVS */

#if JED_HAS_SUBPROCESSES
   MAKE_INTRINSIC_I("kill_process", jed_close_process, VOID_TYPE),
   MAKE_INTRINSIC_IS("send_process", jed_send_process, VOID_TYPE),
   MAKE_INTRINSIC_I("open_process", jed_open_process, INT_TYPE),
#ifdef REAL_UNIX_SYSTEM
   MAKE_INTRINSIC_I("open_process_pipe", jed_open_process_pipe, INT_TYPE),
#endif
   MAKE_INTRINSIC_I("process_mark", jed_get_process_mark, VOID_TYPE),
   MAKE_INTRINSIC_0("set_process", jed_set_process, VOID_TYPE),
   MAKE_INTRINSIC_I("send_process_eof", jed_send_process_eof, VOID_TYPE),
   MAKE_INTRINSIC_I("get_process_input", get_process_input, VOID_TYPE),
   MAKE_INTRINSIC_II("signal_process", jed_signal_process, VOID_TYPE),
   MAKE_INTRINSIC_II("signal_fg_process", jed_signal_fg_process, VOID_TYPE),
   MAKE_INTRINSIC_II("process_query_at_exit", jed_query_process_at_exit, VOID_TYPE),
   MAKE_INTRINSIC_II("set_process_flags", jed_set_process_flags, VOID_TYPE),
   MAKE_INTRINSIC_I("get_process_flags", jed_get_process_flags, INT_TYPE),
#endif

#ifdef HAS_MOUSE
   MAKE_INTRINSIC("mouse_get_event_info", jed_mouse_get_event_info, VOID_TYPE, 0),
   MAKE_INTRINSIC("mouse_set_current_window", jed_set_current_mouse_window, VOID_TYPE, 0),
   MAKE_INTRINSIC_SS("mouse_set_default_hook", jed_set_default_mouse_hook, VOID_TYPE),
   MAKE_INTRINSIC_II("mouse_map_buttons", jed_map_mouse_buttons, VOID_TYPE),
#endif				       /* HAS_MOUSE */

   /* System specific intrinsics */

#ifndef IBMPC_SYSTEM
   MAKE_INTRINSIC_I("set_term_vtxxx", do_tt_set_term_vtxxx, VOID_TYPE),
#endif

#ifdef IBMPC_SYSTEM
   MAKE_INTRINSIC_S("msdos_fixup_dirspec", msdos_pinhead_fix_dir, STRING_TYPE),
#endif

#ifdef VMS
   MAKE_INTRINSIC_SS("vms_get_help", vms_get_help,VOID_TYPE),
   MAKE_INTRINSIC_SS("vms_send_mail", vms_send_mail, INT_TYPE),
#endif

#ifdef REAL_UNIX_SYSTEM
   MAKE_INTRINSIC_I("enable_flow_control", enable_flow_control, VOID_TYPE),
   /* MAKE_INTRINSIC_S("get_passwd_info", get_passwd_info, VOID_TYPE), */
   MAKE_INTRINSIC_S("get_termcap_string", get_termcap_string, STRING_TYPE),
#endif

#ifdef __os2__
   MAKE_INTRINSIC_S("IsHPFSFileSystem", IsHPFSFileSystem, INT_TYPE),
#endif
   MAKE_INTRINSIC_0("TOP_WINDOW_ROW", get_top_window_row, INT_TYPE),

   MAKE_INTRINSIC_S("run_program", run_program, INT_TYPE),
   MAKE_INTRINSIC_I("set_line_number_mode", set_line_number_mode, INT_TYPE),
   MAKE_INTRINSIC_S("strwidth", intrin_strwidth, INT_TYPE),
   MAKE_INTRINSIC_0("set_undo_position", jed_undo_record_position, VOID_TYPE),
   SLANG_END_INTRIN_FUN_TABLE
};

/*}}}*/

#ifndef IBMPC_SYSTEM
static int Obsolete_Int_Variable;
#endif
static SLang_Intrin_Var_Type Jed_Variables [] =
{
   MAKE_VARIABLE("Status_Line_String", &Default_Status_Line_Ptr, STRING_TYPE, 1),
   MAKE_VARIABLE("MINIBUFFER_ACTIVE", &MiniBuffer_Active, INT_TYPE, 1),
   MAKE_VARIABLE("USE_TABS", &Jed_Use_Tabs, INT_TYPE, 0),
   MAKE_VARIABLE("TAB_DEFAULT", &Jed_Tab_Default, INT_TYPE, 0),
   MAKE_VARIABLE("CASE_SEARCH", &Buffer_Local.case_search, INT_TYPE, 0),
   MAKE_VARIABLE("CASE_SEARCH_DEFAULT", &Jed_Case_Search_Default, INT_TYPE, 0),
   MAKE_VARIABLE("REPLACE_PRESERVE_CASE_INTERNAL", &Replace_Preserve_Case, INT_TYPE, 0),
   /* Prototype: Integer REPLACE_PRESERVE_CASE_INTERNAL = 0;
    * This variable affects the @replace@ function.  If the length of the
    * old string is the same as the new string and this variable is
    * non-zero, then then the case of the old string will be preserved by
    * the replace function.  For example, if @REPLACE_PRESERVE_CASE_INTERNAL@
    * is non-zero, and @"abCdE"@ is replaced by @"tuvwx"@, then @"tuVwX"@ will
    * actually be used.
    * Note: This variable's value automatically gets reset to zero after every
    * keystroke.
    */
   MAKE_VARIABLE("what_line", &LineNum,INT_TYPE, 1),
   MAKE_VARIABLE("BLINK", &Blink_Flag, INT_TYPE, 0),
   MAKE_VARIABLE("WRAP_INDENTS", &Indented_Text_Mode, INT_TYPE, 0),
   MAKE_VARIABLE("IGNORE_BEEP", &tt_Ignore_Beep, INTP_TYPE, 0),
   MAKE_VARIABLE("ADD_NEWLINE", &Require_Final_Newline, INT_TYPE, 0),
   MAKE_VARIABLE("DISPLAY_TIME", &Display_Time, INT_TYPE, 0),
   MAKE_VARIABLE("WANT_EOB", &Want_Eob, INT_TYPE, 0),
   MAKE_VARIABLE("WRAP", &Buffer_Local.wrap_column, INT_TYPE, 0),
   MAKE_VARIABLE("WRAP_DEFAULT", &Jed_Wrap_Default, INT_TYPE, 0),
   MAKE_VARIABLE("META_CHAR", &Meta_Char, INT_TYPE, 0),
   MAKE_VARIABLE("DEC_8BIT_HACK", &DEC_8Bit_Hack, INT_TYPE, 0),
   MAKE_VARIABLE("BATCH", &Batch, INT_TYPE, 1),
   MAKE_VARIABLE("TAB", &Buffer_Local.tab, INT_TYPE, 0),
   MAKE_VARIABLE("MAX_HITS", &Jed_Max_Hits, INT_TYPE, 0),
   MAKE_VARIABLE("POINT", &Point, INT_TYPE, 0),
   MAKE_VARIABLE("MESSAGE_BUFFER", &Message_Buffer_Ptr, STRING_TYPE, 1),
   MAKE_VARIABLE("IGNORE_USER_ABORT", &Ignore_User_Abort, INT_TYPE, 0),
   MAKE_VARIABLE("KILL_LINE_FEATURE", &Kill_Line_Feature, INT_TYPE, 0),
   MAKE_VARIABLE("SCREEN_HEIGHT", &Jed_Num_Screen_Rows, INT_TYPE, 1),
   MAKE_VARIABLE("SCREEN_WIDTH", &Jed_Num_Screen_Cols, INT_TYPE, 1),
   MAKE_VARIABLE("JED_ROOT", &Jed_Root_Dir_Ptr, STRING_TYPE, 1),
   MAKE_VARIABLE("LINENUMBERS", &User_Prefers_Line_Numbers, INT_TYPE, 0),
   MAKE_VARIABLE("HIGHLIGHT", &Wants_Attributes, INT_TYPE, 0),
   MAKE_VARIABLE("HORIZONTAL_PAN", &Wants_HScroll, INT_TYPE, 0),
   MAKE_VARIABLE("WANT_SYNTAX_HIGHLIGHT", &Wants_Syntax_Highlight, INT_TYPE, 0),
   MAKE_VARIABLE("DEFINING_MACRO", &Defining_Keyboard_Macro, INT_TYPE, 1),
   MAKE_VARIABLE("EXECUTING_MACRO", &Executing_Keyboard_Macro, INT_TYPE, 1),
   MAKE_VARIABLE("DOLLAR_CHARACTER", &Jed_Dollar, INT_TYPE, 0),
   MAKE_VARIABLE("Highlight_Trailing_Whitespace", &Jed_Highlight_WS, INT_TYPE, 0),
   MAKE_VARIABLE("Highlight_Whitespace", &Jed_Highlight_WS, INT_TYPE, 0),
   MAKE_VARIABLE("_jed_version", &Jed_Version_Number, INT_TYPE, 1),
   MAKE_VARIABLE("_jed_version_string", &Jed_Version_String, STRING_TYPE, 1),
   MAKE_VARIABLE("_jed_secure_mode", &Jed_Secure_Mode, INT_TYPE, 1),

   MAKE_VARIABLE("Simulate_Graphic_Chars", &Jed_Simulate_Graphic_Chars, INT_TYPE, 0),
#ifndef IBMPC_SYSTEM
   MAKE_VARIABLE("OUTPUT_RATE", &Obsolete_Int_Variable, INT_TYPE, 0),
   MAKE_VARIABLE("USE_ANSI_COLORS", &tt_Use_Ansi_Colors, INTP_TYPE, 0),
   MAKE_VARIABLE("TERM_CANNOT_INSERT", &tt_Term_Cannot_Insert, INTP_TYPE, 0),
   MAKE_VARIABLE("TERM_CANNOT_SCROLL", &tt_Term_Cannot_Scroll, INTP_TYPE, 0),
   MAKE_VARIABLE("TERM_BLINK_MODE", &tt_Blink_Mode, INTP_TYPE, 0),
#endif				       /* !IBMPC_SYSTEM */

#ifdef IBMPC_SYSTEM
   MAKE_VARIABLE("Case_Sensitive_Filenames", &Jed_Filename_Case_Sensitive, INT_TYPE, 0),
# if defined(__MSDOS_16BIT__) || defined(__WIN32__)
   MAKE_VARIABLE("NUMLOCK_IS_GOLD", &NumLock_Is_Gold, INT_TYPE, 0),
# endif
# ifndef MSWINDOWS
   MAKE_VARIABLE("CHEAP_VIDEO", &SLtt_Msdos_Cheap_Video, INTP_TYPE, 0),
# endif
# ifdef MSWINDOWS
   MAKE_VARIABLE("FN_CHAR", &PC_Fn_Char, INT_TYPE, 0),
# endif
   MAKE_VARIABLE("ALT_CHAR", &PC_Alt_Char, INT_TYPE, 0),
#endif				       /* IBMPC_SYSTEM */

#ifdef REAL_UNIX_SYSTEM
   MAKE_VARIABLE("BACKUP_BY_COPYING", &Jed_Backup_By_Copying, INT_TYPE, 0),
#endif

#ifndef SIXTEEN_BIT_SYSTEM
   MAKE_VARIABLE("KILL_ARRAY_SIZE", &Kill_Array_Size, INT_TYPE, 1),
#endif
   MAKE_VARIABLE("_Backspace_Key", &_Jed_Backspace_Key, STRING_TYPE, 1),
#if JED_HAS_MENUS
   MAKE_VARIABLE("Menus_Active", &Jed_Menus_Active, INT_TYPE, 1),
#endif
   MAKE_VARIABLE(NULL, NULL, 0, 0)
};

int init_jed_intrinsics (void) /*{{{*/
{
   if ((-1 == SLadd_intrin_fun_table (Jed_Intrinsics, NULL))
       || (-1 == SLadd_intrin_fun_table(Jed_Other_Intrinsics, NULL))
       || (-1 == SLadd_intrin_var_table (Jed_Variables, NULL))
#if JED_HAS_LINE_ATTRIBUTES
       || (-1 == SLadd_intrin_fun_table(JedLine_Intrinsics, NULL))
#endif
       || (-1 == jed_init_color_intrinsics ())
       || (-1 == jed_init_keymap_intrinsics ())
       || (-1 == jed_init_window_intrinsics ())
       )
     return -1;

   /* made here for win32 dll support! */
   if ((-1 == SLadd_intrinsic_variable ("DISPLAY_EIGHT_BIT", (VOID_STAR)&SLsmg_Display_Eight_Bit, INT_TYPE, 0))
       || (-1 == SLadd_intrinsic_variable ("LAST_CHAR", (VOID_STAR)&SLang_Last_Key_Char, INT_TYPE, 0))
       )
     return -1;

   /* overload default slang version of system */
   SLadd_intrinsic_function ("system", (FVOID_STAR) jed_system,
			     SLANG_INT_TYPE, 1, SLANG_STRING_TYPE);
#if JED_HAS_DISPLAY_TABLE
   SLang_add_intrinsic_array ("Display_Map",   /* slang name */
			      SLANG_CHAR_TYPE,
			      0,       /* not read-only */
			      (VOID_STAR) Output_Display_Table,   /* location of the array */
			      1,
			      256);
#endif
#if 0
   SLang_add_intrinsic_array ("TRANSLATE_ARRAY",   /* slang name */
			      SLANG_CHAR_TYPE,
			      0,       /* not-readonly */
			      (VOID_STAR) Translate_Region_Array,   /* location of the array */
			      1,      /* number of dimensions */
			      256);    /* number of elements in X direction */
#endif
#ifdef __MSDOS__
   /* SLang does not define this symbol if WIN32 was used.  However, as
    * far as JED is concerned, WIN32 is MSDOS because of the file system.
    */
   SLdefine_for_ifdef ("MSDOS");
#endif
#ifdef IBMPC_SYSTEM
   SLdefine_for_ifdef ("IBMPC_SYSTEM");
#endif

#ifdef SIXTEEN_BIT_SYSTEM
   SLdefine_for_ifdef("16_BIT_SYSTEM");
#endif
#if JED_HAS_SUBPROCESSES
   SLdefine_for_ifdef("SUBPROCESSES");
#endif
#if JED_HAS_LINE_ATTRIBUTES
   SLdefine_for_ifdef("HAS_LINE_ATTR");
#endif
#if JED_HAS_BUFFER_LOCAL_VARS
   SLdefine_for_ifdef("HAS_BLOCAL_VAR");
#endif

   if (SLang_get_error ()) return -1;

   return 0;
}

/*}}}*/

