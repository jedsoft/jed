/* -*- mode: c; mode: fold -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

/*{{{ #include files */

#include <stdio.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#include <slang.h>

#include "jdmacros.h"

#include <string.h>

#include "buffer.h"
#include "screen.h"
#include "window.h"
#include "ins.h"
#include "ledit.h"
#include "cmds.h"
#include "line.h"
#include "paste.h"
#include "display.h"
#include "sysdep.h"
#include "text.h"
#include "file.h"
#include "misc.h"
#include "search.h"
#include "hooks.h"
#include "abbrev.h"
#include "indent.h"
#if JED_HAS_SUBPROCESSES
# include "jprocess.h"
#endif
#include "sig.h"

#if JED_HAS_LINE_ATTRIBUTES
# include "lineattr.h"
#endif

/*}}}*/

/* fast insert/delete not implemented */

/*{{{ Global variables */

void (*X_Suspend_Hook)(void);
int Blink_Flag = 1;
int Indented_Text_Mode = 0;	       /* if non zero, intent line after wrap */
int Kill_Line_Feature = 1;	       /* non-zero means kill through eol if bolp  */
int Jed_Tab_Default = 8;
int Jed_Case_Search_Default = 0;
int Jed_Wrap_Column = 72;
int Jed_Suspension_Not_Allowed = 0;
int Jed_Use_Tabs = 1;

/*}}}*/

/*{{{ Static variables */

static char *Top_Of_Buffer_Error = "Top Of Buffer.";
static char *End_Of_Buffer_Error = "End Of Buffer.";

/*}}}*/
/*{{{ Static functions */

#if JED_HAS_LINE_ATTRIBUTES
static int check_line_attr_no_modify (Line *l)
{
   if (0 == (l->flags & JED_LINE_HIDDEN))
     return 0;
   
   msg_error ("You cannot edit this hidden line.");
   return -1;
}
#endif

static int next_visible_lines (int n)
{
#if JED_HAS_LINE_ATTRIBUTES
   int dn;
   int i;
   
   i = 0;
   while (i < n)
     {
	Line *l = CLine;
	
	dn = 0;
	do
	  {
	     l = l->next;
	     dn++;
	  }
	while ((l != NULL) && (l->flags & JED_LINE_HIDDEN));
	
	if (l == NULL) break;
	
	CLine = l;
	LineNum += dn;
	i++;
     }
   if (i) Point = 0;
   return i;
#else
   return jed_down(n);
#endif
}

static int prev_visible_lines (int n)
{
#if JED_HAS_LINE_ATTRIBUTES
   int i, dn;
   
   i = 0;
   while (i < n)
     {
	Line *l = CLine;
	
	dn = 0;
	do
	  {
	     l = l->prev;
	     dn++;
	  }
	while ((l != NULL) && (l->flags & JED_LINE_HIDDEN));
	
	if (l == NULL) break;
	
	CLine = l;
	LineNum -= dn;
	i++;
     }
   if (i) eol ();			       /* leave point at eol */
   return i;
#else
   return jed_up (n);
#endif
}

static int prev_visible_chars (int n)
{
#if JED_HAS_LINE_ATTRIBUTES
   jed_push_mark ();
   n = jed_left (n);
   if (n && (CLine->flags & JED_LINE_HIDDEN))
     {
	jed_skip_hidden_lines_backward (&Number_One);
	if (CLine->flags & JED_LINE_HIDDEN)
	  {
	     jed_pop_mark (1);
	     return 0;
	  }
     }
   jed_pop_mark (0);
   return n;
#else
   return jed_left (n);
#endif
}

static int next_visible_chars (int n)
{
#if JED_HAS_LINE_ATTRIBUTES
   jed_push_mark ();
   n = jed_right (n);
   if (n && (CLine->flags & JED_LINE_HIDDEN))
     {
	jed_skip_hidden_lines_forward (&Number_One);
	if (CLine->flags & JED_LINE_HIDDEN)
	  {
	     jed_pop_mark (1);
	     return 0;
	  }
     }
   jed_pop_mark (0);
   return n;
#else
   return jed_right (n);
#endif
}

