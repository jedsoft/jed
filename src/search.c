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
#include "buffer.h"
#include "vfile.h"
#include "search.h"
#include "misc.h"
#include "ins.h"
#include "paste.h"
#include "ledit.h"

/*}}}*/

#if SLANG_VERSION < 20000
static SLsearch_Type Search_Struct;
static SLRegexp_Type Regexp;
#else
static SLRegexp_Type *Regexp;
static unsigned int Regexp_Offset;
#endif

static int search_internal (SLsearch_Type *st, int dir, int n,
                            int key_len) /*{{{*/
{
   unsigned char *beg, *end, *p;
   Line *line;
   unsigned int num = 0;

   line = CLine;
   (void) key_len;

   if (dir == 1)
     {
	beg = line->data + Point;
	end = line->data + line->len;
	do
	  {
#if SLANG_VERSION >= 20000
             p = SLsearch_forward (st, beg, end);
#else
	     p = SLsearch (beg, end, st);
#endif
	     if (p != NULL)
	       {
		  CLine = line;
		  LineNum += num;
		  jed_position_point (p);
                  return 1;
	       }
	     line = line->next; num++;
	     if (line == NULL)
               return 0;
	     beg = line->data;
	     end = line->data + line->len;
	  }
	while (--n);
     }
   else if (dir == -1)
     {
#if SLANG_VERSION < 20000
	end = beg = line->data;
	if (Point)
	  {
	     int tmp = Point - 1 + key_len;
	     if (tmp > line->len) tmp = line->len;
	     end += tmp;
	  }
#else
        beg = line->data;
        end = beg + line->len;
        p = beg + Point;
#endif
	do
	  {
#if SLANG_VERSION < 20000
	     p = SLsearch (beg, end, st);
#else
             p = SLsearch_backward (st, beg, p, end);
#endif

	     if (p != NULL)
	       {
		  CLine = line;
		  LineNum -= num;

		  jed_position_point (p);
		  return 1;
	       }
	     line = line->prev;
	     num++;
	     if (line == NULL) return(0);
	     beg = line->data;
	     end = line->data + line->len;
#if SLANG_VERSION >= 20000
             p = end;
#endif
	  }
	while (--n);
     }
   return(0);
}

/*}}}*/

static void close_search (SLsearch_Type *st)
{
#if SLANG_VERSION >= 20000
   SLsearch_delete (st);
#else
   (void) st;
#endif
}

static SLsearch_Type *open_search (char *str, int dir, int cs, int *key_lenp)
{
#if SLANG_VERSION >= 20000
   unsigned int flags = 0;
   (void) key_lenp;
   (void) dir;
   if (cs == 0) flags |= SLSEARCH_CASELESS;
   if (Jed_UTF8_Mode) flags |= SLSEARCH_UTF8;
   *key_lenp = strlen (str);
   return SLsearch_new ((SLuchar_Type *) str, flags);
#else
   *key_lenp = SLsearch_init (str, dir, cs, &Search_Struct);
   if (*key_lenp <= 0)
     return NULL;
   return &Search_Struct;
#endif
}

int search (char *str, int dir, int n) /*{{{*/
{
   SLsearch_Type *st;
   int key_len=0;
   int status;
   int cs = Buffer_Local.case_search;

   if (NULL == (st = open_search (str, dir, cs, &key_len)))
     return 0;

   status = search_internal (st, dir, n, key_len);

#if SLANG_VERSION >= 20000
   if (status == 1)
     {
        key_len = SLsearch_match_len (st);
        /* We want the actual number of multibyte chars */
        if (Jed_UTF8_Mode)
          {
             unsigned int len;
             unsigned char *p, *pmax;
             p = CLine->data + Point;
             pmax = p + key_len;
             if (pmax >= CLine->data + CLine->len)
               pmax = CLine->data + CLine->len;

             (void) SLutf8_skip_chars (p, pmax, pmax - p, &len, 1);
             key_len = (int) len;
          }
     }

   else
     key_len = 0;
#else
   if (status != 1)
     key_len = 0;
#endif

   close_search (st);
   return key_len;
}
/*}}}*/

