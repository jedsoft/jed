/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2010 John E. Davis
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
#include "misc.h"
#include "cmds.h"
#include "display.h"
#include "ledit.h"
#include "screen.h"
#include "sysdep.h"
#include "search.h"
#include "paste.h"
#include "misc.h"
#include "indent.h"

/*}}}*/

/* Note: In this file, "UTF-8 ASCII safe" means that the routine will work
 * properly as long as we are dealing with ASCII characters for quotes and
 * comment strings.
 *
 * "UTF-8 WORD UNsafe" indicates that the routine is not safe when non-ASCII
 * characters are given the word syntax.
 *
 * ASCII means character codes 0-127.
 */
Syntax_Table_Type *Default_Syntax_Table;

static Syntax_Table_Type *Syntax_Tables;

/* UTF-8 ASCII safe */
static unsigned char *find_comment_end (Syntax_Table_Type *table,
					unsigned char *p, unsigned char *pmax)
{
   unsigned int len;
   char *str;
   unsigned char ch;
   /* unsigned char quote; */

   str = table->comment_stop;
   len = table->comment_stop_len;

   if ((str == NULL) || (len == 0))
     return NULL;

   pmax -= (len - 1);

   ch = *str;
   /* quote = table->quote_char; */

   while (p < pmax)
     {
	if ((*p == ch)
	    && (0 == strncmp (str, (char *)p, len)))
	  return p + len;

	p++;
     }

   return NULL;
}

/* UTF-8 ASCII safe */
static unsigned char *find_string_end (Syntax_Table_Type *table,
				       unsigned char *p, unsigned char *pmax,
				       unsigned char ch)
{
   unsigned char quote;

   quote = table->quote_char;

   while (p < pmax)
     {
	if (*p == quote)
	  {
	     p = jed_multibyte_chars_forward (p+1, pmax, 1, NULL, 0);
	     continue;
	  }
	if (*p++ == ch)
	  return p;
     }

   return NULL;
}

/* UTF-8 ASCII safe */
static int is_fortran_comment (Line *l, Syntax_Table_Type *table)
{
   return l->len && table->fortran_comment_chars[l->data[0]];
}

/* UTF-8 WORD unsafe */
int _jed_is_eol_comment_start (Syntax_Table_Type *table, Line *l,
			       unsigned char *p, unsigned char *pmax, unsigned int *com)
{
   unsigned int i;
   int wsc;

   if (p >= pmax)
     return 0;

   /* Most of the time, num_eol_comments is going to be <= 1, so just
    * use table->whatever.
    */
   wsc = table->flags & EOL_COMMENT_NEEDS_WHITESPACE;

   for (i = 0; i < table->num_eol_comments; i++)
     {
	if (*p != table->eol_comment_starts[i][0])
	  continue;

	if (p + table->eol_comment_lens[i] > pmax)
	  continue;

	if (0 == (table->flags & SYNTAX_NOT_CASE_SENSITIVE))
	  {
	     if (0 != strncmp ((char *) p, table->eol_comment_starts[i],
			       table->eol_comment_lens[i]))
	       continue;
	  }
	else
	  {
	     if (0 != jed_case_strncmp ((char *) p, table->eol_comment_starts[i],
					table->eol_comment_lens[i]))
	       continue;
	  }

	if ((table->char_syntax[*p] & WORD_SYNTAX) || wsc)
	  {
	     unsigned char *p1;
	     unsigned char ch1;

	     p1 = p + table->eol_comment_lens[i];
	     if (p1 < pmax)
	       {
		  ch1 = *p1;
		  if (wsc)
		    {
		       if ((ch1 != ' ') && (ch1 != '\n') && (ch1 != '\t'))
			 {
			    /* This piece of code permits: #### */
			    if ((ch1 != *p)
				|| (table->char_syntax[ch1] & WORD_SYNTAX)
				|| (p1 != p + 1))
			      continue;
			 }
		    }
		  else if (table->char_syntax[ch1] & WORD_SYNTAX)
		    continue;
	       }

	     if (p > l->data)
	       {
		  ch1 = *(p-1);

		  if (wsc)
		    {
		       if ((ch1 != ' ') && (ch1 != '\t'))
			 {
			    /* This piece of code permits: #### */
			    if ((ch1 != *p)
				|| (table->char_syntax[ch1] & WORD_SYNTAX)
				|| (p1 != p + 1))
			      continue;
			 }
		    }
		  else if (table->char_syntax[ch1] & WORD_SYNTAX)
		    continue;
	       }
	  }

	if (com != NULL)
	  *com = i;

	return 1;
     }

   return 0;
}

