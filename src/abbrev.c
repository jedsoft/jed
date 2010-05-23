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
#include "abbrev.h"
#include "text.h"
#include "ledit.h"
#include "ins.h"
#include "cmds.h"
#include "misc.h"

/*}}}*/

#if JED_HAS_ABBREVS
/*{{{ Static Variables */

typedef struct
{
   char *abbrev;
   char *string;
   unsigned int abbrev_len;
   unsigned int flags;
#define ABBREV_LEAVE_POINT	1
}
Abbrev_Type;

typedef struct Abbrev_Table_Type /*{{{*/
{
   unsigned int len;		       /* length of table */
   unsigned int max_len;	       /* number allocated */
   Abbrev_Type *abbrevs;
   char word_chars[256];	       /* word delimiters */
   char *name;			       /* name of table */
   struct Abbrev_Table_Type *next;
}

/*}}}*/

Abbrev_Table_Type;

static Abbrev_Table_Type *Global_Abbrev_Table;
static Abbrev_Table_Type *Abbrev_Tables;

/*}}}*/

/* FIXME: Not multibyte safe */
int jed_expand_abbrev (SLwchar_Type ch) /*{{{*/
{
   Abbrev_Table_Type *tbl;
   unsigned int len;
   unsigned char *p;
   Abbrev_Type *a, *amax;
   char ch1;

   tbl = CBuf->abbrev_table;
   if (tbl == NULL)
     {
	tbl = Global_Abbrev_Table;
	if (tbl == NULL) return 0;
     }

   if (tbl->word_chars[(unsigned char) ch]) return 0;   /* not a delimiter */

   p = CLine->data + (Point - 1);

   while ((p >= CLine->data) && (tbl->word_chars[*p]))
     {
	p--;
     }
   p++;

   len = (unsigned int) ((CLine->data + Point) - p);
   if (len == 0)
     return 0;

   ch1 = *p;
   a = tbl->abbrevs;
   amax = a + tbl->len;

   while (a < amax)
     {
	char *str;

	if ((a->abbrev_len != len)
	    || ((char) ch1 != a->abbrev[0])
	    || (0 != strncmp ((char *) p, a->abbrev, len)))
	  {
	     a++;
	     continue;
	  }

	Point -= len;

	(void) jed_del_nbytes (len);

	str = a->string;
	len = strlen (str);

	if ((CBuf->flags & OVERWRITE_MODE)
	    && (-1 == jed_del_nbytes (len))) /* this does not delete across lines */
	  return -1;

	if (-1 == jed_insert_nbytes ((unsigned char *) str, len))
	  return -1;

	if ((0 == (a->flags & ABBREV_LEAVE_POINT))
	    && (-1 == _jed_ins_byte (ch)))
	  return -1;

	return 1;
     }
   return 0;
}

/*}}}*/

static Abbrev_Table_Type *find_table (char *name, int err) /*{{{*/
{
   Abbrev_Table_Type *tbl;

   if (0 == strcmp (name, "Global"))
     {
	if (Global_Abbrev_Table != NULL)
	  return Global_Abbrev_Table;

	/* Fall through for error */
     }

   tbl = Abbrev_Tables;
   while (tbl != NULL)
     {
	if (0 == strcmp (tbl->name, name))
	  return tbl;

	tbl = tbl->next;
     }
   if (err)
     jed_verror ("Abbrev Table %s does not exist", name);

   return NULL;
}

/*}}}*/

void create_abbrev_table (char *name, char *word_chars) /*{{{*/
{
   Abbrev_Table_Type *tbl;

   tbl = find_table (name, 0);
   if (tbl == NULL)
     {
	tbl = (Abbrev_Table_Type *) jed_malloc0 (sizeof (Abbrev_Table_Type));
	if (tbl == NULL)
	  return;
	if (NULL == (tbl->name = SLang_create_slstring (name)))
	  {
	     SLfree ((char *) tbl);
	     return;
	  }
	if (0 == strcmp (name, "Global"))
	  Global_Abbrev_Table = tbl;
	else
	  {
	     tbl->next = Abbrev_Tables;
	     Abbrev_Tables = tbl;
	  }
     }

   if (*word_chars == 0) word_chars = Jed_Word_Range;

   SLmake_lut ((unsigned char *) tbl->word_chars, (unsigned char *) word_chars, 0);
}

/*}}}*/

static void free_abbrev_table (Abbrev_Table_Type *t)
{
   Abbrev_Type *a, *amax;

   a = t->abbrevs;
   amax = a + t->len;

   while (a < amax)
     {
	SLang_free_slstring (a->abbrev);
	SLang_free_slstring (a->string);
	a++;
     }
   SLfree ((char *) t->abbrevs);
   SLang_free_slstring (t->name);
   SLfree ((char *) t);
}

void delete_abbrev_table (char *name) /*{{{*/
{
   Abbrev_Table_Type *tbl;
   Buffer *b;

   tbl = find_table (name, 1);
   if (tbl == NULL) return;
   if (tbl == Global_Abbrev_Table)
     Global_Abbrev_Table = NULL;
   else
     {
	Abbrev_Table_Type *prev = Abbrev_Tables;
	if (prev != tbl)
	  {
	     while (prev->next != tbl)
	       tbl = tbl->next;
	     prev->next = tbl->next;
	  }
	else Abbrev_Tables = tbl->next;
     }

   b = CBuf;
   do
     {
	if (b->abbrev_table == tbl) b->abbrev_table = NULL;
	b = b->next;
     }
   while (b != CBuf);
   free_abbrev_table (tbl);
}

