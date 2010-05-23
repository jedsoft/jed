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
#include <setjmp.h>
#if defined (__MSDOS__) || defined (__os2_16__)
# include <process.h>
# ifndef __MINGW32__
#  include <dos.h>
# endif
#endif
#include <limits.h>
#include <string.h>

#include "buffer.h"
#include "window.h"
#include "file.h"
#include "ins.h"
#include "misc.h"
#include "paste.h"
#include "sysdep.h"
#include "cmds.h"
#include "indent.h"

#if JED_HAS_SUBPROCESSES
#include "jprocess.h"
#endif


/*}}}*/

typedef struct /*{{{*/
{
   jmp_buf b;
}

/*}}}*/
jmp_buf_struct;

extern jmp_buf_struct Jump_Buffer, *Jump_Buffer_Ptr;

typedef struct Buffer_List_Type /*{{{*/
{
   Buffer *buf;
   struct Buffer_List_Type *next;
}

/*}}}*/
Buffer_List_Type;
Buffer_List_Type *Buffer_Stack;   /* a filo */

#ifndef ULONG_MAX
#define MAX_LONG (0xFFFFFFFFL | (0xFFFFFFFFL << (sizeof(long) / 8)))
#else
#define MAX_LONG ULONG_MAX
#endif

Buffer *CBuf;
Line *CLine;
Buffer_Local_Type Buffer_Local = /*{{{*/
{
   8, 0, 0, 72
};

/*}}}*/

int Number_One = 1;		       /* these should be const but  */
int Number_Zero = 0;		       /* some compilers complain */
int Number_Two = 2;
int Number_Ten = 10;
char *Read_Only_Error = (char *) "Buffer is read only!";

unsigned int LineNum = 1;
unsigned int Max_LineNum;
int Point = 0;

static void delete_buffer_hooks (Jed_Buffer_Hook_Type *);

void jed_set_point (int point)
{
   if (point > CLine->len)
     point = CLine->len;
   if (point < 0)
     point = 0;
   
   Point = point;
}

/* move point to top of buffer */
int bob() /*{{{*/
{
   CLine = CBuf->beg;
   Point = 0;
   LineNum = 1;
   return(1);
}

/*}}}*/

int eob()                      /* point to end of buffer */ /*{{{*/
{
   CLine = CBuf->end;
   LineNum = Max_LineNum;
   if (CLine == NULL)
     {
	Point = 0;
	return(0);
     }
   
   if (LINE_HAS_NEWLINE(CLine)
       && (CBuf != MiniBuffer))
     Point = CLine->len - 1;
   else
     Point = CLine->len;
   
   return 1;
}

/*}}}*/

int bol(void) /*{{{*/
{
   Point = 0;
   Goal_Column = 1;
   return(1);
}

/*}}}*/

int eol (void) /*{{{*/
{
   if (CLine == NULL)
     {
	Point = 0;
	return 1;
     }

   if (LINE_HAS_NEWLINE(CLine)
       && (CBuf != MiniBuffer))
     Point = CLine->len - 1;
   else
     Point = CLine->len;

   return 1;
}

/*}}}*/

int bobp (void)                /* returns 1 if top line of buffer, 0 otherwise */ /*{{{*/
{
   return ((CBuf->beg == CLine) && (Point == 0));
}

/*}}}*/

int eolp (void) /*{{{*/
{
   int len;
   
   if ((CLine == NULL)
       || (0 == (len = CLine->len)))
     return 1;

   if (Point < len - 1) return 0;
   
   if (CBuf == MiniBuffer)
     return (Point >= len);

   /* Point is either len or len - 1.  If it is len-1, then we are at the 
    * end of the line only if the line ends with \n.
    */

   if (LINE_HAS_NEWLINE(CLine)
       || (Point == len))
     return 1;

   Point = len - 1;
   return 0;
}

/*}}}*/

int eobp (void) /*{{{*/
{
   return ((CLine == CBuf->end) && eolp ());
}

/*}}}*/

int bolp (void) /*{{{*/
{
   return (Point == 0);
}

/*}}}*/

/*  Attempt to goback n lines, return actual number. */
unsigned int jed_up (unsigned int n) /*{{{*/
{
   unsigned int i = 0;

   if (n == 0)
     return 0;

   while (i < n)
     {
	Line *prev = CLine->prev;
	
	/* Why would prev->len be 0 ??? */
	while ((prev != NULL) && (prev->len == 0)) 
	  prev = prev->prev;

	if (prev == NULL) break;
	i++;
	CLine = prev;
     }
   LineNum -= i;

   if (i) eol ();
   return i;
}

/*}}}*/

unsigned int jed_down (unsigned int n) /*{{{*/
{
   unsigned int i = 0;

   while (i < n)
     {
	Line *next = CLine->next;
	if (next == NULL) break;
	CLine = next;
	i++;
     }
   LineNum += i;

   if (i) bol ();
   return i;
}

/*}}}*/

unsigned char *jed_multibyte_chars_forward (unsigned char *p, unsigned char *pmax,
					    unsigned int n, unsigned int *dn, 
                                            int ignore_combining)
{
   (void) ignore_combining;
   if (Jed_UTF8_Mode == 0)
     {
	if (p + n > pmax)
	  n = pmax - p;
   
	p += n;
	if (dn != NULL) *dn = n;
	return p;
     }
   return SLutf8_skip_chars (p, pmax, n, dn, ignore_combining);
}

