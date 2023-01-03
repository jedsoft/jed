/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

/*{{{ Include Files */

#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "window.h"
#include "screen.h"
#include "misc.h"
#include "ledit.h"
#include "sysdep.h"
#include "display.h"
#include "paste.h"

/*}}}*/

Window_Type *JMiniWindow;
Window_Type *JWindow;

int Top_Window_SY = 1;

static Window_Type *alloc_window(int sy, int sx, int rows, int col, int width)
{
    Window_Type *w;

    if (NULL == (w = (Window_Type *) jed_malloc0 (sizeof(Window_Type))))
      {
	  exit_error("create_window: malloc error.", 0);
      }

   if (sy < 0) sy = 0;
   if (sx < 0) sx = 0;
   w->sx = sx;
   w->sy = sy;
   if (rows < 1) rows = 1;
   w->rows = rows;
   if (width < 1) width = 1;
   w->width = width;
   w->hscroll_column = col;

   return w;
}

static Window_Type *new_window (int sy, int sx, int rows, int col, int width)
{
   Window_Type *w = alloc_window (sy, sx, rows, col, width);

   if (w == NULL)
     return w;

   if ((jed_new_window_cb != NULL)
       && (-1 == (*jed_new_window_cb)(w)))
     exit_error ("The new_window callback function failed", 0);

   return w;
}

Window_Type *jed_create_minibuffer_window (void)
{
   Window_Type *w;
   w = alloc_window (Jed_Num_Screen_Rows-1, 0, 1, 1, Jed_Num_Screen_Cols);
   if (w == NULL)
     return NULL;

   w->flags |= MINIBUFFER_WINDOW;

   if ((jed_create_mini_window_cb != NULL)
       && (*jed_create_mini_window_cb)(w))
     exit_error ("Unable to create a mini-buffer window", 0);

   return w;
}

static void free_window (Window_Type *w)
{
   if (w == NULL)
     return;

   if (Mini_Info.action_window == w)
     Mini_Info.action_window = NULL;

   if (jed_free_window_cb != NULL)
     (*jed_free_window_cb)(w);

   SLfree ((char *) w);
}

void window_buffer(Buffer *b) /*{{{*/
{
   if (JWindow == NULL)
     {
	JWindow = new_window (Top_Window_SY, 0,
			      Jed_Num_Screen_Rows - 2 - Top_Window_SY,
			      1, Jed_Num_Screen_Cols);
	JWindow->next = JWindow;
     }

   touch_window();

   jed_init_mark_for_buffer (&JWindow->beg, b, 0);
   jed_init_mark_for_buffer (&JWindow->mark, b, 0);
   JWindow->hscroll_column = 1;
   JWindow->buffer = b;
   JWindow->trashed = 1;
}

/*}}}*/

#if JED_HAS_SUBPROCESSES
void move_window_marks (int all) /*{{{*/
{
   Window_Type *w = JWindow;
   if (w == NULL) return;
   do
     {
	if (w->buffer == CBuf)
	  {
	     jed_init_mark (&w->mark, 0);
	     if (all == 0) break;
	  }
	w = w->next;
     }
   while (w != JWindow);
}

/*}}}*/
#endif

int other_window (void) /*{{{*/
{
   switch_to_buffer(JWindow->buffer);
   jed_init_mark (&JWindow->mark,0);
   if (JWindow->next == JWindow)
     return 1;

   if (jed_leave_window_cb != NULL)
     (void) (*jed_leave_window_cb)(JWindow);

   JWindow = JWindow->next;
   switch_to_buffer(JWindow->buffer);
   if (JWindow->mark.line != NULL)
     (void) jed_goto_mark (&JWindow->mark);

   if (jed_enter_window_cb != NULL)
     (void) (*jed_enter_window_cb)(JWindow);

   return 1;
}

/*}}}*/

