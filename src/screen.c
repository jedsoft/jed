/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#include <ctype.h>
#include <slang.h>

#include "jdmacros.h"

#include <string.h>
#if defined(IBMPC_SYSTEM) && !defined(__CYGWIN32__) && !defined(__IBMC__)
# include <dos.h>
#endif

#include "buffer.h"
#include "screen.h"
#include "window.h"
#include "paste.h"

#include "ins.h"
#include "ledit.h"
#include "display.h"
#include "sysdep.h"
#include "misc.h"
#include "file.h"
#include "hooks.h"
#include "menu.h"
#include "version.h"
#include "indent.h"
#include "colors.h"

#if JED_HAS_SUBPROCESSES
# include "jprocess.h"
#endif

#include "sig.h"

volatile int Jed_Resize_Pending;
char *MiniBuf_Get_Response_String;

int Jed_Simulate_Graphic_Chars;

typedef struct Screen_Type
{
   Line *line;		       /* buffer line structure */
   int wrapno;
   unsigned char *hi0, *hi1;	       /* beg end of hilights */
   int is_modified;
}
Screen_Type;

static Screen_Type *JScreen;

int Jed_Num_Screen_Rows;
int Jed_Num_Screen_Cols;

int Screen_Row = 1;
int Screen_Col = 1;
int Cursor_Motion;    /* indicates cursor movement only -1 ^ v +1 < > */
int Jed_Dollar = '$';
int Jed_Highlight_WS = 0;
int User_Prefers_Line_Numbers = 0;
int Mode_Has_Syntax_Highlight;
int Wants_Syntax_Highlight = 1;	       /* if non-zero, highlight the syntax.
					*/
int Wants_Attributes = 1;
int Wants_HScroll = 20;		       /* controls automatic horizontal
					* scrolling.  If positive, scroll
					* only line, if negative, whole window
					*/
int Term_Supports_Color = 1;	       /* optimistic assumption */

int Goal_Column;

int Want_Eob = 0;
int Display_Time = 1;                  /* Turn on %t processing in status line */

void (*X_Update_Open_Hook)(void);      /* hooks called when starting */
void (*X_Update_Close_Hook)(void);     /* and finishing update */

/* site.sl should modify this */
char Jed_Default_Status_Line[JED_MAX_STATUS_LEN] =
  " ^Ke: quit, ^Kg: get file, ^K^W: write file | %b  (%m%n%o) %p";

static Line *HScroll_Line;
static int HScroll;		       /* amount to scroll line by */
static int Absolute_Column;

static int Point_Cursor_Flag = 1;      /* if non-zero, point cursor */
int Jed_Display_Initialized;

static Line Eob_Line =
{
   NULL, NULL, (unsigned char *) "[EOB]", 5
#ifdef KEEP_SPACE_INFO
     ,5
#endif
#if JED_HAS_LINE_ATTRIBUTES
     , JED_LINE_IS_READONLY
#endif
};

static void highlight_line_region (Line *line, int lineno, int sy,
				   int n0, int p0, int n1, int p1,
				   int color)
{
   Scrwrap_Type wt1;
   SLuchar_Type *hi0, *hi1;
   int dn, c;

   lineno += CBuf->nup;

   if ((n0 > n1) || ((n0 == n1) && (p0 > p1)))
     {
	int tmp;
	tmp = n0; n0 = n1; n1 = tmp;
	tmp = p0; p0 = p1; p1 = tmp;
     }
   if ((lineno < n0) || (lineno > n1)) return;

   hi0 = line->data;

   if (lineno == n0) hi0 += p0;
   if (lineno == n1)
     hi1 = line->data + p1;	       /* last line of region */
   else
     hi1 = line->data + (line->len - 1); /* line is interior to region; don't include \n */

   scrwrap_init (&wt1, JWindow, CBuf->flags & VISUAL_WRAP);
   scrwrap_calculate_rel_position (&wt1, line, hi0-line->data, &dn, &c);
   c--;		       /* convert to 0-based */
   wt1.row = sy + dn;
   wt1.col = c;
   wt1.column = dn*(wt1.cmax-1) + c;
   SLsmg_gotorc (wt1.row, wt1.col);
   scrwrap_write_bytes (&wt1, hi0, hi1, color);
}

static void highlight_line (Line *line, int lineno, int sy, Mark *vismark)
{
   highlight_line_region (line, lineno, sy,
			  vismark->n, vismark->point,
			  LineNum + CBuf->nup, Point,
			  JREGION_COLOR);
}

static void display_line (Scrwrap_Type *wt, Line *line, int lineno, int sy, int sx, Mark *vismark)
{
   unsigned int len;
   int hscroll_col;
   int is_mini;
   Screen_Type *s, *smin, *smax;
#if JED_HAS_LINE_MARKS
   Mark *line_marks;
#endif
   int num_columns;
   int color;
   int i, num_wraps;

   wt->row = sy;
   wt->column = wt->col = 0;
   wt->num_wraps = 0;

   s = JScreen + sy;

   SLsmg_Tab_Width = Buffer_Local.tab;
   (void) SLsmg_embedded_escape_mode (CBuf->flags & SMG_EMBEDDED_ESCAPE);
   is_mini = (sy + 1 == Jed_Num_Screen_Rows);

   SLsmg_gotorc (sy, sx);
   SLsmg_set_color (0);

   if (line == NULL)
     {
	SLsmg_erase_eol ();
	goto finish;
     }

   if (wt->wrap_mode)
     hscroll_col = 0;
   else
     {
	hscroll_col = JWindow->hscroll_column - 1;

	if ((line == HScroll_Line)
	    && Wants_HScroll && HScroll)
	  hscroll_col += HScroll;
     }

   num_columns = wt->cmax;

   if (hscroll_col || sx
#if JED_HAS_DISPLAY_LINE_NUMBERS
       || (CBuf->line_num_display_size)
#endif
      )
     {
	int tmp = hscroll_col - sx;
#if JED_HAS_DISPLAY_LINE_NUMBERS
	tmp -= CBuf->line_num_display_size;
#endif
	SLsmg_set_screen_start (NULL, &tmp);
	sx = 0;
     }

   len = line->len;

   if (is_mini)
     {
	SLsmg_Newline_Behavior = SLSMG_NEWLINE_PRINTABLE;
	SLsmg_write_string ((char *)Mini_Info.prompt);
     }
   else
     {
	SLsmg_Newline_Behavior = 0;
	if (len && (line->data[len - 1] == '\n'))
	  len--;
     }

   color = -1;
#if JED_HAS_LINE_ATTRIBUTES
   if (line->flags & JED_LINE_COLOR_BITS)
     {
	color = JED_GET_LINE_COLOR(line);
     }
#endif

#if JED_HAS_LINE_MARKS
   line_marks = CBuf->user_marks;
   if (color == -1) while (line_marks != NULL)
     {
	if ((line_marks->line == line)
	    && (line_marks->flags & JED_LINE_MARK))
	  {
	     color = line_marks->flags & MARK_COLOR_MASK;
	     break;
	  }
	line_marks = line_marks->next;
     }
#endif

   if (len)
     {
	if ((color == -1)
	    && Mode_Has_Syntax_Highlight
	    && (line != &Eob_Line)
#if !defined(IBMPC_SYSTEM)
	    && (*tt_Use_Ansi_Colors && Term_Supports_Color)
#endif
	    && Wants_Syntax_Highlight)
	  write_syntax_highlight (wt, sy, line, len);
	else
	  {
	     if ((is_mini == 0)
		 && Jed_Highlight_WS & HIGHLIGHT_WS_TRAILING)
	       {
		  unsigned char *pmin = line->data;
		  unsigned char *pmax = pmin + len;
		  unsigned char *p = pmax;
		  while (p > pmin)
		    {
		       p--;
		       if ((*p != ' ') && (*p != '\t'))
			 {
			    p++;
			    break;
			 }
		    }
		  scrwrap_write_bytes (wt, pmin, p, color);
		  /* SLsmg_write_nchars ((char *)pmin, p - pmin); */
		  if (p != pmax)
		    {
		       scrwrap_write_bytes (wt, p, pmax, JTWS_COLOR);
		       /* SLsmg_write_nchars ((char *)p, pmax - p); */
		       SLsmg_set_color (0);
		    }
	       }
	     else scrwrap_write_bytes (wt, line->data, line->data+len, color);
	     /* else SLsmg_write_nchars ((char *)line->data, len); */
	  }
     }
#if JED_HAS_LINE_ATTRIBUTES
   if ((line->next != NULL)
       && (line->next->flags & JED_LINE_HIDDEN))
     {
	SLsmg_set_color (JDOTS_COLOR);
	SLsmg_write_string ("...");
	SLsmg_set_color (0);
     }
#endif
   if ((wt->row >= wt->rmin) && (wt->row < wt->rmax))
     SLsmg_erase_eol ();

   if (Jed_Dollar && (wt->wrap_mode == 0))
     {
	char dollar = (char) Jed_Dollar;

	if (hscroll_col + num_columns <= SLsmg_get_column ())
	  {
	     SLsmg_gotorc (sy, hscroll_col + num_columns - 1);
	     SLsmg_set_color (JDOLLAR_COLOR);
	     SLsmg_write_nchars (&dollar, 1);
	  }
	if (hscroll_col)
	  {
	     SLsmg_gotorc (sy, hscroll_col);
	     SLsmg_set_color (JDOLLAR_COLOR);
	     SLsmg_write_nchars (&dollar, 1);
	  }
     }

   if (vismark != NULL)
     highlight_line (line, lineno, sy, vismark);

finish:

   smax = JScreen + wt->rmax;
   smin = JScreen + wt->rmin;
   num_wraps = wt->num_wraps;
   for (i = 0; i <= num_wraps; i++)
     {
	if ((s >= smin) && (s < smax))
	  {
	     s->line = line;
	     s->wrapno = i;
	     s->is_modified = 0;
	  }
	s++;
     }

   SLsmg_set_screen_start (NULL, NULL);
   SLsmg_set_color (0);
}