static void check_last_key_function (FVOID_STAR f)
{
   /* For syntax highlighting */
   if (Last_Key_Function != f)
     touch_window ();
#if 0
       && ((Last_Key_Function == (FVOID_STAR) ins_char_cmd)
	   || (Last_Key_Function == (FVOID_STAR) eol_cmd)
	   || (Last_Key_Function == (FVOID_STAR) delete_char_cmd)
	   || (Last_Key_Function == (FVOID_STAR) backward_delete_char_cmd)
	   || (Last_Key_Function == (FVOID_STAR) backward_delete_char_untabify)))
     
     register_change(0);
#endif
}

   
static void next_line_prev_line_helper (int *gcp, int *ret, int dr, FVOID_STAR f)
{
   int gc;

   (void) dr;
   check_line();
   gc = calculate_column();
   if (Cursor_Motion <= 0) Goal_Column = gc;
   else if (Goal_Column < gc) Goal_Column = gc;
   *gcp = gc;
   
   Cursor_Motion = 2;
   
   check_last_key_function (f);
   *ret = 1;
#if 0
   *ret = (JWindow->trashed 
	   || (CLine == JScreen[JWindow->sy + dr].line)
#if JED_HAS_LINE_ATTRIBUTES
	   || (CLine->flags & JED_LINE_IS_READONLY)
#endif
	   );
#endif
}

static void eob_bob_error (int f)
{   
   char *str;
   
   if ((CBuf->buffer_hooks != NULL)
       && (CBuf->buffer_hooks->bob_eob_error_hook != NULL))
     {
	SLang_push_integer (f);
	SLexecute_function (CBuf->buffer_hooks->bob_eob_error_hook);
	return;
     }
   
   if (f < 0)
     str = Top_Of_Buffer_Error;
   else 
     str = End_Of_Buffer_Error;
   
   msg_error (str);
}

/*}}}*/

/*{{{ interactive insert/delete functions */

/*{{{ Interactive char deletion functions */

int delete_char_cmd (void)
{
   CHECK_READ_ONLY
#if 0
     ;
#endif

#if JED_HAS_LINE_ATTRIBUTES
   if (check_line_attr_no_modify (CLine))
     return 0;
#endif

   if (eobp())
     {
	msg_error(End_Of_Buffer_Error);
	return(0);
     }
   
   (void) jed_del_wchar ();
   return 1;
}

int backward_delete_char_cmd()
{
   /* CHECK_READ_ONLY */

   if (bobp())
     {
	msg_error(Top_Of_Buffer_Error);
	return(0);
     }
   
#if JED_HAS_LINE_ATTRIBUTES
   if (check_line_attr_no_modify (CLine))
     return 0;
#endif

   if (bolp())
     {
#if JED_HAS_LINE_ATTRIBUTES
	if (check_line_attr_no_modify (CLine->prev))
	  return 0;
#endif
     }

   jed_left (1);
   if (-1 == jed_del_wchar ())
     jed_right (1);
   return 1;
}

int backward_delete_char_untabify()
{
   unsigned char *p;
   int n;

   /* CHECK_READ_ONLY */
   
#if JED_HAS_LINE_ATTRIBUTES
   if (check_line_attr_no_modify (CLine))
     return 0;
#endif   
   p = CLine->data + (Point - 1);
   
   if (!Point || (*p != '\t') || !Buffer_Local.tab) 
     return backward_delete_char_cmd();
   
   n = calculate_column() - 1;
   jed_left (1);
   if (-1 == jed_del_wchar ())
     return -1;

   n = n - calculate_column();
   (void) jed_insert_wchar_n_times (' ', n);
   
   return(1);
}

/*}}}*/

/*{{{ newline_and_indent */
int newline_and_indent ()
{
   static int in_function;
   if (in_function) return -1;
   
#if JED_HAS_LINE_ATTRIBUTES
   if (check_line_attr_no_modify (CLine)
       || ((CLine->next != NULL) 
	   && eolp () && check_line_attr_no_modify (CLine->next)))
     return 0;
#endif

   if ((CBuf->buffer_hooks != NULL)
       && (CBuf->buffer_hooks->newline_indent_hook != NULL))
     {
	in_function = 1;
	SLexecute_function(CBuf->buffer_hooks->newline_indent_hook);
	in_function = 0;
	return 1;
     }
   if (0 == jed_insert_newline ())
     indent_line();
   return(1);
}