int search_forward(char *s) /*{{{*/
{
   return search(s, 1, 0);
}

/*}}}*/

int search_backward(char *s) /*{{{*/
{
   return search(s, -1, 0);
}

/*}}}*/

int forward_search_line(char *s) /*{{{*/
{
   return search(s, 1, 1);
}

/*}}}*/

int backward_search_line(char *s) /*{{{*/
{
   return( search(s, -1, 1));
}

/*}}}*/

int bol_fsearch (char *str) /*{{{*/
{
   Line *tthis;
   int max, cs = Buffer_Local.case_search;
   unsigned int num;
   unsigned char ch;

   if (Point)
     {
	tthis = CLine->next;
	if (tthis == NULL)
	  return 0;
	num = 1;
     }
   else
     {
	tthis = CLine;
	num = 0;
     }

   max = strlen (str);
   if (max == 0)
     goto return_success;

   ch = str[0];

   if (cs != 0)
     {
	while (tthis != NULL)
	  {
	     if ((tthis->len >= max)
		 && (*tthis->data == ch))
	       {
		  int n;
		  unsigned char *s;

		  s = tthis->data;
		  n = 1;
		  while (1)
		    {
		       if (n == max)
			 goto return_success;

		       if (s[n] != str[n])
			 break;
		       n++;
		    }
	       }
	     num++;
	     tthis = tthis->next;
	  }
	return 0;
     }

   /* Here we have the case-insensitive match */
   while (tthis != NULL)
     {
	if (tthis->len >= max)
	  {
	     unsigned char *s = tthis->data;
	     int n = 0;

	     while (UPPER_CASE(s[n]) == UPPER_CASE(str[n]))
	       {
		  n++;
		  if (n == max)
		    goto return_success;
	       }
	  }
	num++;
	tthis = tthis->next;
     }
   return 0;

   return_success:
   LineNum += num;
   CLine = tthis;
   bol ();
   return max;
}

/*}}}*/

int bol_bsearch (char *str) /*{{{*/
{
   Line *tthis;
   int max, n, cs = Buffer_Local.case_search;
   unsigned int num;
   unsigned char ch;

   if (bolp ())
     {
	if (NULL == (tthis = CLine->prev))
	  return 0;
	num = 1;
     }
   else
     {
	tthis = CLine;
	num = 0;
     }

   max = strlen (str);
   if (max == 0)
     {
	LineNum -= num;
	CLine = tthis;
	bol ();
	return 0;
     }

   ch = str[0];
   if (cs == 0) ch = UPPER_CASE(ch);

   while (tthis != NULL)
     {
	unsigned char *s;

	if (tthis->len < max)
	  {
	     num++;
	     tthis = tthis->prev;
	     continue;
	  }
	s = tthis->data;

	if ((*s != ch)
	    && ((cs != 0)
		|| (ch != UPPER_CASE(*s))))
	  {
	     num++;
	     tthis = tthis->prev;
	     continue;
	  }

	if (cs)
	  {
	     for (n = 1; n < max; n++)
	       {
		  if (s[n] != str[n])
		    break;
	       }
	  }
	else
	  {
	     for (n = 1; n < max; n++)
	       {
		  if (UPPER_CASE(s[n]) != UPPER_CASE(str[n]))
		    break;
	       }
	  }

	if (n == max)
	  {
	     CLine = tthis;
	     LineNum -= num;
	     bol ();
	     return max;
	  }

	num++;
	tthis = tthis->prev;
     }
   return 0;
}

/*}}}*/

