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
#include <string.h>
#include "buffer.h"
#include "screen.h"
#include "colors.h"
#include "file.h"
#include "misc.h"
#include "ledit.h"
#include "indent.h"

static unsigned short *Char_Syntax;
static char **Keywords = NULL;	       /* array of keywords */
static int Keyword_Not_Case_Sensitive;

static unsigned char *write_using_color (unsigned char *p,
					 unsigned char *pmax,
					 int color)
{
   if ((color == 0) && (Jed_Highlight_WS & HIGHLIGHT_WS_TAB))
     {
	unsigned char *p1 = p;
	while (p1 < pmax)
	  {
	     if (*p1 != '\t')
	       {
		  p1++;
		  continue;
	       }
	     
	     if (p1 != p)
	       {
		  SLsmg_set_color (0);
		  SLsmg_write_nchars ((char *)p, (unsigned int)(p1-p));
		  p = p1;
	       }
	     while ((p1 < pmax) && (*p1 == '\t'))
	       p1++;
	     SLsmg_set_color (JTAB_COLOR);
	     SLsmg_write_nchars ((char *)p, (unsigned int)(p1-p));
	     p = p1;
	  }
     }
   /* drop */
   SLsmg_set_color (color);
   SLsmg_write_chars (p, pmax);
   SLsmg_set_color (0);
   return pmax;
}

#if JED_HAS_COLOR_COLUMNS
static void color_columns (int row, register unsigned char *p, register unsigned char *pmax)
{
   unsigned char *c;
   int color;
   int x0, x1, nx;

   if (p != pmax)
     (void) write_using_color (p, pmax, 0);
   
   if (NULL == (c = CBuf->column_colors))
     return;
   
   nx = CBuf->num_column_colors;
   color = 0;
   x1 = x0 = 0;
   while (x1 < nx)
     {
	if (color != (int)c[x1])
	  {
	     if (x0 != x1)
	       {
		  if (color != 0)
		    SLsmg_set_color_in_region (color, row, x0, 1, x1 - x0);
	       }
	     color = (int) c[x1];
	     x0 = x1;
	  }
	x1++;
     }
   if ((x1 != x0) && (color != 0))
     SLsmg_set_color_in_region (color, row, x0, 1, x1 - x0);
}
#endif				       /* JED_HAS_COLOR_COLUMNS */

static int try_keyword (register unsigned char *q, int n, register char *t, unsigned char color) /*{{{*/
{
   unsigned char *p;

   while (*t)
     {
	p = q - n;
	if (Keyword_Not_Case_Sensitive == 0)
	  {
	     while ((p < q) && (*t == (char) *p))
	       {
		  p++; t++;
	       }
	  }
	else while ((p < q) && (*t == (char) LOWER_CASE (*p)))
	  {
	     p++; t++;
	  }

	if (p == q)
	  {
	     p = q - n;
	     write_using_color (p, q, color);
	     return 0;
	  }

	/* alphabetical */
	if (*t > ((char) *p | Keyword_Not_Case_Sensitive))
	  break;

	t += (int) (q - p);
     }
   return -1;
}

/*}}}*/

static unsigned char *highlight_word (unsigned char *p, unsigned char *pmax) /*{{{*/
{
   char **kwds;
   register unsigned char *q;
   int n;
   int i;
   int color;

   q = p;
   while ((q < pmax) && (Char_Syntax[*q] & WORD_SYNTAX)) q++;

   n = (int) (q - p);

   kwds = Keywords;
   if ((kwds != NULL) && (n <= MAX_KEYWORD_LEN))
     {
	for (i = 0; i < MAX_KEYWORD_TABLES; i++)
	  {
	     char *t;
	     t = kwds[n - 1];
	     if (t != NULL)
	       {
		  color = (JKEY_COLOR + i);
		  if (0 == try_keyword (q, n, t, color))
		    return q;
	       }
	     kwds += MAX_KEYWORD_LEN;
	  }
     }
   return write_using_color (p, q, 0);
}

/*}}}*/

static unsigned char *highlight_string (unsigned char *p, unsigned char *pmax, /*{{{*/
					unsigned char quote, unsigned char str_char,
					int ofs)
{
   unsigned char ch;
   unsigned char *p1;

   p1 = p + ofs;
   while (p1 < pmax)
     {
	ch = (unsigned char) *p1++;
	if (ch == str_char) break;
	if ((ch == quote) && (p1 < pmax))
	  p1++;
     }
   return write_using_color (p, p1, JSTR_COLOR);
}

/*}}}*/

static unsigned char *highlight_number (unsigned char *p, unsigned char *pmax) /*{{{*/
{
   unsigned char *p1;
   unsigned char ch;

   ch = (unsigned char) *p;

   p1 = p + 1;
   if (ch == '-')
     {
	if ((p1 < pmax) && (Char_Syntax[*p1] & NUMBER_SYNTAX))
	  p1++;
	else
	  return write_using_color (p, p1, JOP_COLOR);
     }

   while ((p1 < pmax) && (Char_Syntax[*p1] & NUMBER_SYNTAX))
     {
	if ((*p1 == '-') || (*p1 == '+'))
	  {
	     ch = *(p1 - 1);
	     if ((ch != 'e') && (ch != 'E'))
	       break;
	  }
	p1++;
     }

   return write_using_color (p, p1, JNUM_COLOR);
}

