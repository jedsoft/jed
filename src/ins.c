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
#include <string.h>

#include "buffer.h"
#include "ins.h"
#include "line.h"
#include "screen.h"
#include "window.h"
#include "misc.h"
#include "paste.h"
#include "ledit.h"
#include "undo.h"
#include "file.h"

/*}}}*/

int Suspend_Screen_Update = 0;
int No_Screen_Update;

static void cinsert_update_marks (Mark *m, unsigned int linenum, int n)
{
   (void) linenum;

   while (m != NULL)
     {
	if ((m->line == CLine) && (m->point > Point))
	  m->point += n;
	m = m->next;
     }
}

static void cdelete_update_marks (Mark *m, unsigned int linenum, int n)
{
   (void) linenum;
   while(m != NULL)
     {
	if ((m->line == CLine) && (m->point > Point))
	  {
	     int tmp;

	     /* BCC generates wrong code here with optimization.  So use a
	      * silly tmp variable as a way around the bug.
	      */
	     tmp = m->point;
	     tmp -= n;
	     if (tmp < Point) tmp = Point;
	     m->point = tmp;
	     
	     /*
	      BAD CODE:
	     m->point -= n;
	     if (m->point < Point) m->point = Point; */
	  }
	m = m->next;
     }
}

static void ldelete_update_marks (Mark *m, unsigned int linenum, int n)
{
   (void) n;
   while (m != NULL)
     {
	if (CLine == m->line)
	  {
	     if (CLine->prev != NULL)
	       {
		  m->line = CLine->prev;
	       }
	     else m->line = CBuf->beg;
	     m->point = 0;
	  }
	if (linenum <= m->n) m->n -= 1;
	m = m->next;
     }
}

static void nldelete_update_marks (Mark *m, unsigned int linenum, int n)
{
   /* deletion performed at end of a line (CLine->prev)  */
   
   (void) n;
   while (m != NULL)
      {
	 if (m->line == CLine)
	   {
	      m->line = CLine->prev;
	      m->point += Point;
	   }
	 if (linenum <= m->n) m->n -= 1;
	 m = m->next;
      }
}


static void nlinsert_update_marks (Mark *m, unsigned int linenum, int n)
{
   /* newline added-- affects only marks onward from insertion point */
   (void) n;
   while (m != NULL)
      {
	 /* This is a bit controversial if the mark corresponds to JWindow->beg.
	    In this case, JWindow beg gets shifted if Point = 0.  */

	 if ((linenum < m->n) 
	     || ((linenum == m->n) && (m->point > Point))) m->n += 1;
	    
	 if ((m->line == CLine) && (m->point > Point))
            {
	       m->line = CLine->next;
	       m->point -= Point;
	       if (m->point > m->line->len) m->point = m->line->len;
            }
	 m = m->next;
      }
}


void jed_update_marks (int type, int n) /*{{{*/
{
   register Window_Type *w;
   register Buffer *b = CBuf;
   Mark *m;
#if JED_HAS_SAVE_NARROW
   Jed_Save_Narrow_Type *save_narrow;
#endif
   void (*update_marks_fun) (Mark *, unsigned int, int);
   unsigned int line_num;

   if (!n) return;
   
   switch (type)
     {
      case CINSERT:
	update_marks_fun = cinsert_update_marks;
	break;
	
      case CDELETE:
	update_marks_fun = cdelete_update_marks;
	break;
	
      case LDELETE:
	update_marks_fun = ldelete_update_marks;
	break;
	
      case NLINSERT:
	update_marks_fun = nlinsert_update_marks;
	break;
	
      case NLDELETE:
	update_marks_fun = nldelete_update_marks;
	break;
	
      default:
	update_marks_fun = NULL;       /* crash.  I want to know about this */
     }

   Cursor_Motion = 0;

   if (b->flags & UNDO_ENABLED)
     {
	if (b->undo == NULL) create_undo_ring();
	Undo_Buf_Unch_Flag = !(b->flags & BUFFER_MODIFIED);
     }
   
   mark_buffer_modified (b, 1, 0);

   line_num = LineNum + b->nup;
#if JED_HAS_LINE_ATTRIBUTES
   if ((b->min_unparsed_line_num == 0)
       || (b->min_unparsed_line_num > line_num))
     b->min_unparsed_line_num = line_num;

   if ((b->max_unparsed_line_num == 0)
       || (b->max_unparsed_line_num < line_num))
     b->max_unparsed_line_num = line_num;
#endif
   if ((m = b->spots) != NULL) (*update_marks_fun)(m, line_num, n);
   if ((m = b->marks) != NULL) (*update_marks_fun)(m, line_num, n);
   if ((m = b->user_marks) != NULL) (*update_marks_fun)(m, line_num, n);

#if JED_HAS_SAVE_NARROW
   save_narrow = b->save_narrow;
   while (save_narrow != NULL)
     {
	(*update_marks_fun) (save_narrow->beg, line_num, n);
	(*update_marks_fun) (save_narrow->end, line_num, n);
	save_narrow = save_narrow->next;
     }
#endif
   
   w = JWindow;
   do
     {
	if (w->buffer == b)
	  {
	     (*update_marks_fun) (&w->mark, line_num, n);
	     (*update_marks_fun) (&w->beg, line_num, n);
	  }

	w = w->next;
     }
   while (w != JWindow);

   if (!Suspend_Screen_Update) register_change(type);
}