unsigned char *jed_multibyte_chars_backward (unsigned char *pmin, unsigned char *p, 
					     unsigned int n, unsigned int *dn, 
                                             int ignore_combining)
{
   (void) ignore_combining;
   if (Jed_UTF8_Mode == 0)
     {
	if (pmin + n > p)
	  n = p - pmin;
   
	p -= n;
	if (dn != NULL) *dn = n;
	return p;
     }
   return SLutf8_bskip_chars (pmin, p, n, dn, ignore_combining);
}

int jed_multibyte_charcasecmp (unsigned char **ap, unsigned char *amax,
			       unsigned char **bp, unsigned char *bmax)
{
   SLuchar_Type *a, *b;
   SLwchar_Type cha, chb;

   a = *ap;
   b = *bp;
   
   if ((a >= amax) || (b >= bmax))
     return 0;

   if (Jed_UTF8_Mode)
     {
        unsigned int na, nb;
        int aok, bok;

        aok = (NULL != SLutf8_decode (a, amax, &cha, &na));
        bok = (NULL != SLutf8_decode (b, bmax, &chb, &nb));
        
        *ap = a + na;
        *bp = b + nb;
        
        if (aok && bok)
          {
             cha = SLwchar_toupper (cha);
             chb = SLwchar_toupper (chb);
          }
        else if (aok)
          return 1;
        else if (bok)
          return -1;
        
        if (cha > chb)
          return 1;
        
        if (cha == chb)
          return 0;
        
        return -1;
     }

   *ap = a+1;
   *bp = b+1;

   if ((*a == *b) || ((cha = UPPER_CASE(*a)) == (chb = UPPER_CASE(*b))))
     return 0;
   if (cha > chb) return 1;
   return -1;
}

/* The buffer is required to be at least JED_MAX_MULTIBYTE_SIZE bytes wide.
 * This routine does not \0 terminate the buffer.
 */
unsigned char *jed_wchar_to_multibyte (SLwchar_Type c, unsigned char *buf)
{
   if (Jed_UTF8_Mode && (c >= 0x80))
     {
	unsigned char *b = SLutf8_encode (c, buf, JED_MAX_MULTIBYTE_SIZE);
	if (b == NULL)
	  SLang_verror (SL_Unicode_Error, "Unable to convert 0x%lX to UTF-8", (unsigned long)c);
	return b;
     }
   *buf++ = c;
   return buf;
}

int jed_what_char (SLwchar_Type *w) /*{{{*/
{
   SLuchar_Type *p, *pmax;

   if (eobp ())
     {
	*w = 0;
	return -1;
     }
   p = CLine->data + Point;
   if (Jed_UTF8_Mode == 0)
     {
        *w = (SLwchar_Type) *p;
        return 0;
     }
   pmax = SLutf8_skip_char (p, CLine->data + CLine->len);
   if (NULL == SLutf8_decode (p, pmax, w, NULL))
     {
	if (p < pmax)
	  *w = (SLwchar_Type) *p;
	else
	  *w = 0;
	return -1;
     }

   return 0;
}

/*}}}*/


void jed_count_chars (void) /*{{{*/
{
   unsigned long n = 0, m = 0;
   char buf[256];
   SLwchar_Type w;

   Line *l = CBuf->beg;
   
   while (l != NULL)
     {
	n += l->len;
	l = l->next;
     }
   l = CBuf->beg;
   while (l != CLine)
     {
	m += l->len;
	l = l->next;
     }
   m += Point + 1;

   if (eobp ())
     *buf = 0;
   else
     {
        if (-1 == jed_what_char (&w))
	  sprintf (buf, "-<%0X>, ", (int) w);
        else
          {
             SLuchar_Type *p, *p1, *pmax;
             int skip_combining = 0;

             p = CLine->data + Point;
             if (*p == 0)
               {
                  p = (SLuchar_Type *) "^@";
                  pmax = p + 2;
		  skip_combining = 1;
               }
             else
               pmax = jed_multibyte_chars_forward (p, CLine->data + CLine->len, 1, NULL, 1);
             
             buf[0] = '\'';
             strncpy (buf + 1, (char *)p, pmax - p);
             p1 = (SLuchar_Type *) (buf + 1 + (pmax - p));

             sprintf ((char *)p1, "'=%ld/0x%lX/%#lo", (long)w, (unsigned long)w, (unsigned long)w);
	     /* Check for combining */
	     if ((skip_combining == 0) && Jed_UTF8_Mode)
	       {
		  unsigned int dn;
		  unsigned int count = 1;
		  p = SLutf8_decode (p, pmax, &w, &dn);   /* skip first */
		  while ((p != NULL) && (p < pmax) && (count < SLSMG_MAX_CHARS_PER_CELL))
		    {
		       p = SLutf8_decode (p, pmax, &w, &dn);
		       if (p != NULL)
			 sprintf (buf + strlen(buf), " + 0x%lX", (long)w);
		       count++;
		    }
	       }
          }
     }
   
   sprintf (buf + strlen (buf), ", point %lu of %lu", m, n);
   SLang_push_string(buf);
}

/*}}}*/