static int parse_to_point1 (Syntax_Table_Type *table,
			    Line *l,
			    unsigned char *pmax)
{
   unsigned char ch;
   int quote, flags;
   unsigned char *p;
   unsigned int lflags;
   unsigned char com_start_char;
   char *comment_start;
   unsigned short *syntax;
   unsigned char *sc;
   unsigned char sgml_stop_char;

   flags = table->flags;
   p = l->data;

   if (flags & FORTRAN_TYPE)
     {
	if (is_fortran_comment (l, table))
	  return JED_LINE_HAS_EOL_COMMENT;
     }

   sc = table->string_chars;
   syntax = table->char_syntax;
   sgml_stop_char = table->sgml_stop_char;

   lflags = l->flags & JED_LINE_SYNTAX_BITS;

   if (lflags)
     {
	if (lflags & JED_LINE_IN_COMMENT)
	  {
	     if (NULL == (p = find_comment_end (table, p, pmax)))
	       return JED_LINE_IN_COMMENT;
	  }
	else if (lflags & JED_LINE_IN_STRING0)
	  {
	     if (NULL == (p = find_string_end (table, p, pmax, sc[0])))
	       return JED_LINE_IN_STRING0;
	  }
	else if (lflags & JED_LINE_IN_STRING1)
	  {
	     if (NULL == (p = find_string_end (table, p, pmax, sc[1])))
	       return JED_LINE_IN_STRING1;
	  }
	else if (lflags & JED_LINE_IN_HTML)
	  {
	     if (NULL == (p = find_string_end (table, p, pmax, sgml_stop_char)))
	       return JED_LINE_IN_HTML;
	  }
     }

   /* Now we should be out of comments, strings, etc... */
   quote = table->quote_char;

   comment_start = table->comment_start;

   com_start_char = (comment_start == NULL) ? 0 : *comment_start;

   while (p < pmax)
     {
	ch = *p;
	if (ch == quote)
	  {
	     p += 2;
	     continue;
	  }

	/* Search for a string delimiter, char delimiter, or comment start. */
#if 1
	/* Check comment syntax first */
	if (syntax[ch] & COMMENT_SYNTAX)
	  {
	     if ((ch == com_start_char)
		 && (comment_start != NULL)
		 && (p + table->comment_start_len <= pmax)
		 && (0 == strncmp ((char *) p, comment_start,
				   table->comment_start_len)))
	       {
		  p = find_comment_end (table, p + table->comment_start_len, pmax);
		  if (p == NULL)
		    return JED_LINE_IN_COMMENT;

		  continue;
	       }

	     if (_jed_is_eol_comment_start (table, l, p, pmax, NULL))
	       return JED_LINE_HAS_EOL_COMMENT;
	  }
#endif
	if (syntax[ch] & (STRING_SYNTAX))
	  {
	     p = find_string_end (table, p + 1, pmax, ch);
	     if (p == NULL)
	       {
		  if (sc[0] == ch)
		    return JED_LINE_IN_STRING0;
		  if (sc[1] == ch)
		    return JED_LINE_IN_STRING1;
		  return JED_LINE_IN_STRING0;
	       }
	     continue;
	  }

	if (syntax[ch] & HTML_START_SYNTAX)
	  {
	     p = find_string_end (table, p+1, pmax, table->sgml_stop_char);
	     if (p == NULL)
	       return JED_LINE_IN_HTML;
	     continue;
	  }
#if 0
	if (0 == (syntax[ch] & COMMENT_SYNTAX))
	  {
	     p++;
	     continue;
	  }
	if ((ch == com_start_char)
	    && (comment_start != NULL)
	    && (p + table->comment_start_len <= pmax)
	    && (0 == strncmp ((char *) p, comment_start,
			      table->comment_start_len)))
	  {
	     p = find_comment_end (table, p + table->comment_start_len, pmax);
	     if (p == NULL)
	       return JED_LINE_IN_COMMENT;

	     continue;
	  }

	if (_jed_is_eol_comment_start (table, l, p, pmax, NULL))
	  return JED_LINE_HAS_EOL_COMMENT;
#endif

	p++;
     }

   return 0;
}

static void goto_effective_eol (Syntax_Table_Type *table) /*{{{*/
{
   unsigned int flags;
   unsigned char *p, *pmax;
   unsigned short *syntax;

   flags = CLine->flags;
   if (0 == (flags & JED_LINE_HAS_EOL_COMMENT))
     {
	eol ();
	return;
     }

   if (table->flags & FORTRAN_TYPE)
     {
	bol ();

	if (is_fortran_comment (CLine, table))
	  return;
     }

   if (0 == table->num_eol_comments)
     {
	/* ?? */
	CLine->flags &= ~JED_LINE_HAS_EOL_COMMENT;
	eol ();
	return;
     }

   /* We know that there is an EOL style comment somewhere on this line.  So,
    * just search for it.
    */
   p = CLine->data;
   pmax = CLine->data + CLine->len;

   syntax = table->char_syntax;

   while (p < pmax)
     {
	unsigned int i;

	if ((syntax[*p] & COMMENT_SYNTAX)
	    && (_jed_is_eol_comment_start (table, CLine, p, pmax, &i)))
	  {
	     /* Is this really a comment?  Perhaps is occurs in a string
	      * or in part of another word. Only one way to find out.
	      */
	     jed_position_point (p + table->eol_comment_lens[i]);
	     if (JED_LINE_HAS_EOL_COMMENT == parse_to_point1 (table, CLine,
							      p + table->eol_comment_lens[i]))
	       {
		  jed_position_point (p);
		  return;		       /* we are there */
	       }
	  }
	p++;
     }

   /* ?? */
   CLine->flags &= ~JED_LINE_HAS_EOL_COMMENT;
   eol ();
}

/*}}}*/

static Syntax_Table_Type *setup_for_match (unsigned char **pp,
					   unsigned char *chp,
					   unsigned char *want_chp)
{
   register unsigned char ch, want_ch;
   Syntax_Table_Type *table;

#if JED_HAS_LINE_ATTRIBUTES
   /* Make sure that the line flags are ok. */
   if (CBuf->min_unparsed_line_num)
     jed_syntax_parse_buffer (0);
#endif

   *pp = CLine->data + Point;

   ch = *chp;
   if (ch == 0) ch = **pp;

   table = CBuf->syntax_table;

   want_ch = 0;
   if (table != NULL)
     want_ch = table->matching_delim [ch];

   if (want_ch == 0)
     {
	table = Default_Syntax_Table;
	if (table != NULL)
	  want_ch = table->matching_delim [ch];
     }

   if (want_ch == 0)
     return NULL;

   *chp = ch;
   *want_chp = want_ch;

   return table;
}

static int is_quoted (unsigned char *pmin, unsigned char **pp, unsigned char q)
{
   unsigned char *p;
   int iq;

   iq = 0;
   p = *pp;

   while ((p >= pmin) && (*p == q))
     {
	p--;
	iq = !iq;
     }

   if (iq)
     *pp = p;

   return iq;
}

