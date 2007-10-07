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

#include <string.h>
#include <ctype.h>
#include "buffer.h"
#include "ins.h"
#include "ledit.h"
#include "text.h"
#include "screen.h"
#include "cmds.h"
#include "paste.h"
#include "misc.h"

/*}}}*/
/* This routine deletes multiple spaces except those following a period, '?'
 * or a '!'.
   Returns the address of beginning of non whitespace */
static unsigned char *text_format_line(void) /*{{{*/
{
   unsigned char *p, *pmin, *pmax;
   unsigned int point_min;

   bol (); jed_skip_whitespace ();
   point_min = Point;

   p = CLine->data + point_min;
   pmax = jed_eol_position (CLine);
   while (p < pmax)
     {
	if (*p == '\t')
	  {
	     jed_position_point (p);
	     if (-1 == _jed_replace_wchar (' '))
	       return NULL;

	     /* In case the underlying pointer changed, reset these */
	     p = CLine->data + Point;
	     pmax = jed_eol_position (CLine);
	  }
	p++;
     }

   pmin = CLine->data + point_min;
   p = pmax - 1;

   while (p > pmin)
     {
	unsigned char *p1;
	unsigned int n;

	if ((*p != ' ') && (*p != '\t'))
	  {
	     p--;
	     continue;
	  }

	jed_position_point (p);
	p1 = jed_bskip_whitespace ();

	if (p == p1)
	  {
	     p--;		       /* single space */
	     continue;
	  }

	/* multiple spaces */
	n = p - p1;
	p = p1;
	p1--;
	if ((*p1 == '.') || (*p1 == '?') || (*p1 == '!'))
	  n--;

	if (n)
	  {
	     jed_position_point (p);
	     jed_del_nbytes (n);
	     pmin = CLine->data + point_min;
	     p = CLine->data + Point;
	  }

	p--;
     }

   return pmin;
}

/*}}}*/

/* returns 0 if to wrapped, 1 if wrapped, -1 if error */
static int wrap_line1(int format, int trim) /*{{{*/
{
   unsigned char *p, *pmin;
   int col;

   if (format)
     pmin = text_format_line();
   else
     {
	bol ();
	/* Ignore leading whitespace */
	pmin = jed_skip_whitespace ();
	/* pmin = CLine->data; */
     }
   if (pmin == NULL)
     return -1;

   point_column(Jed_Wrap_Column - 1);
   col = calculate_column();
   if ((col < Jed_Wrap_Column) && eolp ())
     return 0;

   p = CLine->data + Point;

   while(p > pmin)
     {
	if ((*p == ' ') || (*p == '\t')) break;
	p--;
     }

   if (p == pmin)
     {
	/* that failed, so go the other way */
	p = CLine->data + CLine->len;
	while(pmin < p)
	  {
	     if ((*pmin == ' ') || (*pmin == '\t')) break;
	     pmin++;
	  }
	if (p == pmin) return 0;
	p = pmin;
     }

   jed_position_point (p);
   if (trim) 
     (void)jed_trim_whitespace();
   (void) jed_insert_newline();
   jed_up(1);
   return 1;
}

/*}}}*/

int wrap_line (int format) /*{{{*/
{
   int ret;
   push_spot();
   ret = wrap_line1(format, 1);
   pop_spot();
   return ret;
}

/*}}}*/

static int is_paragraph_sep(void) /*{{{*/
{
   int ret;
   Jed_Buffer_Hook_Type *h = CBuf->buffer_hooks;

   if ((h != NULL) 
       && (h->par_sep != NULL))
     {
	if ((-1 == SLexecute_function(h->par_sep))
	    || (-1 == SLang_pop_integer(&ret)))
	  ret = -1;

	return ret;
     }

   push_spot ();
   (void) bol ();
   jed_skip_whitespace ();
   if (eolp ())
     {
	pop_spot ();
	return 1;
     }
   pop_spot ();
   return 0;
}