unsigned char *jed_eol_position (Line *l)
{
   unsigned char *p = l->data + l->len;
   if (LINE_HAS_NEWLINE(l))
     p--;
   
   return p;
}

void jed_position_point (unsigned char *p)
{
   if (p < CLine->data + CLine->len)
     {
	Point = p - CLine->data;
	if (Point < 0)
	  Point = 0;
	return;
     }
   eol ();
}

/* The algorithm:  go to Point = len - 1.  Then forward a line counted as 1 */
unsigned int jed_right (unsigned int n) /*{{{*/
{
   unsigned int total = 0;
   
   while (1)
     {
	int len = CLine->len;
	unsigned char *p = CLine->data + Point;
	unsigned char *pmax = CLine->data + len;
	int has_newline = LINE_HAS_NEWLINE(CLine);
	unsigned int dn;

	if (has_newline)
	  pmax--;

	p = jed_multibyte_chars_forward (p, pmax, n, &dn, 1);
	Point = p - CLine->data;
	n -= dn;
	total += dn;

	if (n == 0)
	  return total;

	/* skip over the newline */
	if (has_newline)
	  {
	     Point++;
	     total++;
	     n--;
	  }

	if (0 == jed_down (1))
	  {
	     if (has_newline && (CBuf != MiniBuffer))
	       {
		  Point--;
		  total--;
	       }
	     return total;
	  }
     }
}

/*}}}*/

unsigned int jed_right_bytes (unsigned int n)
{
   int mode = Jed_UTF8_Mode;
   Jed_UTF8_Mode = 0;
   n = jed_right (n);
   Jed_UTF8_Mode = mode;
   return n;
}

/*}}}*/

unsigned int jed_left (unsigned int n) /*{{{*/
{
   unsigned int total = 0;
   
   while (1)
     {
	unsigned char *pmin = CLine->data;
	unsigned char *p = pmin + Point;
	unsigned int dn;

	p = jed_multibyte_chars_backward (pmin, p, n, &dn, 1);

	total += dn;
	n -= dn;
	Point = p - pmin;

	if (n == 0)
	  return total;

	if (0 == jed_up (1))
	  return total;

	if (LINE_HAS_NEWLINE(CLine))
	  {
	     n--;
	     total++;
	  }
     }
}

/*}}}*/

/* assuming 8 bit bytes */
#define BUNCH_SIZE 8 * sizeof(long)
static unsigned char NewLine_Buffer[1] = /*{{{*/
{
   '\n'
};

/*}}}*/

typedef struct Bunch_Lines_Type /*{{{*/
{
   struct Bunch_Lines_Type *next;
   unsigned long flags;			       /* describes which are free */
   Line lines[BUNCH_SIZE];
}

/*}}}*/
Bunch_Lines_Type;

static unsigned int Next_Free_Offset = BUNCH_SIZE;
static unsigned int Last_Free_Offset;
static Bunch_Lines_Type *Last_Free_Group;
static Bunch_Lines_Type *Bunch_Lines;
static unsigned int Number_Freed;      /* amount of Lines available */

static Line *create_line_from_bunch(void) /*{{{*/
{
   register Bunch_Lines_Type *b, *bsave;
   Line *l;
   unsigned long flags;
   int count;
   
   if (Last_Free_Group != NULL)
     {
	l = &Last_Free_Group->lines[Last_Free_Offset];
	flags = ((unsigned long) 1L << Last_Free_Offset);
	if ((Last_Free_Group->flags & flags) == 0)
	  {
	     exit_error("create group: internal error 1", 1);
	  }
	
	Last_Free_Group->flags &= ~flags;
	Last_Free_Group = NULL;
	Number_Freed--;
	return (l);
     }
   
   if (Next_Free_Offset < BUNCH_SIZE)
     {
	flags = ((unsigned long) 1L << Next_Free_Offset);
	if ((Bunch_Lines->flags & flags) == 0)
	  {
	     exit_error("free group: internal error 2", 1);
	  }
	
	
	Bunch_Lines->flags &= ~flags;
	Number_Freed--;
	return(&Bunch_Lines->lines[Next_Free_Offset++]);
     }
   
   /* search list */
   b = Bunch_Lines;
   if (b != NULL)
     {
	b = b->next;
     }
   
   if ((b != NULL) && Number_Freed)
     {
	bsave = b;
	do
	  {
	     if (b->flags)
	       {
		  flags = b->flags;
		  count = 0;
		  while ((flags & 1) == 0)
		    {
		       flags = flags >> 1;
		       count++;
		    }
		  l = &b->lines[count];
		  flags = (unsigned long) 1 << count;
		  if ((b->flags & flags) == 0)
		    {
		       exit_error("free group: internal error 2", 1);
		    }
		  b->flags &= ~flags;
		  Number_Freed--;
		  return (l);
	       }
	     b = b->next;
	  }
	while (b != bsave);
     }
   
   /* failed so now malloc new bunch */
   
   if (NULL == (b = (Bunch_Lines_Type *) SLmalloc (sizeof(Bunch_Lines_Type))))
     return NULL;
   
   if (Bunch_Lines == NULL)
     {
	Bunch_Lines = b;
	b->next = b;
     }
   else
     {
	b->next = Bunch_Lines->next;
	Bunch_Lines->next = b;
	Bunch_Lines = b;
     }
   
   b->flags = MAX_LONG;
   Next_Free_Offset = 1;
   b->flags &= ~(unsigned long) 1;
   Number_Freed += BUNCH_SIZE - 1;
   return(&b->lines[0]);
}