#if SLANG_VERSION < 20000
static int re_search_dir(unsigned char *pat, int dir) /*{{{*/
{
   int psave, skip, n, epos;
   unsigned char rbuf[512], *match;
   Line *l;

   Regexp.case_sensitive = Buffer_Local.case_search;
   Regexp.buf = rbuf;
   Regexp.pat = pat;
   Regexp.buf_len = 512;
   Regexp.offset = 0;

   if (SLang_regexp_compile(&Regexp))
     {
	msg_error("Unable to compile pattern.");
	return(0);
     }

   if (Regexp.osearch)
     {
	if (Regexp.must_match_bol)
	  {
	     if (dir > 0)
	       {
		  if (0 == bol_fsearch ((char *) pat))
		    return 0;
	       }
	     else if (0 == bol_bsearch ((char *) pat))
	       return 0;
	  }
	else if (!search((char *) pat, dir, 0)) return (0);

	Regexp.beg_matches[0] = Point;
	n = strlen((char *) pat);
	Regexp.end_matches[0] = n;
	return n + 1;
     }

   if (Regexp.must_match_bol)
     {
	if (dir < 0)
	  {
	     if (bolp ())
	       {
		  if (0 == jed_left (1)) return 0;
	       }
	  }
	else if (0 == bolp ())
	  {
	     if (0 == jed_down(1))
	       return 0;
	  }
     }

   if (Regexp.must_match && (0 != Regexp.must_match_str[1])) skip = 0; else skip = 1;

   while (1)
     {
	psave = Point;
	if (!skip)
	  {
	     l = CLine;
	     if (!search((char *) Regexp.must_match_str, dir, 0)) return (0);

	     if (l != CLine)
	       {
		  if (dir < 0) eol(); else bol ();
		  /* if ((dir == -1) && (!Regexp.must_match_bol)) eol(); else Point = 0; */
		  psave = Point;
	       }
	  }

	Point = psave;
	if (dir == 1)
	  {
	     match = SLang_regexp_match(CLine->data + Point, CLine->len - Point, &Regexp);
	     if (match != NULL)
	       {
		  /* adjust offsets */
		  Regexp.offset = Point;
	       }
	  }

	else if (NULL != (match = SLang_regexp_match(CLine->data,
						     Point, /* was CLine->len */
						     &Regexp)))
	  {
	     if (Point && (Regexp.beg_matches[0] >= Point)) match = NULL;
	     else if (Regexp.must_match_bol == 0)
	       {
		  epos = Point - 1;
		  /* found a match on line now find one closest to current point */
		  while (epos >= 0)
		    {
		       match = SLang_regexp_match(CLine->data + epos,
						  Point - epos, /* was: CLine->len - epos, */
						  &Regexp);
		       if (match != NULL)
			 {
			    Regexp.offset = epos;
			    break;
			 }
		       epos--;
		    }
	       }
	  }
	if (match != NULL)
	  {
	     jed_position_point (match);
	     n = Regexp.end_matches[0];
	     return (n + 1);
	  }
	if (dir > 0)
	  {
	     if (0 == jed_down (1))
	       break;
	  }
	else
	  {
	     if (0 == jed_up(1))
	       break;
	  }
     }
   return (0);
}
/*}}}*/
#else
static int re_search_dir(unsigned char *pat, int dir) /*{{{*/
{
   char *match;
   unsigned int flags = 0;
   int must_match_bol;
   int max_point;

   if (Buffer_Local.case_search == 0)
     flags |= SLREGEXP_CASELESS;
#if 0
   if (Jed_UTF8_Mode)
     flags |= SLREGEXP_UTF8;
#endif
   if (Regexp != NULL)
     SLregexp_free (Regexp);

   if (NULL == (Regexp = SLregexp_compile ((char *)pat, flags)))
     return 0;

   (void) SLregexp_get_hints (Regexp, &flags);
   must_match_bol = flags & SLREGEXP_HINT_BOL;
   if (must_match_bol 
       && (dir == 1) && (Point != 0)
       && (0 == jed_down (1)))
     return 0;

   max_point = Point;
   while (1)
     {
	if (dir == 1)
	  {
	     match = SLregexp_match (Regexp, (char *)CLine->data + Point, CLine->len - Point);
	     if (match != NULL)
	       {
		  /* adjust offsets */
		  Regexp_Offset = Point;
	       }
	  }
	else if (NULL != (match = SLregexp_match(Regexp, (char *)CLine->data, CLine->len)))
	  {
	     char *max_match = (char *)CLine->data + max_point;

	     Regexp_Offset = 0;
	     if (match >= max_match)
	       {
		  /* Match occurs to the right of where it is expected to be */
		  match = NULL;
	       }
	     else if (must_match_bol == 0)
	       {
		  int epos = Point - 1;

		  /* found a match on line now find one closest to current point */
		  while (epos >= 0)
		    {
		       match = SLregexp_match(Regexp, (char *)CLine->data + epos,
					      CLine->len - epos);
		       if ((match == NULL)
			   || (match >= max_match))
			 {
			    epos--;
			    continue;
			 }
		       Regexp_Offset = epos;
		       break;
		    }
	       }
	  }
	if (match != NULL)
	  {
	     unsigned int ofs, len;
	     jed_position_point ((unsigned char *)match);
	     
	     (void) SLregexp_nth_match (Regexp, 0, &ofs, &len);
	     return (len + 1);
	  }
	if (dir > 0)
	  {
	     if (0 == jed_down (1))
	       break;
	  }
	else
	  {
	     if (0 == jed_up(1))
	       break;
	     /* For the purposes of searching, the Point lies between the end
	      * of this one and the beginning of the next.
	      */
	     max_point = CLine->len;
	  }
     }
   return (0);
}
/*}}}*/
#endif				       /* SLANG_VERSION < 20000 */