/*}}}*/
/*{{{ newline */

int newline (void)
{
   CHECK_READ_ONLY
#if 0
     ;
#endif

   if (CBuf == MiniBuffer) return exit_minibuffer();
   
   (void) jed_insert_newline ();
   return(1);
}

int newline_cmd (void)
{
#if JED_HAS_LINE_ATTRIBUTES
   if (check_line_attr_no_modify (CLine)
       || ((CLine->next != NULL) 
	   && eolp () && check_line_attr_no_modify (CLine->next)))
     return 0;
#endif
   return newline ();
}

/*}}}*/

static int execute_is_ok_hook (SLang_Name_Type *hook)
{
   int ret;
   if (-1 == SLexecute_function (hook))
     return -1;
   if (-1 == SLang_pop_integer (&ret))
     return -1;
   return (ret != 0);
}

/*{{{ ins_char_cmd */
int ins_char_cmd (void)
{
   unsigned char ch;
   int wrap = Jed_Wrap_Column;
   int do_blink;
   int did_abbrev = 0;
   SLang_Name_Type *wrapok_hook;

   CHECK_READ_ONLY
#if 0
     ;
#endif
#if JED_HAS_LINE_ATTRIBUTES
   if (check_line_attr_no_modify (CLine))
     return 0;
#endif
   
   ch = SLang_Last_Key_Char;
   
   if (ch == '\n')
     {
	newline();
	return(1);
     }
   
#if JED_HAS_ABBREVS
   if (CBuf->flags & ABBREV_MODE)
     {
	if (-1 == (did_abbrev = jed_expand_abbrev (ch)))
	  return -1;
     }
#endif
   
   if ((CBuf->flags & OVERWRITE_MODE) && !eolp())
     {
	/* FIXME: jed_del_wchar should be called for the last byte of a
	 * UTF-8 sequence
	 */
	if ((did_abbrev == 0)
	    && (-1 == jed_del_wchar ()))
	  return -1;
     }

   /* It is ok to use Point as an estimator of the current column.  This 
    * avoids the more expensive call to calculate_column.
    */
   if (CBuf->buffer_hooks != NULL) 
     wrapok_hook = CBuf->buffer_hooks->wrapok_hook;
   else
     wrapok_hook = NULL;
   
   if (((ch == ' ') || (Point >= wrap)) 
       && ((CBuf->modes & WRAP_MODE) || (wrapok_hook != NULL))
       && (calculate_column() > wrap)
       && ((wrapok_hook == NULL)
	   || (1 == execute_is_ok_hook (wrapok_hook))))
     {
	unsigned int this_line_num = LineNum;
	
	if ((did_abbrev == 0)
	    && (-1 == jed_insert_byte (ch)))
	  return -1;

	if (-1 == wrap_line(0))	       /* do not format--- just wrap */
	  return -1;		       /* line isn't wrapable */

	/* There is a bug involving wrapping a very long line containing
	 * no whitespace and then we try to insert a character.  This work
	 * arounds the bug.
	 */
	if ((this_line_num == LineNum)
	    && (ch == ' ')
	    && (calculate_column () > wrap))
	  {
	     if (0 == jed_right (1))
	       newline ();
	  }

	if ((CBuf->buffer_hooks != NULL)
	    && (CBuf->buffer_hooks->wrap_hook != NULL))
	  SLexecute_function(CBuf->buffer_hooks->wrap_hook);
	else if (Indented_Text_Mode) indent_line ();

	return(1);
     }
   
   do_blink = ((((CBuf->syntax_table != NULL)
		 && ((CBuf->syntax_table->char_syntax[(unsigned char) ch] & CLOSE_DELIM_SYNTAX)))
		|| ((ch == ')') || (ch == '}') || (ch == ']')))
	       && !input_pending(&Number_Zero));
   if (did_abbrev == 0)
     (void) jed_insert_byte (ch);
   if (do_blink) blink_match ();
   return 1;
}

