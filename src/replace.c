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

#include <slang.h>

#include "jdmacros.h"

#include "buffer.h"
#include "replace.h"
#include "search.h"
#include "screen.h"
#include "ins.h"
#include "ledit.h"
#include "misc.h"
#include "paste.h"
#include "cmds.h"

/*}}}*/

int Replace_Preserve_Case = 0;
#define USE_PRESERVE_CASE 0

/* returns the length of the region inserted. */
static int replace_chars (int n, char *neew) /*{{{*/
{
   int len;
#if USE_PRESERVE_CASE
   int preserve_case;
   char *old = NULL;
   int i;
#endif

   CHECK_READ_ONLY
   if (n < 0) return 0;
   len = strlen (neew);
#if USE_PRESERVE_CASE
   preserve_case = (Replace_Preserve_Case && (len == n));
#endif
   jed_push_mark ();
#if USE_PRESERVE_CASE
   if (preserve_case) jed_push_mark ();
#endif
   n = jed_right (n);
#if USE_PRESERVE_CASE
   if (preserve_case)
     {
	if (n == len)
	  {
	     if (NULL == (old = make_buffer_substring(&n))) return 0;
	  }
	else
	  {
	     preserve_case = 0;
	     jed_pop_mark (&Number_Zero);
	  }
     }
#endif

   delete_region ();
#if USE_PRESERVE_CASE
   if (preserve_case)
     {
	unsigned char ch;

	for (i = 0; i < len; i++)
	  {
	     ch = (unsigned char) old[i];
	     if (ch == UPPER_CASE(ch))
	       {
		  if (ch == LOWER_CASE(ch))
		    old[i] = neew[i];
		  else
		    old[i] = UPPER_CASE(neew[i]);
	       }
	     else old[i] = LOWER_CASE(neew[i]);
	  }
	neew = old;
     }
#endif
   (void) jed_insert_nbytes ((unsigned char *) neew, len);
#if USE_PRESERVE_CASE
   if (preserve_case) SLfree (old);
#endif
   return len;
}

/*}}}*/

static int replace_bytes (unsigned int n, unsigned char *str)
{
   unsigned int len = strlen ((char *)str);
   if (-1 == jed_push_mark ())
     return -1;
   if (n != jed_right_bytes (n))
     return -1;
   if (-1 == delete_region ())
     return -1;
   if (-1 == jed_insert_nbytes (str, len))
     return -1;
   return (int) len;
}

/* This code implements a kill ring of sorts. It could be done in S-Lang but
 * S-Lang cannot handle strings with embedded NULL characters.  I do not
 * want to lose compatability with C or I would allow S-Lang strings to handle
 * the NULL chars.
 */

#ifndef SIXTEEN_BIT_SYSTEM

# define MAX_KILL_ARRAY_SIZE 16
int Kill_Array_Size = MAX_KILL_ARRAY_SIZE;

typedef struct /*{{{*/
{
   unsigned char *buf;
   int len;
}

/*}}}*/
Char_Array_Type;

Char_Array_Type Kill_Array [MAX_KILL_ARRAY_SIZE];

void copy_region_to_kill_array (int *np) /*{{{*/
{
   int n = *np;

   if (n < 0) n = 0;
   n = n % MAX_KILL_ARRAY_SIZE;

   SLfree ((char *) Kill_Array[n].buf);
   Kill_Array[n].buf = (unsigned char *) make_buffer_substring (&Kill_Array[n].len);
}

/*}}}*/

void insert_from_kill_array (int *np) /*{{{*/
{
   int n = *np;
   unsigned char *buf;

   if (CBuf->flags & READ_ONLY)
     {
	msg_error (Read_Only_Error);
	return;
     }

   if (n < 0) n = 0;
   n = n % MAX_KILL_ARRAY_SIZE;

   if ((buf = Kill_Array[n].buf) == NULL) return;
   (void) jed_insert_nbytes (buf, Kill_Array[n].len);
}

/*}}}*/