int re_search_forward(char *pat) /*{{{*/
{
   int n, p, len;
   Line *l;

   if (eobp ())
     return 0;

   p = Point; n = LineNum; l = CLine;
   if (0 != (len = re_search_dir((unsigned char *) pat, 1))) return (len);
   Point = p; LineNum = n; CLine = l;
   return (0);
}

/*}}}*/

int re_search_backward(char *pat) /*{{{*/
{
   int n, p, len;
   Line *l;

   p = Point; n = LineNum; l = CLine;
   if (0 != (len = re_search_dir((unsigned char *) pat, -1))) return (len);
   Point = p; LineNum = n; CLine = l;
   return (0);
}

/*}}}*/

int replace_match(char *s, int *literal) /*{{{*/
{
   int n, nmax;
   char ch;
   unsigned int offset;
   unsigned int end_of_match_point;
   Line *match_line;
#if SLANG_VERSION < 20000
   int *beg_matches;
   unsigned int *len_matches;
#else
   unsigned int i;
   int beg_matches[10];
   unsigned int len_matches[10];
#endif
   
#if SLANG_VERSION < 20000
   if (Regexp.pat == NULL)
     return 0;
   beg_matches = Regexp.beg_matches;
   len_matches = Regexp.end_matches;
   offset = Regexp.offset;
#else
   if (Regexp == NULL)
     return 0;
   offset = Regexp_Offset;

   for (i = 0; i < 10; i++)
     {
	unsigned int bm, lm;
	if (-1 == SLregexp_nth_match (Regexp, i, &bm, &lm))
	  {
	     beg_matches[i] = -1;
	     len_matches[i] = 0;
	     continue;
	  }
	beg_matches[i] = (int) bm;
	len_matches[i] = lm;
     }
#endif

   end_of_match_point = beg_matches[0] + offset + len_matches[0];
   if ((beg_matches[0] == -1)
       || (end_of_match_point > (unsigned int) CLine->len))
     return 0;

   if (*literal)
     {
	Point = beg_matches[0] + offset;
	n = len_matches[0];
	if (-1 == jed_generic_del_nbytes (n))
	  return -1;
	jed_insert_string(s);
	return (1);
     }
   /* This is painful --- \& means whole expression, \x x = 1..9 means a
    sub expression */

   match_line = CLine;
   /* must work with relative numbers since ins/del may do a realloc */
   if (end_of_match_point == (unsigned int) match_line->len)
     {
	(void) eol ();
	(void) jed_right(1);
     }
   else Point = end_of_match_point;

   while ((ch = *s++) != 0)
     {
	if (ch != '\\')
	  {
	     if (-1 == jed_insert_byte (ch))
	       return -1;
	     continue;
	  }
	ch = *s++;
	if (ch == 0) break;

	switch (ch)
	  {
	   case 'n': ch = '\n'; goto default_label;
	   case 't': ch = '\t'; goto default_label;
	   case 'r': ch = '\r'; goto default_label;
	   case 'v': ch = '\v'; goto default_label;
	   case 'b': ch = '\b'; goto default_label;
	   case 'f': ch = '\f'; goto default_label;
	   case 'e': ch = 27;   goto default_label;
	   case 'a': ch = 7;    goto default_label;
	   case '\\':
	     default_label:
	   default:
	     if (-1 == jed_insert_byte (ch))
	       return -1;
	     break;

	   case '&': ch = 0; /* drop */
	   case '0': case '1': case '2': case '3': case '4':
	   case '5': case '6': case '7': case '8': case '9':
	     nmax = ch - '0';
	     if ((n = beg_matches[nmax]) == -1) continue;
	     nmax = len_matches[nmax] + beg_matches[nmax];
	
	     while (n < nmax)
	       {
		  /* _jed_ins_byte may reallocate CLine->data */
		  unsigned char byte = *(match_line->data + offset + n);
		  if (-1 == jed_insert_byte (byte))
		    return -1;
		  n++;
	       }
	  }
     }
   
   push_spot();
   while (CLine != match_line)
     {
	if (1 != jed_up (1))
	  {
	     SLang_verror (SL_APPLICATION_ERROR, "Internal problem in replace_match");
	     return -1;
	  }
     }
   
   Point = beg_matches[0] + offset;
   n = len_matches[0];
   (void) jed_generic_del_nbytes (n);
   pop_spot();
   return (1);
}