/*}}}*/
/*{{{ quoted_insert */
int quoted_insert()
{
   SLwchar_Type ch;
   int ins_byte = 1;

   CHECK_READ_ONLY
   if (*Error_Buffer || SLKeyBoard_Quit) return(0);

   if (Repeat_Factor != NULL)
     {
	ch = *Repeat_Factor;
	ins_byte = 0;
	Repeat_Factor = NULL;
     }
   else
     {
	SLang_Key_TimeOut_Flag = 1;
	ch = jed_getkey();
	SLang_Key_TimeOut_Flag = 0;
     }
   
   if (SLang_get_error () == SL_USER_BREAK)
     SLang_set_error (0);
   
   if ((ch == '\n') && (CBuf == MiniBuffer))
     {
	(void) _jed_ins_byte ('\n');
	/* msg_error("Not allowed!"); */
	return (1);
     }
   
   SLKeyBoard_Quit = 0;

   if (ins_byte == 0)
     {
	if (-1 == jed_insert_wchar_n_times(ch, 1))
	  return -1;
     }
   else 
     {
	unsigned char byte = (unsigned char) ch;
	if (-1 == jed_insert_nbytes (&byte, 1))
	  return -1;
     }

   if ((CBuf->syntax_table != NULL)
       && (CBuf->syntax_table->char_syntax[(unsigned char) ch] & CLOSE_DELIM_SYNTAX)
       && !input_pending(&Number_Zero)) blink_match (); /* (ch); */
   
   return(1);
}

/*}}}*/

/*{{{ kill_line */

/* FIXME: MULTIBYTE unsafe */
int kill_line (void)
{
   int n, pnt, flag = 0;
   
   CHECK_READ_ONLY
     
     if (eobp())
     {
	msg_error(End_Of_Buffer_Error);
	return(0);
     }
   
   jed_push_mark();
   push_spot();
   pnt = Point;
   eol();
   n = Point - pnt;
   if ((!pnt && Kill_Line_Feature) || !n)
     {
	
	/* Either of these (flag =0,1) have the same effect on the buffer.
	 *  However, the first sets the mark at the end of the line and moves
	 *  the point to the end of the previous line.  This way the current
	 *  line structure is deleted and the screen update looks better.
	 */
	if (!pnt && (CLine->prev != NULL) && (CLine->next != NULL))
	  {
	     flag = 1;
	     jed_right (1);
	  }
	else n += jed_right (1);
	
     }
   
   if ((Last_Key_Function == (FVOID_STAR) kill_line) && (Paste_Buffer != NULL))
     {
	copy_region_to_buffer(Paste_Buffer);
     }
   else copy_to_pastebuffer();
   pop_spot();
   if (flag) n += jed_left (1);
   (void) jed_generic_del_nbytes (n);
   if (flag) (void) jed_right (1);
   return(1);
}

/*}}}*/

/*}}}*/
/*{{{ interactive cursor movement functions */

/*{{{ Interactive char/line movement functions */

int previous_line_cmd (void)
{
   int ret, gc;
   
   next_line_prev_line_helper (&gc, &ret, 0, (FVOID_STAR) previous_line_cmd);
   
   if ((CLine == CBuf->beg)
       || (1 != prev_visible_lines (1)))
     {
	eob_bob_error (-1);
	return 1;
     }
   
   point_column(Goal_Column);
   return(ret);
}


int next_line_cmd (void)
{
   int ret, gc;
   
   next_line_prev_line_helper (&gc, &ret, JWindow->rows - 1, (FVOID_STAR) next_line_cmd);
   
   if ((CLine == CBuf->end)
       || (1 != next_visible_lines (1)))
     {
	eob_bob_error (1);
	return 1;
     }
   
   point_column(Goal_Column);
   return(ret);
}

int previous_char_cmd (void)
{
   int b;
   Cursor_Motion = 1;
   
   /* check_last_key_function ((FVOID_STAR) previous_char_cmd); */
   b = bolp ();
   if (1 != prev_visible_chars (1))
     {
	eob_bob_error (-2);
	return 1;
     }
   
   Goal_Column = calculate_column();
   return b || JWindow->trashed;
}