int split_window (void) /*{{{*/
{
   int n, sy, width, row;
   Window_Type *w, *neew;
   /* Line *l, cline; */

    if (JWindow->rows < 5)
      {
	  msg_error("Window too small.");
	  return(0);
      }

   switch_to_buffer(JWindow->buffer);
   n = JWindow->rows / 2;
   sy = JWindow->sy + n + 1;
   width = JWindow->width;
   n = JWindow->rows - n - 1;
   JWindow->rows = JWindow->rows / 2;

   jed_init_mark (&JWindow->beg, 0);

   w = JWindow->next;
   JWindow->next = neew = new_window (sy, JWindow->sx, n, JWindow->hscroll_column, width);

   neew->next = w;
   neew->buffer = CBuf;
   jed_init_mark (&neew->mark, 0);
   jed_copy_mark (&neew->beg, &JWindow->beg);

   if (jed_split_window_cb != NULL)
     {
	(void) (*jed_split_window_cb)(JWindow, neew);
     }

   n = JWindow->sy;

   other_window();
   touch_window();

   if (-1 != (row = jed_find_line_on_screen (CLine, n)))
     {
	/* The screen row below (not above) the current window containing
	 * the current line was found.  Switch to that buffer to avoid
	 * the cursor jumping.  This is not necessary but it has a more
	 * appealing visual effect.
	 */
	row += 1;

	w = JWindow;
	do
	  {
	     if ((JWindow->buffer == CBuf) && (JWindow->sy < row)
		 && (JWindow->sy + JWindow->rows >= row)) break;
	     other_window();
	  }
	while (w != JWindow);
     }
   return 1;
}

/*}}}*/

int one_window (void) /*{{{*/
{
   Window_Type *w, *next, *mini;
   Buffer *b;
   mini = NULL;
   if (IN_MINI_WINDOW) return 0;
   w = JWindow->next;
   b = JWindow->buffer;
   while (w != JWindow)
     {
	next = w->next;
	if (w != JMiniWindow)
	  {
	     if (w->buffer != b) touch_window_hard (w, 0);
	     free_window (w);
	  }
	else mini = w;
	w = next;
     }
   if (mini == NULL) mini = JWindow;
   JWindow->next = mini;
   mini->next = JWindow;
   JWindow->sy = Top_Window_SY;
   JWindow->sx = 0;
   JWindow->width = Jed_Num_Screen_Cols;
   JWindow->rows = Jed_Num_Screen_Rows - 2 - Top_Window_SY;
   touch_window();
   return(1);
}
/*}}}*/

int enlarge_window (void) /*{{{*/
{
   Window_Type *w, *w1;
   int min = 2;

   if (IN_MINI_WINDOW) return 0;

   if (JWindow == JWindow->next) return(0);
   w = JWindow->next;
   while(w->rows <= min) w = w->next;
   if (w == JWindow) return(0);

   if (w->sy < JWindow->sy)
     {
	w->rows -= 1;
	JWindow->rows += 1;
	do
	  {
	     w = w->next;
	     w->sy -= 1;
	  }
	while (w != JWindow);
     }
   else
     {
	JWindow->rows += 1;
	w1 = JWindow;
	while(w1 != w)
	  {
	     w1 = w1->next;
	     w1->sy += 1;
	  }
	w->rows -= 1;
     }
   w = JWindow;
   do
     {
	touch_window();
	JWindow = JWindow->next;
     }
   while (w != JWindow);

   if ((jed_window_geom_change_cb != NULL)
       && (-1 == (*jed_window_geom_change_cb) ()))
     exit_error ("window_geom_change callback failed", 0);

   return 1;
}

/*}}}*/
static void adjust_windows (int height) /*{{{*/
{
   Window_Type *w = JWindow;
   int rows;

   do
     {
	if (w->rows + w->sy + 2 == Jed_Num_Screen_Rows)
	  {
	     /* bottom window */
	     rows = height - 2 - w->sy;
	     if (rows > 1)
	       {
		  w->rows = rows;
		  return;
	       }

	     while (JWindow->sy != Top_Window_SY) other_window();

	     one_window();
	     JWindow->rows = height - 2 - Top_Window_SY;
	     if (JWindow->rows < 1) JWindow->rows = 1;
	     return;
	  }
	w = w->next;
     }
   while (w != JWindow);
   /* not reached! */
}

/*}}}*/

void jed_update_window_sizes (int height, int width) /*{{{*/
{
   Window_Type *w;

   if (JWindow == NULL) return;

   if (height < 5) height = 5;
   if (width < 5) width = 5;

   if (height != Jed_Num_Screen_Rows)
     adjust_windows(height);

   w = JWindow;
   do
     {
	JWindow->trashed = 1;
	JWindow->width = width;
	JWindow = JWindow->next;
     }
   while (w != JWindow);

   if (JMiniWindow != NULL)
     {
	JMiniWindow->sy = height-1;
	JMiniWindow->width = width;
     }

   if ((jed_window_geom_change_cb != NULL)
       && (-1 == (*jed_window_geom_change_cb) ()))
     exit_error ("window_geom_change callback failed", 0);
}

