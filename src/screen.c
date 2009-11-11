/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
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

#if SLANG_VERSION < 20000
#define SLstack_depth _SLstack_depth
extern int _SLstack_depth(void);
#endif

volatile int Jed_Resize_Pending;
char *MiniBuf_Get_Response_String;

int Jed_Simulate_Graphic_Chars;

typedef struct Screen_Type
{
   Line *line;		       /* buffer line structure */
   int is_modified;
   unsigned char *hi0, *hi1;	       /* beg end of hilights */
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
					* only line, if negative, whole wind
					*/
int Term_Supports_Color = 1;	       /* optimistic assumption */

int Goal_Column;

int Want_Eob = 0;
int Display_Time = 1;                  /* Turn on %t processing in status line */

void (*X_Update_Open_Hook)(void);      /* hooks called when starting */
void (*X_Update_Close_Hook)(void);     /* and finishing update */

/* site.sl should modify this */
char Default_Status_Line[80] =
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

#if SLANG_VERSION < 20000
static unsigned char Char_Width[256];
static int Display_Eight_Bit = 0x7FFF;
# define FIX_CHAR_WIDTH \
     if (SLsmg_Display_Eight_Bit != Display_Eight_Bit) fix_char_width ()
#endif

static void display_line (Line *line, int sy, int sx)
{
   unsigned int len;
   int hscroll_col;
   int is_mini;
   Screen_Type *s;
#if JED_HAS_LINE_MARKS
   Mark *line_marks;
#endif
   int num_columns;
   int color_set;

   SLsmg_Tab_Width = Buffer_Local.tab;
#if SLANG_VERSION >= 20000
   (void) SLsmg_embedded_escape_mode (CBuf->flags & SMG_EMBEDDED_ESCAPE);
#endif
   is_mini = (sy + 1 == Jed_Num_Screen_Rows);

   SLsmg_gotorc (sy, sx);
   SLsmg_set_color (0);

   s = JScreen + sy;

   s->line = line;
   s->is_modified = 0;

   if (line == NULL)
     {
	SLsmg_erase_eol ();
	return;
     }

   hscroll_col = JWindow->hscroll_column - 1;

   if ((line == HScroll_Line)
       && Wants_HScroll && HScroll)
     hscroll_col += HScroll;

   num_columns = JWindow->width;

   if (hscroll_col || sx
#if JED_HAS_DISPLAY_LINE_NUMBERS
       || (CBuf->line_num_display_size)
#endif
      )
     {
	int tmp = hscroll_col - sx;
#if JED_HAS_DISPLAY_LINE_NUMBERS
	tmp -= CBuf->line_num_display_size;
	num_columns -= CBuf->line_num_display_size;
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

   color_set = 0;
#if JED_HAS_LINE_ATTRIBUTES
   if (line->flags & JED_LINE_COLOR_BITS)
     {
	SLsmg_set_color (JED_GET_LINE_COLOR(line));
	color_set = 1;
     }
#endif

#if JED_HAS_LINE_MARKS
   line_marks = CBuf->user_marks;
   if (color_set == 0) while (line_marks != NULL)
     {
	if ((line_marks->line == line)
	    && (line_marks->flags & JED_LINE_MARK))
	  {
	     SLsmg_set_color (line_marks->flags & MARK_COLOR_MASK);
	     color_set = 1;
	     break;
	  }
	line_marks = line_marks->next;
     }
#endif

   if (len)
     {
	if ((color_set == 0)
	    && Mode_Has_Syntax_Highlight
	    && (line != &Eob_Line)
#if !defined(IBMPC_SYSTEM)
	    && (*tt_Use_Ansi_Colors && Term_Supports_Color)
#endif
	    && Wants_Syntax_Highlight)
	  write_syntax_highlight (sy, line, len);
	else
	  {
	     if (Jed_Highlight_WS & HIGHLIGHT_WS_TRAILING)
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
		  SLsmg_write_nchars ((char *)pmin, p - pmin);
		  if (p != pmax)
		    {
		       SLsmg_set_color (JTWS_COLOR);
		       SLsmg_write_nchars ((char *)p, pmax - p);
		    }
	       }
	     else SLsmg_write_nchars ((char *)line->data, len);
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
   SLsmg_erase_eol ();

   if (Jed_Dollar)
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

   if ((s->hi0 != NULL) && Wants_Attributes)
     {
	int c;
	len = (int) (s->hi1 - s->hi0);

	if (len && (s->hi0[len - 1] == '\n'))
	  len--;

	if (len)
	  {
	     c = jed_compute_effective_length (line->data, s->hi0);
	     if (is_mini)
	       c += Mini_Info.effective_prompt_len;
	     SLsmg_gotorc (sy, c);
	     SLsmg_set_color (JREGION_COLOR);
	     SLsmg_write_nchars ((char *)s->hi0, len);
	  }
     }

   /* if (hscroll_col + sx) */
   SLsmg_set_screen_start (NULL, NULL);

   SLsmg_set_color (0);
}

#if JED_HAS_DISPLAY_LINE_NUMBERS
static void display_line_numbers (void)
{
   unsigned int i, imin, imax, linenum;
   Line *line_start, *line;
   char buf[32];
   unsigned int c;

   imin = JWindow->sy;
   imax = imin + JWindow->rows;
   line = NULL;

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

   for (i = imin; i < imax; i++)
     {
	line_start = JScreen[i].line;
	if (line_start == NULL)
	  break;

        while (line != line_start)
	  {
	     if (line == NULL)
	       return;		       /* ??? */

	     linenum++;
	     line = line->next;
	  }

	SLsmg_gotorc (i, c);
	sprintf (buf, "%*d ", CBuf->line_num_display_size-1, linenum);
	SLsmg_write_string (buf);

	line = line->next;
	linenum++;
     }

   if (i < imax)
     {
	memset (buf, ' ', CBuf->line_num_display_size);
	buf[CBuf->line_num_display_size] = 0;

	while (i < imax)
	  {
	     SLsmg_gotorc (i, c);
	     SLsmg_write_string (buf);
	     i++;
	  }
     }

   SLsmg_set_color (0);
}
#endif

#if JED_HAS_LINE_ATTRIBUTES
static int Non_Hidden_Point;
Line *jed_find_non_hidden_line (Line *l)
{
   int dir;
   Line *cline;

   Non_Hidden_Point = 0;
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

   if (dir == 1)
     {
	Non_Hidden_Point = cline->len;
     }

   return cline;
}

#endif

Line *jed_find_top_to_recenter (Line *cline)
{
   int n;
   Line *prev, *last_prev;

   n = JWindow->rows / 2;

   last_prev = prev = cline;

   while ((n > 0) && (prev != NULL))
     {
	n--;
	last_prev = prev;
#if JED_HAS_LINE_ATTRIBUTES
	do
	  {
	     prev = prev->prev;
	  }
	while ((prev != NULL)
	       && (prev->flags & JED_LINE_HIDDEN));
#else
	prev = prev->prev;
#endif
     }

   if (prev != NULL) return prev;
   return last_prev;
}

Line *find_top (void)
{
   int nrows, i;
   Line *cline, *prev, *next;
   Line *top_window_line;

   cline = CLine;

#if JED_HAS_LINE_ATTRIBUTES
   if (cline->flags & JED_LINE_HIDDEN)
     cline = jed_find_non_hidden_line (cline);
   if (cline == NULL)
     return NULL;
#endif

   nrows = JWindow->rows;

   if (nrows <= 1)
     return cline;

   /* Note: top_window_line might be a bogus pointer.  This means that I cannot
    * access it unless it really corresponds to a pointer in the buffer.
    */
   top_window_line = JScreen [JWindow->sy].line;

   if (top_window_line == NULL)
     return jed_find_top_to_recenter (cline);

   /* Chances are that the current line is visible in the window.  This means
    * that the top window line should be above it.
    */
   prev = cline;

   i = 0;
   while ((i < nrows) && (prev != NULL))
     {
	if (prev == top_window_line)
	  {
	     return top_window_line;
	  }

#if JED_HAS_LINE_ATTRIBUTES
	do
	  {
	     prev = prev->prev;
	  }
	while ((prev != NULL)
	       && (prev->flags & JED_LINE_HIDDEN));
#else
	prev = prev->prev;
#endif
	i++;
     }

   /* Now check the borders of the window.  Perhaps the current line lies
    * outsider the border by a line.  Only do this if terminal can scroll.
    */

   if ((*tt_Term_Cannot_Scroll)
       && (*tt_Term_Cannot_Scroll != -1))
     return jed_find_top_to_recenter (cline);

   next = cline->next;
#if JED_HAS_LINE_ATTRIBUTES
   while ((next != NULL) && (next->flags & JED_LINE_HIDDEN))
     next = next->next;
#endif

   if ((next != NULL)
       && (next == top_window_line))
     return cline;

   prev = cline->prev;
#if JED_HAS_LINE_ATTRIBUTES
   while ((prev != NULL) && (prev->flags & JED_LINE_HIDDEN))
     prev = prev->prev;
#endif

   top_window_line = JScreen [nrows + JWindow->sy - 1].line;

   if ((prev == NULL)
       || (prev != top_window_line))
     return jed_find_top_to_recenter (cline);

   /* It looks like cline is below window by one line.  See what line should
    * be at top to scroll it into view.
    */

   i = 2;
   while ((i < nrows) && (prev != NULL))
     {
#if JED_HAS_LINE_ATTRIBUTES
	do
	  {
	     prev = prev->prev;
	  }
	while ((prev != NULL)
	       && (prev->flags & JED_LINE_HIDDEN));
#else
	prev = prev->prev;
#endif
	i++;
     }

   if (prev != NULL)
     return prev;

   return jed_find_top_to_recenter (cline);
}

#if SLANG_VERSION >= 20000

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

#else

/* This function has to be consistent with SLsmg. */
static void fix_char_width(void)
{
   int i, ebit;

   ebit = SLsmg_Display_Eight_Bit;
   if (ebit < 128)
     ebit = 128;

   /* Control Characters */
   for (i = 0; i < 32; i++)
     Char_Width[i] = 2;

   for (i = 32; i < 127; i++)
     Char_Width[i] = 1;

   Char_Width[127] = 2;

   for (i = 128; i < ebit; i++)
     Char_Width[i] = 3;		       /* ~^X */

   for (i = ebit; i < 256; i++)
     Char_Width[i] = 1;

   Display_Eight_Bit = ebit;
}
   
void point_column (int n)
{
   register unsigned char *p, *pmax;
   register int i;
   int tab, w;

   FIX_CHAR_WIDTH;

   if (IN_MINI_WINDOW) n -= Mini_Info.effective_prompt_len;

   p = CLine->data;
   pmax = p + CLine->len;
   if (LINE_HAS_NEWLINE (CLine))
     pmax--;

   tab = Buffer_Local.tab;
   i = 0;
   n--;   /* start at 0 */
   while (p < pmax)
     {
	unsigned int nconsumed = 1;

	if ((*p == '\t') && tab)
	  {
	     i = tab * (i / tab + 1);
	  }
	else
	  {
	     w = Char_Width[*p];
	     i +=  w;
	  }
	if (i > n) break;
	p += nconsumed;
     }
   jed_set_point (p - CLine->data);
}

int jed_compute_effective_length (unsigned char *pos, unsigned char *pmax)
{
   int i;
   int tab, w;
   register unsigned char *cw = Char_Width;
   register unsigned char ch;

   FIX_CHAR_WIDTH;

   i = 0;
   tab = Buffer_Local.tab;

   while (pos < pmax)
     {
	ch = *pos++;
	if ((ch == '\t') && tab)
	  {
	     i = tab * (i/tab + 1);  /* tab column tabs */
	  }
	else
	  {
	     w = cw[ch];
	     i += w;
	     if (w == 0) pos++;
	  }
     }
   return i;
}
#endif				       /* SLANG_VERSION >= 20000 */

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
   int r, row;
   Line *tthis, *cline;

   if (JWindow->trashed) return;

   cline = CLine;
#if JED_HAS_LINE_ATTRIBUTES
   if (cline->flags & JED_LINE_HIDDEN)
     {
	cline = jed_find_non_hidden_line (cline);
	if (cline != NULL)
	  c = Non_Hidden_Point;
     }
#endif

   r = JWindow->sy + 1;
   Point_Cursor_Flag = 0;
   for (row = r; row < r + JWindow->rows; row++)
     {
	tthis = JScreen[row-1].line;
	if (tthis == NULL) break;
	if ((tthis == cline) || (tthis == &Eob_Line))
	  {
	     r = row;
	     break;
	  }
     }

   if (Point >= CLine->len)
     {
	Point = CLine->len - 1;

	if (Point < 0) Point = 0;
	else if ((*(CLine->data + Point) != '\n')
		 || (CBuf == MiniBuffer)) Point++;
     }

   if (c == 0)
     c = calculate_column ();
   c -= (JWindow->hscroll_column - 1);

   if (cline == HScroll_Line) c -= HScroll;
   if (c < 1) c = 1; else if (c > JWindow->width) c = JWindow->width;

   c += JWindow->sx;
#if JED_HAS_DISPLAY_LINE_NUMBERS
   c += CBuf->line_num_display_size;
#endif

   SLsmg_gotorc (r - 1, c - 1);
   Screen_Row = r;
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
   char buf [256];
   char *str;

   v = CBuf->status_line;
   if (*v == 0) v = Default_Status_Line;

   while (1)
     {
	char *v0 = v;
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

	switch (ch)
	  {
	   case 'F':
	     SLsmg_write_string (CBuf->dir);
	     str = CBuf->file;
	     break;

	   case 'S':
	     /* stack depth */
	     sprintf(buf, "%03d", SLstack_depth());
	     str = buf;
	     break;

	   case 'a':
	     if (CBuf->flags & ABBREV_MODE) str = " abbrev"; else str = NULL;
	     break;
	   case 'f': str = CBuf->file; break;
	   case 'n':
	     narrows = jed_count_narrows ();
	     if (narrows)
	       {
		  sprintf (buf, " Narrow[%d]", narrows);
		  str = buf;
	       }
	     else str = NULL;
	     break;

	   case 'o':
	     if (CBuf->flags & OVERWRITE_MODE) str = " Ovwrt"; else str = NULL;
	     break;
	   case 'O':
	     if (CBuf->flags & OVERWRITE_MODE) str = " ovr"; else str = " ins";
	     break;

	   case 'b': str = CBuf->name; break;

	   case 'p':
	     str = buf;
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
		       sprintf(buf, "%d%%",
			       (int) ((LineNum * 100L) / (long) Max_LineNum));
		    }
	       }
	     else
	       {
		  if (User_Prefers_Line_Numbers == 1)
		    sprintf(buf, "%d/%d", LineNum, Max_LineNum);
		  else
		    {
		       if (col_flag) (void) calculate_column ();
		       sprintf(buf, "%d/%d,%d", LineNum, Max_LineNum, Absolute_Column);
		    }
	       }
	     break;

	   case 'l':
	     sprintf(buf, "%d", LineNum);
	     str=buf;
	     break;

	   case 'L':
	     sprintf(buf, "%d", Max_LineNum);
	     str=buf;
	     break;

	   case 'v':
#if JED_HAS_UTF8_SUPPORT
	     SLsmg_write_string (Jed_Version_String);
	     if (Jed_UTF8_Mode) 
	       str = "U";
	     else
	       str = NULL;
#else
	     str = Jed_Version_String;
#endif
	     break;
	   case 'm': str = CBuf->mode_string; break;
	   case 't': str = status_get_time(); break;
	   case 'c':
	     if (col_flag) (void) calculate_column ();
	     sprintf(buf, "%d",  Absolute_Column);
	     str = buf;
	     break;

	   case '%': str = "%"; break;
	   case 0:
	     return;

	   default:
	     str = NULL;
	  }
	if (str != NULL)
	  SLsmg_write_string (str);
     }
}

