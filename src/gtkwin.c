/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#define _BUILD_GTK_JED

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

#include "gtkjed.h"

/*}}}*/

/* window.c will call these callback functions.  They need to be mapped onto
 * the appropriate combination of functions in gtkterm.c
 */
int (*jed_new_window_cb) (Window_Type *) = NULL;
void (*jed_free_window_cb) (Window_Type *) = NULL;
int (*jed_create_mini_window_cb) (Window_Type *) = jgtk_createEditorMiniWin;
int (*jed_leave_window_cb) (Window_Type *) = NULL;
int (*jed_enter_window_cb) (Window_Type *) = NULL;
int (*jed_split_window_cb) (Window_Type *, Window_Type *) = NULL;
int (*jed_window_geom_change_cb) (void) = NULL;

#if 0 /* These rest of the file commented out */

Window_Type *JWindow;
int Top_Window_SY = 1;

/************************************
* create_window
*
* debug print:
*
************************************/

Window_Type *
create_window( int sy, int sx, int rows, int col, int width ) /*{{{*/
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

   createEditorMiniWin( w );

   return(w);
}

/************************************
* createGJedwindow
*
* debug print:
*
************************************/

static Window_Type *
createGJedWindow(int sy, int sx, int rows, int col, int width) /*{{{*/
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

   return(w);
}

/*}}}*/

/************************************
* free_window
*
* debug print:
*
************************************/

static void free_window (Window_Type *w)
{
   if (w == NULL)
     return;

   if (Mini_Info.action_window == w)
     Mini_Info.action_window = NULL;

   SLfree ((char *) w);
}

/************************************
* window_buffer
*
* debug print:
*
************************************/