/*}}}*/

int backward_paragraph(void) /*{{{*/
{
   Jed_Buffer_Hook_Type *h = CBuf->buffer_hooks;

   if ((h != NULL)
       && (h->backward_paragraph_hook != NULL))
     {
	if (0 == SLexecute_function (h->backward_paragraph_hook))
	  return 1;
	
	return -1;
     }

   bol ();
   while ((1 == is_paragraph_sep ())
	  && jed_up (1))
     bol ();

   while ((0 == is_paragraph_sep ())
	  && jed_up (1))
     bol ();

   return 1;
}

/*}}}*/

int forward_paragraph(void) /*{{{*/
{
   Jed_Buffer_Hook_Type *h = CBuf->buffer_hooks;

   if ((h != NULL)
       && (h->forward_paragraph_hook != NULL))
     {
	if (0 == SLexecute_function (h->forward_paragraph_hook))
	  return 1;

	return -1;
     }

   while (1)
     {
	if (0 == jed_down (1))
	  {
	     eol ();
	     break;
	  }
	if (is_paragraph_sep ())
	  break;
     }

   return(1);
}

/*}}}*/

static int mark_paragraph (Line **begp, Line **endp)
{
   Jed_Buffer_Hook_Type *h = CBuf->buffer_hooks;

   if ((h != NULL)
       && (h->mark_paragraph_hook != NULL))
     {
	int point;
	Line *end;

	if (-1 == SLexecute_function (h->mark_paragraph_hook))
	  return -1;
	
	point = Point;
	end = CLine;
	if (Point && eobp ())
	  *endp = NULL;
	else
	  *endp = end;

	(void) jed_pop_mark (1);
	*begp = CLine;
	if ((end == CLine) && (point == Point))
	  return -1;

	return 0;
     }
   
   if (is_paragraph_sep())
     {
	pop_spot();
	return -1;
     }

   /* find end */
   forward_paragraph();
   if (Point && eobp ())
     *endp = NULL;
   else
     *endp = CLine;

   /* find paragraph start */
   backward_paragraph();
   if (is_paragraph_sep())
     (void) jed_down (1);

   *begp = CLine;
   return 0;
}


/* format paragraph and if Prefix argument justify_hook is called. */
int text_format_paragraph () /*{{{*/
{
   unsigned char *p;
   int indent_col, col;
   Line *end, *beg, *next;
   Jed_Buffer_Hook_Type *h = CBuf->buffer_hooks;

   CHECK_READ_ONLY
     
     if ((h != NULL) 
	 && (h->format_paragraph_hook != NULL))
       return SLexecute_function (h->format_paragraph_hook);

   push_spot();
   
   get_current_indent(&indent_col);
   if (indent_col + 1 >= Jed_Wrap_Column)
     indent_col = 0;

   if (-1 == mark_paragraph (&beg, &end))
     return 0;

   get_current_indent (&col);
   /* col is the indentation of the first line of the paragraph-- don't change it */

   bol ();
   while (CLine != end)
     {
	if (-1 == wrap_line1 (0, 0))
	  {
	     pop_spot ();
	     return -1;
	  }
	if (0 == jed_down (1))
	  break;
     }

   while ((CLine != beg) 
	  && (0 != jed_up (1)))
     ;

   if (col + 1 >= Jed_Wrap_Column)
     indent_to (indent_col);

   bol ();

   /* Now loop formatting as we go until the end is reached */
   while(CLine != end)
     {
	int status;

	/* eol(); */
	if (CLine != beg) indent_to(indent_col);
	status = wrap_line1(1, 1);
	if (status == -1)
	  {
	     pop_spot ();
	     return -1;
	  }
	if (status == 1)
	  {
	     (void) jed_down(1);
	     /* indent_to(indent_col); */
	     continue;
	  }
	else if (CLine->next == end)
	  break;

	next = CLine->next;
	if (next != end)
	  {
	     unsigned char *pmax;

	     /* Now count the length of the word on the next line. */
	     (void) jed_down(1);       /* at bol too */
	     jed_trim_whitespace();
	     p = CLine->data;
	     pmax = jed_eol_position (CLine);

	     /* FIXME for other multibyte whitespace chars */
	     while ((p < pmax) && (*p > ' '))
	       p++;

	     jed_up(1);		       /* at eol too */

	     col = calculate_column();

	     /* FIXME for multibyte */
	     if ((p - next->data) + col < Jed_Wrap_Column - 1)
	       {
		  if (-1 == _jed_replace_wchar (' '))
		    return -1;
	       }
	     else
	       {
		  (void) jed_down(1);
	       }
	  }
     }
   if (Repeat_Factor != NULL)
     {
	SLang_run_hooks("format_paragraph_hook", 0);
	Repeat_Factor = NULL;
     }
   pop_spot();
   return(1);
}