/*}}}*/

/* Someday it might be necessary to deal with embedded \0 characters. */
static int push_string(char *b, int n) /*{{{*/
{
   char *s;
   int ret = -1;
   s = SLang_create_nslstring (b, n);
   if (s != NULL)
     {
	ret = SLang_push_string (s);
	SLang_free_slstring (s);
     }
   return ret;
}

/*}}}*/

#if SLANG_VERSION < 20000
void regexp_nth_match (int *np) /*{{{*/
{
   int b = 0, n = *np;

   if ((Regexp.pat == NULL) || (Regexp.beg_matches[0] == -1)
       || (Regexp.beg_matches[0] + Regexp.offset + Regexp.end_matches[0] > (unsigned int) CLine->len))
     {
	SLang_set_error (SL_UNKNOWN_ERROR);
	return;
     }

   if ((n < 0) || (n > 9)) n = 0;
   else
     {
	if ((b = Regexp.beg_matches[n]) == -1) n = 0;
	else
	  {
	     n = Regexp.end_matches[n];
	  }
     }
   b += Regexp.offset;
   (void) push_string((char *) CLine->data + b, n);
}
/*}}}*/
#else
void regexp_nth_match (int *np) /*{{{*/
{
   unsigned int ofs, len;
   unsigned char *p, *pmax;

   if (Regexp == NULL)
     {
	(void) push_string ("", 0);
	return;
     }
   
   if (-1 == SLregexp_nth_match (Regexp, (unsigned int) *np, &ofs, &len))
     {
	(void) push_string ("", 0);
	return;
     }
   
   p = CLine->data + Regexp_Offset + ofs;
   pmax = p + len;
   
   if (pmax > CLine->data + CLine->len)
     {
	SLang_set_error (SL_RunTime_Error);
	return;
     }
   (void) push_string((char *) p, len);
}
/*}}}*/
#endif