/*}}}*/

void define_abbrev (char *table, char *abbrev, char *expans) /*{{{*/
{
   Abbrev_Table_Type *tbl;
   Abbrev_Type *a, *amax;
   unsigned int n;
   unsigned int abbrev_len, expans_len;
   unsigned int flags;

   tbl = find_table (table, 1);
   if (tbl == NULL) return;

   n = tbl->len;
   a = tbl->abbrevs;

   if (n >= tbl->max_len)
     {
	unsigned int max_len = tbl->max_len + 32;

	/* SLrealloc handles NULL via malloc */
	if (NULL == (a = (Abbrev_Type *) SLrealloc ((char *) a, max_len * sizeof (Abbrev_Type))))
	  return;

	tbl->abbrevs = a;
	tbl->max_len = max_len;
     }

   /* Before actually creating an abbrev, make sure we can do it without
    * failure.
    */
   flags = 0;
   if (NULL == (abbrev = SLang_create_slstring (abbrev)))
     return;

   abbrev_len = strlen (abbrev);
   expans_len = strlen (expans);

   if ((expans_len > 0) && (expans[expans_len-1] == 8))   /* ^H */
     {
	expans_len--;
	flags |= ABBREV_LEAVE_POINT;
     }

   if (NULL == (expans = SLang_create_nslstring (expans, expans_len)))
     {
	SLang_free_slstring (abbrev);
	return;
     }

   amax = a + n;
   while (a < amax)
     {
	if (a->abbrev == abbrev)       /* SLstring comparison */
	  {
	     SLang_free_slstring (a->abbrev);
	     SLang_free_slstring (a->string);
	     break;
	  }
	a++;
     }

   if (a == amax)
     tbl->len++;

   a->string = expans;
   a->abbrev = abbrev;
   a->flags = flags;
   a->abbrev_len = abbrev_len;
}

/*}}}*/

void use_abbrev_table (char *name) /*{{{*/
{
   Abbrev_Table_Type *tbl;

   if (NULL != (tbl = find_table (name, 1)))
     CBuf->abbrev_table = tbl;
}

/*}}}*/

int abbrev_table_p (char *name) /*{{{*/
{
   return (NULL != find_table (name, 0));
}

/*}}}*/

static void push_word (Abbrev_Table_Type *tbl) /*{{{*/
{
   char buf[256], *b, *w;
   int i, in_range;

   b = buf;
   w = tbl->word_chars;

   if (w[(unsigned char) '-'] != 0) *b++ = '-';
   in_range = 0;

   for (i = 33; i < 256; i++)
     {
	if ((i != '-') && (0 != w[i]))
	  {
	     if (i == '\\') *b++ = (char) i;
	     *b = (char) i;
	     if (in_range == 0)
	       {
		  *(b + 1) = '-';
		  b += 2;  *b = 0;
		  in_range = 1;
	       }
	  }
	else
	  {
	     if (in_range)
	       {
		  if (*b == 0) b--; else b++;
		  in_range = 0;
	       }
	  }
     }
   *b = 0;
   SLang_push_string (buf);
}

/*}}}*/

void what_abbrev_table (void) /*{{{*/
{
   Abbrev_Table_Type *tbl;

   tbl = CBuf->abbrev_table;
   if (tbl == NULL)
     {
	tbl = Global_Abbrev_Table;

	if (tbl == NULL)
	  {
	     (void) SLang_push_string ("");
	     (void) SLang_push_string ("");
	     return;
	  }
     }

   (void) SLang_push_string (tbl->name);
   push_word (tbl);
}

/*}}}*/

void dump_abbrev_table (char *name) /*{{{*/
{
   Abbrev_Table_Type *tbl;
   Abbrev_Type *a, *amax;

   if (NULL == (tbl = find_table (name, 1))) return;

   a = tbl->abbrevs;
   amax = a + tbl->len;

   while (a < amax)
     {
	if ((-1 == jed_insert_string (a->abbrev))
	    || (-1 == jed_insert_byte ('\t'))
	    || (-1 == jed_insert_string (a->string))
	    || ((a->flags & ABBREV_LEAVE_POINT)
		&& (-1 == jed_insert_byte (8)))
	    || (-1 == jed_insert_newline ()))
	  return;
	a++;
     }
   push_word (tbl);
}

/*}}}*/

int list_abbrev_tables (void) /*{{{*/
{
   Abbrev_Table_Type *tbl;
   int n = 0;

   if (Global_Abbrev_Table != NULL)
     {
	(void) SLang_push_string (Global_Abbrev_Table->name);
	n++;
     }

   tbl = Abbrev_Tables;
   while (tbl != NULL)
     {
	(void) SLang_push_string (tbl->name);
	tbl = tbl->next;
	n++;
     }

   return n;
}

/*}}}*/
#endif   /* JED_HAS_ABBREVS */