/*}}}*/


static void destroy_bunch_line(Line *l) /*{{{*/
{
#ifdef _MSC_VER
   Line *ll;
#else
   register Line *ll;
#endif
   
   register Bunch_Lines_Type *b, *bsave, *last, *next;
   static Bunch_Lines_Type *slast;
   
   if (slast != NULL) last = slast;
   else last = Bunch_Lines;
   
   b = bsave = last;
   
   do
     {
	ll = b->lines;
	if (
#if defined(SIXTEEN_BIT_SYSTEM)
	    /* stupid DOS and its memory segmentation forces me to consider
	     segment then offset */
	    (FP_SEG(ll) == FP_SEG(l)) &&
#endif
	    ((ll <= l) && (l < ll + BUNCH_SIZE)))
	  {
	     Last_Free_Offset = (unsigned int) (l - ll);

	     if (b->flags & (1L << Last_Free_Offset))
	       {
		  exit_error("free group: internal error 2", 1);
	       }
	     
	     b->flags |= (unsigned long) 1L << Last_Free_Offset;
	     
	     /* if this whole structure is free, free it */
	     if (b->flags == MAX_LONG)
	       {
		  if (last == b)
		    {
		       while ((next = last->next) != b) last = next;
		    }
		  
		  last->next = b->next;
		  
		  if (b == Bunch_Lines)
		    {
		       if (last == b)
			 {
			    last = NULL;
			    /*
			     last = last->next;
			     if (last == b) last = NULL; */
			 }
		       
		       Bunch_Lines = last;
		       Next_Free_Offset = BUNCH_SIZE;
		    }
		  
		  SLfree ((char *)b);
		  
		  if (last == b) last = NULL;
		  b = NULL;
		  Number_Freed -= BUNCH_SIZE - 1;
		  slast = last;
	       }
	     else
	       {
		  Number_Freed++;
	       }
	     
	     Last_Free_Group = b;
	     if (Bunch_Lines == NULL) goto L1;
	     return;
	  }
	last = b;
	b = b->next;
     }
   while (b != bsave);
   L1:
   exit_error("destroy_bunch_line: internal error 1", 1);
}

/*}}}*/

Line *make_line1(unsigned int size) /*{{{*/
{
   Line *new_line;
   unsigned char *data = NULL;
   unsigned int chunk;
   
   /* 4 byte chunks */
#if defined(SIXTEEN_BIT_SYSTEM)
   chunk = (size + 3) & 0xFFFCU;
#else
   chunk = ((size + 3)) & 0xFFFFFFFCU;
#endif
   new_line = (Line *) create_line_from_bunch();
   if (new_line != NULL)
     {
	if (size == 1)
	  {
	     data = NewLine_Buffer;
#ifdef KEEP_SPACE_INFO
	     chunk = 1;
#endif
	  }
	else data = (unsigned char *) SLmalloc (chunk);   /* was chunk + 1 */
     }
   
   if ((new_line == NULL) || (data == NULL))
     {
	*Error_Buffer = 0;	       /* this is critical */
	jed_verror ("Malloc Error in make_line: requested size: %d.", size);
	longjmp(Jump_Buffer_Ptr->b, 1);
	/* exit_error(buff); */
     }
   new_line->data = data;
   new_line->len = 0;
#ifdef KEEP_SPACE_INFO
   new_line->space = chunk;
#endif
#if JED_HAS_LINE_ATTRIBUTES
   new_line->flags = 0;
#endif
   return(new_line);
}

/*}}}*/

/* adds a new link to list of lines at current point */
unsigned char *make_line(unsigned int size) /*{{{*/
{
   Line *new_line;
   
   new_line = make_line1(size);
   /* if CLine is Null, then we are at the top of a NEW buffer.  Make this
    explicit. */
   if (CLine == NULL)
     {
	new_line -> prev = NULL;
	new_line -> next = NULL;
	CBuf -> beg = CBuf ->end = new_line;
     }
   else if (CLine == CBuf->end) /* at end of buffer */
     {
	CBuf->end  = new_line;
	new_line->next = NULL;
	new_line->prev = CLine;
	CLine->next = new_line;
     }
   else
     {
	new_line -> next = CLine -> next;
	if (CLine->next != NULL) CLine->next->prev = new_line;
	CLine->next = new_line;
	new_line->prev = CLine;
     }
   
   if (CLine == NULL)
     {
	Max_LineNum = LineNum = 1;
     }
   else
     {
	Max_LineNum++;
	LineNum++;
     }
   CLine = new_line;
   
   return(CLine->data);
}

/*}}}*/

void free_line(Line *line) /*{{{*/
{
   register unsigned char *dat = line->data;
   
   if (dat != NewLine_Buffer) SLfree ((char *)dat);
   destroy_bunch_line(line);
}

/*}}}*/

/* deletes the line we are on and returns the prev one.  It does not
 * delete the top line of the buffer.   Furthermore, it does not
 *  update any marks.  */