void
window_buffer(Buffer *b) /*{{{*/
{
   if (JWindow == NULL)
     {
	JWindow = createGJedWindow( Top_Window_SY, 0,
				    Jed_Num_Screen_Rows - 2 - Top_Window_SY,
				    1, Jed_Num_Screen_Cols);
	JWindow->next = JWindow;
	createTopEdWin( JWindow, Top_Window_SY, 0,
			Jed_Num_Screen_Rows - 2 - Top_Window_SY,
			1, Jed_Num_Screen_Cols );
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
/************************************
* move_window_marks
*
* debug print: "All: %d\n", all
*
************************************/

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

/************************************
* other_window
*
* debug print:
*
************************************/

int
other_window() /*{{{*/
{
   switch_to_buffer( JWindow->buffer );
   jed_init_mark( &JWindow->mark, 0 );

   updateScrollbar( JWindow );

   JWindow = JWindow->next;
   switch_to_buffer( JWindow->buffer );
   if ( JWindow->mark.line != NULL )
     ( void ) jed_goto_mark( &JWindow->mark );

   /* updateScrollbar( JWindow ); */

   return 1;
}

/*}}}*/

/************************************
* split_window
*
* debug print:
*
************************************/

int split_window (void) /*{{{*/
{
   int n, sy, width, row;
   Window_Type *w, *neew, *tmpWin;
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

#if 0
   /* l = find_top (); */
   l = jed_find_top_to_recenter (CLine);
   if (l == NULL)
     l = CLine;
   (void) jed_init_mark_for_line (&JWindow->beg, l, 0);
#else
   jed_init_mark (&JWindow->beg, 0);
#endif
   w = JWindow->next;
   JWindow->next = neew = createGJedWindow( sy, JWindow->sx, n, JWindow->hscroll_column, width );
   neew->next = w;

   neew->buffer = CBuf;
   jed_init_mark (&neew->mark, 0);
   jed_copy_mark (&neew->beg, &JWindow->beg);

   tmpWin = JWindow;

   /* splitEdWin( JWindow, neew, sy, JWindow->sx, n, JWindow->hscroll_column, width ); */

   n = JWindow->sy;

   other_window();
   touch_window();

   if (-1 != (row = jed_find_line_on_screen (CLine, n)))
     {
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

   splitEdWin( tmpWin, neew, sy, tmpWin->sx, neew->rows, tmpWin->hscroll_column, width );

   /* printWinList(); */

   return 1;
}

/*}}}*/
/************************************
* one_window
*
* debug print:
*
************************************/

int
one_window( void ) /*{{{*/
{
   Window_Type *w, *next, *mini;
   Buffer *b;
   mini = NULL;
   if ( JWindow->sy + 1 == Jed_Num_Screen_Rows ) return(0);  /* mini-buffer */
   w = JWindow->next;
   b = JWindow->buffer;
   while( w != JWindow )
     {
	next = w->next;
	if (w != The_MiniWindow)
	  {
	     if ( w->buffer != b ) touch_window_hard( w, 0 );
	     free_window (w);
	  }
	else mini = w;
	w = next;
     }
   if (mini == NULL) mini = JWindow;
   JWindow->next = mini;
   mini->next = JWindow;

   updOWEdWin( w, Top_Window_SY, 0,
	       Jed_Num_Screen_Rows - 2 - Top_Window_SY,
	       Jed_Num_Screen_Cols );

   JWindow->sy = Top_Window_SY;
   JWindow->sx = 0;
   JWindow->width = Jed_Num_Screen_Cols;
   JWindow->rows = Jed_Num_Screen_Rows - 2 - Top_Window_SY;

   /**********
   printf( "1 (oneWindow): vvvvvv---------------------------------------vvvvvv\n" );
   printWidgetWinXRef();
   printSubWins();
   printf( "1:(oneWindow): ^^^^^^---------------------------------------^^^^^^\n" );
   ***********/

   touch_window();
   return(1);
}

/*}}}*/
/************************************
* enlarge_window
*
* debug print:
*
************************************/

int
enlarge_window() /*{{{*/
{
   Window_Type *w, *w1;
   int min = 2;

   if (JWindow == The_MiniWindow) return(0);
   /* if (IN_MINI_WINDOW) Return(0); */
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
	     updEdWidgetWinXRef( w, w->sy + w->rows, 1 );

         /* updEdWidgetWinXRef( w, w->sy + w->rows - 1, 1 ); */
	     w = w->next;
	     w->sy -= 1;
	     updEdWidgetWinXRef( w, w->sy, 1 );
	  }
	while (w != JWindow);
     }
   else
     {
	JWindow->rows += 1;
	w1 = JWindow;
	while(w1 != w)
	  {
	     updEdWidgetWinXRef( w1, w1->sy + w1->rows, 1 );
	     updEdWidgetWinXRef( w1, w1->sy + w1->rows - 1, 1 );
	     w1 = w1->next;
	     w1->sy += 1;
	  }
	w->rows -= 1;
     }

   jGtkSetWinSizes();

   w = JWindow;
   do
     {
	touch_window();
	JWindow = JWindow->next;
     }
   while (w != JWindow);

   return(1);
}

/*}}}*/
/************************************
* adjust_windows
*   Search for the last window, the bottom window.
*   Check whether num of rows > 1
*   if not delete all but one window
*
* debug print: "Height: %d\n", height
*
************************************/

static void
adjust_windows( int height ) /*{{{*/
{
   Window_Type *w = JWindow;
   int rows;

   do
     {
      /* printf( "Win(w): %x, w->rows: %d, w->sy: %d, Jed_Num_Screen_Rows: %d\n", */
      /*              w, w->rows, w->sy, Jed_Num_Screen_Rows ); */
	if (w->rows + w->sy + 2 == Jed_Num_Screen_Rows)
	  {
         /* printf( "Win(w): %x, w->rows: %d, w->sy: %d, Jed_Num_Screen_Rows: %d\n", */
         /*            w, w->rows, w->sy, Jed_Num_Screen_Rows ); */
	 /* Db; */
	 /* bottom window */
	     rows = height - 2 - w->sy;
	     if ( rows > 1 )
	       {
	    /* if ( rows > w->rows ) */
	    /*  { */
	    /*   carryForwGtkWidgetWinXRefLines( w, w->rows + w->sy, w->sx, rows - w->rows, w->col ); */
	    /*   } */

	    /*  Update miniwindow in widgetWinXRef */

		  updMiniWinWidgetWinXRef( height - 1 );

		  if ( rows > w->rows )
		    updEdWidgetWinXRef( w, w->sy + w->rows + 1, rows - w->rows );

		  w->rows = rows;

		  return;
	       }

	 /* Db; */

	     while( JWindow->sy != Top_Window_SY ) other_window();

	     one_window();
	 /* Adjust Height done in one window???!! */

	     JWindow->rows = height - 2 - Top_Window_SY;
	     if (JWindow->rows < 1) JWindow->rows = 1;

	     return;
	  }
      /* Dbp( "w: %x\n", w ); */
	w = w->next;
      /* Dbp( "w: %x\n", w ); */
     }
   while (w != JWindow);
   /* not reached! */
}

/*}}}*/

/************************************
* jed_update_window_sizes
*
*  Jed_Num_Screen_Rows and Jed_Num_Screen_Cols still old values
*
* debug print: "Heigth: %d, Width: %d\n", height, width
*
************************************/

void
jed_update_window_sizes( int height, int width ) /*{{{*/
{
   Window_Type *w;

   /************
   printf( "1 (jed_update_window_sizes): vvvvvv---------------------------------------vvvvvv\n" );
   printWidgetWinXRef();
   printSubWins();
   printf( "1:(jed_update_window_sizes): ^^^^^^---------------------------------------^^^^^^\n" );
   **************/

   if ( JWindow == NULL ) return;

   if ( height < 5 ) height = 5;
   if ( width < 5 ) width = 5;

   jGtkCheckEdSize( height, width );

   if ( height != Jed_Num_Screen_Rows )
     adjust_windows( height );

   /* jedGtkUpdateEdSize( height, width ); */

   if ( width > JWindow->width )
     {
	jGtkWidenEd( height, width, JWindow->width );
     }

   w = JWindow;
   do
     {
	JWindow->trashed = 1;
	JWindow->width = width;
	JWindow = JWindow->next;
     }
   while ( w != JWindow );

   if ( The_MiniWindow != NULL )
     {
      /* jedGtkUpdateMiniWinSizeLoc( The_MiniWindow, height - 1, width ); */
	The_MiniWindow->sy = height - 1;
	The_MiniWindow->width = width;
     }
}

/*}}}*/
/************************************
* buffer_visible
*
* debug print:
*
************************************/

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

/************************************
* delete_window
*
* debug print:
*
************************************/

int
delete_window (void) /*{{{*/
{
   Window_Type *tthis, *prev, *next;
   int nr1;

   tthis = JWindow;
   next = tthis->next;
   if ((MiniBuffer_Active && ((tthis == The_MiniWindow) || (tthis == next->next)))
       || (tthis == next)) return(0);

   nr1 = tthis->sy + tthis->rows + 2;
   if (nr1 != Jed_Num_Screen_Rows)
     {
	while (JWindow->sy + 1 != nr1) other_window();
	JWindow->sy = tthis->sy;
     }
   else
     {
	while(JWindow->sy + JWindow->rows + 1 != tthis->sy) other_window();
     }

   JWindow->rows += tthis->rows + 1;
   touch_window();

   prev = next;
   while(prev->next != tthis) prev = prev->next;
   prev->next = next;

   delEdWin( tthis, JWindow,
	     tthis->sy, tthis->sx, tthis->rows, tthis->width,
	     nr1 == Jed_Num_Screen_Rows ? JWindow->sy + JWindow->rows  : tthis->sy + tthis->rows );

   free_window (tthis);
   return(1);
}

/*}}}*/

/************************************
* touch_screen_for_buffer
*
* debug print:
*
************************************/

void
touch_screen_for_buffer(Buffer *b) /*{{{*/
{
   Window_Type *w;

   w = JWindow;
   do
     {
	if ( w->buffer == b )
	  {
	     touch_window_hard( w, 0 );
	  }
	w = w->next;
     }
   while ( w != JWindow );
}

/*}}}*/
/************************************
* is_line_visible
*
* debug print:
*
************************************/

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

/************************************
* jed_num_windows
*
* debug print:
*
************************************/

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

/************************************
* jed_set_scroll_column
*
* debug print:
*
************************************/

void jed_set_scroll_column (int sc)
{
   if (sc < 1) sc = 1;
   JWindow->hscroll_column = sc;
   touch_window();
}

/************************************
* jed_scroll_right
*
* debug print:
*
************************************/

void jed_scroll_right (int dn)
{
   jed_set_scroll_column (JWindow->hscroll_column - dn);
}

/************************************
* jed_scroll_left
*
* debug print:
*
************************************/

void jed_scroll_left (int dn)
{
   jed_set_scroll_column (JWindow->hscroll_column + dn);
}

/************************************
* jed_get_scroll_column
*
* debug print:
*
************************************/

int jed_get_scroll_column (void)
{
   return JWindow->hscroll_column;
}

/************************************
* set_scroll_column_intrin
*
* debug print:
*
************************************/

static void set_scroll_column_intrin (int *sc)
{
   jed_set_scroll_column (*sc);
}

/************************************
* window_info_intrin
*
* debug print:
*
************************************/

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

/************************************
* printWinList
*
* debug print:
*
************************************/

int
printWinList(void)
{
   int i = 0;
   Window_Type *w = JWindow;

   printf( "Window List: \n" );

   do
     {
	++i;
	printf( "I: %3d, Sx: %4d, Sy: %4d, Rows: %4d, Colums: %4d\n",
		i, w->sx, w->sy, w->rows, w->width );
	w = w->next;
     }
   while ( w != JWindow );

   return( 1 );
}

static SLang_Intrin_Fun_Type Window_Intrinsics [] =
{
   MAKE_INTRINSIC_0("printWinList", printWinList, SLANG_VOID_TYPE),
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

/************************************
* jed_init_window_intrinsics
*
* debug print:
*
************************************/

int jed_init_window_intrinsics (void)
{
   if (-1 == SLadd_intrin_fun_table (Window_Intrinsics, NULL))
     return -1;

   return 0;
}

#endif				       /* REST of FILE */