int next_char_cmd ()
{
   Cursor_Motion = 1;
   
   /* check_last_key_function ((FVOID_STAR) next_char_cmd); */
   
   if (1 != next_visible_chars (1))
     {
	eob_bob_error (2);
	return 1;
     }

   Goal_Column = calculate_column ();
   return JWindow->trashed || bolp ();  /* Point = 0 ==> moved a line */
}

/*}}}*/

/*{{{ eol_cmd */
int eol_cmd (void)
{
   eol();
   if ((0 == (CBuf->flags & READ_ONLY))
#if JED_HAS_LINE_ATTRIBUTES
       && (0 == (CLine->flags & JED_LINE_IS_READONLY))
#endif
       )
     jed_trim_whitespace();
   return(1);
}

/*}}}*/

/*{{{ Scrolling/pageup/down functions */

int jed_buffer_visible (char *b)
{
   return buffer_visible (find_buffer(b));
}

static void scroll_completion (int dir)
{
   Window_Type *w;
   
   if (jed_buffer_visible (Completion_Buffer))
     {
	pop_to_buffer (Completion_Buffer);
   	if (dir > 0) pagedown_cmd (); else pageup_cmd ();
	while (!IS_MINIBUFFER) other_window ();
     }
   else
     {
	w = JWindow;
	other_window();
	if (!IS_MINIBUFFER)
	  {
	     if (dir > 0) pagedown_cmd (); else pageup_cmd ();
	  }
	while (JWindow != w) other_window ();
     }
}

int goto_bottom_of_window (void)
{
   int n;
   
   n = JWindow->rows - window_line ();

   return n == next_visible_lines (n);
}

void goto_top_of_window (void)
{
   int n;
   
   n = window_line () - 1;
   (void) prev_visible_lines (n);
}


static int Last_Page_Line;
static int Last_Page_Point;

/* the page up/down commands set cursor_motion to -1 because we do not
 want to use any goal column information */
int pagedown_cmd()
{
   int col, this_line, this_point;
   int n;
   
   Cursor_Motion = -1;
   if (IS_MINIBUFFER)
     {
	scroll_completion (1);
	return 1;
     }
   
   if (eobp())
     {
	eob_bob_error (3);
	return 1;
     }
   
   n = JWindow->rows;
   if ((CBuf != JWindow->buffer) || (n == 1))
     {
	return next_visible_lines (n);
     }
   
   if (JWindow->trashed)
     {
	update (NULL, 0, 0, 1);
	if (JWindow->trashed) return next_visible_lines (n);
     }

   /* This is ugly. */
   
   this_line = LineNum;
   this_point = Point;
   
   col = calculate_column ();
   if (goto_bottom_of_window ())
     {
	recenter (&Number_One);
     }
   
   goto_column1 (&col);
   
   if ((Last_Key_Function == (FVOID_STAR) pageup_cmd)
       && (Jed_This_Key_Function == (FVOID_STAR) pagedown_cmd))
     {
	goto_line (&Last_Page_Line);
	if (Last_Page_Point < CLine->len)
	  Point = Last_Page_Point;
     }
   else if (CLine->next == NULL) eol(); 
   else bol ();
   
   Last_Page_Line = this_line;
   Last_Page_Point = this_point;
   
   return(1);
}

int pageup_cmd (void)
{
   int col, this_line, this_point;
   int n;
   
   Cursor_Motion = -1;
   
   if (IS_MINIBUFFER)
     {
	scroll_completion (-1);
	return 1;
     }
   
   if (bobp())
     {
	eob_bob_error (-3);
	return 1;
     }
   
   n = JWindow->rows;
   if ((CBuf != JWindow->buffer) || (n == 1))
     {
	return prev_visible_lines (n);
     }
   
   if (JWindow->trashed)
     {
	update (NULL, 0, 0, 1);
	if (JWindow->trashed) return prev_visible_lines (n);
     }
   
   this_line = LineNum;
   this_point = Point;
   col = calculate_column ();
   goto_top_of_window ();
   (void) goto_column1(&col);
   recenter(&JWindow->rows);
   
   if ((Last_Key_Function == (FVOID_STAR) pagedown_cmd) 
       && (Jed_This_Key_Function == (FVOID_STAR) pageup_cmd))
     {
	goto_line (&Last_Page_Line);
	if (Last_Page_Point < CLine->len)
	  Point = Last_Page_Point;
     }
   else 
     bol (); /* something like: Point = point_column(JWindow->column) better? */
   Last_Page_Line = this_line;
   Last_Page_Point = this_point;
   return(1);
}