void append_region_to_kill_array (int *np) /*{{{*/
{
   int n = *np, len, oldlen;
   unsigned char *buf, *newbuf;

   if (n < 0) n = 0;
   n = n % MAX_KILL_ARRAY_SIZE;

   buf = (unsigned char *) make_buffer_substring (&len);
   if (buf == NULL) return;

   oldlen = Kill_Array[n].len;
   newbuf = (unsigned char *) SLrealloc ((char *)Kill_Array[n].buf, oldlen + len + 1);

   if (newbuf != NULL)
     {
	SLMEMCPY ((char *) newbuf + oldlen, (char *) buf, len);
	Kill_Array[n].buf = newbuf;
	Kill_Array[n].len = oldlen + len;
     }

   SLfree ((char *)buf);
}

/*}}}*/

void prepend_region_to_kill_array (int *np) /*{{{*/
{
   int n = *np, len, oldlen;
   unsigned char *buf, *newbuf;

   if (n < 0) n = 0;
   n = n % MAX_KILL_ARRAY_SIZE;

   buf = (unsigned char *) make_buffer_substring (&len);
   if (buf == NULL) return;

   oldlen = Kill_Array[n].len;
   newbuf = (unsigned char *) SLrealloc ((char *)Kill_Array[n].buf, oldlen + len + 1);

   if (newbuf != NULL)
     {
#if 0
	/* there's no SLMEMMOVE - and memmove isn't always there (pre-ANSI
	 * systems have bcopy instead, with ptr arguments swapped)
	 */
	memmove ((char *) newbuf + len, (char *) newbuf, oldlen);
#else
	/* SLMEMCPY doesn't handle overlapping regions. */
	int i;
	for (i = oldlen - 1; i >= 0; i--) newbuf[len + i] = newbuf[i];
#endif
	SLMEMCPY ((char *) newbuf, (char *) buf, len);
	Kill_Array[n].buf = newbuf;
	Kill_Array[n].len = oldlen + len;
     }

   SLfree ((char *)buf);
}

/*}}}*/

#endif  /* NOT 16bit system */

static unsigned char *char_to_string (SLwchar_Type wch, unsigned char buf[JED_MAX_MULTIBYTE_SIZE+1])
{
   /* Will only fail if the buffer is too small-- it is not here */
   unsigned char *b;
   if (NULL == (b = jed_wchar_to_multibyte (wch, buf)))
     return NULL;
   *b = 0;			       /* \0 terminate buf */
   return buf;
}

static int bol_bsearch_char (SLwchar_Type *wch) /*{{{*/
{
   unsigned char buf[JED_MAX_MULTIBYTE_SIZE+1];
   return bol_bsearch ((char *)char_to_string (*wch, buf));
}

/*}}}*/

static int bol_fsearch_char (SLwchar_Type *wch) /*{{{*/
{
   unsigned char buf[JED_MAX_MULTIBYTE_SIZE+1];
   return bol_fsearch ((char *)char_to_string (*wch, buf));
}

/*}}}*/

static int fsearch_char (SLwchar_Type *wch) /*{{{*/
{
   unsigned char buf[JED_MAX_MULTIBYTE_SIZE+1];
   return search_forward ((char *)char_to_string (*wch, buf));
}

/*}}}*/

static int bsearch_char (SLwchar_Type *wch) /*{{{*/
{
   unsigned char buf[JED_MAX_MULTIBYTE_SIZE+1];
   return search_backward ((char *)char_to_string (*wch, buf));
}

/*}}}*/

static int ffind_char (SLwchar_Type *wch) /*{{{*/
{
   unsigned char buf[JED_MAX_MULTIBYTE_SIZE+1];
   return forward_search_line ((char *)char_to_string (*wch, buf));
}

/*}}}*/

static int bfind_char (SLwchar_Type *wch) /*{{{*/
{
   unsigned char buf[JED_MAX_MULTIBYTE_SIZE+1];
   return backward_search_line ((char *)char_to_string (*wch, buf));
}

/*}}}*/

static void widen_this_buffer (void) /*{{{*/
{
   jed_widen_whole_buffer (CBuf);
}