/*}}}*/

int jed_prepare_for_modification (int check_line_readonly)
{
   if (CBuf->flags & READ_ONLY)
     {
	jed_verror ("Buffer %s is read-only", CBuf->name);
	return -1;
     }

#if JED_HAS_LINE_ATTRIBUTES
   if (check_line_readonly
       && (CLine->flags & JED_LINE_IS_READONLY))
     {
	jed_verror ("This line is read-only");
	return -1;
     }
#endif

   if (CBuf->flags & BUFFER_MODIFIED)
     return 0;
   
   if (0 == CBuf->file[0])
     return 0;			       /* not attached to a file */
   
   /* if ((SLang_get_error () == 0) */
   if (0 == (CBuf->flags & BUFFER_NON_LOCKING))
     {
	if (0 == (CBuf->flags & FILE_MODIFIED))
	  check_buffer (CBuf);
	if (CBuf->flags & FILE_MODIFIED)
	  {
	     if (1 != jed_get_y_n ("File changed on disk.  Do you really want to edit this buffer"))
	       return -1;
	  }
#if 0  /* If the buffer has had the read_only flag unset, then assume the user
	* knows what he/she is doing.
	*/
	if (1 == jed_buffer_file_is_readonly (CBuf))
	  {
	     if (1 != jed_get_y_n ("Disk file is read-only.  Do you really want to edit this buffer"))
	       {
		  CBuf->flags |= READ_ONLY;
		  return -1;
	       }
	  }
#endif

	if ((-1 == jed_lock_buffer_file (CBuf))
	    && SLang_get_error ())
	  return -1;
     }
#if 0
   if (SLang_get_error ())
     return -1;
#endif
   return 0;
}

/* This function does not handle newlines, that is, it does not split the
 * line if one is inserted.
 */
int _jed_ins_byte (unsigned char c) /*{{{*/
{
   register unsigned char *p, *p1, *p2;

   if (-1 == jed_prepare_for_modification (0))
     return -1;

#ifdef KEEP_SPACE_INFO
   if (CLine->space <= CLine->len + 1)
     remake_line(CLine->space + 15);
#else
   remake_line (CLine->len + 1);
#endif

   p = CLine->data + Point;
   if (Point < CLine->len)
     {
	p1 = CLine->data + (CLine->len - 1);
	p2 = p1 + 1;
	while(p1 >= p)
	  {
	     *p2 = *p1;
	     p2 = p1;
	     p1--;
	     /* *(p1 + 1) = *p1; p1--; */
	  }
     }
   *p = c;
   CLine->len += 1;
   jed_update_marks(CINSERT,1);
   if ((c != '\n') || (CBuf == MiniBuffer)) record_insertion(1);
   Point++;
   
   return 0;
}

/*}}}*/