int delete_line() /*{{{*/
{
   Line *n, *p, *tthis;
   
   p = CLine -> prev;
   if (p == NULL) return(1);
   
   n = CLine -> next;
   tthis = CLine;
   if (n == NULL)
     {
	CBuf->end = p;
	p->next = NULL;
     }
   else
     {
	p->next = n;
	n->prev = p;
     }
   
   free_line(tthis);
   CLine = p;
   LineNum--;
   Max_LineNum--;
   
   return(0);
}

/*}}}*/

unsigned char *remake_line(unsigned int size) /*{{{*/
{
   unsigned char *d = CLine->data;
   unsigned int mask;
#if defined(SIXTEEN_BIT_SYSTEM)
   mask = 0xFFFCu;
#else
   mask = 0xFFFFFFFCu;
#endif
   size = (unsigned) (size + 3) & mask;   /* 4 byte chunks */
   
   if (d == NewLine_Buffer)
     {
	if (NULL != (d = (unsigned char *) SLmalloc (size))) *d = '\n';
     }
   else
     {
#if 1				       /* NOTE: This was #if 0.  Why??? */
#ifndef KEEP_SPACE_INFO
	if ((((CLine->len + 3) & mask) == size)
	    && (size + 8 < (unsigned) CLine->len))/* realloc if the new size is less than the old */
	  return d;
#endif
#endif
	d = (unsigned char *) SLrealloc ((char *) d, size);
     }
   
   if (d == NULL)
     {
	*Error_Buffer = 0;	       /* critical error */
	jed_verror ("remake_line: realloc error!, size = %d", size);
	longjmp(Jump_Buffer_Ptr->b, 1);
	/* exit_error(buf); */
     }
   
#ifdef KEEP_SPACE_INFO
   CLine->space = size;
#endif
   CLine->data = d;
   return(d);
}

/*}}}*/

void uniquely_name_buffer (Buffer *b, SLFUTURE_CONST char *trry) /*{{{*/
{
   Buffer *bnext;
   int version = 0, n;
   char *neew;
   char *name;
   
   n = strlen(trry);
   neew = SLmalloc (n + 12);
   if (neew == NULL)
     exit_error ("Out of memory", 0);

   strcpy (neew, trry);

   while (1)
     {
	bnext = b->next;
	while (bnext != b)
	  {
	     if ((bnext->name != NULL)
		 && (0 == strcmp(neew, bnext->name)))
	       break;

	     bnext = bnext->next;
	  }
	
	if (bnext == b)
	  {
	     name = SLang_create_slstring (neew);
	     if (name == NULL)
	       exit_error ("Out of memory", 0);
	     SLang_free_slstring (b->name);
	     b->name = name;
	     break;
	  }

	version++;
	sprintf (neew + n, "<%d>", version);
     }
   SLfree (neew);
}

/*}}}*/

/* make a buffer and insert it in the list */
Buffer *make_buffer (char *name, char *dir, char *file) /*{{{*/
{
   Buffer *newB;
   
   newB = (Buffer *) jed_malloc0 (sizeof(Buffer));
   
   if (newB == NULL)
     exit_error("Out of memory", 0);

   newB->keymap = Global_Map;
   jed_set_buffer_ctime (newB);
   newB->local_vars.tab = Jed_Tab_Default;
   newB->local_vars.case_search = Jed_Case_Search_Default;
   newB->local_vars.wrap_column = Jed_Wrap_Default;

   if (CBuf == NULL)
     {
	newB->next = newB;
	newB->prev = newB;
     }
   else
     {
	newB->next = CBuf;
	newB->prev = CBuf->prev;
	CBuf->prev->next = newB;
	CBuf->prev = newB;
     }
   
#ifdef IBMPC_SYSTEM
   newB->flags |= ADD_CR_ON_WRITE_FLAG;
#endif
   newB->syntax_table = Default_Syntax_Table;

   buffer_filename (newB, dir, file);
   if ((name != NULL)
       || (newB->name == NULL))
     {
	if (name == NULL) name = "*scratch*";
	uniquely_name_buffer (newB, name);
     }

   return (newB);
}

/*}}}*/


/* if there is a window attached to this buffer, then there are problems
 *  if we get to update() without attaching another one to it.  So
 *  beware! Always make sure CBuf is set too!   kill_buffer command
 *  takes care of this */
void delete_buffer(Buffer *buf) /*{{{*/
{
   Line *l,*n;
   Jed_Mark_Array_Type *m, *m1;

   if (1 != buffer_exists (buf))
     return;

   jed_widen_whole_buffer (buf);

   jed_unlock_buffer_file (buf);

   if (buf -> beg != NULL) for (l = buf -> beg; l != NULL; l = n)
     {
	n = l -> next;
	free_line(l);
	/* SLfree( l->data); SLfree( l); */
     }

   m = buf->mark_array;
   while (m != NULL)
     {
	m1 = m->next;
	SLfree ((char *)m);
	m = m1;
     }
   
   m = buf->spot_array;
   while (m != NULL)
     {
	m1 = m->next;
	SLfree ((char *)m);
	m = m1;
     }

   if (buf->user_marks != NULL) free_user_marks (buf);
   
   if (buf->undo != NULL) delete_undo_ring (buf);
   
#if JED_HAS_BUFFER_LOCAL_VARS 
   jed_delete_blocal_vars (buf->blocal_table);
#endif
   
#if JED_HAS_SAVE_NARROW
   while (buf->save_narrow != NULL)
     jed_free_saved_narrow (buf);
#endif
   
#if JED_HAS_COLOR_COLUMNS
   SLfree ((char *)buf->column_colors);
#endif
   buf->prev->next = buf->next;
   buf->next->prev = buf->prev;

#if JED_HAS_MENUS
   jed_delete_menu_bar (buf->menubar);
#endif

#if JED_HAS_SUBPROCESSES
   if (buf->subprocess) jed_kill_process (buf->subprocess - 1);
#endif
   
   SLang_free_slstring (buf->dir);
   SLang_free_slstring (buf->file);
   SLfree (buf->canonical_dirfile);
   SLfree (buf->dirfile);
   SLang_free_slstring (buf->name);
   SLang_free_slstring (buf->mode_string);
   
   delete_buffer_hooks (buf->buffer_hooks);
   SLfree ((char *)buf);
}