/*}}}*/

static unsigned char *highlight_comment (unsigned char *p,
					 unsigned char *p1,
					 unsigned char *pmax, /*{{{*/
					 Syntax_Table_Type *st)
{
   unsigned char stop_char;
   char *s;
   unsigned int len;

   if (NULL == (s = st->comment_stop)) s = "";

   stop_char = *s;
   len = st->comment_stop_len;

   while (p1 < pmax)
     {
	if (*p1 == stop_char)
	  {
	     if ((p1 + len < pmax)
		 && (0 == strncmp ((char *)p1, s, len)))
	       {
		  p1 += len;
		  break;
	       }
	  }
	p1++;
     }

   return write_using_color (p, p1, JCOM_COLOR);
}

/*}}}*/

#if JED_HAS_DFA_SYNTAX
# include "dfasyntx.c"
#endif

static unsigned char *write_whitespace (unsigned char *p, unsigned char *pmax, int trailing_color)
{
   unsigned char *p1;

   p1 = p;
   while ((p1 < pmax) && ((*p1 == ' ') || (*p1 == '\t')))
     p1++;
   if (p1 == p)
     return p;

   if ((p1 == pmax) && (Jed_Highlight_WS & HIGHLIGHT_WS_TRAILING))
     return write_using_color (p, pmax, trailing_color);

   return write_using_color (p, p1, 0);
}