#if JED_HAS_DISPLAY_LINE_NUMBERS
static void display_line_numbers (void)
{
   unsigned int i, imin, imax, linenum;
   Line *line_start, *line;
   char buf[32], spaces[32];
   unsigned int c;

   memset (spaces, ' ', CBuf->line_num_display_size);
   spaces[CBuf->line_num_display_size] = 0;

   imin = JWindow->sy;
   imax = imin + JWindow->rows;
   line = NULL;

   /* Search for the current line in the window */
   for (i = imin; i < imax; i++)
     {
	if (JScreen[i].line != CLine)
	  continue;

	line = JScreen[i].line;
	break;
     }
   if (line == NULL)
     return;			       /* ??? */

   line_start = JScreen[imin].line;
   linenum = LineNum + CBuf->nup;

   while (line != line_start)
     {
	if (line == NULL)
	  return;		       /* ??? */

	linenum--;
	line = line->prev;
     }

   SLsmg_set_color (JLINENUM_COLOR);
   c = JWindow->sx;

   i = imin;
   while (i < imax)
     {
	line_start = JScreen[i].line;
	if (line_start == NULL)
	  break;

	/* Skip past lines not displayed (hidden) in the window */
        while (line != line_start)
	  {
	     if (line == NULL)
	       return;		       /* ??? */

	     linenum++;
	     line = line->next;
	  }

	SLsmg_gotorc (i, c);
	sprintf (buf, "%*u ", CBuf->line_num_display_size-1, linenum);
	SLsmg_write_string (buf);

	i++;
	/* Skip past continuations */
	while ((i < imax) && (JScreen[i].line == line))
	  {
	     SLsmg_gotorc (i, c);
	     SLsmg_write_string (spaces);
	     i++;
	  }
	line = line->next;
	linenum++;
     }

   if (i < imax)
     {
	while (i < imax)
	  {
	     SLsmg_gotorc (i, c);
	     SLsmg_write_string (spaces);
	     i++;
	  }
     }

   SLsmg_set_color (0);
}
#endif

#if JED_HAS_LINE_ATTRIBUTES
static Line *find_non_hidden_line (Line *l, int *non_hidden_pointp)
{
   int dir;
   Line *cline;

   if (non_hidden_pointp != NULL) *non_hidden_pointp = 0;

   if (0 == (l->flags & JED_LINE_HIDDEN))
     {
	return l;
     }

   cline = l;

   dir = 1;
   while ((cline != NULL)
	  && (cline->flags & JED_LINE_HIDDEN))
     cline = cline->prev;

   if (cline == NULL)
     {
	cline = l;
	dir = -1;
	while ((cline != NULL)
	       && (cline->flags & JED_LINE_HIDDEN))
	  cline = cline->next;

	if (cline == NULL)
	  return NULL;
     }

   if ((dir == 1)
       && (non_hidden_pointp != NULL))
     *non_hidden_pointp = cline->len;

   return cline;
}

#endif

/* This function computes the line to appear at the top of
 * the current window such that the specified line+point will appear
 * in the specified row in the window, where row is numbered from 0
 * and is relative to the top of the window.
 *
 * Upon return, *winrowp will get the row offset of the start of the line
 * that is to appear at the top of the window.  It will either be 0, which
 * indicates that line to appear at the top actually starts at the top of the window,
 * or negative, which means that the start of the line occurs above the window by |*winrowp|
 * rows and then wraps to the top row of the window.
 */
static Line *jed_find_top_to_recenter (Scrwrap_Type *wt, Line *cline, int point, int cline_dr, int target_row, int *winrowp)
{
   Line *top;
   int n;

   top = cline;
   if (cline_dr == -1)
     scrwrap_calculate_rel_position (wt, top, point, &cline_dr, NULL);

   n = target_row - cline_dr;
   /* Case 1: If target_row > cline_dr, n>0, then a previous line will be at the top.
    * Case 2: If target_row == cline_dr, n==0, then the start of the cline will be at the top
    * Case 3: If target_row < cline, n<0, then the start of cline will appear
    *   -n rows above the window
    */

   while ((n > 0) && (top->prev != NULL))
     {
	/* Case 1 */
	Line *prev;
	int dn;

	prev = top->prev;
	/* Skip past hidden rows */
	while ((prev != NULL) && (prev->flags & JED_LINE_HIDDEN))
	  prev = prev->prev;
	if (prev == NULL) break;

	top = prev;
	scrwrap_calculate_rel_position (wt, top, top->len, &dn, NULL);
	/* If dn is 0, top does not wrap, but occupies a row */
	n -= (1+dn);
     }
   if (n > 0) n = 0;
   /* n<0 implies that the line to appear at the top of the screen is wrapped */
   *winrowp = n;
   return top;
}

/* Compute the line to appear at the top of the window such
 * that the current line will appear somewhere in the window.
 * Note that the solution is not unique.
 */