/*}}}*/

void goto_line (int *np) /*{{{*/
{
   unsigned int n;
   unsigned int half1 = LineNum / 2;
   unsigned int half2 = (Max_LineNum + LineNum) / 2;
   
   if (*np <= 1) n = 0; else n = (unsigned int) *np;

   if (n < LineNum)
     {
	if (n > half1)
	  jed_up (LineNum - n);
	else
	  {
	     bob();
	     if (n > 0) jed_down (n-1);
	  }
     }
   else if (n < half2)
     jed_down (n - LineNum);
   else
     {
	eob();
	if (n < Max_LineNum) jed_up (Max_LineNum - n);
     }
   bol ();
}

/*}}}*/

Line *dup_line(Line *l) /*{{{*/
{
   Line *neew;
   int n;
   unsigned char *p, *q;
   
#ifdef KEEP_SPACE_INFO
   n = l->space;
#else
   n = l->len;
#endif
   
   neew = (Line *) SLmalloc (sizeof(Line));
   if ((neew == NULL) ||
       (NULL == (neew->data = (unsigned char *) SLmalloc (n))))
     {
	exit_error("Out of memory", 0);
     }
   neew->next = l->next;
   neew->prev = l->prev;
   neew->len = n;
#ifdef KEEP_SPACE_INFO
   neew->space = l->space;
#endif
   p = neew->data;
   q = l->data;
   
   while (n--) *p++ = *q++;
   return(neew);
}

/*}}}*/

Buffer *find_buffer(char *name) /*{{{*/
{
   Buffer *b;
   
   b = CBuf;
   do
     {
	if (!strcmp(b->name, name)) return(b);
	b = b->next;
     }
   while(b != CBuf);
   
#if !JED_FILE_PRESERVE_CASE
   b = CBuf;
   do
     {
	if (!jed_case_strcmp (b->name, name)) return(b);
	b = b->next;
     }
   while(b != CBuf);
#endif
   return(NULL);
}

/*}}}*/

int buffer_exists(Buffer *b) /*{{{*/
{
   Buffer *c = CBuf;

   if (b == NULL)
     return 0;

   do
     {
	if (b == c) return 1;
	c = c->next;
     }
   while (c != CBuf);
   return 0;
}

/*}}}*/

int switch_to_buffer(Buffer *buf) /*{{{*/
{
   /*  save this buffer position */
   CBuf->line = CLine;
   CBuf->point = Point;
   CBuf->linenum = LineNum;
   CBuf->max_linenum = Max_LineNum;
   
   /* local variables */
   SLMEMCPY ((char *) &CBuf->local_vars, (char *) &Buffer_Local, sizeof (Buffer_Local_Type));
   
   if (buf == CBuf) return(0);
   
#if 0
   buf->prev->next = buf->next;
   buf->next->prev = buf->prev;
   
   buf->next = CBuf;
   buf->prev = CBuf->prev;
   CBuf->prev->next = buf;
   CBuf->prev = buf;
#endif
   
   /* now restore new buffer */
   CBuf = buf;
   CLine = CBuf->line;
   Point = CBuf->point;
   
   /* Buffer local variables */
   SLMEMCPY ((char *) &Buffer_Local, (char *) &CBuf->local_vars, sizeof (Buffer_Local_Type));
	   
   LineNum = CBuf->linenum;
   Max_LineNum = CBuf->max_linenum;
   
   if (CLine == NULL)
     {
	make_line(25);
	Point = 0;
     }
   CBuf->line = CLine;
   CBuf->point = Point;
   jed_set_umask (CBuf->umask);
   return(1);
}

/*}}}*/

#ifdef REAL_UNIX_SYSTEM
static Buffer *find_file_buffer_via_inode (char *file)
{
   int device, inode;
   Buffer *b;

   if (-1 == jed_get_inode_info (file, &device, &inode))
     return NULL;
   
   if ((device == -1) || (inode == -1))
     return NULL;

   b = CBuf;
   do
     {
	if ((inode == b->inode) && (device == b->device))
	  return b;

	b = b->next;
     }
   while (b != CBuf);
   
   return NULL;
}
#endif