/*}}}*/

int jed_scroll_left_cmd (void)
{
   int dc = JWindow->width/2;
   if (Repeat_Factor != NULL)
     {
	dc = *Repeat_Factor;
	Repeat_Factor = NULL;
     }
   jed_scroll_left (dc);
   return 1;
}

int jed_scroll_right_cmd (void)
{
   int dc = JWindow->width/2;
   if (Repeat_Factor != NULL)
     {
	dc = *Repeat_Factor;
	Repeat_Factor = NULL;
     }
   jed_scroll_right (dc);
   return 1;
}


/*}}}*/

/*{{{ Whitespace and Indention functions */

void insert_whitespace(int *n)
{
   int tab = Buffer_Local.tab;
   int c1, c2, i, k, nspace;
   
   if ((nspace = *n) <= 0) return;
   CHECK_READ_ONLY_VOID
     c1 = calculate_column() - 1;
   c2 = c1 + nspace;
   
   if (tab && Jed_Use_Tabs)
     {
	i = c1 / tab;
	k = c2 / tab - i;
	if (k) nspace = c2 - (i + k) * tab;
	(void) jed_insert_wchar_n_times('\t', k);
     }
   (void) jed_insert_wchar_n_times(' ', nspace);
}

/* get indent value of current line, n is the column */
unsigned char *get_current_indent(int *np)
{
   unsigned char *p, *pmax;
   int tab, n;
   
   tab = Buffer_Local.tab;
   p = CLine->data;
   pmax = CLine->data + CLine->len;
   n = 0;
   while((p < pmax) && ((*p == ' ') || (tab && (*p == '\t'))))
     {
	if (*p == '\t')
	  n = tab * (n / tab + 1);
	else n++;
	p++;
     }
   *np = n;
   return p;
}

#define IS_WHITESPACE(p) ((*(p)==' ')||(*(p)=='\t'))
int jed_trim_whitespace ()
{
   int n;
   unsigned char *p;

   /* CHECK_READ_ONLY */

   p = CLine->data + Point;
   if ((0 == eolp ()) && (0 == IS_WHITESPACE(p)))
     return 0;

   (void) jed_skip_whitespace ();
   n = Point;
   
   (void) jed_bskip_whitespace ();
   jed_del_nbytes (n - Point);
   return 1;
}

/* indent line to column n */
void indent_to(int n)
{
   int m;
   
   get_current_indent(&m);
   
   if (n != m)
     {
	bol ();
	jed_trim_whitespace();
	if (n >= 0) insert_whitespace(&n);
     }
}

int indent_line ()
{
   int n, n1;
   static int in_function;
   
   if (in_function) return -1;
   
   if ((CBuf->buffer_hooks != NULL)
       && (CBuf->buffer_hooks->indent_hook != NULL))
     {
	in_function = 1;
	SLexecute_function(CBuf->buffer_hooks->indent_hook);
	in_function = 0;
	return 1;
     }
   
   /* CHECK_READ_ONLY */
     
     if (CLine == CBuf->beg) return(0);
   
   push_spot();
   CLine = CLine->prev;
   get_current_indent (&n);
   CLine = CLine->next;
   indent_to(n);
   pop_spot();
   
   /* This moves the cursor to the first non whitspace char if we are
    before it.  Otherwise leave it alone */
   
   n1 = calculate_column();
   get_current_indent(&n);
   if (n1 <= n) point_column(n + 1);
   return 1;
}

/*}}}*/

/*{{{ goto_column and goto_column1 */