/*}}}*/
int buffer_visible(Buffer *b) /*{{{*/
{
   Window_Type *w = JWindow;
   int n;

   if ((b == NULL) || (w == NULL))
     return 0;

   n = 0;
   do
     {
	if (w->buffer == b) n++;
	w = w->next;
     }
   while (w != JWindow);
   return n;
}

/*}}}*/

int delete_window (void) /*{{{*/
{
   Window_Type *tthis, *prev, *next;
   int nr1;

   tthis = JWindow;
   next = tthis->next;
   if ((MiniBuffer_Active && ((tthis == JMiniWindow) || (tthis == next->next)))
       || (tthis == next)) return(0);

   nr1 = tthis->sy + tthis->rows + 2;
   if (nr1 != Jed_Num_Screen_Rows)
     {
	/* Not the bottom window.  Move to the window below this one */
	while (JWindow->sy + 1 != nr1)
	  other_window();
	JWindow->sy = tthis->sy;
     }
   else
     {
	/* The bottom window.  Move to the window above this one. */
	while (JWindow->sy + JWindow->rows + 1 != tthis->sy)
	  other_window();
     }

   JWindow->rows += tthis->rows + 1;
   touch_window();

   prev = next;
   while(prev->next != tthis) prev = prev->next;
   prev->next = next;

   free_window (tthis);
   return(1);
}

/*}}}*/

void touch_screen_for_buffer(Buffer *b) /*{{{*/
{
   Window_Type *w;

   w = JWindow;
   do
     {
	if (w->buffer == b)
	  {
	     touch_window_hard (w, 0);
	  }
	w = w->next;
     }
   while(w != JWindow);
}

/*}}}*/
int is_line_visible (int lnum) /*{{{*/
{
   int n = JWindow->rows;
   Line *l, *beg = JWindow->beg.line;

   push_spot ();
   goto_line (&lnum);
   l = CLine;
   pop_spot ();

#if JED_HAS_LINE_ATTRIBUTES
   if (l->flags & JED_LINE_HIDDEN) return 0;
#endif

   while (n && (beg != NULL))
     {
	if (l == beg) return 1;

#if JED_HAS_LINE_ATTRIBUTES
	if (0 == (beg->flags & JED_LINE_HIDDEN))
#endif
	  n--;

	beg = beg->next;
     }
   return 0;
}

/*}}}*/

int jed_num_windows (void)
{
   Window_Type *w = JWindow;
   int n = 0;
   do
     {
	n++;
	w = w->next;
     }
   while (w != JWindow);
   return n;
}

void jed_set_scroll_column (int sc)
{
   if (sc < 1) sc = 1;
   JWindow->hscroll_column = sc;
   touch_window();
}

void jed_scroll_right (int dn)
{
   jed_set_scroll_column (JWindow->hscroll_column - dn);
}

void jed_scroll_left (int dn)
{
   jed_set_scroll_column (JWindow->hscroll_column + dn);
}

int jed_get_scroll_column (void)
{
   return JWindow->hscroll_column;
}

static void set_scroll_column_intrin (int *sc)
{
   jed_set_scroll_column (*sc);
}

static int window_info_intrin(int *what)
{
   register int n = 0;
   switch (*what)
     {
      case 'x': n = JWindow->sx; break;
      case 'y': n = JWindow->sy; break;
      case 'r': n = JWindow->rows; break;
      case 'c': n = JWindow->hscroll_column; break;
      case 't': n = JWindow->sy + 1; break;
      case 'w': n = JWindow->width; break;
      default:
	SLang_set_error (SL_INVALID_PARM);
     }
   return (n);
}

static SLang_Intrin_Fun_Type Window_Intrinsics [] =
{
   MAKE_INTRINSIC_0("enlargewin", enlarge_window, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("splitwindow", split_window, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("window_line", window_line, INT_TYPE),
   MAKE_INTRINSIC_0("nwindows", jed_num_windows, SLANG_INT_TYPE),
   MAKE_INTRINSIC_0("otherwindow", other_window, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("onewindow", one_window, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_I("window_info", window_info_intrin, SLANG_INT_TYPE),
   MAKE_INTRINSIC_I("set_scroll_column", set_scroll_column_intrin, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("get_scroll_column", jed_get_scroll_column, SLANG_INT_TYPE),
   SLANG_END_INTRIN_FUN_TABLE
};

int jed_init_window_intrinsics (void)
{
   if (-1 == SLadd_intrin_fun_table (Window_Intrinsics, NULL))
     return -1;

   return 0;
}