static Line *find_line_for_top_of_window (Scrwrap_Type *wt, Line *cline, int point, int cline_dr, int *winrowp)
{
   Screen_Type *s_top;
   Line *prev, *next;
   int i, n, n1;

   if (cline_dr == -1)
     scrwrap_calculate_rel_position (wt, cline, point, &cline_dr, NULL);

   s_top = JScreen + JWindow->sy;
   n = JWindow->rows;
   for (i = 0; i < n; i++)
     {
	Screen_Type *s = s_top + i;
	if (s->line != cline)
	  continue;

	n1 = i + cline_dr - s->wrapno;
	/* case 1: n1 >= 0, the point occurs at window row n1
	 * case 2: n1 < 0, the point lies -n1 rows above this one
	 */
	if (n1 >= 0)
	  {
	     if (n1 == n) n1 = n-1;    /* scroll one line  */
	     else if (n1 > n) n1 = n/2;/* recenter */
	  }
	else if (n1 == -1) n1 = 0;     /* scroll back one line */
	else n1 = n/2;		       /* recenter */
	return jed_find_top_to_recenter (wt, cline, point, cline_dr, n1, winrowp);
     }

   /* Not found.  Look to see if the previous or next line is in the window */

   /* Check the previous line.  If it is in the window, then cline is below the window */
   prev = cline->prev;
   while ((prev != NULL) && (prev->flags & JED_LINE_HIDDEN)) prev = prev->prev;
   if (prev == NULL) return jed_find_top_to_recenter (wt, cline, point, cline_dr, n/2, winrowp);
   i = 0;
   while (i < n)
     {
	Screen_Type *s = s_top + i;

	i++;
	if (s->line != prev) continue;
	while ((i < n) && (s_top[i].line == prev))
	  i++;			       /* handle wrapping of prev */
	if (i < n)
	  {
	     /* prev is in window */
	     n1 = i + cline_dr;
	     if (n1 == n) n1 = n-1;
	     else if (n1 > n) n1 = n/2;
	  }
	else n1 = n-1;		       /* put it on the bottom row */
	return jed_find_top_to_recenter (wt, cline, point, cline_dr, n1, winrowp);
     }

   /* See if the next line is in the window.  If so, the cline is above the window */
   next = cline->next;
   while ((next != NULL) && (next->flags & JED_LINE_HIDDEN)) next = next->next;
   if (next == NULL) return jed_find_top_to_recenter (wt, cline, point, cline_dr, n/2, winrowp);
   i = 0;
   while (i < n)
     {
	Screen_Type *s = s_top + i;

	if (s->line != next)
	  {
	     i++;
	     continue;
	  }
	if (i != 0) i--;
	return jed_find_top_to_recenter (wt, cline, point, cline_dr, i, winrowp);
     }
   return jed_find_top_to_recenter (wt, cline, point, cline_dr, n/2, winrowp);
}

static Line *find_top (Scrwrap_Type *wt, int *winrowp)
{
   Line *cline;
   int point;

   *winrowp = 0;
   cline = CLine;
   point = Point;

#if JED_HAS_LINE_ATTRIBUTES
   if (cline->flags & JED_LINE_HIDDEN)
     cline = find_non_hidden_line (cline, &point);
   if (cline == NULL)
     return NULL;
#endif
   return find_line_for_top_of_window (wt, cline, point, -1, winrowp);
}

/* Set the SLSMG variables for values appropriate for the current buffer.
 * Then move the cursor to (0,0).  The screen position of the cursor
 * before the move is returned via the (*rowp,*colp) parameters.  This routine
 * is used in conjunction with SLsmg_strwidth
 */
static void init_smg_for_buffer (int *rowp, int *colp)
{
   SLsmg_Tab_Width = Buffer_Local.tab;
   if (IN_MINI_WINDOW)
     SLsmg_Newline_Behavior = SLSMG_NEWLINE_PRINTABLE;
   else
     SLsmg_Newline_Behavior = 0;
   (void) SLsmg_embedded_escape_mode (CBuf->flags & SMG_EMBEDDED_ESCAPE);

   *rowp = SLsmg_get_row ();
   *colp = SLsmg_get_column ();
   SLsmg_gotorc (0, 0);
}

/* Move the editing point to the specified screen column.
 */
void point_column (int n)
{
   SLuchar_Type *p, *pmax;
   int row, col;

   /* Compensate for the prompt */
   if (IN_MINI_WINDOW)
     n -= Mini_Info.effective_prompt_len;

   p = CLine->data;
   pmax = p + CLine->len;
   if (LINE_HAS_NEWLINE (CLine))
     pmax--;

   init_smg_for_buffer (&row, &col);
   n = (int) SLsmg_strbytes (p, pmax, (unsigned int) n);
   SLsmg_gotorc (row, col);

   jed_set_point (n);
}

int jed_compute_effective_length (unsigned char *pos, unsigned char *pmax)
{
   int len, row, col;
   init_smg_for_buffer (&row, &col);
   len = (int) SLsmg_strwidth (pos, pmax);
   SLsmg_gotorc (row, col);
   return len;
}

int calculate_column (void)
{
   int c;

   c = 1 + jed_compute_effective_length (CLine->data, CLine->data + Point);
   Absolute_Column = c;

   if (IN_MINI_WINDOW) c += Mini_Info.effective_prompt_len;
   Screen_Col = c;
   return (c);
}

void point_cursor (int c)
{
   Line *tthis, *cline;
   int r, row, wrapno;

   if (JWindow->trashed) return;

   cline = CLine;
#if JED_HAS_LINE_ATTRIBUTES
   if (cline->flags & JED_LINE_HIDDEN)
     {
	int non_hidden_point;
	cline = find_non_hidden_line (cline, &non_hidden_point);
	if (cline != NULL)
	  c = non_hidden_point;
     }
#endif

   if (Point >= CLine->len)
     {
	Point = CLine->len - 1;

	if (Point < 0) Point = 0;
	else if ((*(CLine->data + Point) != '\n')
		 || (CBuf == MiniBuffer)) Point++;
     }

   wrapno = 0;
   if ((c == 0) || CBuf->flags & VISUAL_WRAP)
     {
	int dr;
	Scrwrap_Type wt;
	scrwrap_init (&wt, JWindow, CBuf->flags & VISUAL_WRAP);
	scrwrap_calculate_rel_position (&wt, CLine, Point, &dr, &c);
	wrapno = dr;
     }
   else c -= (JWindow->hscroll_column - 1);

   r = JWindow->sy;		       /* 0-based */
   Point_Cursor_Flag = 0;
   for (row = r; row < r + JWindow->rows; row++)
     {
	Screen_Type *s = JScreen + row;
	tthis = s->line;
	if (tthis == NULL) break;
	if (((tthis == cline) && (s->wrapno == wrapno))
	    || (tthis == &Eob_Line))
	  {
	     r = row;
	     break;
	  }
     }

   if (cline == HScroll_Line) c -= HScroll;
   if (c < 1) c = 1; else if (c > JWindow->width) c = JWindow->width;

   c += JWindow->sx;
#if JED_HAS_DISPLAY_LINE_NUMBERS
   c += CBuf->line_num_display_size;
#endif

   SLsmg_gotorc (r, c - 1);
   Screen_Row = r+1;
   Screen_Col = c;

   SLsmg_refresh ();

   if (!Cursor_Motion) Goal_Column = c;
}

static unsigned long Status_Last_Time;
static unsigned long Status_This_Time;

static char *status_get_time(void)
{
   static char status_time[10];
   register char *t, ch, *t1;
   char am;
   int n;

   if (Display_Time == 0) return (NULL);
   if (Status_This_Time == 0) Status_This_Time = sys_time();
   if (Status_This_Time - Status_Last_Time >= 30)
     {
	Status_Last_Time = Status_This_Time;
	am = 'a';
	t = SLcurrent_time_string ();
	/* returns a string like:  "Tue Nov 2 13:18:19 1993" */
	t1 = status_time;
	while (ch = *t, (ch <= '0') || (ch > '9')) t++;
	/* on date number, skip it */
	while (*t++ != ' ');
	if (*t == '0') t++;
	if (Display_Time > 0)
	  {
	     n = 0;
	     while ((ch = *t++) != ':')
	       {
		  n = 10 * n + (int) (ch - '0');
	       }
	     if (n >= 12) am = 'p';
	     n = n % 12;
	     if (n == 0) n = 12;
	     if (n >= 10)
	       {
		  n -= 10;
		  *t1++ = '1';
	       }
	     *t1++ = '0' + n;
	     *t1++ = ':';
	     while ((*t1++ = *t++) != ':');
	     *(t1 - 1) = am; *t1++ = 'm'; *t1 = 0;
	  }
	else
	  {
	     *t1++ = '[';
	     while ((*t1++ = *t++) != ':');
	     while ((*t1++ = *t++) != ':');
	     *--t1 = ']'; *++t1 = 0;
	  }
     }
   return (status_time);
}