/* goto to column c, returns actual column */
int goto_column1(int *cp)
{
   int c1, c = *cp;
   if (c <= 0) c = 1;
   eol();
   c1 = calculate_column();
   if (c1 > c)
     {
	point_column(c);
	c1 = calculate_column();
     }
   return(c1);
}

/* move cursor to column c adding spaces if necessary */
void goto_column(int *c)
{
   int c1 = *c;
   if (c1 <= 0) c1 = 1;
   c1 = c1 - goto_column1(&c1);
   insert_whitespace(&c1);
}

/*}}}*/
/*{{{ skip_whitespace */

/* does not leave current line */
unsigned char *jed_skip_whitespace (void)
{
   unsigned char *p, *pmax;

   p = CLine->data + Point;
   pmax = jed_eol_position (CLine);

   while ((p < pmax) && IS_WHITESPACE(p))
     p = jed_multibyte_chars_forward (p, pmax, 1, NULL, 0);
   
   jed_position_point (p);
   return p;
}

unsigned char *jed_bskip_whitespace (void)
{
   unsigned char *p, *pmin, *pmax;

   pmin = CLine->data;
   pmax = p = CLine->data + Point;

   while (p > pmin) 
     {
	unsigned char *p1;
	p1 = jed_multibyte_chars_backward (pmin, p, 1, NULL, 0);
	if (0 == IS_WHITESPACE(p1))
	  break;
	p = p1;
     }
   jed_position_point (p);
   return p;
}

/*}}}*/

/*{{{ looking_at */

/* MULTIBYTE OK */
int jed_looking_at (char *what)
{
   unsigned char *p, *pmax;
   unsigned char *w, *wmax;
   Line *l = CLine;
   int cs = Buffer_Local.case_search;
   
   w = (unsigned char *) what;
   wmax = w + strlen (what);

   p = l->data + Point;
   while (1)
     {
	pmax = l->data + l->len;
	
	if (cs)
	  {
	     while ((w < wmax) && (p < pmax))
	       {
		  if (*w++ != *p++)
		    return 0;
	       }
	  }
	else 
	  {
	     /* Here we have to use multibyte routines */
	     while ((w < wmax) && (p < pmax))
	       {
		  if (0 != jed_multibyte_charcasecmp (&w, wmax, &p, pmax))
		    return 0;
	       }
	  }
	if (w == wmax) return 1;

	l = l->next;
	if (l == NULL) return 0;
	p = l->data;
     }
}

/*}}}*/

/*{{{ Exiting the editor */

/*{{{ sys_spawn_cmd */

int jed_spawn_fg_process (int (*f)(VOID_STAR), VOID_STAR cd)
{
   int status;
   int inited;

   if ((Jed_Secure_Mode)
       || (Jed_Suspension_Not_Allowed))
     {
	msg_error ("Access to shell denied.");
	return -1;
     }

   /* FIXME: X_Suspend_Hook should not be here.  Currently, this hook is
    * used only by GUI jed, where suspension makes no sense.  Of course in
    * this case, spawning a foreground process also does not make sense.
    */
   if (Batch || (X_Suspend_Hook != NULL))
     return (*f) (cd);

   SLsig_block_signals ();
   inited = Jed_Display_Initialized;
   SLsig_unblock_signals ();
   
   jed_reset_display();
#if !defined(IBMPC_SYSTEM) && !defined(VMS)
   jed_reset_signals ();
#endif
   reset_tty();

   status = (*f) (cd);

   if (inited)
     {
#if !defined(IBMPC_SYSTEM) && !defined(VMS)
	init_signals();
#endif
	if (-1 == init_tty())
	  {
	     exit_error ("Unable to initialize terminal.", 0);
	  }
   
	flush_input ();
	jed_init_display ();
     }

   check_buffers();
   return status;
}

static int suspend_func (void *unused)
{
   (void) unused;
   
   if (X_Suspend_Hook != NULL)
    (*X_Suspend_Hook) ();
   else
     sys_suspend();
   
   return 0;
}
   