Buffer *find_file_buffer(char *file) /*{{{*/
{
   Buffer *b;

   file = jed_get_canonical_pathname (file);
   if (file == NULL)
     return NULL;

   b = CBuf;
   do
     {
	if ((b->canonical_dirfile != NULL)
# if JED_FILE_PRESERVE_CASE
	    && (0 == strcmp(b->canonical_dirfile, file))
# else
	    && (0 == jed_case_strcmp (b->canonical_dirfile, file))
# endif
	    )
	  {
	     SLfree (file);
	     return b;
	  }
	b = b->next;
     }
   while (b != CBuf);

# ifdef REAL_UNIX_SYSTEM
   if (NULL != (b = find_file_buffer_via_inode (file)))
     {
	if (0 != strcmp (file, b->canonical_dirfile))
	  jed_vmessage (0, "%s and %s are the same", file, b->canonical_dirfile);
	SLfree (file);
	return b;
     }
# endif
   
   SLfree (file);
   return NULL;
}
/*}}}*/

/* take a dir and a filename, expand them then put in buffer structure */
void buffer_filename(Buffer *b, SLFUTURE_CONST char *dir, SLFUTURE_CONST char *file) /*{{{*/
{
   unsigned int dirlen;
   char *dirfile;
   char *canonical_dirfile;

   if (dir == NULL)
     dir = jed_get_cwd ();

   if (file == NULL)
     {
	file = extract_file (dir);
	dirlen = (unsigned int) (file - dir);
     }
   else
     dirlen = strlen (dir);
   
   dirfile = jed_dir_file_merge (dir, file);
   if (dirfile != NULL)
     canonical_dirfile = jed_get_canonical_pathname (dirfile);
   else 
     canonical_dirfile = NULL;

   SLfree (b->dirfile);
   SLfree (b->canonical_dirfile);
   SLang_free_slstring (b->file);
   SLang_free_slstring (b->dir);
   
   if ((NULL == (b->dirfile = dirfile))
       || (NULL == (b->canonical_dirfile = canonical_dirfile))
       || (NULL == (b->file = SLang_create_slstring (file)))
       || (NULL == (b->dir = SLang_create_nslstring (dir, dirlen))))
     {
	exit_error ("Out of memory", 0);
     }

#ifdef REAL_UNIX_SYSTEM
   jed_get_inode_info (b->canonical_dirfile, &b->device, &b->inode);
#endif

   if (*file)
     {
	uniquely_name_buffer (b, file);
	jed_set_buffer_ctime (b);
     }
}

/*}}}*/


/* This kills all undo information! */
int erase_buffer (void) /*{{{*/
{
   Line *beg, *tthis;

   if (-1 == jed_prepare_for_modification (0))
     return -1;

   /* CLine = CBuf->end; */
   beg = CBuf->beg;
   bob();
   CLine = CLine->next; LineNum++;
   Suspend_Screen_Update = 1;
   while (CLine != NULL)
     {
	tthis = CLine;
	beg->next = tthis->next;
	tthis->prev = beg;
	
	jed_update_marks(LDELETE, 1);
	CLine = tthis->next;
	Max_LineNum--;
	free_line(tthis);
     }
   
   CLine = CBuf->beg; LineNum = 1;
   Point = 0;
   jed_update_marks(CDELETE, CLine->len);
   CLine->len = 0;
   /*   CLine->next = NULL; */
   if (CBuf->undo != NULL) delete_undo_ring(CBuf);
   CBuf->undo = NULL;
   CBuf->end = CLine;
   touch_screen_for_buffer(CBuf);
   return(1);
}

/*}}}*/

void mark_buffer_modified (Buffer *b, int modif, int with_lock) /*{{{*/
{
   if (modif)
     {
	if (b->flags & BUFFER_MODIFIED) return;
#if 0
	b->m_time = sys_time();
#endif
	b->flags |= BUFFER_MODIFIED;
	if (with_lock)
	  (void) jed_lock_buffer_file (b);
	return;
     }

   b->flags &= ~BUFFER_MODIFIED;
   if (with_lock)
     jed_unlock_buffer_file (b);
}

/*}}}*/

void jed_set_buffer_flags (Buffer *b, unsigned int flags)
{
   unsigned int modif;

#if 1
   if (((flags & FILE_MODIFIED) == 0)
       && (b->flags & FILE_MODIFIED))
     jed_set_buffer_ctime (b);
#endif

   modif = flags & BUFFER_MODIFIED;
   b->flags &= BUFFER_MODIFIED;
   b->flags |= flags & ~BUFFER_MODIFIED;
   mark_buffer_modified (b, (int) modif, 1);
#if 0
   if ((b->flags & BUFFER_MODIFIED) != (flags & BUFFER_MODIFIED))
     mark_buffer_modified (b, flags & BUFFER_MODIFIED, 1);

   b->flags = flags;
#endif
}

void check_line() /*{{{*/
{
   if ((CLine->len == 1) && (*CLine->data == '\n') && (CLine->data != NewLine_Buffer))
     {
	SLfree ((char *)CLine->data);
	CLine->data = NewLine_Buffer;
#ifdef KEEP_SPACE_INFO
	CLine->space = 1;
#endif
     }
}

/*}}}*/