int search_file(char *file, char *pat, int *np) /*{{{*/
{
   unsigned int n;
   VFILE *vp;
   int n_matches = 0, n_max = *np, key_len = 0;
   SLsearch_Type *st;
   unsigned char *buf;
#if SLANG_VERSION < 20000
   SLRegexp_Type regexp_buf;
   unsigned char rbuf[512];
#else
   unsigned int flags;
#endif
   SLRegexp_Type *reg;
   int osearch;

#if SLANG_VERSION < 20000
   reg = &regexp_buf;
   memset ((char *)reg, 0, sizeof (SLRegexp_Type));
   reg->case_sensitive = Buffer_Local.case_search;
   reg->buf = rbuf;
   reg->pat = (unsigned char *) pat;
   reg->buf_len = 512;

   if (SLang_regexp_compile(reg))
     {
	msg_error("Unable to compile pattern.");
	return(0);
     }
   osearch = Regexp.osearch;
#else
   flags = 0;
   if (Buffer_Local.case_search == 0) flags |= SLREGEXP_CASELESS;
   if (Jed_UTF8_Mode) flags |= SLREGEXP_UTF8;
   if (NULL == (reg = SLregexp_compile (pat, flags)))
     return 0;
   (void) SLregexp_get_hints (reg, &flags);
   osearch = flags & SLREGEXP_HINT_OSEARCH;
#endif

   st = NULL;
   if (osearch)
     {
        st = open_search ((char *) pat, 1, Buffer_Local.case_search, &key_len);
	if (st == NULL)
	  return 0;
     }
#if SLANG_VERSION < 20000
   else if (Regexp.must_match)
     {
	st = open_search ((char *) Regexp.must_match_str, 1, Buffer_Local.case_search, &key_len);
	if (st == NULL)
	  return 0;
     }
#endif

   if (NULL == (vp = vopen(file, 0, VFILE_TEXT)))
     {
        if (st != NULL) close_search (st);
#if SLANG_VERSION >= 20000
	SLregexp_free (reg);
#endif
        return 0;
     }

   while (NULL != (buf = (unsigned char *) vgets(vp, &n)))
     {
#if SLANG_VERSION < 20000
	if (Regexp.must_match)
	  {
# if SLANG_VERSION < 20000
	     if ((unsigned int) key_len > n) continue;
	     if (NULL == SLsearch (buf, buf + n, st))
               continue;
# else
             if (NULL == SLsearch_forward (st, buf, buf + n))
               continue;
# endif
	     if (osearch)
               goto match_found;
	  }
#endif

#if SLANG_VERSION < 20000
	if (!SLang_regexp_match(buf, (int) n, reg)) continue;
	match_found:
#else
	if (NULL == SLregexp_match (reg, (char *)buf, n))
	  continue;
#endif
	n_matches++;
	
	if (-1 == push_string ((char *) buf, n))
	  break;
	
	n_max--;
	if (n_max == 0)
	  break;
     }
   vclose(vp);
   if (st != NULL)
     close_search (st);
#if SLANG_VERSION >= 20000
   SLregexp_free (reg);
#endif
   return(n_matches);
}

/*}}}*/

int insert_file_region (char *file, char *rbeg, char *rend) /*{{{*/
{
   VFILE *vp;
   unsigned int n;
   unsigned int len = (unsigned int) strlen (rbeg);
   int num = 0;
   unsigned char *buf;

   if (NULL == (vp = vopen(file, 0, VFILE_TEXT))) return (-1);

   while (NULL != (buf = (unsigned char *) vgets(vp, &n)))
     {
	if ((len == 0) ||
	    ((n >= len) && !strncmp ((char *) buf, rbeg, len)))
	  {
	     Suspend_Screen_Update = 1;
	     if (-1 == jed_quick_insert (buf, (int) n))
	       {
		  vclose (vp);
		  return -1;
	       }

	     num++;

	     len = (unsigned int) strlen (rend);

	     while (NULL != (buf = (unsigned char *) vgets(vp, &n)))
	       {
		  if (len &&
		      ((n >= len) && !strncmp ((char *) buf, rend, len)))
		    break;

		  if (-1 == jed_quick_insert(buf, (int) n))
		    {
		       vclose (vp);
		       return -1;
		    }

		  if (SLang_get_error ()) break;
		  num++;
	       }
	     break;
	  }
     }
   vclose (vp);
   return num;
}

/*}}}*/