int sys_spawn_cmd (void)
{
   if (X_Suspend_Hook != NULL)
     {
	(*X_Suspend_Hook) ();
	return 0;
     }

   if (jed_va_run_hooks ("_jed_suspend_hooks", JED_HOOKS_RUN_UNTIL_0, 0) <= 0)
     return 0;
   
   (void) jed_spawn_fg_process (suspend_func, NULL);

   (void) jed_va_run_hooks ("_jed_resume_hooks", JED_HOOKS_RUN_ALL, 0);

   return 0;
}
   

/*}}}*/
   
/*{{{ jed_quit_jed */
int jed_quit_jed(int status)
{
   Buffer *b;

   (void) jed_va_run_hooks ("_jed_quit_hooks", JED_HOOKS_RUN_ALL, 0);

   b = CBuf;
   /* Any buffer marked with AUTO_SAVE_JUST_SAVE flag should be saved
    * if it has not already been.  That is what the flag is for and this
    * code fragment carries this out.
    */
   do
     {
	if ((b->flags & AUTO_SAVE_JUST_SAVE)
	    && (b->flags & BUFFER_MODIFIED)
	    && (*b->file))
	  {
	     while (b->narrow != NULL) widen_buffer(b);
	     auto_save_buffer(b);      /* actually, it will save it */
	  }
	
#if defined(__WIN32__) && JED_HAS_SUBPROCESSES
 	if (b->subprocess)
 	  jed_kill_process (b->subprocess - 1);
#endif

	(void) jed_unlock_buffer_file (b);
	
	b = b->next;
     }
   while (b != CBuf);
   
   jed_reset_display();
   reset_tty();
#ifdef VMS
   vms_cancel_exithandler();
#endif
#ifdef SLANG_STATS
   SLang_dump_stats("slang.dat");
#endif
#ifdef MALLOC_DEBUG
   SLmalloc_dump_statistics ();
#endif
   /* SLstring_dump_stats (); */
   exit (status);
   return(1);
}


/*}}}*/


/*{{{ save_some_buffers */
/* I should try it like emacs--- if prefix argument, then save all without user
 intervention */
int save_some_buffers (void)
{
   Buffer *b, *tmp;
   int ans = 0;
   int err;
   
   b = CBuf;
   do
     {
	if ((b->flags & BUFFER_MODIFIED)
	    && (*b->file))
	  {
	     if (b->flags & AUTO_SAVE_JUST_SAVE) ans = 1;
	     else ans = jed_vget_y_n ("Buffer %s not saved. Save it",
				      b->name);
	     
	     if (ans == -1)
	       /* warning--- bug here if user edits file at
		startup and forgets to save it then aborts. */
	       return -1;
	     
	     if (ans == 1)
	       {
		  tmp = CBuf;
		  switch_to_buffer(b);

		  /* It should not be necessary to do this here.  Lower
		   * level routines will do it.
		   */
		  /* while (b->narrow != NULL) widen_buffer(b); */

		  err = jed_save_buffer_cmd ();
		  switch_to_buffer(tmp);
		  if (err < 0) return -1;
		  /* b->flags &= ~BUFFER_MODIFIED; */
		  /* b->flags |= AUTO_SAVE_BUFFER; */
		  b->hits = 0;
	       }
	  }
	
	b = b->next;
     }
   while (b != CBuf);

   clear_message ();
   return 1;
}

/*}}}*/
/*{{{ exit_jed */
int jed_exit_jed (int status)
{
   static int in_exit_jed = 0;
   
   if (in_exit_jed == 0)
     {
	in_exit_jed = 1;
	if (jed_va_run_hooks ("_jed_exit_hooks", JED_HOOKS_RUN_UNTIL_0, 0) <= 0)
	  {
	     in_exit_jed = 0;
	     return -1;
	  }
     }

   in_exit_jed = 0;
   if (SLang_get_error ())
     return -1;

#if JED_HAS_SUBPROCESSES
   if (1 != jed_processes_ok_to_exit ())
     return 1;
#endif
   
   if (save_some_buffers() > 0) jed_quit_jed(status);
   return 1;
}

/*}}}*/

/*}}}*/

int jed_exit_jed_cmd (void)
{
   return jed_exit_jed (0);
}