static void finish_status(int col_flag)
{
   char *v, ch;
   Line *l;
   int top, rows, narrows;
   char buf[256];

   v = CBuf->status_line;
   if (*v == 0) v = Jed_Default_Status_Line;

   while (1)
     {
	char int_format[16], uint_format[16], str_format[16];
	char *str;
	char *v0 = v;
	unsigned int uvalue;
	int value, val_mult;
	int is_num;

	strcpy (int_format, "%d");
	strcpy (uint_format, "%u");
	strcpy (str_format, "%s");
	v0 = v;

	while (1)
	  {
	     ch = *v;
	     if (ch == 0)
	       {
		  SLsmg_write_nchars (v0, (unsigned int) (v-v0));
		  return;
	       }
	     if (ch == '%')
	       {
		  SLsmg_write_nchars (v0, (unsigned int) (v-v0));
		  break;
	       }
	     v++;
	  }

	/* At this point *v == '%' */
	v++;
	ch = *v++;

       /* If the first character after the '%' is a '-' then treat it as the
	* start of a negative field specifier
	*/
	val_mult = 1;
	if(ch == '-')
	  {
	     val_mult = -1;
	     /* v++; */
	     ch = *v++;
	     if (!ch)
	       break;
	  }

       /* If the first character after the '%' is a digit, treat it as
	* a formatting prefix value, or a color specifier
	*/
	value = 0;
	while (isdigit(ch))
	  {
	     value = 10*value + (ch - '0');
	     ch = *v++;
	     if(!ch)
	       break;
	  }

	/* Special case: Color */
	if (ch == 'C')
	  {
	     if (value > 0) SLsmg_set_color (value);
	     continue;
	  }

	if (value > 0)
	  {
	     value = val_mult*value;

	     if (-1 == SLsnprintf (int_format, sizeof(int_format), "%%%dd", value))
	       strcpy (int_format, "%d");
	     if (-1 == SLsnprintf (uint_format, sizeof(int_format), "%%%du", value))
	       strcpy (uint_format, "%u");
	     if (-1 == SLsnprintf (str_format, sizeof(str_format), "%%%ds", value))
	       strcpy (str_format, "%s");
	  }

	str = NULL;
	is_num = 0;    /* +1 ==> use value, -1 ==> use uvalue */
	value = 0;
	uvalue = 0;

	switch (ch)
	  {
	   case 'F':
	     str = CBuf->dirfile;
	     break;

	   case 'S':
	     /* stack depth */
	     value = SLstack_depth ();
	     is_num = 1;
	     break;

	   case 'a':
	     if (CBuf->flags & ABBREV_MODE) str = " abbrev";
	     break;
	   case 'f': str = CBuf->file; break;
	   case 'n':
	     narrows = jed_count_narrows ();
	     if (narrows)
	       {
		  SLsmg_write_string (" Narrow[");
		  SLsmg_printf (int_format, narrows);
		  SLsmg_write_string ("]");
	       }
	     break;

	   case 'o':
	     if (CBuf->flags & OVERWRITE_MODE) str = " Ovwrt";
	     break;
	   case 'O':
	     if (CBuf->flags & OVERWRITE_MODE)
	       str = " ovr";
	     else
	       str = " ins";
	     break;

	   case 'b': str = CBuf->name; break;

	   case 'p':
	     if (0 == User_Prefers_Line_Numbers)
	       {
		  top = JWindow->sy;
		  rows = JWindow->rows - 1;
		  l = JScreen[top + rows].line;
		  if (l == CBuf->end) l = NULL;
		  if (JScreen[top].line == CBuf->beg)
		    {
		       if (l == NULL) str = "All";
		       else str = "Top";
		    }
		  else if (l == NULL) str = "Bot";
		  else
		    {
		       SLsmg_printf (int_format, (int) ((LineNum * 100L) / (long) Max_LineNum));
		       SLsmg_write_string ("%");
		    }
	       }
	     else
	       {
		  if (User_Prefers_Line_Numbers == 1)
		    (void) SLsnprintf (buf, sizeof(buf), "%u/%u", LineNum, Max_LineNum);
		  else
		    {
		       if (col_flag) (void) calculate_column ();
		       (void) SLsnprintf (buf, sizeof(buf), "%u/%u,%d", LineNum, Max_LineNum, Absolute_Column);
		    }
		  str = buf;
	       }
	     break;

	   case 'l':
	     uvalue = LineNum;
	     is_num = -1;
	     break;

	   case 'L':
	     uvalue = Max_LineNum;
	     is_num = -1;
	     break;

	   case 'v':
	     (void) SLsnprintf (buf, sizeof(buf), "%s%s",
				Jed_Version_String,
				(Jed_UTF8_Mode ? "U" : ""));
	     str = buf;
	     break;

	   case 'm': str = CBuf->mode_string; break;
	   case 't': str = status_get_time(); break;
	   case 'c':
	     if (col_flag) (void) calculate_column ();
	     value = Absolute_Column;
	     is_num = 1;
	     break;
          case 'T':
	     value = CBuf->local_vars.tab;
	     is_num = 1;
	     if (CBuf->local_vars.use_tabs)
	       SLsmg_write_string ("tab:");
	     else
               SLsmg_write_string ("spc:");
	     break;
	   case 'W':
	     if (CBuf->modes & WRAP_MODE)
	       {
		  SLsmg_write_string ("wrap:");
		  value = CBuf->local_vars.wrap_column;
		  is_num = 1;
	       }
	     else str = "nowrap";
	     break;

	   case '%': str = "%"; break;
	   case 0:
	     return;

	   default:
	     str = NULL;
	  }

	if (str != NULL)
	  SLsmg_printf (str_format, str);
	else if (is_num > 0)
	  SLsmg_printf (int_format, value);
	else if (is_num < 0)
	  SLsmg_printf (uint_format, uvalue);
     }
}

void set_status_format(char *f, int *local)
{
   char *s;
   if (*local) s = Jed_Default_Status_Line; else s = CBuf->status_line;
   strncpy(s, f, JED_MAX_STATUS_LEN-1);
   s[JED_MAX_STATUS_LEN-1] = 0;
}

static int update_status_line (int col_flag)
{
   unsigned char star0, star1;
   char buf[32], *b;
   int num;

   if (IN_MINI_WINDOW)
     return 0;

   SLsmg_gotorc (JWindow->rows + JWindow->sy, 0);
   SLsmg_set_color (JSTATUS_COLOR);

   b = buf;
   if (JWindow->hscroll_column != 1) *b++ = '<'; else *b++ = '-';

   if (CBuf->flags & BUFFER_MODIFIED) star0 = star1 = '*';
   else star0 = star1 = '-';

   if ((CBuf->flags & READ_ONLY)
#if JED_HAS_LINE_ATTRIBUTES
       || (CLine->flags & JED_LINE_IS_READONLY)
#endif
       )
     star0 = star1 = '%';

#if JED_HAS_SUBPROCESSES
   if (CBuf->subprocess) star1 = 'S';
#endif
   *b++ = star0;
   *b++ = star1;

   if (CBuf->marks != NULL) *b++ = 'm'; else *b++ = '-';
   if (CBuf->flags & FILE_MODIFIED) *b++ = 'd'; else *b++ = '-';
   if (CBuf->spots != NULL) *b++ = 's'; else *b++ = '-';

   if (CBuf->flags & BINARY_FILE) *b++ = 'B';
#ifdef IBMPC_SYSTEM
   else if ((CBuf->flags & ADD_CR_ON_WRITE_FLAG) == 0) *b++ = 'L';
#else
# ifdef __unix__
   else if (CBuf->flags & ADD_CR_ON_WRITE_FLAG) *b++ = 'C';
# endif
#endif
   else *b++ = '-';
   if (CBuf->flags & UNDO_ENABLED) *b++ = '+'; else *b++ = '-';
   if (CBuf->flags & OVERWRITE_MODE) *b++ = 'O'; else *b++ = '-';
   SLsmg_write_nchars (buf, (unsigned int) (b - buf));

   finish_status (col_flag);
   SLsmg_set_color (JSTATUS_COLOR);    /* finish_status may have changed it */

   if (Defining_Keyboard_Macro) SLsmg_write_string (" [Macro]");

   num = Jed_Num_Screen_Cols - SLsmg_get_column ();
   star1 = '-';
   while (num > 0)
     {
	SLsmg_write_nchars ((char *) &star1, 1);
	num--;
     }
   SLsmg_set_color (0);
   Point_Cursor_Flag = 1;
   return 1;
}