static int goto_comment_begin (Syntax_Table_Type *table)
{
   unsigned char *p, *pmin, *pmax;
   unsigned char ch;
   char *s;
   unsigned int len;

   if (NULL == (s = table->comment_start))
     return -1;

   len = table->comment_start_len;
   ch = *s;

   while (1)
     {
	pmin = CLine->data;
	p = pmin + Point;
	pmax = pmin + CLine->len;

	while (p >= pmin)
	  {
	     if ((*p == ch)
		 && (p + len <= pmax)
		 && (0 == strncmp (s, (char *)p, len))
		 && (0 == parse_to_point1 (table, CLine, p)))
	       {
		  jed_position_point (p);
		  return 0;
	       }

	     p--;
	  }

	if (0 == jed_up (1))
	  {
	     bol ();
	     return -1;
	  }
     }
}

static int goto_comment_end (Syntax_Table_Type *table)
{
   unsigned char *p, *pmax;
   unsigned char ch;
   char *s;
   unsigned int len;

   if (NULL == (s = table->comment_stop))
     return -1;

   len = table->comment_stop_len;
   ch = *s;

   while (1)
     {
	p = CLine->data + Point;
	pmax = CLine->data + CLine->len;

	while (p < pmax)
	  {
	     if ((*p == ch)
		 && (p + len <= pmax)
		 && (0 == strncmp (s, (char *)p, len)))
	       {
		  jed_position_point (p + len);
		  return 0;
	       }
	     p++;
	  }

	if (0 == jed_down (1))
	  {
	     eol ();
	     return -1;
	  }
     }
}

/* Note that this routine may be called with p corresponding to Point = -1.
 * This will happen when the end quote character is at the beginning of a
 * line.
 */
static int goto_string_begin (Syntax_Table_Type *table,
			      unsigned char ch, unsigned char *p)
{
   unsigned char *pmin;
   unsigned char quote;

   quote = table->quote_char;
   pmin = CLine->data;

   while (1)
     {
	while (p >= pmin)
	  {
	     if (*p-- == ch)
	       {
		  if ((p >= pmin)
		      && (*p == quote)
		      && (is_quoted (pmin, &p, quote)))
		    continue;

		  jed_position_point (p + 1);
		  return 0;
	       }
	  }

	if ((table->flags & SINGLE_LINE_STRINGS)
	    || (0 == jed_up (1)))
	  {
	     bol ();
	     return -1;
	  }
	pmin = CLine->data;
	p = pmin + Point;
     }
}

static int goto_string_end (Syntax_Table_Type *table, unsigned char ch)
{
   unsigned char *p, *pmax;
   unsigned char quote;

   quote = table->quote_char;
   while (1)
     {
	p = CLine->data + Point;
	pmax = CLine->data + CLine->len;
	while (p < pmax)
	  {
	     if (*p == ch)
	       {
		  jed_position_point (p + 1);  /* go around it */
		  return 0;
	       }
	     if (*p == quote) p++;
	     p++;
	  }

	if ((table->flags & SINGLE_LINE_STRINGS)
	    || (0 == jed_down (1)))
	  {
	     eol ();
	     return -1;
	  }
     }
}

/* Go backward looking for the matching ch--- not the char that matches ch.
 * Rather, ch is the matching character.
 * This routine returns:
 *   1 if found and leaves the point on the match
 *  -2 if not found but we appear to be in a comment.  In this case, the point
 *     if left at the beginning of the comment
 *  -1 Not found but we appear to be in a string.  This leaves the point at the
 *     beginning of the string.
 *   0 if not found.  The point is left where we gave up
 *   2 if went back too far
 * count is the number of lines to go back
 */