/*}}}*/

int narrow_paragraph(void) /*{{{*/
{
   int wrap, n;

   CHECK_READ_ONLY
   /* if (CBuf->modes != WRAP_MODE) return(0); */
   get_current_indent(&n);
   wrap = Jed_Wrap_Column;
   if (wrap - n <= wrap/2) return(0);
   Jed_Wrap_Column -= n;
   text_format_paragraph();
   Jed_Wrap_Column = wrap;
   return(1);
}

/*}}}*/

int center_line(void) /*{{{*/
{
   unsigned char *p, *pmax;
   int len;

   CHECK_READ_ONLY
   push_spot();
   (void) eol_cmd();
   p = CLine->data;
   pmax = p + CLine->len;

   while(p < pmax)
     {
	if (*p > ' ') break;
	p++;
     }
   if ((len = (int)(pmax - p)) < 0) len = 0;
   if ((len = (Jed_Wrap_Column - len) / 2) < 0) len = 0;
   indent_to(len);
   pop_spot();
   return(1);
}

/*}}}*/

int text_smart_quote(void) /*{{{*/
{
   unsigned char c;
   int upd, last;

   /* Force a screen update.  This help syntax highlighting */
   JWindow->trashed = 1;

   if (Point) c = *(CLine->data + (Point - 1)); else c = 0;
   if (!(CBuf->modes & WRAP_MODE) || (c == '\\')) return ins_char_cmd();

   last = SLang_Last_Key_Char;
   if ((c == '(') || (c == '[') || (c == '{') || (c <= ' ') || !Point)
     SLang_Last_Key_Char = '`';
   else
     SLang_Last_Key_Char = '\'';

   upd = ins_char_cmd();
   if (last == '"') upd = ins_char_cmd();
   SLang_Last_Key_Char = last;
   return upd;
}

/*}}}*/

/* FIXME: This needs to be corrected for UTF-8 */
char Jed_Word_Range[256];
void define_word(char *w) /*{{{*/
{
   strncpy(Jed_Word_Range, w, sizeof (Jed_Word_Range));
   Jed_Word_Range[sizeof(Jed_Word_Range) - 1] = 0;
}

/*}}}*/

char *jed_get_word_chars (void)
{
   return Jed_Word_Range;
}

#if SLANG_VERSION < 20000
static void skip_chars_forward (char *range, int invert)
{
   unsigned char lut[256];
   
   SLmake_lut(lut, (unsigned char *) range, (unsigned char) invert);
   /* more flexibility is achieved with following line */
   if (invert && lut['\n']) lut['\n'] = 0;
   
   do
     {
	unsigned char *p = CLine->data + Point;
	unsigned char *pmax = CLine->data + CLine->len;

	while (p < pmax)
	  {
	     if (0 == lut[*p])
	       {
		  jed_position_point (p);
		  return;
	       }
	     p++;
	  }
     }
   while (jed_down (1));
   eob();
}