int jed_del_newline(void) /*{{{*/
{
   if (-1 == jed_prepare_for_modification (1))
     return -1;

#if JED_HAS_LINE_ATTRIBUTES
   if ((CLine->next != NULL)
       && (CLine->len > 1)
       && (CLine->next->flags & JED_LINE_IS_READONLY))
     {
	msg_error (Line_Read_Only_Error);
	return -1;
     }
#endif

   if (!eol() || eobp()) return -1;
   
   CLine->len -= 1;
   jed_update_marks(CDELETE,1);
   record_deletion((unsigned char *) "\n", 1);
   splice_line();
   return 0;
}

/*}}}*/

/* del *np chars up until newline.  Return actual number deleted. */
int jed_del_nbytes (int n) /*{{{*/
{
   register int nn;
   register unsigned char *p, *pmax;

   if ((n == 0) || !CLine->len) return 0;

   nn = CLine->len - 1;
   p = CLine->data + nn;
   if ((*p == '\n') && (CBuf != MiniBuffer)) nn = nn - Point; else nn = nn - Point + 1;
   
   p = CLine->data + Point;

   nn = nn > n ? n : nn;
   if (!nn) return (0);

   if (-1 == jed_prepare_for_modification (1))
     return -1;

   jed_update_marks(CDELETE, nn);
   record_deletion(p, nn);
   CLine->len -= nn;
   pmax = CLine->data + CLine->len;
   while (p < pmax)
     {
	*p = *(p + nn);
	p++;
     }
   return nn;
}

/*}}}*/

/* delete n characters, crossing nl if necessary */
int jed_generic_del_nbytes (int n) /*{{{*/
{
   /* while ((n > 0) && (SLang_get_error () == 0)) */
   while (n > 0)
     {
	int dn;

	if (eobp())
	  {
	     msg_error("End of Buffer.");
	     return -1;
	  }
	
	dn = jed_del_nbytes (n);
	if (dn == -1)
	  return -1;
	
	n -= dn;

	if (n && (-1 == jed_del_newline()))
	  return -1;

	n--;
     }
   return 0;
}

/*}}}*/

int jed_del_through_eol (void)
{
   return jed_generic_del_nbytes (CLine->len - Point);
}

int jed_insert_newline (void)
{
#if JED_HAS_LINE_ATTRIBUTES
   unsigned int flags = CLine->flags;
#endif
   if (-1 == jed_prepare_for_modification (Point != 0))
     return -1;

   split_line();
#if JED_HAS_LINE_ATTRIBUTES
   if (Point == 0)
     CLine->flags = 0;
#endif
   if (-1 == _jed_ins_byte ('\n'))
     return -1;
   (void) jed_down (1);
#if JED_HAS_LINE_ATTRIBUTES
   CLine->flags = flags;
#endif
   return 0;
}
   

/* MULTIBYTE OK */
int jed_del_wchar (void)
{
   unsigned char *p, *pmax;

   if (eolp ())
     return jed_del_through_eol ();
   
   p = CLine->data + Point;
   pmax = jed_multibyte_chars_forward (p, CLine->data + CLine->len, 1, NULL, 1);
   return jed_generic_del_nbytes (pmax - p);
}

int jed_quick_insert(register unsigned char *s, int n) /*{{{*/
{
   register unsigned char *p, *p1;
   int nl = 0;
   
   if (n == 0) return 0;

   if (-1 == jed_prepare_for_modification (0))
     return -1;
   
#if JED_HAS_LINE_ATTRIBUTES
   if ((CLine->flags & JED_LINE_IS_READONLY)
       && ((Point != 0) || (s[n-1] != '\n')))
     {
	jed_verror (Line_Read_Only_Error);
	return -1;
     }
#endif

   if ((*(s + (n - 1)) == '\n') && (CBuf != MiniBuffer))
     {
	n--;
	nl = 1;

	if (-1 == jed_insert_newline ())
	  return -1;
	(void) jed_up(1);
     }
   
#ifdef KEEP_SPACE_INFO
   if (CLine->space <= CLine->len + n + 1) remake_line(CLine->space + n + 8);
#else
   if (n) remake_line (CLine->len + n);
#endif

   if (n)
     {
	/* shove n chars over to make space */
	p = CLine->data + Point;
	if (Point < CLine->len)   /* could be equal for last line of buffer */
	  {
	     p1 = CLine->data + CLine->len - 1;
	     while(p1 >= p)
	       {
		  *(p1 + n) = *p1;
		  p1--;
	       }
	  }
	CLine->len += n;
	SLMEMCPY((char *) p, (char *) s, n);
   
	jed_update_marks(CINSERT, n);
	record_insertion(n);
	Point += n;
     }

   if (nl) 
     jed_down (1);

   return 0;
}