static int backward_goto_match (int count, unsigned char ch) /*{{{*/
{
   unsigned char *p, *pmin, want_ch;
   unsigned short *syntax;
   int in_string, in_comment, in_html, level;
   int quote;
   Syntax_Table_Type *table;
   unsigned int this_syntax;
   unsigned char *pmax;
   unsigned char com_start_char, com_stop_char;
   unsigned char sgml_start_char, sgml_stop_char;

   if (NULL == (table = setup_for_match (&p, &ch, &want_ch)))
     return 0;

   syntax = table->char_syntax;
   quote = table->quote_char;

   level = 1;

   /* Get some context */
   in_string = 0; in_comment = 0;
   switch (parse_to_point1 (table, CLine, CLine->data + Point))
     {
      case JED_LINE_HAS_EOL_COMMENT:
      case JED_LINE_IN_COMMENT:
	in_comment = 1;
	break;

      case JED_LINE_IN_STRING0:
	in_string = (int) table->string_chars[0];
	break;
      case JED_LINE_IN_STRING1:
	in_string = (int) table->string_chars[1];
	break;
      case JED_LINE_IN_HTML:
	in_html = 1;
	break;
     }
   Point--;

   if (table->comment_start != NULL)
     com_start_char = (unsigned char) table->comment_start[0];
   else
     com_start_char = 0;

   if (table->comment_stop != NULL)
     com_stop_char = (unsigned char) table->comment_stop[0];
   else com_stop_char = 0;

   sgml_start_char = table->sgml_start_char;
   sgml_stop_char = table->sgml_stop_char;

   /* FIXME: handle sgml_start/stop_char */
   (void) sgml_start_char; (void) sgml_stop_char; (void) in_html;

   pmax = CLine->data + CLine->len;
   while (count)
     {
	pmin = CLine->data;
	p = pmin + Point;

	/* This loop here is where it all happens. In this loop, we are
	 * either in a string, a comment, or in code.  If we are in a string
	 * or comment, then that is where we started.  So, upon hitting the
	 * left boundary of the string or comment, quit parsing and return.
	 * If in code, then skip all comments and strings because the match
	 * must be in code.
	 */
	while (p >= pmin)
	  {
	     ch = *p--;

	     if ((syntax[ch] & SYNTAX_MASK) == 0) continue;

	     /* Check to see if it is quoted. */
	     if ((p >= pmin) && (*p == quote))
	       {
		  if (is_quoted (pmin, &p, quote))
		    continue;
	       }

	     this_syntax = syntax[ch];

	     /* Check for strings and characters since those occur quite
	      * frequently.
	      */
	     if (this_syntax & STRING_SYNTAX)
	       {
		  /* a string is meaningless in a comment */
		  if (in_comment) continue;
		  if (in_string)
		    {
		       if (in_string == (int) ch)
			 {
			    /* Match not found.  Leave point at beginning
			     * of string and return -1.
			     */
			    /* Some syntaxes permit double quotes in a string
			     * to represent a single quote.  Allow that here.
			     */
			    if ((p < pmin) || ((int)*p != in_string))
			      {
				 jed_position_point (p + 1);
				 return -1;
			      }
			    p--;
			 }
		       continue;
		    }

		  jed_position_point (p);

		  if (-1 == goto_string_begin (table, ch, p))
		    {
		       /* Can't find it.  This should not happen except in the
			* case of a run-away character.
			*/
		       return 0;
		    }

		  pmin = CLine->data;
		  pmax = pmin + CLine->len;
		  p = pmin + (Point - 1);
		  continue;
	       }

	     /* Before checking for comments, check for matching
	      * delimiters to give those a priority.
	      */
	     if (this_syntax & OPEN_DELIM_SYNTAX)
	       {
		  if (level == 1)
		    {
		       jed_position_point (p+1);
		       if (ch == want_ch) return 1;
		       return 0;
		    }
		  level--;
		  /* Drop down through because the open delimiter may also
		   * be a comment character as in HTML and PASCAL.
		   */
	       }

	     if (this_syntax & CLOSE_DELIM_SYNTAX)
	       {
		  level++;
		  /* Drop */
	       }

	     /* The last thing to check is a comment. */
	     if ((0 == (this_syntax & COMMENT_SYNTAX))
		 || (in_string))
	       continue;

	     if (in_comment)
	       {
		  /* Just look for the character that denotes the start
		   * of the comment.  In C++ this is complicated by something
		   * like "*(no space)//".  If you use such a construct then
		   * you lose.
		   */
		  p++;
		  if (_jed_is_eol_comment_start (table, CLine, p, pmax, NULL))
		    {
		       jed_position_point (p);
		       return -2;
		    }

		  if ((ch == com_start_char)
		      && (table->comment_start != NULL)
		      && (p + table->comment_start_len <= pmax)
		      && (0 == strncmp ((char *) p, table->comment_start, table->comment_start_len)))
		    {
		       jed_position_point (p);
		       return -2;
		    }
		  p--;
		  continue;
	       }

	     /* We are not in a comment but we may be moving into the tail
	      * end of one.  I am not going to consider eol type comments
	      * here because if this is coded correctly and the algorithm
	      * works, then that was already handled.
	      */
	     if (ch != com_stop_char)
	       continue;

	     p++;
	     if ((table->comment_stop == NULL)
		 || (p + table->comment_stop_len > pmax)
		 || (0 != strncmp ((char *) p, table->comment_stop, table->comment_stop_len)))
	       {
		  p--;
		  continue;
	       }

	     jed_position_point (p);
	     if (-1 == goto_comment_begin (table))
	       return 0;

	     pmin = CLine->data;
	     pmax = pmin + CLine->len;
	     p = pmin + (Point - 1);
	  }

	if (0 == jed_up (1))
	  {
	     bol ();
	     break;
	  }
	count--;
	if (table->flags & SINGLE_LINE_STRINGS)
	  in_string = 0;
	goto_effective_eol (table);
	pmax = CLine->data + Point;
     }

   /* What have we learned? */

   if (Point < 0) Point = 0;
   if (count == 0)
     {
	/* In this case, we went back as far as permitted.  Nothing much can be
	 * said.
	 */
	bol ();
	return 2;
     }

   if (in_string) return -1;
   if (in_comment) return -2;

   /* If we are here, then we have a mismatch */
   return 0;
}

/*}}}*/