static int screen_input_pending (int tsec)
{
   if (Input_Buffer_Len
#ifdef HAS_RESIZE_PENDING
       || Jed_Resize_Pending
#endif
       )
     return 1;

   return input_pending (&tsec);
}

static Mark *mark_window_attributes (int wa)
{
   Mark *m;

   if ((CBuf->vis_marks == 0) || (wa == 0) || (Wants_Attributes == 0))
     {
	return NULL;
     }

   m = CBuf->marks;

   while ((m != NULL)
	  && ((m->flags & VISIBLE_MARK_MASK) == 0))
     m = m->next;

   return m;
}

static void compute_linenum_display_size (void)
{
#if JED_HAS_DISPLAY_LINE_NUMBERS
   int max_num_len = 0;
   int n;

   if (CBuf->line_num_display_size == 0)
     return;

   n = Max_LineNum + CBuf->nup;
   while (n != 0)
     {
	n /= 10;
	max_num_len++;
     }
   max_num_len++;		       /* add one as a separator */
   if (CBuf->line_num_display_size != max_num_len)
     {
	JWindow->trashed = 1;
	CBuf->line_num_display_size = max_num_len;
     }
#endif
}

static int get_top_lineno (Line *top)
{
   int dn = 0;

   /* CLine is below top by construction */
   while ((top != NULL) && (top != CLine))
     {
	top = top->next;
	dn++;
     }
   return LineNum - dn;
}

/* if force then do update otherwise return 1 if update or 0 if not */
static int update_1(Line *top, int force)
{
   Window_Type *start_win;
   int i;
   int did_eob, time_has_expired = 0;
   int winrow = 0;

   if (Batch
       || ((force == 0)
	   && (Executing_Keyboard_Macro || (Repeat_Factor != NULL)
	       || screen_input_pending (0)
	       || (Read_This_Character != NULL)))
       || (CBuf != JWindow->buffer))
     return 0;

   if (Suspend_Screen_Update != 0)
     {
	Suspend_Screen_Update = 0;
	touch_screen ();
     }

   JWindow->mark.line = CLine;
   JWindow->mark.point = Point;
   JWindow->mark.n = LineNum + CBuf->nup;
   CBuf->linenum = LineNum;
   CBuf->max_linenum = Max_LineNum;

   if ((Wants_Attributes && CBuf->vis_marks)
       || (CLine == JScreen[JWindow->sy].line)   /* moving cursor left at top */
       || (CLine == JScreen[JWindow->sy + JWindow->rows-1].line))   /* moving cursor right at bottom */
     {
	JWindow->trashed = 1;
     }

   /* Do not bother setting this unless it is really needed */
   if (Display_Time)
     {
	Status_This_Time = sys_time();
	time_has_expired = (Status_This_Time > Status_Last_Time + 45);
     }

   /* if cursor moves just left or right, do not update status line */
   if ((force == 0)
       && !JWindow->trashed
       && ((JWindow == JWindow->next) || (User_Prefers_Line_Numbers && Cursor_Motion))
       /* if % wanted, assume user is like me and gets annoyed with
	* screen updates
	*/
       && (User_Prefers_Line_Numbers || time_has_expired))
     {
	update_status_line(0);
	return 1;
     }

   if (!JWindow->trashed && Cursor_Motion)
     {
#if JED_HAS_LINE_ATTRIBUTES
	if (CLine->flags & JED_LINE_IS_READONLY)
	  update_status_line (0);
#endif
	return 1;
     }

   start_win = JWindow;
   do
     {
	Scrwrap_Type wt;
	Mark *vismark;
	int imin, imax, lineno;
	unsigned int start_column;

#if JED_HAS_LINE_ATTRIBUTES
	if (CBuf->min_unparsed_line_num)
	  jed_syntax_parse_buffer (0);
#endif
	if (Wants_Syntax_Highlight) init_syntax_highlight ();

#if JED_HAS_LINE_ATTRIBUTES
	if (top != NULL) top = find_non_hidden_line (top, NULL);
#endif

	compute_linenum_display_size ();
	scrwrap_init (&wt, JWindow, CBuf->flags & VISUAL_WRAP);	       /* This requires the display size to be set */

	/* (void) SLsmg_utf8_enable (CBuf->local_vars.is_utf8); */
	if (top == NULL)
	  {
	     top = find_top (&wt, &winrow);
	     if (top == NULL) top = CLine;
	  }

	lineno = get_top_lineno (top);

	JWindow->beg.line = top;
	JWindow->beg_winrow = winrow;

	vismark = NULL;

#if JED_HAS_LINE_ATTRIBUTES
	if (top->flags & JED_LINE_HIDDEN)
	  top = NULL;
	else
#endif
	  vismark = mark_window_attributes ((start_win == JWindow) || (start_win->buffer != CBuf));

	did_eob = 0;

	imin = JWindow->sy;
	imax = imin + JWindow->rows;
	i = imin + winrow;      /* winrow<= 0 ==> i could be less than imin if the start of the line is above the window */

	start_column = JWindow->sx;

	while (i < imax)
	  {
#if JED_HAS_LINE_ATTRIBUTES
	     if ((top != NULL) && (top->flags & JED_LINE_HIDDEN))
	       {
		  top = top->next;
		  lineno++;
		  continue;
	       }
#endif

	     /* the next line is really optional */
#if 0
	     if (!force && (Exit_From_MiniBuffer || screen_input_pending (0))) break;
#endif

	     if ((i < imin)
		 || (JScreen[i].line != top)
		 || JScreen[i].is_modified
		 || (JScreen[i].wrapno != 0)
		 || (vismark != NULL)
		 || (Want_Eob
		     && !did_eob
		     && (i != Jed_Num_Screen_Rows - 1)
		     && (top == NULL)))
	       {
		  if (((top == NULL) || (top->len == 0))
		      && (Want_Eob && !did_eob && !(CBuf->flags & READ_ONLY)))
		    {
		       display_line(&wt, &Eob_Line, lineno, i, start_column, vismark);
		       did_eob = 1;
		    }
		  else display_line (&wt, top, lineno, i, start_column, vismark);
		  i += 1 + wt.num_wraps;
	       }
	     else
	       {
		  i++;
		  /* Skip wrapped continuations */
		  while ((i < imax)
			 && (JScreen[i].line == top))
		    i++;
	       }
	     if (top != NULL)
	       {
		  top = top->next;
		  lineno++;
	       }
	  }

	HScroll_Line = NULL;
	Mode_Has_Syntax_Highlight = 0;
	if (!force && screen_input_pending (0))
	  {
	     while (JWindow != start_win) other_window();
	     JWindow->trashed = 1;  /* since cursor not pointed */
	     /* (void) SLsmg_utf8_enable (1); */  /* default state */

	     return(0);
	  }
	else update_status_line (start_win != JWindow);

#if JED_HAS_DISPLAY_LINE_NUMBERS
	if (CBuf->line_num_display_size)
	  display_line_numbers ();
#endif
	JWindow->trashed = 0;

	other_window();
	top = NULL;
	/* if (!JWindow->trashed) top = JWindow->beg.line; else  top = NULL; */

     }
   while (JWindow != start_win);
   /* SLsmg_utf8_enable (1); */	       /* default state */
   return 1;
}

int Mini_Ghost = 0;

char Message_Buffer[256];

static void do_dialog(char *b)
{
   char *quit = "Quit!";

   if (Batch) return;
#ifdef FIX_CHAR_WIDTH
   FIX_CHAR_WIDTH;
#endif
   if (! *b)
     {
	if(!SLKeyBoard_Quit) return;
	b = quit;
     }

   if ((b == Error_Buffer) || (b == quit))
     {
	SLsmg_set_color (JERROR_COLOR);
	touch_screen();
     }
   else
     SLsmg_set_color (JMESSAGE_COLOR);

   SLsmg_Newline_Behavior = SLSMG_NEWLINE_PRINTABLE;
   SLsmg_gotorc (Jed_Num_Screen_Rows - 1, 0);
   SLsmg_write_string (b);
   SLsmg_set_color (0);
   SLsmg_erase_eol ();
   SLsmg_Newline_Behavior = 0;

   if ((b == Error_Buffer) || (SLKeyBoard_Quit))
     {
	jed_beep();
	flush_input();
     }

   if (*b)
     {
	if (MiniBuffer != NULL)
	  {
	     SLsmg_refresh ();
	     (void) input_pending(&Number_Ten);
	  }
	Mini_Ghost = -1;
     }
   else Mini_Ghost = 0;
}