void write_syntax_highlight (int row, Line *l, unsigned int len)
{
   Syntax_Table_Type *st = CBuf->syntax_table;
   unsigned char ch;
   unsigned int flags;
   unsigned char *p1;
   unsigned short syntax;
   register unsigned char *p;
   register unsigned char *pmax;
   int context;

   p = l->data;
   pmax = p + len;

#if JED_HAS_COLOR_COLUMNS
   if (CBuf->coloring_style)
     {
	color_columns (row, p, pmax);
	return;
     }
#endif

   if (st == NULL) return;

#if JED_HAS_DFA_SYNTAX
   if (st->use_dfa_syntax
       && (st->hilite != NULL)
       && (st->hilite->dfa != NULL))
     {
	dfa_syntax_highlight (p, pmax, st);
	return;
     }
#endif

   flags = st->flags;

#if JED_HAS_LINE_ATTRIBUTES
   context = l->flags & JED_LINE_SYNTAX_BITS;
#else
   context = 0;
#endif

   if (context & JED_LINE_IN_COMMENT)
     {
	/* I do not like the whitespace that preceeds the
	 * '*' on the current line to be highlighted.  So...
	 */
	if (flags & C_COMMENT_TYPE)
	  {
	     p1 = p;
	     while ((p1 < pmax) && ((*p1 == ' ') || (*p1 == '\t')))
	       p1++;

	     if ((p1 < pmax) && (*p1 == '*'))
	       p = write_using_color (p, p1, 0);
	  }

	p = highlight_comment (p, p, pmax, st);
     }
   else if (context & JED_LINE_IN_STRING0)
     {
	p = highlight_string (p, pmax, st->quote_char, st->string_chars[0], 0);
     }
   else if (context & JED_LINE_IN_STRING1)
     {
	p = highlight_string (p, pmax, st->quote_char, st->string_chars[1], 0);
     }
   else if (context & JED_LINE_IN_HTML)
     {
	p1 = p;
	ch = st->sgml_stop_char;
	while ((p1 < pmax) && (*p1++ != ch))
	  ;
	p = write_using_color (p, p1, JHTML_KEY_COLOR);
     }
   else if ((flags & FORTRAN_TYPE)
	    && st->fortran_comment_chars[*p])
     {
	(void) write_using_color (p, pmax, JCOM_COLOR);
	return;
     }
   else
     {
	/* Handle the preprocessor */
	if (flags & PREPROCESS_IGNORE_WHITESPACE)
	  {
	     p = write_whitespace (p, pmax, JTWS_COLOR);
	     if (p == pmax)
	       return;
	  }
	if (*p == st->preprocess)
	  {
	     if (flags & PREPROCESS_COLOR_WHOLE_LINE)
	       {
		  (void) write_using_color (p, pmax, JPREPROC_COLOR);
		  return;
	       }
	     p1 = p + 1;
	     /* FIXME!!! I need to add a whitespace syntax */
	     while ((p1 < pmax) && ((*p1 == ' ') || (*p1 == '\t')))
	       p1++;
	     while ((p1 < pmax) && (Char_Syntax[*p1] != 0))
	       p1++;

	     p = write_using_color (p, p1, JPREPROC_COLOR);
	  }
     }

   /* Now the preliminary stuff is done so do the hard part */
   while (p < pmax)
     {
	syntax = Char_Syntax[*p];

	/* Do comment syntax before word syntax since the comment start
	 * may be a word.
	 */
	if (syntax & COMMENT_SYNTAX)
	  {
	     p1 = p + 1;
	     ch = *p;

	     if ((st->comment_start != NULL)
		 && (ch == st->comment_start[0])
		 && (p + st->comment_start_len <= pmax)
		 && (0 == strncmp ((char *)p, st->comment_start, st->comment_start_len)))
	       {
		  p = highlight_comment (p, p + st->comment_start_len, pmax, st);
		  continue;
	       }

	     if (_jed_is_eol_comment_start (st, l, p, pmax, NULL))
	       {
		  (void) write_using_color (p, pmax, JCOM_COLOR);
		  return;
	       }
	  }

	if (syntax & WORD_SYNTAX)
	  {
	     if ((*p > '9') || (0 == (syntax & NUMBER_SYNTAX)))
	       {
		  p = highlight_word (p, pmax);
		  continue;
	       }
	  }

	if (syntax == 0)
	  {
	     p1 = p;
	     while ((p1 < pmax) && (Char_Syntax[*p1] == 0))
	       p1++;
	     if ((p1 != pmax)
		 || ((Jed_Highlight_WS & HIGHLIGHT_WS_TRAILING) == 0))
	       {
		  p = write_using_color (p, p1, 0);
		  continue;
	       }
	     while (p1 > p)
	       {
		  p1--;
		  if ((*p1 != ' ') && (*p1 != '\t'))
		    {
		       p1++;
		       break;
		    }
	       }
	     p = write_using_color (p, p1, 0);
	     if (p1 != pmax)
	       (void) write_using_color (p1, pmax, JTWS_COLOR);
	     return;
	  }

	if (syntax & DELIM_SYNTAX)
	  {
	     p = write_using_color (p, p + 1, JDELIM_COLOR);
	     continue;
	  }

	if (syntax & STRING_SYNTAX)
	  {
	     p = highlight_string (p, pmax, st->quote_char, *p, 1);
	     continue;
	  }

	if (syntax & OP_SYNTAX)
	  {
	     p = write_using_color (p, p + 1, JOP_COLOR);
	     continue;
	  }

	if (syntax & NUMBER_SYNTAX)
	  {
	     p = highlight_number (p, pmax);
	     continue;
	  }

	if (syntax & HTML_START_SYNTAX)
	  {
	     p1 = p;
	     while (p1 < pmax)
	       {
		  if (Char_Syntax[*p1] & HTML_END_SYNTAX)
		    {
		       p1++;
		       break;
		    }
		  p1++;
	       }
	     p = write_using_color (p, p1, JHTML_KEY_COLOR);
	     continue;
	  }

	if (syntax & HTML_END_SYNTAX)  /* missed start from previous line */
	  {
	     /* FIXME!!! Start from beginning */
	     p = write_using_color (p, p + 1, JHTML_KEY_COLOR);
	     continue;
	  }

	if ((syntax & OPEN_DELIM_SYNTAX) || (syntax & CLOSE_DELIM_SYNTAX))
	  {
	     p = write_using_color (p, p + 1, JDELIM_COLOR);
	     continue;
	  }

	if ((syntax & QUOTE_SYNTAX) && (flags & TEX_LIKE_KEYWORDS))
	  {
	     p1 = p + 1;
	     if (p1 < pmax)
	       {
		  if (Char_Syntax[*p1] & WORD_SYNTAX)
		    {
		       do
			 {
			    p1++;
			 }
		       while ((p1 < pmax) && (Char_Syntax[*p1] & WORD_SYNTAX));
		    }
		  else p1++;
	       }
	     p = write_using_color (p, p1, JKEY_COLOR);
	     continue;
	  }

	if (syntax & QUOTE_SYNTAX)
	  {
	     p1 = jed_multibyte_chars_forward (p, pmax, 2, NULL, 1);
	     /* p1 = p + 2; */
	     if (p1 < pmax)
	       {
		  p = write_using_color (p, p1, 0);
		  continue;
	       }
	  }

	/* Undefined. */
	p = write_using_color (p, p + 1, 0);
     }
}

/*}}}*/

void init_syntax_highlight (void) /*{{{*/
{
    Syntax_Table_Type *st = CBuf->syntax_table;

    Mode_Has_Syntax_Highlight = 1;
    if (CBuf->coloring_style) return;

   if ((st == NULL) || (st == Default_Syntax_Table))
     {
        Mode_Has_Syntax_Highlight = 0;
        return;
    }

#if JED_HAS_DFA_SYNTAX
   if (st->use_dfa_syntax
       && ((NULL != st->hilite) && (NULL != st->hilite->dfa)))
     return;
#endif

   Char_Syntax = st->char_syntax;

   if (st->flags & SYNTAX_NOT_CASE_SENSITIVE)
     Keyword_Not_Case_Sensitive = 0x20;
   else
     Keyword_Not_Case_Sensitive = 0;

   Keywords = (char **) st->keywords;
}

/*}}}*/