static void skip_chars_backward (char *range, int invert)
{
   unsigned char lut[256];
   
   SLmake_lut(lut, (unsigned char *) range, (unsigned char) invert);
   /* more flexibility is achieved with following line */
   if (invert && lut['\n']) lut['\n'] = 0;
   
   do
     {
	unsigned char *pmin = CLine->data;
	unsigned char *p = pmin + Point;
	
	while (p > pmin)
	  {
	     p--;
	     if (0 == lut[*p])
	       {
		  jed_position_point (p+1);
		  return;
	       }
	  }

	bol ();
	if (lut['\n'] == 0) 
	  return;
     }
   while (jed_up (1));
   bob();
}

#else				       /* SLANG_VERSION >= 20000 */

static void skip_chars_backward (char *range, int invert)
{
   SLwchar_Lut_Type *lut;
   int ignore_combining = 1;
   int no_newline;

   if (*range == '^')
     {
	invert = !invert;
	range++;
     }

   if (NULL == (lut = SLwchar_strtolut ((SLuchar_Type *)range, 1, 1)))
     return;

   no_newline = (0 == SLwchar_in_lut (lut, '\n'));
   if (invert)
     no_newline = !no_newline;

   do
     {
	unsigned char *p = CLine->data + Point;
	unsigned char *pmin = CLine->data;

	p = SLwchar_bskip_range (lut, pmin, p, ignore_combining, invert);
	if (p == NULL)
	  {
	     SLwchar_free_lut (lut);
	     return;
	  }
	jed_position_point (p);
	if (p > pmin)
	  {
	     SLwchar_free_lut (lut);
	     return;
	  }
	if (no_newline)
	  {
	     SLwchar_free_lut (lut);
	     return;
	  }
     }
   while (jed_up (1));
   SLwchar_free_lut (lut);
   bob ();
}

static void skip_chars_forward (char *range, int invert)
{
   SLwchar_Lut_Type *lut;
   int ignore_combining = 1;

   if (*range == '^')
     {
	invert = !invert;
	range++;
     }

   if (NULL == (lut = SLwchar_strtolut ((SLuchar_Type *)range, 1, 1)))
     return;

   do
     {
	unsigned char *p = CLine->data + Point;
	unsigned char *pmax = CLine->data + CLine->len;

	p = SLwchar_skip_range (lut, p, pmax, ignore_combining, invert);
	if (p == NULL)
	  {
	     SLwchar_free_lut (lut);
	     return;
	  }
	if (p < pmax)
	  {
	     jed_position_point (p);
	     SLwchar_free_lut (lut);
	     return;
	  }
     }
   while (jed_down (1));
   SLwchar_free_lut (lut);
   eob();
}
#endif

void skip_word_chars(void) /*{{{*/
{
   skip_chars_forward (Jed_Word_Range, 0);
}

/*}}}*/

void skip_non_word_chars(void) /*{{{*/
{
   skip_chars_forward (Jed_Word_Range, 1);
}

/*}}}*/

void bskip_word_chars(void) /*{{{*/
{
   skip_chars_backward (Jed_Word_Range, 0);
}

/*}}}*/

void bskip_non_word_chars(void) /*{{{*/
{
   skip_chars_backward (Jed_Word_Range, 1);
}

/*}}}*/

/*{{{ skip_chars */
void jed_skip_chars (char *range)
{
   if (*range == '^') skip_chars_forward (range + 1, 1);
   else
     {
	if ((*range == '\\') && (range[1] == '^')) range++;
	skip_chars_forward (range, 0);
     }
}

/*}}}*/
/*{{{ bskip_chars */
void jed_bskip_chars (char *range)
{
   if (*range == '^') skip_chars_backward (range + 1, 1);
   else
     {
	if ((*range == '\\') && (range[1] == '^')) range++;
	skip_chars_backward (range, 0);
     }
}

/*}}}*/