void clear_message (void) /*{{{*/
{
   message (NULL);
}

/*}}}*/

void message (char *msg)
{
   if (Executing_Keyboard_Macro) return;
   if (msg == NULL)
     {
	if (Batch)
	  return;
	msg = "";
     }

   if (Batch) fprintf(stdout, "%s\n", msg);

   if (*msg == 0)
     Mini_Ghost = 1;

   strncpy(Message_Buffer, msg, sizeof(Message_Buffer));
   Message_Buffer[sizeof(Message_Buffer)-1] = 0;
}

void flush_message (char *m)
{
   message(m);
   if (Batch || (JWindow == NULL)) return;
   do_dialog(Message_Buffer);
   SLsmg_gotorc (Jed_Num_Screen_Rows - 1, strlen(Message_Buffer));
   *Message_Buffer = 0;
   JWindow->trashed = 1;
   SLsmg_refresh ();
}

static void update_minibuffer(void)
{
   Scrwrap_Type wt;
   Window_Type *w;
   int wrapmode = 0;

   if (Executing_Keyboard_Macro) return;

   if (MiniBuffer != NULL)
     {
	Mark *vismark;
	w = JWindow;
	while (!IN_MINI_WINDOW) other_window();

	JWindow->beg.line = CLine;
	JWindow->beg_winrow = 0;

	vismark = mark_window_attributes (1);
	scrwrap_init (&wt, JWindow, wrapmode);	       /* This requires the display size to be set */
	display_line (&wt, CLine, 1, Jed_Num_Screen_Rows-1, 0, vismark);
	while (w != JWindow) other_window();
	Mini_Ghost = 1;
     }
   else if (Mini_Ghost && !*Error_Buffer && !*Message_Buffer)
     {
	/* if < 0, it is a result of flush message so let it pass this round */
	if (Mini_Ghost < 0) Mini_Ghost = 1;
	else Mini_Ghost = 0;
     }
   else Mini_Ghost = ((*Message_Buffer) || (*Error_Buffer));

   if (Mini_Ghost == 0)
     {
	scrwrap_init (&wt, JWindow, wrapmode);	       /* This requires the display size to be set */
	display_line (&wt, NULL, 1, Jed_Num_Screen_Rows-1, 0, NULL);
     }
}


/* Let "|" denote the window edges,
 *   "." denote text, "*" denotes the current location
 *
 *  .........|....*..........|....
 *  <---- col --->
 *  <-- wc -><-------sw------>
 * The contraints are: wc >= 0, col >= 0, sw > 0.
 * Let whs be the value of abs(Wants_HScroll).  We want to wc to satisfy
 *   wc+1 <= col <= wc + sw - 1
 * ==> col-sw+1 <= wc <= col-1
 * However, if wc does not satisfy this, then set wc such that:
 *
 *   wc + whs <= col <= wc + sw - whs
 *   col+whs-sw <= wc <= col-whs
 * Evidently, this requires:
 *    col+whs-sw < col-whs
 *    whs-sw < -whs
 *    2whs < sw ==> whs < sw/2
 *
 * As a special case, if col < sw, then set wc=0.
 */
static void set_hscroll(int col)
{
   int whs = abs(Wants_HScroll), wc = JWindow->hscroll_column - 1;
   int sw = Jed_Num_Screen_Cols-1;
   static Line *last;
   Line *tmp;
   int wc_min, wc_max;

#if JED_HAS_DISPLAY_LINE_NUMBERS
   sw -= CBuf->line_num_display_size;
#endif
   if (sw < 2)
     sw = 2;

   if (Wants_HScroll > 0)
     wc += HScroll;		       /* only this line scrolled */

   /* take care of last effect of horizontal scroll */
   if (last != NULL)
     {
	tmp = CLine;
	CLine = last;
	register_change(0);
	CLine = tmp;
	if (last != CLine)
	  HScroll = 0;

	last = NULL;
     }
   col--;			       /* use 0 origin */
   if (2*whs > sw)
     whs = sw/2;

   wc_min = col - sw + 1;
   if (wc_min < 0) wc_min = 0;
   wc_max = col - 1;
   if (wc < wc_min)
     wc = wc_min + whs;
   if (wc > wc_max)
     wc = wc_max - whs;

   if (col < sw)
     {
	if ((CBuf->modes & WRAP_MODE)
	    || (col <= sw - whs))
	  wc = 0;
     }

   if (Wants_HScroll < 0)
     {
	/* Scroll whole window */
	if (wc + 1 != JWindow->hscroll_column)
	  {
	     JWindow->hscroll_column = wc + 1;
	     touch_window();
	  }
	HScroll = 0;
     }
   else
     {
	/* Scroll just this line -- do not change hscroll_column */
	register_change(0);
	last = HScroll_Line = CLine;
	HScroll = wc - (JWindow->hscroll_column-1);
     }
}

static char Top_Screen_Line_Buffer[132] = "If you see this, you have an installation problem.";

void define_top_screen_line (char *neew)
{
   SLang_push_string (Top_Screen_Line_Buffer);
   strncpy (Top_Screen_Line_Buffer, neew, sizeof (Top_Screen_Line_Buffer) - 1);
   Top_Screen_Line_Buffer[sizeof(Top_Screen_Line_Buffer) - 1] = 0;
   if (JScreen != NULL)
     JScreen[0].is_modified = 1;
}

static void update_top_screen_line (void)
{
#if JED_HAS_MENUS
   if (Jed_Menus_Active
       || (Top_Window_SY > 0))
     {
	jed_redraw_menus ();
	return;
     }
#else
   if (Top_Window_SY == 0)
     return;

   SLsmg_gotorc (0,0);
   SLsmg_set_color (JMENU_COLOR);
   SLsmg_write_string (Top_Screen_Line_Buffer);
   SLsmg_erase_eol ();
   JScreen[0].is_modified = 0;
#endif
}

/* if flag is non-zero, do not touch the message/error buffers */
/* This routine is a mess and it, do_dialog, and the Mini_Ghost flag needs
 * to be totally redesigned.
 */