static int forward_goto_match (unsigned char ch) /*{{{*/
{
   unsigned char *p, *pmax, want_ch;
   unsigned short *syntax;
   int in_string, in_comment, level;
   unsigned int this_syntax;
   Syntax_Table_Type *table;
   unsigned char com_start_char;

   if (NULL == (table = setup_for_match (&p, &ch, &want_ch)))
     return 0;

   syntax = table->char_syntax;

   /* Here we go */

   in_string = 0; in_comment = 0;
   switch (parse_to_point1 (table, CLine, CLine->data + Point))
     {
      case JED_LINE_HAS_EOL_COMMENT:
	in_comment = JED_LINE_HAS_EOL_COMMENT;
	break;

      case JED_LINE_IN_COMMENT:
	in_comment = JED_LINE_IN_COMMENT;
	break;

      case JED_LINE_IN_STRING0:
	in_string = (int) table->string_chars[0];
	break;
      case JED_LINE_IN_STRING1:
	in_string = (int) table->string_chars[1];
	break;
     }

   com_start_char = 0;

   if (table->comment_start != NULL)
     com_start_char = (unsigned char) table->comment_start[0];

   level = 1;

   Point++;
   while (1)
     {
	p = CLine->data + Point;
	pmax = CLine->data + CLine->len;

	while (p < pmax)
	  {
	     ch = *p++;

	     if ((syntax[ch] & SYNTAX_MASK) == 0) continue;

	     this_syntax = syntax[ch];

	     if (this_syntax & COMMENT_SYNTAX)
	       {
		  if (in_string) continue;

		  if (in_comment == JED_LINE_IN_COMMENT)
		    {
		       p--;
		       if ((table->comment_stop != NULL)
			   && (p + table->comment_stop_len <= pmax)
			   && (0 == strncmp ((char *) p, table->comment_stop, table->comment_start_len)))
			 {
			    p += table->comment_stop_len;
			    in_comment = 0;
			    continue;
			 }
		       p++;
		       continue;
		    }

		  /* We may have run into a comment.  If so, go around */
		  if (in_comment == 0)
		    {
		       p--;
		       if ((ch == com_start_char)
			   && (table->comment_start != NULL)
			   && (p + table->comment_start_len <= pmax)
			   && (0 == strncmp ((char *) p, table->comment_start, table->comment_start_len)))
			 {
			    jed_position_point (p);
			    if (-1 == goto_comment_end (table))
			      return 0;

			    p = CLine->data + Point;
			    pmax = CLine->data + CLine->len;
			    continue;
			 }

		       if (_jed_is_eol_comment_start (table, CLine, p, pmax, NULL))
			 {
			    p = pmax;
			    continue;
			 }
		       p++;
		    }
		  /* drop */
	       }

	     if (this_syntax & STRING_SYNTAX)
	       {
		  /* string/char */
		  if (in_comment) continue;
		  if (in_string)
		    {
		       if ((int) ch == in_string)
			 {
			    /* Some syntaxes permit double quotes in a string
			     * to represent a single quote.  Allow that here.
			     */
			    if ((p >= pmax) || ((int)*p != in_string))
			      in_string = 0;
			    else
			      p++;     /* skip second quote */
			 }
		    }
		  else
		    {
		       jed_position_point (p);
		       if (-1 == goto_string_end (table, ch))
			 return -1;

		       p = CLine->data + Point;
		       pmax = CLine->data + CLine->len;
		    }
		  continue;
	       }

	     if (this_syntax & OPEN_DELIM_SYNTAX)
	       {
		  level++;
		  continue;
	       }

	     if (this_syntax & CLOSE_DELIM_SYNTAX)
	       {
		  if (level == 1)
		    {
		       jed_position_point (p-1);
		       if (ch == want_ch) return 1;
		       return 0;
		    }
		  level--;
		  continue;
	       }

	     if (this_syntax & QUOTE_SYNTAX) p++; /* skip next char */
	  }
	/* END OF MAIN LOOP: while (p < pmax) */

	if (in_comment == JED_LINE_HAS_EOL_COMMENT)
	  return -2;

	/* Move to the next line. */
	while (1)
	  {
	     if (0 == jed_down (1))
	       {
		  eol ();
		  if (in_string) return -1;
		  if (in_comment) return -2;

		  /* If we are here, then we have a mismatch */
		  return 0;
	       }

	     if (table->flags & SINGLE_LINE_STRINGS)
	       in_string = 0;

	     if ((0 == (table->flags & FORTRAN_TYPE))
		 || (0 == is_fortran_comment (CLine, table)))
	       break;
	  }
     }
}

/*}}}*/

static int find_matching_delimiter_1 (unsigned char ch, int nlines)
{
   unsigned char want_ch;
   Syntax_Table_Type *table;
   unsigned char *p;

   table = setup_for_match (&p, &ch, &want_ch);
   if (table == NULL)
     return 0;

   if (table->char_syntax[ch] & OPEN_DELIM_SYNTAX)
     return forward_goto_match (ch);
   else
     return backward_goto_match (nlines, ch);
}

static int find_matching_delimiter (int *ch)
{
   return find_matching_delimiter_1 ((unsigned char) *ch, 5000);
}

int goto_match (void) /*{{{*/
{
   if (1 != find_matching_delimiter_1 (0, LineNum))
     {
	if (!IN_MINI_WINDOW) msg_error("Mismatch!!");
	return 0;
     }
   return 1;
}

/*}}}*/

static int parse_to_point (void) /*{{{*/
{
   Syntax_Table_Type *table = CBuf->syntax_table;
   if (table == NULL) return 0;
#if JED_HAS_LINE_ATTRIBUTES
   jed_syntax_parse_buffer (0);
#endif

   switch (parse_to_point1 (table, CLine, CLine->data + Point))
     {
      case JED_LINE_IN_COMMENT:
      case JED_LINE_HAS_EOL_COMMENT:
	return -2;
      case JED_LINE_IN_STRING0:
      case JED_LINE_IN_STRING1:
	return -1;
     }

   return 0;
}

/*}}}*/

/* blink the matching fence.  This assumes that the window is ok */
void blink_match (void) /*{{{*/
{
   Line *save;
   int pnt, code, matchp;
   unsigned int l;
   char buf[600], strbuf[256];

   if (!Blink_Flag || (Repeat_Factor != NULL) || Batch) return;
   if (JWindow->trashed) update((Line *) NULL, 0, 0, 0);
   if (JWindow->trashed) return;
   pnt = Point;
   save = CLine;
   l = LineNum;

   if (Point) Point--;
   code = backward_goto_match (1000, 0);

   if (code == 0)
     {
	if ((! (CBuf->modes == WRAP_MODE)) && (!IN_MINI_WINDOW))
	  message("Mismatch??");
     }
   else if ((code == 1) && is_line_visible (LineNum))
     {
	point_cursor(0);
	input_pending(&Number_Ten);
	Point = pnt;
	CLine = save;
	LineNum = l;
	point_cursor(0);
	return;
     }
   else if (code == 1)
     {
	unsigned int len;

	matchp = Point;
	bol ();
	strcpy(buf, "Matches ");
	(void) jed_skip_whitespace();
	if ((matchp == Point) && jed_up(1))
	  {
	     bol ();
	     safe_strcat (buf,
			  make_line_string(strbuf, sizeof(strbuf)),
			  sizeof (buf));
	     jed_down(1);
	  }

	safe_strcat(buf,
		    make_line_string(strbuf, sizeof (strbuf)),
		    sizeof(buf));

	/* Apparantly there are some who think that it is a bug to see
	 * ^J in the mini-buffer.  Sigh.
	 */
	len = strlen (buf);
	if (len && (buf[len - 1] == '\n'))
	  buf[len - 1] = 0;

	message(buf);
     }
   Point = pnt;
   CLine = save;
   LineNum = l;
}