static SLang_Name_Type **find_buffer_hook (Jed_Buffer_Hook_Type *b, char *s)
{
   if (b == NULL)
     return NULL;

   if (0 == strcmp("par_sep", s)) return &b->par_sep;
   if (0 == strcmp("format_paragraph_hook", s)) return &b->format_paragraph_hook;
   if (0 == strcmp("mark_paragraph_hook", s)) return &b->mark_paragraph_hook;
   if (0 == strcmp("forward_paragraph_hook", s)) return &b->forward_paragraph_hook;
   if (0 == strcmp("backward_paragraph_hook", s)) return &b->backward_paragraph_hook;
   if (0 == strcmp("indent_hook", s)) return &b->indent_hook;
   if (0 == strcmp("wrap_hook", s)) return &b->wrap_hook;
   if (0 == strcmp("wrapok_hook", s)) return &b->wrapok_hook;
   if (0 == strcmp("newline_indent_hook", s)) return &b->newline_indent_hook;
   if (0 == strcmp("bob_eob_error_hook", s)) return &b->bob_eob_error_hook;
#ifdef HAS_MOUSE
   if (0 == strcmp("mouse_down", s)) return &b->mouse_down_hook;
   if (0 == strcmp("mouse_up", s)) return &b->mouse_up_hook;
   if (0 == strcmp("mouse_drag", s)) return &b->mouse_drag_hook;
# if JED_HAS_MULTICLICK
   if (0 == strcmp("mouse_2click", s)) return &b->mouse_2click_hook;
   if (0 == strcmp("mouse_3click", s)) return &b->mouse_3click_hook;
# endif
#endif
   if (0 == strcmp("update_hook", s)) return &b->update_hook;
   if (0 == strcmp("color_region_hook", s)) return &b->color_region_hook;

   return NULL;
}


int jed_unset_buffer_hook (Buffer *buffer, char *name)
{
   Jed_Buffer_Hook_Type *b;

   if ((buffer == NULL)
       || (NULL == (b = buffer->buffer_hooks)))
     return -1;

   if ((name != NULL) && (*name != 0))
     {
	SLang_Name_Type **nt = find_buffer_hook (b, name);
	if (nt == NULL)
	  return -1;
	
	*nt = NULL;
	return 0;
     }
   
   memset ((char *)b, 0, sizeof (Jed_Buffer_Hook_Type));
   return 0;
}

int jed_set_buffer_hook (Buffer *buffer, char *name, SLang_Name_Type *nt)
{
   SLang_Name_Type **ntp;
   Jed_Buffer_Hook_Type *b;
   
   if (buffer == NULL)
     return -1;
   
   b = buffer->buffer_hooks;

   if (b == NULL)
     {
	b = (Jed_Buffer_Hook_Type *)SLmalloc (sizeof (Jed_Buffer_Hook_Type));
	if (b == NULL)
	  return -1;
	memset ((char *) b, 0, sizeof (Jed_Buffer_Hook_Type));
	buffer->buffer_hooks = b;
     }
   
   ntp = find_buffer_hook (b, name);
   if (ntp == NULL)
     return -1;
   
   SLang_free_function (*ntp);
   *ntp = nt;
   return 0;
}

SLang_Name_Type *jed_get_buffer_hook (Buffer *buffer, char *name)
{
   SLang_Name_Type **ntp;
   Jed_Buffer_Hook_Type *b;

   if (buffer == NULL)
     return NULL;

   b = buffer->buffer_hooks;

   if ((b == NULL)
       || (NULL == (ntp = find_buffer_hook (b, name)))
       || (*ntp == NULL))
     return NULL;

   return SLang_copy_function (*ntp);
}

static void delete_buffer_hooks (Jed_Buffer_Hook_Type *h)
{
   if (h == NULL)
     return;

   if (h->mark_paragraph_hook != NULL) SLang_free_function (h->mark_paragraph_hook);
   if (h->format_paragraph_hook != NULL) SLang_free_function (h->format_paragraph_hook);
   if (h->forward_paragraph_hook != NULL) SLang_free_function (h->forward_paragraph_hook);
   if (h->backward_paragraph_hook != NULL) SLang_free_function (h->backward_paragraph_hook);
   if (h->par_sep != NULL) SLang_free_function (h->par_sep);
   if (h->indent_hook != NULL) SLang_free_function (h->indent_hook);
   if (h->newline_indent_hook != NULL) SLang_free_function (h->newline_indent_hook);
   if (h->wrap_hook != NULL) SLang_free_function (h->wrap_hook);
   if (h->wrapok_hook != NULL) SLang_free_function (h->wrapok_hook);
   if (h->bob_eob_error_hook != NULL) SLang_free_function (h->bob_eob_error_hook);
#ifdef HAS_MOUSE
   if (h->mouse_down_hook != NULL) SLang_free_function (h->mouse_down_hook);
   if (h->mouse_up_hook != NULL) SLang_free_function (h->mouse_up_hook);
   if (h->mouse_drag_hook != NULL) SLang_free_function (h->mouse_drag_hook);
# if JED_HAS_MULTICLICK
   if (h->mouse_2click_hook != NULL) SLang_free_function (h->mouse_2click_hook);
   if (h->mouse_3click_hook != NULL) SLang_free_function (h->mouse_3click_hook);
# endif
#endif
   if (h->update_hook != NULL) SLang_free_function (h->update_hook);
   if (h->color_region_hook != NULL) SLang_free_function (h->color_region_hook);

   SLfree ((char *) h);
}