void update(Line *line, int force, int flag, int run_update_hook)
{
   int pc_flag = 1;
   int col;
   static unsigned long last_time;
   Line *hscroll_line_save;

   if (0 == Jed_Display_Initialized)
     return;

#if JED_HAS_SUBPROCESSES
   if (Child_Status_Changed_Flag)
     {
	jed_get_child_status ();
	force = 1;
     }
#endif

   if (Batch) return;

   if (!force && !SLang_get_error () && !SLKeyBoard_Quit && (!*Error_Buffer))
     {
	if (screen_input_pending (0))
	  {
	     JWindow->trashed = 1;
	     return;
	  }
     }

   if (last_time + 30 < Status_This_Time)
     {
	if (last_time == 0) last_time = Status_This_Time;
	else
	  {
	     last_time = Status_This_Time;
	     if (SLang_run_hooks ("update_timer_hook", 0))
	       flag = 0;
	  }
     }

   if (run_update_hook
       && (CBuf->buffer_hooks != NULL)
       && (CBuf->buffer_hooks->update_hook != NULL)
       && (SLang_get_error () == 0))
     {
	Suspend_Screen_Update = 1;
	SLexecute_function (CBuf->buffer_hooks->update_hook);
	if (SLang_get_error ()) CBuf->buffer_hooks->update_hook = NULL;
     }

   if (Suspend_Screen_Update != 0)
     {
	Suspend_Screen_Update = 0;
	touch_screen ();
     }

   if (X_Update_Open_Hook != NULL) (*X_Update_Open_Hook) ();
#ifdef FIX_CHAR_WIDTH
   FIX_CHAR_WIDTH;
#endif
   col = calculate_column ();
   HScroll_Line = NULL;
   if (Wants_HScroll && (0 == (CBuf->flags & VISUAL_WRAP)))
     set_hscroll(col);
   else HScroll = 0;
   hscroll_line_save = HScroll_Line;

   if (SLang_get_error ()) flag = 0;	       /* update hook invalidates flag */

   if (SLang_get_error () && !(*Error_Buffer || SLKeyBoard_Quit))
     {
	SLang_verror (0, "%s", SLerr_strerror (0));
     }

   if (!flag && (*Error_Buffer || SLKeyBoard_Quit))
     {
	do_dialog(Error_Buffer);
#if 0
	SLKeyBoard_Quit = 0;
	SLang_restart(0);
	SLang_set_error (0);
#endif
	Mini_Ghost = 1;
	(void) update_1(line, 1);
	update_minibuffer();
     }
   else if ((flag == 0) && *Message_Buffer)
     {
	if (!update_1(line, force))
	  goto done;

	do_dialog(Message_Buffer);
	Mini_Ghost = 1;
	update_minibuffer();
     }
   else
     {
	pc_flag = JWindow->trashed || (JWindow != JWindow->next) || Cursor_Motion;
	if (!flag) update_minibuffer();
	if (!update_1(line, force)) goto done;
     }
   if (!flag) *Error_Buffer = *Message_Buffer = 0;

#if JED_HAS_MENUS
   update_top_screen_line ();
#else
   if ((Top_Window_SY > 0) && JScreen[0].is_modified)
     {
	update_top_screen_line ();
     }
#endif
   done:

   HScroll_Line = hscroll_line_save;

   if (MiniBuf_Get_Response_String != NULL)
     {
	do_dialog (MiniBuf_Get_Response_String);
	Mini_Ghost = 1;
     }
   else if (Point_Cursor_Flag || pc_flag)
     point_cursor(col);

   if (X_Update_Close_Hook != NULL) (*X_Update_Close_Hook) ();

   SLsmg_refresh ();
}

/* search for the CLine in the SCreen and flag it as changed */
/* n = 0 means line was changed, n = 1 means it was destroyed */
void register_change(int n)
{
   Window_Type *w;
   register Screen_Type *s, *smax;
   register Line *cl = CLine;

   if (JScreen == NULL)
     return;

   JWindow->trashed = 1;
   if (Suspend_Screen_Update) return;
   if (No_Screen_Update)
     {
	No_Screen_Update = 0;
	if (((n == CINSERT) || (n == CDELETE)) && (JWindow->next == JWindow))
	  {
	     /* Since no screen update, we are probably safe to do: */
	     /* JScreen[Screen_Row - 1].flags = 1; */
	     return;
	  }
	w = JWindow->next;  /* skip this window */
     }
   else w = JWindow;

   do
     {
	s = &JScreen[w->sy];
	smax = s + w->rows;

	while (s < smax)
	  {
	     if (s->line == cl)
	       {
		  s->is_modified = 1;
		  if ((n == NLDELETE) || (n == LDELETE)) s->line = NULL;
		  w->trashed = 1;
	       }
	     s++;
	  }
	w = w->next;
     }
   while(w != JWindow);
}

int jed_get_screen_size (int *r, int *c)
{
   if (tt_get_screen_size == NULL)
     {
	*r = 24;
	*c = 80;
	return 0;
     }

   (void) (*tt_get_screen_size) (r, c);
   if (*r <= 0) *r = 24;
   if (*c <= 0) *c = 80;

   if (*r > 1024) *r = 24;
   if (*c > 1024) *c = 80;

   /* Let's not be ridiculous */
   if (*r <= 5) *r = 5;
   if (*c <= 5) *c = 5;

   return 0;
}

static void dealloc_display (void)
{
   if (JScreen != NULL)
     {
	SLfree ((char *) JScreen);
	JScreen = NULL;
     }
}

void jed_reset_display (void)
{
   if (Jed_Display_Initialized == 0) return;
   if (Batch) return;

   Jed_Display_Initialized = 0;

#if defined(VMS) || defined(REAL_UNIX_SYSTEM)
   (void) jed_va_run_hooks ("_jed_reset_display_hooks", JED_HOOKS_RUN_ALL, 0);
#endif

   dealloc_display ();
   SLsmg_reset_smg ();
}

static void alloc_display (void)
{
   int r, c, i;

   jed_get_screen_size (&r, &c);
   jed_update_window_sizes (r, c);

   if (NULL == (JScreen = (Screen_Type *) jed_malloc0 (sizeof (Screen_Type) * r)))
     exit_error ("Out of memory", 0);

   for (i = 0; i < r; i++)
     JScreen[i].is_modified = 1;

   Jed_Num_Screen_Cols = c;
   Jed_Num_Screen_Rows = r;
}

void jed_init_display (void)
{
   if (Batch) return;

   jed_reset_display ();
   alloc_display ();

   if (-1 == SLsmg_init_smg ())
     exit_error ("init_display: error initializing display", 0);

#if defined(VMS) || defined(REAL_UNIX_SYSTEM)
   (void) jed_va_run_hooks ("_jed_init_display_hooks", JED_HOOKS_RUN_ALL, 0);
#endif
   Jed_Display_Initialized = 1;
}

void jed_resize_display (void)
{
   Jed_Resize_Pending = 0;

   if (Jed_Display_Initialized == 0)
     return;

   dealloc_display ();
   alloc_display ();
   SLsmg_reinit_smg ();

   (void) jed_va_run_hooks ("_jed_resize_display_hooks", JED_HOOKS_RUN_ALL, 0);

   jed_redraw_screen (0);
}

void jed_redraw_screen (int force)
{
   int row, center;
   Window_Type *w;
   Line *l;

   if (Batch) return;
   SLsmg_set_color (0);
   SLsmg_cls ();

   if (JScreen == NULL)
     return;

   for (row = 0; row < Jed_Num_Screen_Rows; row++)
     {
	/* JScreen[row].line = NULL; */
	JScreen[row].is_modified = 1;
     }

   if (NULL == (w = JWindow))
     return;

   center = JWindow->trashed;
   do
     {
	w->trashed = 1;
	w = w->next;
     }
   while(w != JWindow);

   if (center)
     {
	for (row = 0; row < JWindow->rows; row++)
	  {
	     JScreen[row + JWindow->sy].line = NULL;
	  }
	l = NULL;
     }
   else l = JWindow->beg.line;

   update(l, force, 0, 1);
}

void recenter (int *np)
{
   Scrwrap_Type wt;
   Line *cline = CLine, *top;
   int point = Point;
   int dr, winrow, i, n;
   Screen_Type *s, *smax;

   JWindow->trashed = 1;
   n = *np;

   if (n == 0)
     {
	jed_redraw_screen (0);
	return;
     }

   n--;				       /* convert to 0 based */

   if ((n < 0) || (n >= JWindow->rows)) n = JWindow->rows / 2;

   scrwrap_init (&wt, JWindow, CBuf->flags & VISUAL_WRAP);
   scrwrap_calculate_rel_position (&wt, cline, point, &dr, NULL);
   top = jed_find_top_to_recenter (&wt, cline, point, dr, n, &winrow);
   scrwrap_calculate_rel_position (&wt, top, top->len, &dr, NULL);

   winrow = -winrow;
   s = JScreen + JWindow->sy;
   smax = s + JWindow->rows;
   while ((s < smax) && (winrow <= dr))
     {
	s->line = top;
	s->wrapno = winrow;
	s->is_modified = 1;
	winrow++;
	s++;
     }
   while (s < smax)
     {
	if (top == NULL)
	  {
	     s->line = top;
	     s->wrapno = 0;
	     s->is_modified = 1;
	     s++;
	     continue;
	  }

	top = top->next;
	while ((top != NULL) && (top->flags & JED_LINE_HIDDEN))
	  top = top->next;
	if (top == NULL) continue;

	scrwrap_calculate_rel_position (&wt, top, cline->len, &dr, NULL);
	i = 0;
	while ((s < smax) && (i <= dr))
	  {
	     s->line = top;
	     s->wrapno = i;
	     s->is_modified = 1;
	     i++;
	     s++;
	  }
     }
}