/*}}}*/

Syntax_Table_Type *jed_find_syntax_table (char *name, int err) /*{{{*/
{
   Syntax_Table_Type *table = Syntax_Tables;
   while (table != NULL)
     {
	if (!strcmp (table->name, name)) return table;
	table = table->next;
     }
   if (err) msg_error ("Syntax table undefined.");
   return table;
}

/*}}}*/

static void set_syntax_flags (char *name, int *flags) /*{{{*/
{
   Syntax_Table_Type *table;

   table = jed_find_syntax_table (name, 1);
   if (table == NULL) return;

   table->flags |= *flags & 0xFF;
}

/*}}}*/

static int get_syntax_flags (char *name) /*{{{*/
{
   Syntax_Table_Type *table;

   table = jed_find_syntax_table (name, 1);
   if (table == NULL) return -1;

   return table->flags & 0xFF;
}

/*}}}*/

/* UTF-8 WORD UNsafe */
static void define_syntax (int *what, char *name) /*{{{*/
{
   Syntax_Table_Type *table;
   int c2;
   unsigned int i;
   char *s1 = NULL, *s2 = NULL;
   unsigned char lut[256], *s;

   table = jed_find_syntax_table (name, 1);
   if (table == NULL) return;

   switch (*what)
     {
      case '%':
	if (SLang_pop_slstring (&s2)) break;
	if (SLang_pop_slstring (&s1)) break;

	table->char_syntax[(unsigned char) *s1] |= COMMENT_SYNTAX;

	if ((*s2 == 0) || (*s2 == '\n'))
	  {
	     SLang_free_slstring (s2);

	     i = table->num_eol_comments;
	     if (i == MAX_EOL_COMMENTS)
	       {
		  SLang_free_slstring (s1);
		  return;
	       }

	     table->eol_comment_starts[i] = s1;
	     table->eol_comment_lens[i] = strlen (s1);
	     table->num_eol_comments = i + 1;
	     return;
	  }

	table->char_syntax[(unsigned char) *s2] |= COMMENT_SYNTAX;

	SLang_free_slstring (table->comment_start);
	SLang_free_slstring (table->comment_stop);
	table->comment_start = s1;
	table->comment_stop = s2;
	table->comment_start_len = strlen (s1);
	table->comment_stop_len = strlen (s2);
	return;

      case '\\':
	if (SLang_pop_integer (&c2)) break;
	table->char_syntax[(unsigned char) c2] |= QUOTE_SYNTAX;
	table->quote_char = (unsigned char) c2;
	break;

      case '#':
	if (SLang_pop_integer (&c2)) break;
	table->preprocess = (unsigned char) c2;
	break;

      case '\'':
	if (SLang_pop_integer (&c2)) break;
	table->char_syntax[(unsigned char) c2] |= STRING_SYNTAX;
	table->char_char = (unsigned char) c2;
	break;
      case '"':
	if (SLang_pop_integer (&c2)) break;
	if (table->num_string_chars == MAX_STRING_CHARS)
	  break;
	table->char_syntax[(unsigned char) c2] |= STRING_SYNTAX;
	table->string_chars[table->num_string_chars++] = (unsigned char) c2;
	break;

      case '<':
      case '>':
	if (SLang_pop_slstring (&s1)) break;
	s2 = s1;
	while (*s2 != 0)
	  {
	     if (*(s2 + 1) == 0) break;
	     table->char_syntax[(unsigned char) *s2] |= HTML_START_SYNTAX;
	     table->char_syntax[(unsigned char) *(s2 + 1)] |= HTML_END_SYNTAX;
	     s2 += 2;
	  }
	table->sgml_start_char = *s1;
	if (*s1 != 0)
	  table->sgml_stop_char = s1[1];
	else
	  table->sgml_stop_char = 0;
	s2 = NULL;
	break;

      case '(':
      case ')':
	if (SLang_pop_slstring (&s2)) break;
	if (SLang_pop_slstring (&s1)) break;

	i = strlen (s1);
	if (i != strlen (s2))
	  {
	     msg_error ("Delimiter set does not match.");
	  }
	while (i > 0)
	  {
	     unsigned char ch1, ch2;
	     i--;
	     ch1 = (unsigned char) s1[i]; ch2 = (unsigned char) s2[i];
	     table->char_syntax[ch1] |= OPEN_DELIM_SYNTAX;
	     table->char_syntax[ch2] |= CLOSE_DELIM_SYNTAX;
	     table->matching_delim[ch2] = ch1;
	     table->matching_delim[ch1] = ch2;
	  }
	break;

      case '+':
	if (SLang_pop_slstring (&s1)) break;
	for (i = 0; i < 256; i++) table->char_syntax[i] &= ~OP_SYNTAX;

	s = (unsigned char *) s1;
	while (*s)
	  {
	     table->char_syntax[*s] |= OP_SYNTAX;
	     s++;
	  }
	break;

      case '0':
	if (SLang_pop_slstring (&s1)) break;
	SLmake_lut (lut, (unsigned char *) s1, 0);

	for (i = 0; i < 256; i++)
	  {
	     if (lut[i]) table->char_syntax[i] |= NUMBER_SYNTAX;
	     else table->char_syntax[i] &= ~NUMBER_SYNTAX;
	  }
	break;

      case ',':
	if (SLang_pop_slstring (&s1)) break;
	s = (unsigned char *) s1;
	for (i = 0; i < 256; i++) table->char_syntax[i] &= ~DELIM_SYNTAX;
	while (*s)
	  {
	     table->char_syntax[*s] |= DELIM_SYNTAX;
	     s++;
	  }
	break;

      case 'w':
	if (SLang_pop_slstring (&s1)) break;
	SLmake_lut (lut, (unsigned char *) s1, 0);

	for (i = 0; i < 256; i++)
	  {
	     if (lut[i]) table->char_syntax[i] |= WORD_SYNTAX;
	     else table->char_syntax[i] &= ~WORD_SYNTAX;
	  }
	break;

      default:
	msg_error ("Bad parameter to define_syntax");
     }

   if (s1 != NULL) SLang_free_slstring (s1);
   if (s2 != NULL) SLang_free_slstring (s2);
}