void set_status_format(char *f, int *local)
{
   char *s;
   if (*local) s = Default_Status_Line; else s = CBuf->status_line;
   strncpy(s, f, 79);
   s[79] = 0;
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
   SLsmg_write_nchars (buf, (unsigned int) (b - buf));

   finish_status (col_flag);

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

/* This routine is called before the window is updated.  This means that the
 * line structures attached to the screen array cannot be trusted.  However,
 * the current line (or nearest visible one) can be assumed to lie in
 * the window.
 */
static void mark_window_attributes (int wa)
{
   register Screen_Type *s = &JScreen[JWindow->sy],
     *smax = s + JWindow->rows, *s1, *s2;
   Mark *m;
   register Line *l = JWindow->beg.line, *ml, *cline;
   unsigned char *hi0, *hi1;
   int mn, pn, dn, point, mpoint;

   s1 = s;

#if JED_HAS_LINE_ATTRIBUTES
   cline = jed_find_non_hidden_line (CLine);
   if (cline != CLine) point = Non_Hidden_Point;   /* updated by jed_find_non_hidden_line */
   else point = Point;
#else
   cline = CLine;
   point = Point;
#endif

   if ((CBuf->vis_marks == 0) || (wa == 0) || (Wants_Attributes == 0)
#if JED_HAS_LINE_ATTRIBUTES
       || (l->flags & JED_LINE_HIDDEN)
#endif
       )
     {
	s2 = s;
	goto done;		       /* I hate gotos but they are convenient */
     }

   m = CBuf->marks;

   while ((m->flags & VISIBLE_MARK_MASK) == 0) m = m->next;
   ml = m->line;
   mn = m->n;			       /* already in canonical form */
   pn = LineNum + CBuf->nup;	       /* not in canonical form */
   dn = pn - mn;

#if JED_HAS_LINE_ATTRIBUTES
   ml = jed_find_non_hidden_line (ml);
   if (ml == cline) dn = 0;
   if (ml != m->line)
     {
	mpoint = Non_Hidden_Point;     /* updated by jed_find_non_hidden_line */
     }
   else mpoint = m->point;
#else
   mpoint = m->point;
#endif

   /* find Screen Pos of point in window.  It has to be there */
   while (l != cline)
     {
	l = l->next;
#if JED_HAS_LINE_ATTRIBUTES
	if (l->flags & JED_LINE_HIDDEN) continue;
#endif
	s1++;
     }

   /* s1 now points at current line */
   /* The whole point of all of this is to preserve the screen flags without
    * touching the screen.
    */

   if (dn > 0)			       /* mark on prev lines */
     {
	s2 = s1 + 1;
	hi0 = l->data;
	hi1 = l->data + point;
	if ((s1->hi0 != hi0) || (s1->hi1 != hi1))
	  {
	     s1->hi0 = hi0; s1->hi1 = hi1;
	     s1->is_modified = 1;
	  }

	l = l->prev; s1--;
	while ((s1 >= s) && (l != ml) && (l != NULL))
	  {
#if JED_HAS_LINE_ATTRIBUTES
	     if (l->flags & JED_LINE_HIDDEN)
	       {
		  l = l->prev;
		  continue;
	       }
#endif
	     hi0 = l->data;
	     hi1 = l->data + l->len;
	     if ((s1->hi0 != hi0) || (s1->hi1 != hi1))
	       {
		  s1->hi0 = hi0; s1->hi1 = hi1;
		  s1->is_modified = 1;
	       }
	     s1--;
	     l = l->prev;
	  }

	if (s1 >= s)
	  {
	     hi0 = ml->data + mpoint;
	     hi1 = ml->data + ml->len;
	     if ((s1->hi0 != hi0) || (s1->hi1 != hi1))
	       {
		  s1->hi0 = hi0; s1->hi1 = hi1;
		  s1->is_modified = 1;
	       }
	     s1--;
	  }
     }
   else if (dn < 0)		       /* mark ahead of point */
     {
	s2 = s1;
	s1--;
	hi0 = l->data + point;
	hi1 = l->data + l->len;
	if ((s2->hi0 != hi0) || (s2->hi1 != hi1))
	  {
	     s2->hi0 = hi0; s2->hi1 = hi1;
	     s2->is_modified = 1;
	  }

	l = l->next;
	s2++;
	while ((s2 < smax) && (l != ml) && (l != NULL))
	  {
#if JED_HAS_LINE_ATTRIBUTES
	     if (l->flags & JED_LINE_HIDDEN)
	       {
		  l = l->next;
		  continue;
	       }
#endif
	     hi0 = l->data;
	     hi1 = l->data + l->len;
	     if ((s2->hi0 != hi0) || (s2->hi1 != hi1))
	       {
		  s2->hi0 = hi0; s2->hi1 = hi1;
		  s2->is_modified = 1;
	       }
	     l = l->next;
	     s2++;
	  }

	if (s2 < smax)
	  {
	     hi0 = ml->data;
	     hi1 = ml->data + mpoint;
	     if ((s2->hi0 != hi0) || (s2->hi1 != hi1))
	       {
		  s2->hi0 = hi0; s2->hi1 = hi1;
		  s2->is_modified = 1;
	       }
	     s2++;
	  }
     }
   else				       /* same line */
     {
	if (point < mpoint)
	  {
	     s1->hi0 = l->data + point;
	     s1->hi1 = l->data + mpoint;
	  }
	else
	  {
	     s1->hi1 = l->data + point;
	     s1->hi0 = l->data + mpoint;
	  }
	s1->is_modified = 1;
	s2 = s1 + 1;
	s1--;
     }

   done:			       /* reached if there is no mark */

   /* now do area outside the region */
   while (s1 >= s)
     {
	if (s1->hi0 != NULL)
	  {
	     s2->hi1 = s1->hi0 = NULL;
	     s1->is_modified = 1;
	  }
	s1--;
     }

   while (s2 < smax)
     {
	if (s2->hi0 != NULL)
	  {
	     s2->hi1 = s2->hi0 = NULL;
	     s2->is_modified = 1;
	  }
	s2++;
     }
}

static void compute_line_display_size (void)
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

/* if force then do update otherwise return 1 if update or 0 if not */
static int update_1(Line *top, int force)
{
   int i;
   Window_Type *start_win;
   int did_eob, time_has_expired = 0;

   if (Batch ||
       (!force
	&& (Executing_Keyboard_Macro || (Repeat_Factor != NULL)
	    || screen_input_pending (0)
	    || (Read_This_Character != NULL)))
       || (CBuf != JWindow->buffer))

     {
	return(0);
     }

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

   if (Wants_Attributes && CBuf->vis_marks)
     {
	JWindow->trashed = 1;
     }

   /* Do not bother setting this unless it is really needed */
   if (Display_Time)
     {
	Status_This_Time = sys_time();
	time_has_expired = (Status_This_Time > Status_Last_Time + 45);
     }

   /* if cursor moves just left right, do not update status line */
   if (!force && !JWindow->trashed &&
       ((JWindow == JWindow->next) || (User_Prefers_Line_Numbers && Cursor_Motion))
       /* if % wanted, assume user is like me and gets annoyed with
	* screen updates
	*/
       && (User_Prefers_Line_Numbers
	   || time_has_expired))
     {
	update_status_line(0);
	return(1);
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
	int imax;
	unsigned int start_column;

#if JED_HAS_LINE_ATTRIBUTES
	if (CBuf->min_unparsed_line_num)
	  jed_syntax_parse_buffer (0);
#endif
	if (Wants_Syntax_Highlight) init_syntax_highlight ();

#if JED_HAS_LINE_ATTRIBUTES
	if (top != NULL) top = jed_find_non_hidden_line (top);
#endif

#if SLANG_VERSION >= 20000
	/* (void) SLsmg_utf8_enable (CBuf->local_vars.is_utf8); */
#endif
	if (top == NULL)
	  {
	     top = find_top();
	     if (top == NULL) top = CLine;
	  }

	JWindow->beg.line = top;
#if JED_HAS_LINE_ATTRIBUTES
	if (top->flags & JED_LINE_HIDDEN)
	  top = NULL;
	else
#endif
	  mark_window_attributes ((start_win == JWindow) || (start_win->buffer != CBuf));

	did_eob = 0;

	i = JWindow->sy;
	imax = i + JWindow->rows;

	compute_line_display_size ();
	start_column = JWindow->sx;

	while (i < imax)
	  {
#if JED_HAS_LINE_ATTRIBUTES
	     if ((top != NULL) && (top->flags & JED_LINE_HIDDEN))
	       {
		  top = top->next;
		  continue;
	       }
#endif

	     /* the next line is really optional */
#if 0
	     if (!force && (Exit_From_MiniBuffer ||
			    screen_input_pending (0))) break;
#endif

	     if ((JScreen[i].line != top)
		 || JScreen[i].is_modified
		 || (Want_Eob
		     && !did_eob
		     && (i != Jed_Num_Screen_Rows - 1)
		     && (top == NULL)))
	       {
		  if (((top == NULL) || (top->len == 0))
		      && (Want_Eob && !did_eob && !(CBuf->flags & READ_ONLY)))
		    {
		       display_line(&Eob_Line, i, start_column);

		       /* JScreen[i].line = top; */
		       did_eob = 1;
		    }
		  else display_line(top, i, start_column);
	       }

	     if (top != NULL)
	       top = top->next;
	     i++;
	  }

	HScroll_Line = NULL;
	Mode_Has_Syntax_Highlight = 0;
	if (!force && screen_input_pending (0))
	  {
	     while (JWindow != start_win) other_window();
	     JWindow->trashed = 1;  /* since cursor not pointed */
#if SLANG_VERSION >= 20000
	     /* (void) SLsmg_utf8_enable (1); */  /* default state */
#endif

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
#if SLANG_VERSION >= 20000
   /* SLsmg_utf8_enable (1); */	       /* default state */
#endif
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
   Window_Type *w;

   if (Executing_Keyboard_Macro) return;

   if (MiniBuffer != NULL)
     {
	w = JWindow;
	while (!IN_MINI_WINDOW) other_window();

	JWindow->beg.line = CLine;
	mark_window_attributes (1);
	display_line(CLine, Jed_Num_Screen_Rows-1, 0);
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
     display_line(NULL, Jed_Num_Screen_Rows-1, 0);
}

#if 0
static void set_hscroll(int col)
{
   int hdiff, whs = abs(Wants_HScroll), wc = JWindow->hscroll_column - 1;
   int sw = Jed_Num_Screen_Cols - 1;
   static Line *last;
   Line *tmp;

#if JED_HAS_DISPLAY_LINE_NUMBERS
   sw -= CBuf->line_num_display_size;
#endif
   if (sw < 1)
     sw = 1;

   /* take care of last effect of horizontal scroll */
   if (last != NULL)
     {
	tmp = CLine;
	CLine = last;
	register_change(0);
	CLine = tmp;
	if (last != CLine)
	  {
#if 0
	     /* I need to think about this more */
	     if (Wants_HScroll < 0)
	       {
		  if (wc != 0)
		    {
		       JWindow->column = 1;
		       wc = 0;
		       touch_window ();
		    }
	       }
#endif
	     HScroll = 0;
	  }

	last = NULL;
     }

   col--;			       /* use 0 origin */
   hdiff = col - wc;
   if ((HScroll >= hdiff)
       || (HScroll <= hdiff - sw))
     {
	if (hdiff >= sw)
	  {
	     HScroll = hdiff - sw + whs;
	  }
	else if ((hdiff == 0) && (wc == 0)) HScroll = 0;
	else if (hdiff <= 1)
	  {
	     HScroll = hdiff - whs - 1;
	  }
	else HScroll = 0;
     }

   if (HScroll)
     {
	if (wc + HScroll < 0) HScroll = -wc;

	if (Wants_HScroll < 0)
	  {
	     JWindow->hscroll_column += HScroll;
	     touch_window();
	     HScroll = 0;
	  }
	else
	  {
	     register_change(0);
	     last = HScroll_Line = CLine;
	  }
     }
}
#endif
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
   if (Wants_HScroll) set_hscroll(col); else HScroll = 0;
   hscroll_line_save = HScroll_Line;

   if (SLang_get_error ()) flag = 0;	       /* update hook invalidates flag */

   if (SLang_get_error () && !(*Error_Buffer || SLKeyBoard_Quit))
     {
#if SLANG_VERSION < 20000
	SLang_doerror ("");
#else
	SLang_verror (0, "%s", SLerr_strerror (0));
#endif
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
   
   /* Add support for a resize-hook here */
   /* (void) jed_va_run_hooks ("_jed_resize_display_hooks", JED_HOOKS_RUN_ALL, 0); */

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

void recenter(int *np)
{
   Line *l = CLine;
   int i, n = *np;

   if (Batch)
     return;

   JWindow->trashed = 1;
   if (n == 0)
     {
	n = JWindow->rows / 2;
	i = 0;
	while (i < n)
	  {
	     if (l->prev == NULL) break;
	     l = l->prev;
#if JED_HAS_LINE_ATTRIBUTES
	     if (l->flags & JED_LINE_HIDDEN) continue;
#endif
	     i++;
	  }
	JWindow->beg.line = l;
	JWindow->beg.n -= i;
	JWindow->beg.point = 0;
	jed_redraw_screen (0);
	return;
     }

   if (CBuf != JWindow->buffer) return;

   if ((n <= 0) || (n > JWindow->rows)) n = JWindow->rows / 2;

   while (n > 1)
     {
	l = l->prev;
	if (l == NULL)
	  {
	     l = CBuf->beg;
	     break;
	  }
#if JED_HAS_LINE_ATTRIBUTES
	if (l->flags & JED_LINE_HIDDEN) continue;
#endif
	n--;
     }

   /* update(l, 1, 0, 1); */
   JScreen [JWindow->sy].line = l;
   JScreen [JWindow->sy].is_modified = 1;
}

int window_line (void)
{
   Line *cline;
   Line *top;
   int n;

   if (CBuf != JWindow->buffer) return 0;

   top = find_top ();

#if JED_HAS_LINE_ATTRIBUTES
   cline = jed_find_non_hidden_line (CLine);
#else
   cline = CLine;
#endif

   n = 1;

   while ((top != NULL) && (top != cline))
     {
	top = top->next;
#if JED_HAS_LINE_ATTRIBUTES
	while ((top != NULL) && (top->flags & JED_LINE_HIDDEN))
	  top = top->next;
#endif
	n++;
     }
   return n;
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
	     fprintf(stderr, "Max_LineNum: %d, LineNum: %d\n", Max_LineNum, LineNum);
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