int window_line (void)
{
   Scrwrap_Type wt;
   Line *cline;
   Line *top;
   int n, dn;
   int winrow, point;

   if (CBuf != JWindow->buffer) return 0;

   scrwrap_init (&wt, JWindow, CBuf->flags & VISUAL_WRAP);
   top = find_top (&wt, &winrow);

#if JED_HAS_LINE_ATTRIBUTES
   cline = find_non_hidden_line (CLine, &point);
#else
   cline = CLine;
   point = Point;
#endif

   n = winrow;

   while ((top != NULL) && (top != cline))
     {
	scrwrap_calculate_rel_position (&wt, top, top->len, &dn, NULL);
	n += 1 + dn;

	top = top->next;
#if JED_HAS_LINE_ATTRIBUTES
	while ((top != NULL) && (top->flags & JED_LINE_HIDDEN))
	  top = top->next;
#endif
     }
   if (top == cline)
     {
	scrwrap_calculate_rel_position (&wt, cline, point, &dn, NULL);
	n += dn;
     }

   return 1 + n;		       /* 1-based */
}

/* returns the buffer line, wrap number, and Point at the 1-based window row
 * and column
 */
Line *jed_get_window_line (int row, int col, int *wrapnop, int *pointp)
{
   Scrwrap_Type wt;
   Line *top;
   int winrow;
   int nrows, i;

   *wrapnop = 0;
   *pointp = 0;

   if (CBuf != JWindow->buffer) return NULL;

   scrwrap_init (&wt, JWindow, CBuf->flags & VISUAL_WRAP);
   top = find_top (&wt, &winrow);
   if (top == NULL) return NULL;

   nrows = JWindow->rows;

   row--;			       /* make 0-based */
   if ((row < 0) || (row >= nrows)) return NULL;

   i = winrow;
   while (i < nrows)
     {
	int dr;
	scrwrap_calculate_rel_position (&wt, top, top->len, &dr, NULL);
	if ((i <= row) && (row <= i + dr))
	  {
	     int wrapno;
	     int srow, scol;

	     *wrapnop = wrapno = (row - i);

	     if (0 == (CBuf->flags & VISUAL_WRAP))
	       {
		  col += (JWindow->hscroll_column-1);
		  if ((top == HScroll_Line)
		      && (Wants_HScroll && HScroll))
		    col += HScroll;
	       }

	     /* Compute the unwrapped column for the character at the beginning of the window row */
	     col = wrapno * (wt.cmax-1) + col;
	     init_smg_for_buffer (&srow, &scol);
	     *pointp = SLsmg_strbytes (top->data, top->data + top->len, col);
	     SLsmg_gotorc (srow, scol);

	     return top;
	  }
	/* winrow = 0;	*/	       /* reset */
	i += (1+dr);
	top = top->next;
	while ((top != NULL) && (top->flags & JED_LINE_HIDDEN))
	  top = top->next;
	if (top == NULL) return NULL;
     }
   return NULL;
}

/* row is 1-based */
int jed_goto_window_rc (int row, int col)
{
   Line *l, *next, *prev;
   int point, wrapno;
   int dn_prev, dn_next, nrows;

   if (NULL == (l = jed_get_window_line (row, col, &wrapno, &point)))
     {
	eob ();
	return 0;
     }

   /* We know (by construction) that CLine is in the window as well as
    * the line l.  And: l-CLine <= n where n is the number of window
    * rows
    */
   nrows = JWindow->rows;
   next = prev = CLine;
   dn_next = dn_prev = 0;
   while (nrows)
     {
	nrows--;
	if (l == next)
	  {
	     CLine = l;
	     Point = point;
	     LineNum += dn_next;
	     return 1;
	  }
	if (l == prev)
	  {
	     CLine = l;
	     Point = point;
	     LineNum -= dn_prev;
	     return 1;
	  }
	if (next != NULL)
	  {
	     do
	       {
		  next = next->next;
		  dn_next++;
	       }
	     while ((next != NULL) && (next->flags & JED_LINE_HIDDEN));
	  }
	if (prev != NULL)
	  {
	     do
	       {
		  prev = prev->prev;
		  dn_prev++;
	       }
	     while ((prev != NULL) && (prev->flags & JED_LINE_HIDDEN));
	  }
     }

   return 0;
}


void touch_window (void)
{
   Screen_Type *s, *smax;

   if (Suspend_Screen_Update) return;

   if (JScreen == NULL)
     return;

   s = JScreen + (JWindow->sy);
   if (JWindow->rows > Jed_Num_Screen_Rows)
     smax = s + Jed_Num_Screen_Rows;
   else
     smax = s + JWindow->rows;

   while (s < smax)
     {
	s->is_modified = 1;
	s++;
     }
   JWindow->trashed = 1;
}

void touch_screen (void)
{
   Window_Type *w;

   No_Screen_Update = 0;

   if (Suspend_Screen_Update) return;
   if (JScreen == NULL) return;

   w = JWindow;
   do
     {
	touch_window();
	JWindow = JWindow->next;
     }
   while(w != JWindow);
   if (Top_Window_SY > 0) JScreen[0].is_modified = 1;
}

void exit_error(char *str, int severity)
{
   static int already_here;

   if (already_here)
     return;

   SLang_set_error (0);
   SLKeyBoard_Quit = 0;
   auto_save_all();
#if JED_HAS_EMACS_LOCKING
   (void) jed_unlock_buffer_files ();
#endif
   jed_reset_display();
   reset_tty();

   fprintf (stderr, "***Fatal Error: %s\n\n", str);
   jed_show_version (stderr);

   if (*Error_Buffer) fprintf (stderr, "%s\n", Error_Buffer);
   if (CBuf != NULL)
     {
	if (Batch == 0)
	  {
	     fprintf(stderr, "CBuf: %p, CLine: %p, Point %d\n", (void *)CBuf, (void *) CLine, Point);
	     if (CLine != NULL) fprintf(stderr, "CLine: data: %p, len = %d, next: %p, prev %p\n",
					(void *) CLine->data, CLine->len, (void *)CLine->next, (void *)CLine->prev);
	     fprintf(stderr, "Max_LineNum: %u, LineNum: %u\n", Max_LineNum, LineNum);
	     if (JWindow != NULL) fprintf(stderr, "JWindow: %p, top: %d, rows: %d, buffer: %p\n",
					  (void *)JWindow, JWindow->sy, JWindow->rows, (void *)JWindow->buffer);
	  }
     }
   if (severity)
     {
#ifdef __unix__
	fprintf(stderr, "Dumping Core.");
	abort ();
#endif
     }
   exit (1);
}

void touch_window_hard(Window_Type *w, int all)
{
   Window_Type *wsave = w;

   if (JScreen == NULL)
     return;

   do
     {
	Screen_Type *s, *smax;

	s = JScreen + (w->sy);
	if (w->rows > Jed_Num_Screen_Rows)
	  smax = s + Jed_Num_Screen_Rows;
	else
	  smax = s + w->rows;

	while (s < smax)
	  {
	     s->is_modified = 1;
	     s->line = NULL;
	     s->wrapno = 0;
	     s++;
	  }
	w->trashed = 1;
	w = w->next;
     }
   while (all && (w != wsave));
}

#if 1
int jed_find_line_on_screen (Line *l, int min_line)
{
   int i, imax;

   if (JScreen == NULL)
     return -1;

   imax = Jed_Num_Screen_Rows - 2;
   for (i = min_line; i < imax; i++)
     {
	if (JScreen[i].line == l)
	  return i;
     }
   return -1;
}
#endif

void update_cmd (int *force)
{
   if (Batch) return;
   JWindow->trashed = 1;
   update((Line *) NULL, *force, 0, 1);
}

void update_sans_update_hook_cmd (int *force)
{
   if (Batch) return;
   JWindow->trashed = 1;
   update((Line *) NULL, *force, 0, 0);
}