/*}}}*/

static void set_fortran_comment_style (char *table_name, char *str)
{
   Syntax_Table_Type *table;
   int reverse;

   if (NULL == (table = jed_find_syntax_table (table_name, 1)))
     return;

   reverse = 0;
   if ((*str == '^') && (str[1] != 0))
     {
	str++;
	reverse = 1;
     }
   SLmake_lut(table->fortran_comment_chars, (unsigned char *) str, reverse);
}

static void use_syntax_table (char *s) /*{{{*/
{
   Syntax_Table_Type *table;

   if ((s == NULL) || (*s == 0))
     {
	s = "";
	table = Default_Syntax_Table;
     }
   else
     {
	table = jed_find_syntax_table (s, 1);
	if (table == NULL) return;
     }
   CBuf->syntax_table = table;

   (void) SLang_run_hooks ("use_syntax_table_hook", 1, s);
}

/*}}}*/

/* Clears everything except for the name, and links to other tables */
static void clear_syntax_table (Syntax_Table_Type *t)
{
   unsigned int i;
   char *name;
   Syntax_Table_Type *next;

   for (i = 0; i < t->num_eol_comments; i++)
     SLang_free_slstring (t->eol_comment_starts[i]);
   SLang_free_slstring (t->comment_start);
   SLang_free_slstring (t->comment_stop);

#if JED_HAS_DFA_SYNTAX
   if (t->init_dfa_callback != NULL)
     SLang_free_function (t->init_dfa_callback);
   if (t->hilite != NULL)
     jed_dfa_free_highlight_table (t->hilite);
#endif

   for (i = 0; i < MAX_KEYWORD_TABLES; i++)
     {
	unsigned int j;
	for (j = 0; j < MAX_KEYWORD_LEN; j++)
	  {
	     char *kwds = t->keywords[i][j];
	     if (kwds != NULL)
	       SLang_free_slstring (kwds);
	  }
     }

   next = t->next;
   name = t->name;
   memset ((char *)t, 0, sizeof (Syntax_Table_Type));
   t->next = next;
   t->name = name;
}

static Syntax_Table_Type *allocate_syntax_table (char *name)
{
   Syntax_Table_Type *table;

   if (NULL == (table = (Syntax_Table_Type *) jed_malloc0 (sizeof (Syntax_Table_Type))))
     return NULL;

   if (NULL == (name = SLang_create_slstring (name)))
     {
	SLfree ((char *) table);
	return NULL;
     }
   table->name = name;
   return table;
}

static void create_syntax_table (char *name) /*{{{*/
{
   Syntax_Table_Type *table;

   if (NULL != (table = jed_find_syntax_table (name, 0)))
     {
	clear_syntax_table (table);
	return;
     }

   if (NULL == (table = allocate_syntax_table (name)))
     return;

   table->next = Syntax_Tables;
   Syntax_Tables = table;
}

void init_syntax_tables (void) /*{{{*/
{
   unsigned short *a;
   unsigned char *m;

   Default_Syntax_Table = allocate_syntax_table ("DEFAULT");
   if (Default_Syntax_Table == NULL) return;

   a = Default_Syntax_Table->char_syntax;
   m = Default_Syntax_Table->matching_delim;

   a [(unsigned char) '['] = OPEN_DELIM_SYNTAX; m[(unsigned char) '['] = ']';
   a [(unsigned char) ']'] = CLOSE_DELIM_SYNTAX; m[(unsigned char) ']'] = '[';
   a [(unsigned char) '('] = OPEN_DELIM_SYNTAX; m[(unsigned char) '('] = ')';
   a [(unsigned char) ')'] = CLOSE_DELIM_SYNTAX; m[(unsigned char) ')'] = '(';
   a [(unsigned char) '{'] = OPEN_DELIM_SYNTAX; m[(unsigned char) '{'] = '}';
   a [(unsigned char) '}'] = CLOSE_DELIM_SYNTAX; m[(unsigned char) '}'] = '{';
}

/*}}}*/

/* Currently this assumes byte-semantics.  It should be changed to assume character
 * semantics.
 */