/*}}}*/

int jed_insert_nbytes (unsigned char *ss, int n) /*{{{*/
{
   register unsigned char nl, *pmax;
   register unsigned char *p, *p1, *s = ss;
   int n1;

   if (CBuf == MiniBuffer) nl = 0; else nl = '\n';
   
   p1 = s;
   while (1)
     {
	p = p1;
	/* count the number until a new line is reached */
	pmax = p1 + n;
	while((p1 < pmax) && (*p1 != nl)) p1++;
	
	n1 = (int) (p1 - p);
	if (p1 != pmax) n1++;
	if (-1 == jed_quick_insert(p, n1))
	  return -1;
	if (p1++ == pmax) return 0;
	n -= n1;
     }
}

/*}}}*/

/* Multibyte-safe */
int jed_insert_wchar_n_times (SLwchar_Type c, unsigned int n) /*{{{*/
{
   unsigned char wchar_buf [JED_MAX_MULTIBYTE_SIZE];
   unsigned char buf[20*JED_MAX_MULTIBYTE_SIZE];
   unsigned char *pmax;
   unsigned int len;

   if (n == 0)
     return 0;

   if (NULL == (pmax = jed_wchar_to_multibyte (c, wchar_buf)))
     return -1;

   len = pmax - wchar_buf;
   pmax = buf + (sizeof (buf) - len);

   while (n)
     {
	unsigned char *p = buf;

	while ((p < pmax) && n)
	  {
	     memcpy (p, wchar_buf, len);
	     n--;
	     p += len;
	  }
	if (-1 == jed_insert_nbytes (buf, p - buf))
	  return -1;
     }
   return 0;
}

/*}}}*/

void insert_buffer(Buffer *b) /*{{{*/
{
   Buffer *cb;

   if ((cb = CBuf) == b) return;

   switch_to_buffer(b);
   push_spot();
   bob(); jed_push_mark();
   eob();
   copy_region_to_buffer(cb);
   pop_spot();
   switch_to_buffer(cb);

   touch_window();
}

/*}}}*/

/* Multibyte safe */
int _jed_replace_wchar (SLwchar_Type ch) /*{{{*/
{
   unsigned char buf[JED_MAX_MULTIBYTE_SIZE];
   unsigned char *b, *bmax;
   unsigned char *p, *pmax;
   
   if (ch == '\n') return -1;

   b = buf;
   bmax = jed_wchar_to_multibyte (ch, buf);
   if (bmax == NULL)
     return -1;

   p = CLine->data + Point;
   pmax = CLine->data + CLine->len;
   
   while ((p < pmax) && (b < bmax) && (*b == *p))
     {
	b++;
	p++;
     }
   if (b == bmax)
     return 0;

   if (-1 == jed_insert_nbytes (buf, bmax - buf))
     return -1;

   if (-1 == jed_del_wchar ())
     return -1;

   (void) jed_left(1);
   return 0;
}

/*}}}*/

int jed_insert_string (SLFUTURE_CONST char *s)
{
   return jed_insert_nbytes ((unsigned char *) s, strlen (s));
}

int jed_insert_byte (unsigned char ch)
{
   return jed_insert_nbytes (&ch, 1);
}

int jed_insert_wchar (SLwchar_Type ch)
{
   unsigned char buf[JED_MAX_MULTIBYTE_SIZE];
   unsigned char *b;

   if (NULL == (b = jed_wchar_to_multibyte (ch, buf)))
     return -1;

   return jed_insert_nbytes (buf, b-buf);
}