/*}}}*/

static int is_visible_mark (void) /*{{{*/
{
   if ((CBuf->marks == NULL) || (0 == (CBuf->marks->flags & VISIBLE_MARK)))
     return 0;
   return 1;
}

/*}}}*/

static int set_buffer_umask (int *cmask)
{
   int u = CBuf->umask;
   if (*cmask != -1)
     CBuf->umask = *cmask;
   return u;
}

static int replace_chars_intrinsic (int *np, char *s)
{
   return replace_chars (*np, s);
}

static int replace_next(char *old, char *neew) /*{{{*/
{
   int n;

   if (search(old, 1, 0) == 0) return(0);
   n = strlen (old);
   if (-1 == replace_bytes (n, (unsigned char *)neew))
     return -1;
   return(1);
}

/*}}}*/

static void replace_cmd(char *old, char *neew) /*{{{*/
{
   CHECK_READ_ONLY_VOID
     push_spot ();
   if (search(old, 1, 0))
     while(1 == replace_next(old, neew))
       ;
   pop_spot();
   return;
}

/*}}}*/

SLang_Intrin_Fun_Type Jed_Other_Intrinsics [] = /*{{{*/
{
   MAKE_INTRINSIC_I("set_buffer_umask", set_buffer_umask, INT_TYPE),
   MAKE_INTRINSIC_S("fsearch", search_forward, INT_TYPE),
   MAKE_INTRINSIC_S("bsearch", search_backward, INT_TYPE),
   MAKE_INTRINSIC_S("bfind", backward_search_line, INT_TYPE),
   MAKE_INTRINSIC_S("ffind", forward_search_line, INT_TYPE),
   MAKE_INTRINSIC_S("bol_fsearch", bol_fsearch, INT_TYPE),
   MAKE_INTRINSIC_S("bol_bsearch", bol_bsearch, INT_TYPE),
   MAKE_INTRINSIC_1("bol_fsearch_char", bol_fsearch_char, INT_TYPE, SLANG_WCHAR_TYPE),
   MAKE_INTRINSIC_1("bol_bsearch_char", bol_bsearch_char, INT_TYPE, SLANG_WCHAR_TYPE),
   MAKE_INTRINSIC_1("fsearch_char", fsearch_char, INT_TYPE, SLANG_WCHAR_TYPE),
   MAKE_INTRINSIC_1("bsearch_char", bsearch_char, INT_TYPE, SLANG_WCHAR_TYPE),
   MAKE_INTRINSIC_1("bfind_char", bfind_char, INT_TYPE, SLANG_WCHAR_TYPE),
   MAKE_INTRINSIC_1("ffind_char", ffind_char, INT_TYPE, SLANG_WCHAR_TYPE),
   MAKE_INTRINSIC_SS("replace", replace_cmd, VOID_TYPE),
   MAKE_INTRINSIC_IS("replace_chars", replace_chars_intrinsic, INT_TYPE),
   MAKE_INTRINSIC_I("regexp_nth_match", regexp_nth_match, VOID_TYPE),
   MAKE_INTRINSIC_SI("replace_match", replace_match, INT_TYPE),
   MAKE_INTRINSIC_S("re_fsearch", re_search_forward, INT_TYPE),
   MAKE_INTRINSIC_S("re_bsearch", re_search_backward, INT_TYPE),
   MAKE_INTRINSIC("is_visible_mark", is_visible_mark, INT_TYPE, 0),
#if JED_HAS_SAVE_NARROW
   MAKE_INTRINSIC("push_narrow", jed_push_narrow, VOID_TYPE, 0),
   MAKE_INTRINSIC("pop_narrow", jed_pop_narrow, VOID_TYPE, 0),
#endif
   MAKE_INTRINSIC("count_narrows", jed_count_narrows, INT_TYPE, 0),
   MAKE_INTRINSIC("widen_buffer", widen_this_buffer, VOID_TYPE, 0),
   MAKE_INTRINSIC(NULL, NULL, 0, 0)
};

/*}}}*/