static void define_keywords (char *name, char *kwords, int *lenp, int *tbl_nump) /*{{{*/
{
   char *kw;
   int len;
   int kwlen;
   unsigned int table_number = (unsigned int) *tbl_nump;
   Syntax_Table_Type *table = jed_find_syntax_table (name, 1);

   if (table == NULL) return;

   if (table_number >= MAX_KEYWORD_TABLES)
     {
	msg_error ("Table number too high.");
	return;
     }

   len = *lenp;

   if ((len < 1) || (len > MAX_KEYWORD_LEN))
     {
	msg_error ("Keyword length not supported.");
	return;
     }

   kwlen = strlen (kwords);
   if (kwlen % len)
     {
	msg_error ("Keyword list is improperly formed.");
	return;
     }

   len--;
   kw = table->keywords[table_number][len];
   if (kw == NULL) SLang_push_string ("");
   else
     {
	SLang_push_string (kw);
	SLang_free_slstring (kw);
     }

   table->keywords[table_number][len] = SLang_create_slstring (kwords);
}

/*}}}*/

#if JED_HAS_LINE_ATTRIBUTES
static void syntax_parse_lines (Syntax_Table_Type *table, Line *l, unsigned int num)
{
   int state;

   if (l == NULL)
     return;

   if (l->prev != NULL)
     {
	l = l->prev;
	num++;
     }

   while (1)
     {
	state = parse_to_point1 (table, l, l->data + l->len);
	if (state == JED_LINE_HAS_EOL_COMMENT)
	  {
	     l->flags |= state;
	     state = 0;
	  }
	else if (((state == JED_LINE_IN_STRING0) || (state == JED_LINE_IN_STRING1))
	    && (table->flags & SINGLE_LINE_STRINGS))
	  state = 0;

	l = l->next;

	if (l == NULL)
	  break;

	if (num == 0)
	  {
	     if (state == (int) (l->flags & JED_LINE_SYNTAX_IN_BITS))
	       return;
	  }
	else num--;

	l->flags &= ~JED_LINE_SYNTAX_BITS;
	l->flags |= state;
     }
}

void jed_syntax_parse_buffer (int do_all)
{
   unsigned int min_line_num;
   unsigned int max_line_num;
   int n;
   int is_narrow;
   Line *l;
   Syntax_Table_Type *table;
   unsigned int undo_bit = 0;
   SLang_Name_Type *color_region_hook = NULL;

   if (CBuf->buffer_hooks != NULL)
     color_region_hook = CBuf->buffer_hooks->color_region_hook;

   table = CBuf->syntax_table;
   if ((table == NULL) && (color_region_hook == NULL))
     {
	CBuf->min_unparsed_line_num = 0;
	CBuf->max_unparsed_line_num = 0;
	return;
     }

   min_line_num = CBuf->min_unparsed_line_num;
   max_line_num = CBuf->max_unparsed_line_num;

   if (min_line_num == 0)
     {
	if (do_all)
	  return;
     }

   is_narrow = (CBuf->narrow != NULL);
   if (is_narrow)
     {
	/* yuk--- a major hack: turn off undo temporally */
	undo_bit = CBuf->flags & UNDO_ENABLED;
	CBuf->flags &= ~UNDO_ENABLED;

	jed_push_narrow ();
	jed_widen_whole_buffer (CBuf);
     }

   CBuf->beg->flags &= ~JED_LINE_SYNTAX_BITS; /* 0.99-17.98 */

   if (do_all)
     {
	min_line_num = 1;
	max_line_num = Max_LineNum;
     }

   if (color_region_hook != NULL)
     {
	(void) SLang_start_arg_list ();
	if ((0 == SLang_push_integer ((int) min_line_num))
	    && (0 == SLang_push_integer ((int) max_line_num))
	    && (0 == SLang_end_arg_list ()))
	  {
	     if (-1 == SLexecute_function (color_region_hook))
	       CBuf->buffer_hooks->color_region_hook = NULL;
	  }
     }
   else
     {
	n = (int) min_line_num;

	push_spot ();
	goto_line (&n);
	l = CLine;
	pop_spot ();

	syntax_parse_lines (table, l, 1 + (unsigned int) (max_line_num - min_line_num));
     }

   if (is_narrow)
     {
	jed_pop_narrow ();
	CBuf->flags |= undo_bit;       /* hack */
     }

   CBuf->min_unparsed_line_num = CBuf->max_unparsed_line_num = 0;
}
#endif

static char *what_syntax_table (void)
{
   Syntax_Table_Type *s;

   if ((NULL == (s = CBuf->syntax_table))
       && (NULL == (s = Default_Syntax_Table)))
     return NULL;

   return s->name;		       /* not thread safe */
}

static SLang_Intrin_Fun_Type Intrinsics [] =
{
   MAKE_INTRINSIC("parse_to_point", parse_to_point, INT_TYPE, 0),
   MAKE_INTRINSIC_SI("set_syntax_flags", set_syntax_flags, VOID_TYPE),
   MAKE_INTRINSIC_S("get_syntax_flags", get_syntax_flags, INT_TYPE),
   MAKE_INTRINSIC_IS("define_syntax", define_syntax, VOID_TYPE),
   MAKE_INTRINSIC_S("use_syntax_table", use_syntax_table, VOID_TYPE),
   MAKE_INTRINSIC_0("what_syntax_table", what_syntax_table, STRING_TYPE),
   MAKE_INTRINSIC_S("create_syntax_table", create_syntax_table, VOID_TYPE),
   MAKE_INTRINSIC_4("define_keywords_n", define_keywords, VOID_TYPE, STRING_TYPE, STRING_TYPE, INT_TYPE, INT_TYPE),
   MAKE_INTRINSIC_SS("set_fortran_comment_chars", set_fortran_comment_style, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_I("find_matching_delimiter", find_matching_delimiter, INT_TYPE),
   SLANG_END_INTRIN_FUN_TABLE
};

int jed_init_syntax (void)
{
#if JED_HAS_DFA_SYNTAX
   if (-1 == jed_init_dfa_syntax ())
     return -1;
#endif

   return SLadd_intrin_fun_table (Intrinsics, NULL);
}
