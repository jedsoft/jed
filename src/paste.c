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
#include "ins.h"
#include "line.h"
#include "paste.h"
#include "screen.h"
#include "misc.h"
#include "cmds.h"

/*}}}*/

/* SLang user object be larger than 128 */
#define JED_MARK_TYPE 128

Buffer *Paste_Buffer;
static Buffer *Rectangle_Buffer;

/* This is used by the narrow command so that multiple windows are
 * handled properly. The assumption is that we are dealing with a canonical
 * region */

static void touch_windows(void) /*{{{*/
{
   Window_Type *w;
   Line *l;
   unsigned int n;

   w = JWindow;
   JWindow = JWindow->next;
   while (JWindow != w)
     {
	if (CBuf == JWindow->buffer)
	  {
	     /* The mark is set with line at top of buffer */
	     l = CBuf->marks->line;
	     n = CBuf->marks->n;
	     while ((l != NULL) && (l != JWindow->mark.line)) l = l->next, n++;
	     if (l == NULL)
	       jed_init_mark (&JWindow->mark, 0);
	     touch_window();
	  }
	JWindow = JWindow->next;
     }
   jed_pop_mark(0);
}

/*}}}*/

#if 0
static int line_in_buffer(Line *line) /*{{{*/
{
   Line *l;

   l = CBuf->beg;
   while (l != NULL) if (l == line) return(1); else l = l->next;
   return(0);
}

/*}}}*/
#endif

/*{{{ Spot/Mark Functions */

void jed_copy_mark (Mark *dest, Mark *src)
{
   *dest = *src;
   dest->next = NULL;
}

void jed_init_mark (Mark *m, unsigned int flags) /*{{{*/
{
   m->line = CLine;
   m->point = Point;
   m->n = LineNum + CBuf->nup;
   m->flags = flags;
   /* m->next = NULL; */
}

/*}}}*/

void jed_init_mark_for_buffer (Mark *m, Buffer *b, unsigned int flags) /*{{{*/
{
   m->line = b->line;
   m->point = b->point;
   m->n = b->linenum + b->nup;
   m->flags = flags;
   /* m->next = NULL; */
}

/*}}}*/
#if 0
int jed_init_mark_for_line (Mark *m, Line *l, unsigned int flags) /*{{{*/
{
   Line *ll;
   unsigned int n;

   n = 1;
   ll = CBuf->beg;
   while ((ll != NULL) && (ll != l))
     {
	ll = ll->next;
	n++;
     }
   if (ll == NULL)
     return -1;

   m->line = l;
   m->point = 0;
   m->n = n + CBuf->nup;
   m->flags = flags;
   /* m->next = NULL; */
   return 0;
}

/*}}}*/
#endif
#if JED_HAS_SAVE_NARROW
static Mark *create_mark (unsigned int flags) /*{{{*/
{
   Mark *m;

   if (NULL == (m = (Mark *) jed_malloc0 (sizeof(Mark))))
     exit_error("create-mark: malloc error", 0);

   jed_init_mark (m, flags);

   return m;
}

/*}}}*/
#endif
/* with prefix argument, pop marks */
int set_mark_cmd (void) /*{{{*/
{
   Mark *m;
   if (Repeat_Factor != NULL)
     {
	while (CBuf->marks != NULL) jed_pop_mark(0);
	Repeat_Factor = NULL;
	return(1);
     }

   if (CBuf->marks == NULL)
     {
	jed_push_mark ();
     }

   m = CBuf->marks;

   jed_init_mark (m, m->flags);

   if ((m->flags & VISIBLE_MARK) == 0)
     {
	m->flags |= VISIBLE_MARK;
	CBuf->vis_marks++;
     }

   /* if (m != CBuf->marks) m->next = CBuf->marks;
    CBuf->marks = m; */
   if (Last_Key_Function == (FVOID_STAR) set_mark_cmd) message("Mark Set.");
   return(1);
}

/*}}}*/

static Mark *allocate_mark (Jed_Mark_Array_Type **ap, Mark **mp, unsigned int flags) /*{{{*/
{
   Jed_Mark_Array_Type *a, *b;
   Mark *m;

   a = *ap;
   if ((a == NULL)
       || (a->num_marks == JED_MAX_MARK_ARRAY_SIZE))
     {
	if (NULL == (b = (Jed_Mark_Array_Type *) jed_malloc0 (sizeof(Jed_Mark_Array_Type))))
	  {
	     exit_error("allocate_mark: malloc error", 0);
	  }
	b->next = a;
	a = b;
	*ap = a;
     }

   m = &a->marks[a->num_marks];
   jed_init_mark (m, flags);

   a->num_marks += 1;
   m->next = *mp;
   *mp = m;
   return m;
}

/*}}}*/

static void deallocate_mark (Jed_Mark_Array_Type **ap, Mark **mp) /*{{{*/
{
   Jed_Mark_Array_Type *a;
   Mark *m;

   m = *mp;
   *mp = m->next;

   a = *ap;
   a->num_marks -= 1;

   if (a->num_marks == 0)
     {
	*ap = a->next;
	SLfree ((char *)a);
     }
}

/*}}}*/

int push_spot() /*{{{*/
{
   (void) allocate_mark (&CBuf->spot_array, &CBuf->spots, 0);
   return 1;
}

/*}}}*/

int jed_push_mark() /*{{{*/
{
   (void) allocate_mark (&CBuf->mark_array, &CBuf->marks, 0);
   return 1;
}

/*}}}*/

int jed_goto_mark(Mark *m) /*{{{*/
{
   Line *l;
   int ret = -1;

   l = m->line;
   LineNum = m->n;

   if (LineNum <= CBuf->nup) bob();
   else if (LineNum > CBuf->nup + Max_LineNum) eob();
   else
     {
	CLine = l;
	Point = m->point;
	LineNum -= CBuf->nup;
	ret = 0;
     }
   return ret;
}

/*}}}*/

int jed_pop_mark (int go) /*{{{*/
{
   Mark *m;

   m = CBuf->marks;
   if (m == NULL) return(0);

   if (go) jed_goto_mark(m);
   if (m->flags & VISIBLE_MARK_MASK)
     {
	CBuf->vis_marks--;
	/* touch screen since region may be highlighted */
	if (CBuf->vis_marks == 0) touch_screen();
     }

   deallocate_mark (&CBuf->mark_array, &CBuf->marks);

   return 1;
}

/*}}}*/

int mark_spot() /*{{{*/
{
    push_spot();
    message("Spot Marked.");
    return(1);
}

/*}}}*/

static int pop_spot_go (int go) /*{{{*/
{
   Mark *m;

   m = CBuf->spots;
   if (m == NULL) return(0);

   if (go) jed_goto_mark (m);

   deallocate_mark (&CBuf->spot_array, &CBuf->spots);
   return 1;
}

/*}}}*/

int pop_spot () /*{{{*/
{
   return pop_spot_go (1);
}

/*}}}*/

int exchange_point_mark(void) /*{{{*/
{
   Line *save_line;
   int save_point;
   unsigned int save_n;
   Mark *m;

   if ((m = CBuf->marks) == NULL) return(0);

   save_point = Point;
   save_line = CLine;
   save_n = LineNum + CBuf->nup;

   jed_goto_mark (m);

   m->point = save_point; m->line = save_line; m->n = save_n;
   return(1);
}

/*}}}*/

/*}}}*/

/*{{{ Narrow/Widen/Region Functions */

 /*returns 0 if the mark is not set and gives error.  Exchanges point and mark
  * to produce valid region.  A valid region is one with mark
  * earlier in the buffer than point.  Always call this if using a region
  * which reqires point > mark.  Also, push spot first then pop at end.
  */
int check_region(int *push) /*{{{*/
{
   register Line *beg, *tthis = CLine;
   int pbeg;

   if (CBuf->marks == NULL)
     {
	msg_error("No region defined");
	return(0);
     }

   if (*push) push_spot();
   beg = CBuf->marks->line;
   pbeg = CBuf->marks->point;

   if (beg == CLine)
     {
	if (pbeg <= Point) return(1);
     }

   else
     {
	while((beg != NULL) && (beg != tthis)) beg = beg->next;
	if (beg == tthis) return(1);
     }

   exchange_point_mark();
   return(1);
}

/*}}}*/

static int widen_buffer_lines (Buffer *b) /*{{{*/
{
   Narrow_Type *n;
   Buffer *save = CBuf;

   if (NULL == (n = b->narrow)) return(0);

   /* make sure buffer ends in final newline */

   switch_to_buffer(b);
   push_spot();
   eob();
   if ((n->end != NULL)
       && (0 == LINE_HAS_NEWLINE (CLine)))
     {
	/* This is a hack to avoid messing with the lock file. Yuk. */
	unsigned int flags = CBuf->flags;
	CBuf->flags &= ~(READ_ONLY);
	CBuf->flags |= BUFFER_NON_LOCKING;
	(void) _jed_ins_byte ('\n');
	CBuf->flags = flags;  /* jed_set_buffer_flags (CBuf, flags); */
     }

   pop_spot();

   if (n->end != NULL) n->end->prev = b->end;
   if (n->beg != NULL) n->beg->next = b->beg;
   b->end->next = n->end;
   b->beg->prev = n->beg;
   b->beg = n->beg1;
   if (n->end != NULL) b->end = n->end1;

   Max_LineNum += n->ndown + n->nup;
   LineNum += n->nup;

   /* adjust absolute offsets */
   b->nup -= n->nup;
   b->ndown -= n->ndown;
   b->narrow = n->next;

   /* mark_undo_boundary (b); */

   SLfree ((char *)n);
   switch_to_buffer(save);
   return(1);
}

/*}}}*/

int widen_buffer (Buffer *b) /*{{{*/
{
   unsigned int flags;
#if JED_HAS_LINE_ATTRIBUTES
   unsigned int line_flags;
#endif
   Buffer *save;

   if (b->narrow == NULL) return 0;

   if (b->narrow->is_region == 0)
     return widen_buffer_lines (b);

   flags = b->flags;
   b->flags &= ~(READ_ONLY);

   /* A hack to avoid locking file */
   b->flags |= BUFFER_NON_LOCKING;

   save = CBuf;
   if (b != CBuf) switch_to_buffer (b);
   push_spot ();
   bob ();
   push_spot ();
   eob ();
   widen_buffer_lines (CBuf);

#if JED_HAS_LINE_ATTRIBUTES
   line_flags = CLine->flags;
   CLine->flags &= ~JED_LINE_IS_READONLY;
#endif
   (void) jed_del_wchar ();
#if JED_HAS_LINE_ATTRIBUTES
   CLine->flags = line_flags;
#endif

   pop_spot ();
   (void) jed_up (1);
#if JED_HAS_LINE_ATTRIBUTES
   line_flags = CLine->flags;
   CLine->flags &= ~JED_LINE_IS_READONLY;
#endif
   (void) jed_del_wchar ();
#if JED_HAS_LINE_ATTRIBUTES
   CLine->flags = line_flags;
#endif
   /* mark_undo_boundary (b); */

   pop_spot ();
   if (save != CBuf) switch_to_buffer (save);

   b->flags = flags;
   return 1;
}

/*}}}*/

int narrow_to_region (void) /*{{{*/
{
   int pnt;
   Line *line;
   unsigned int flags;
#if JED_HAS_LINE_ATTRIBUTES
   unsigned int line_flags;
#endif

   if (0 == check_region (&Number_One))/* spot pushed */
     return 0;

   /* unmark_undo_boundary (CBuf); */

   flags = CBuf->flags;
   CBuf->flags &= ~(READ_ONLY);
   /* A hack to avoid locking file */
   CBuf->flags |= BUFFER_NON_LOCKING;

   push_spot ();

   line = CLine;
   pnt = Point;

   jed_pop_mark (1);
   /* Special case if region is empty */
   if ((CLine == line) && (pnt == Point))
     {
	pop_spot ();
	pop_spot ();

#if JED_HAS_LINE_ATTRIBUTES
	line_flags = CLine->flags;
	CLine->flags &= ~JED_LINE_IS_READONLY;
#endif

	(void) jed_insert_newline ();

#if JED_HAS_LINE_ATTRIBUTES
	if (CLine->prev != NULL) CLine->prev->flags = line_flags;
#endif

	jed_push_mark ();

	(void) jed_insert_newline ();

#if JED_HAS_LINE_ATTRIBUTES
	CLine->flags = line_flags;
#endif
	(void) jed_up (1);

#if JED_HAS_LINE_ATTRIBUTES
	CLine->flags = line_flags;
#endif

	if (narrow_to_lines ())
	  CBuf->narrow->is_region = 1;
     }
   else
     {
#if JED_HAS_LINE_ATTRIBUTES
	line_flags = CLine->flags;
	CLine->flags &= ~JED_LINE_IS_READONLY;
#endif
	jed_insert_newline ();
#if JED_HAS_LINE_ATTRIBUTES
	CLine->flags = line_flags;
	if (CLine->prev != NULL) CLine->prev->flags = line_flags;
#endif

	jed_push_mark ();
	pop_spot ();

#if JED_HAS_LINE_ATTRIBUTES
	line_flags = CLine->flags;
	CLine->flags &= ~JED_LINE_IS_READONLY;
#endif
	jed_insert_newline ();
#if JED_HAS_LINE_ATTRIBUTES
	CLine->flags = line_flags;
#endif
	jed_up (1);

#if JED_HAS_LINE_ATTRIBUTES
	CLine->flags = line_flags;
#endif
	if (narrow_to_lines ())
	  CBuf->narrow->is_region = 1;

	pop_spot ();
     }

   /* mark_undo_boundary (CBuf); */
   CBuf->flags = flags;
   return 1;
}

/*}}}*/

int widen (void) /*{{{*/
{
   if (CBuf->narrow == NULL)
     return 0;

   if (CBuf->narrow->is_region == 0)
     return widen_buffer_lines (CBuf);

   return widen_buffer (CBuf);
}

/*}}}*/

int widen_region (void) /*{{{*/
{
   return widen ();
}

/*}}}*/

/* not really a region of points but a region of lines. */
int narrow_to_lines () /*{{{*/
{
   Line *beg;
   Narrow_Type *nt;

   if (NULL == (nt = (Narrow_Type *) jed_malloc0 (sizeof(Narrow_Type))))
     return 0;

   if (!check_region(&Number_One)) return(0);       /* spot pushed */

   /* unmark_undo_boundary (CBuf); */

   push_spot();
   jed_pop_mark(1);
   jed_push_mark();			       /* popped and used in touch_windows! */
   beg = CLine;
   nt->nup = LineNum - 1;

   pop_spot();  /* eor now */

   nt->ndown = Max_LineNum - LineNum;

   Max_LineNum = LineNum = LineNum - nt->nup;
   CBuf->nup += nt->nup;
   CBuf->ndown += nt->ndown;

   nt->next = CBuf->narrow;
   CBuf->narrow = nt;
   nt->beg = beg->prev;
   nt->end = CLine->next;
   nt->beg1 = CBuf->beg;
   nt->end1 = CBuf->end;

   nt->is_region = 0;
   CBuf->beg = beg;
   CBuf->end = CLine;
   beg->prev = NULL;
   CLine->next = NULL;

   if (CLine->len && (CLine->data[CLine->len - 1] == '\n'))
     {
	/* I do not think that this will affect undo. */
	CLine->len--;
     }

   pop_spot();
   touch_windows();
   return(1);
}

/*}}}*/

/*}}}*/

/*{{{ Pastebuffer, Delete Region functions */

int yank() /*{{{*/
{
   CHECK_READ_ONLY
    if (Paste_Buffer == NULL) return(0);
    insert_buffer(Paste_Buffer);
    return(1);
}

/*}}}*/

int copy_region_to_buffer(Buffer *b) /*{{{*/
{
   int first_point, last_point, n;
   Line *first, *last;
   Buffer *save_buf;

   if (b->flags & READ_ONLY)
     {
	msg_error(Read_Only_Error);
	return (0);
     }

   if (!check_region(&Number_One)) return(0);  /* spot pushed */
   last = CLine;
   last_point = Point;

   jed_pop_mark(1);
   if (b == CBuf)
     {
	msg_error("A buffer cannot be inserted upon itself.");
	pop_spot();
	return(0);
     }

   first = CLine;
   first_point = Point;

   save_buf = CBuf;
   switch_to_buffer(b);

   /* go through standard routines for undo comapatability */
   Suspend_Screen_Update = 1;
   if (first == last)
     {
	n = last_point - first_point;
	if (save_buf == MiniBuffer)
	  {
	     (void) jed_insert_nbytes (first->data + first_point, n);
	  }
	else (void) jed_quick_insert (first->data + first_point, n);
     }
   else
     {
	n = first->len - first_point;
	if (-1 == jed_quick_insert(first->data + first_point, n))
	  goto the_return;

	while (first = first->next, first != last)
	  {
	     if (-1 == jed_quick_insert (first->data, first->len))
	       goto the_return;
	  }
	(void) jed_quick_insert(first->data, last_point);
     }

   the_return:
   switch_to_buffer(save_buf);
   pop_spot();
   return(1);
}

/*}}}*/

int copy_to_pastebuffer() /*{{{*/
{
   /* delete paste buffer */
   if (Paste_Buffer != NULL) delete_buffer(Paste_Buffer);
   Paste_Buffer = make_buffer (" <paste>", NULL, NULL);

   copy_region_to_buffer(Paste_Buffer);
   return(0);
}

/*}}}*/

static int jed_check_readonly_region (void)
{
   int beg_point, end_point;
   Line *beg, *end;

   if (CBuf->flags & READ_ONLY)
     {
	msg_error(Read_Only_Error);
	return -1;
     }

   if (!check_region(&Number_Zero))
     return -1;

   end = CLine; end_point = Point;
   (void) exchange_point_mark ();
   beg = CLine; beg_point = Point;
   (void) exchange_point_mark ();

   while (1)
     {
	if (beg->flags & JED_LINE_IS_READONLY)
	  {
	     if ((beg == end)
		 && (end_point == 0)
		 && (beg_point == 0))
	       return 0;

	     msg_error (Line_Read_Only_Error);
	     return -1;
	  }
	if (beg == end)
	  break;

	beg = beg->next;
     }
   return 0;
}

int delete_region (void) /*{{{*/
{
   int beg_point, end_point;
   Line *beg, *end;

   if (0 != jed_check_readonly_region ())
     return -1;

   /* make this go through standard ins/del routines to ensure undo */

   end = CLine; end_point = Point;
   push_spot();
   jed_pop_mark(1);
   beg = CLine; beg_point = Point;
   pop_spot();

   if (end != beg)
     {
	bol ();

	if (-1 == jed_del_nbytes (end_point))
	  return -1;

	/* go back because we do not want to mess with Line structures
	   changing on us --- shouldn't happen anyway */

	while (jed_up (1) && (CLine != beg))
	  {
	     bol ();
	     if (-1 == jed_del_through_eol ())
	       return -1;
	  }
	end_point = CLine->len;	       /* include \n */
     }

   jed_set_point (beg_point);
   if (-1 == jed_generic_del_nbytes (end_point - Point))
     return -1;

   return 1;
}

/*}}}*/

int kill_region() /*{{{*/
{
   if (-1 == jed_check_readonly_region ())
     return -1;

   /* need two marks for this one */
   push_spot();
   if (!jed_pop_mark(1))
     {
	check_region(&Number_Zero);
	pop_spot();
	return(0);
     }
   jed_push_mark();
   jed_push_mark();
   pop_spot();

   copy_to_pastebuffer();
   delete_region();
   return(1);
}

/*}}}*/

/*}}}*/

/*{{{ Rectangle Functions */

static char *Rect_Error = "Rectangle has 0 width.";
int insert_rectangle() /*{{{*/
{
   int c1;
   Line *rline;

   CHECK_READ_ONLY
   if (0 == buffer_exists (Rectangle_Buffer))
     {
	Rectangle_Buffer = NULL;
	return 0;
     }

   Suspend_Screen_Update = 1;
   c1 = calculate_column();
   rline = Rectangle_Buffer->beg;
   if (rline != NULL) while (1)
     {
	goto_column(&c1);
	if (-1 == jed_quick_insert (rline->data, rline->len))
	  return -1;

	rline = rline->next;
	if (rline == NULL) break;
	if (0 == jed_down (1))
	  {
	     eol();
	     jed_insert_newline();
	  }
     }
   return(1);
}

/*}}}*/

int open_rectangle (void) /*{{{*/
{
   int c1, n, c2;
   Line *save_line;

   CHECK_READ_ONLY
   if (!check_region(&Number_One)) return(0); /* push_spot(); performed */

   c1 = calculate_column();
   save_line = CLine;
   jed_pop_mark(1);
   c2 = calculate_column();
   n = c2 - c1;
   if (n < 0)
     {
	n = -n;
	c1 = c2;
     }

   Suspend_Screen_Update = 1;
   while(1)
     {
	goto_column(&c1);
	if (-1 == jed_insert_wchar_n_times(' ', n))
	  break;
	if (CLine == save_line) break;
	(void) jed_down (1);
     }
   pop_spot();

   return(1);
}

/*}}}*/

/* MB Safe */
static int copy_or_kill_rectangle (int kill) /*{{{*/
{
   Line *save_line, *line, *beg;
   int c1, c2, dc, tmp;

   if (!check_region(&Number_One)) return(0);       /* spot pushed */
   /* delete Rectangle buffer */
   if (Rectangle_Buffer != NULL) delete_buffer(Rectangle_Buffer);

   Rectangle_Buffer = make_buffer (" <rect>", NULL, NULL);
   c2 = calculate_column();
   save_line = CLine;

   jed_pop_mark(1);
   c1 = calculate_column();
   if (c1 == c2)
     {
	msg_error(Rect_Error);
	pop_spot();
	return(0);
     }
   if (c1 > c2)
     {
	tmp = c1;
	c1 = c2;
	c2 = tmp;
	goto_column(&c1);
     }

   dc = c2 - c1;

   /* go through the region copying rectanglar blocks to Rectanglebuffer */

   line = beg = NULL;
   while (1)
     {
	int col2, col1;
	unsigned int len, len1;
	unsigned int nspaces;
	unsigned int nbytes;
	unsigned char *p1, *p2;

	col1 = goto_column1 (&c1);
	p1 = CLine->data + Point;

	if (col1 == c1)
	  {
	     col2 = goto_column1(&c2);
	     p2 = CLine->data + Point;
	  }
	else
	  {
	     col2 = col1;
	     p2 = p1;
	  }

	nspaces = (unsigned int) (dc - (col2 - col1));
	nbytes = (unsigned int) (p2 - p1);
	len1 = len = nbytes + nspaces;

	/* Need to allocate at least 2 bytes, since a single byte is a
	 * signature of a line with a single character whose value is a
	 * newline character.  See make_line1 for details.
	 */
	if (len <= 1)
	  len1++;

	if (beg == NULL)
	  {
	     beg = line = make_line1 (len1);
	     beg->prev = NULL;
	  }
	else
	  {
	     line->next = make_line1 (len1);
	     line->next->prev = line;
	     line = line->next;
	  }
	line->len = len;

	memcpy ((char *) line->data, (char *) p1, nbytes);
	memset ((char *) line->data + nbytes, ' ', nspaces);

	if (kill && (nbytes != 0))
	  {
	     jed_position_point (p1);
	     if (-1 == jed_del_nbytes (nbytes))
	       break;
	  }

	 if (CLine == save_line) break;
	 (void) jed_down(1);
      }

    line->next = NULL;

    Rectangle_Buffer->line = Rectangle_Buffer->beg = beg;
    Rectangle_Buffer->end = line;
    Rectangle_Buffer->point = 0;

    pop_spot();
    return(0);
}

/*}}}*/

int copy_rectangle (void)
{
   return copy_or_kill_rectangle (0);
}

int kill_rectangle (void)
{
   return copy_or_kill_rectangle (1);
}

int blank_rectangle (void) /*{{{*/
{
   int c1, c2;
   Line *save_line;
   int ncols;

   CHECK_READ_ONLY
   if (!check_region(&Number_One)) return(0); /* push_spot(); performed */

   c1 = calculate_column();
   save_line = CLine;
   jed_pop_mark(1);
   c2 = calculate_column();
   if (c1 > c2)
     {
	int tmp = c1;
	c1 = c2;
	c2 = tmp;
     }
   ncols = c2 - c1;

   Suspend_Screen_Update = 1;
   while(1)
     {
	int pnt;

	goto_column (&c2);
	pnt = Point;
	goto_column (&c1);

	if (-1 == jed_del_nbytes (pnt - Point))
	  break;

	if (-1 == jed_insert_wchar_n_times( ' ', ncols))
	  break;

	if (CLine == save_line) break;
	(void) jed_down (1);
     }
   pop_spot();
   return(1);
}

/*}}}*/

/*}}}*/

/*{{{ User Mark Functions */

typedef struct /*{{{*/
{
   Mark m;			       /* MUST be the first */
   Buffer *b;
}

/*}}}*/
User_Mark_Type;

static void free_user_mark (SLtype type, VOID_STAR um_alias) /*{{{*/
{
   Mark *m, *m1;
   Buffer *b;
   User_Mark_Type *um;

   (void) type;
   um = (User_Mark_Type *) um_alias;
   m1 = &um->m;

   /* The mark is only valid if the buffer that it was created for still
    * exists.
    */
   if ((m1->flags & MARK_INVALID) == 0)
     {
	/* Unlink the mark from the chain. */
	b = um->b;
	m = b->user_marks;
#if JED_HAS_LINE_MARKS
	if (m1->flags & JED_LINE_MARK)
	  touch_screen ();
#endif
	if (m == m1)	b->user_marks = m1->next;
	else
	  {
	     while (m->next != m1) m = m->next;
	     m->next = m1->next;
	  }
     }

   SLfree ((char *)um);
}

/*}}}*/

void free_user_marks (Buffer *b) /*{{{*/
{
   Mark *m = b->user_marks;

   while (m != NULL)
     {
	m->flags |= MARK_INVALID;
	m = m->next;
     }
}

/*}}}*/

static int mark_valid (Mark *m) /*{{{*/
{
   if (m->flags & MARK_INVALID)
     {
	msg_error ("Mark is invalid.");
	return 0;
     }
   return 1;
}

/*}}}*/

static SLang_MMT_Type *pop_valid_user_mark (User_Mark_Type **ump)
{
   SLang_MMT_Type *mmt;
   User_Mark_Type *um;

   *ump = NULL;

   if (NULL == (mmt = SLang_pop_mmt (JED_MARK_TYPE)))
     return NULL;

   um = (User_Mark_Type *) SLang_object_from_mmt (mmt);

   if (0 == mark_valid (&um->m))
     {
	SLang_free_mmt (mmt);
	return NULL;
     }

   *ump = um;
   return mmt;
}

int jed_move_user_object_mark (SLang_MMT_Type *mmt) /*{{{*/
{
   User_Mark_Type *um;
   Mark *m;

   um = (User_Mark_Type *) SLang_object_from_mmt (mmt);
   m = &um->m;

   if (!mark_valid (m)) return 0;

   if (CBuf != um->b)
     {
	msg_error ("Mark not in buffer.");
	return 0;
     }

   m->line = CLine;
   m->point = Point;
   m->n = LineNum + CBuf->nup;
   return 1;
}

/*}}}*/

void move_user_mark (void) /*{{{*/
{
   SLang_MMT_Type *mmt;

   if (NULL == (mmt = SLang_pop_mmt (JED_MARK_TYPE)))
     return;

   (void) jed_move_user_object_mark (mmt);
   SLang_free_mmt (mmt);
}

/*}}}*/

static int x_user_mark_fun (int (*xfun)(Mark *)) /*{{{*/
{
   SLang_MMT_Type *mmt;
   User_Mark_Type *um;
   int ret = -1;

   if (NULL == (mmt = pop_valid_user_mark (&um)))
     return -1;

   if (CBuf != um->b) msg_error ("Mark not in buffer.");
   else
     ret = (*xfun) (&um->m);

   SLang_free_mmt (mmt);
   return ret;
}

/*}}}*/

/* It is up to calling routine to ensure that mark is in buffer. */
static int is_mark_in_narrow (Mark *m)
{
   return ((m->n > CBuf->nup)
	   && (m->n <= CBuf->nup + Max_LineNum));
}

int jed_is_user_mark_in_narrow (void)
{
   return x_user_mark_fun (is_mark_in_narrow);
}

void goto_user_mark (void)
{
   (void) x_user_mark_fun (jed_goto_mark);
}

SLang_MMT_Type *jed_make_user_object_mark (void) /*{{{*/
{
   User_Mark_Type *um;
   SLang_MMT_Type *mmt;
   Mark *m;

   if (NULL == (um = (User_Mark_Type *) jed_malloc0 (sizeof(User_Mark_Type))))
     return NULL;

   if (NULL == (mmt = SLang_create_mmt (JED_MARK_TYPE, (VOID_STAR) um)))
     {
	SLfree ((char *) um);
	return NULL;
     }

   m = &um->m;

   jed_init_mark (m, 0);

   m->next = CBuf->user_marks;

   CBuf->user_marks = m;

   um->b = CBuf;

   return mmt;
}

/*}}}*/

void create_user_mark (void) /*{{{*/
{
   SLang_MMT_Type *mmt;

   if (NULL != (mmt = jed_make_user_object_mark ()))
     {
	if (-1 == SLang_push_mmt (mmt))
	  SLang_free_mmt (mmt);
     }
}

/*}}}*/

char *user_mark_buffer (void) /*{{{*/
{
   SLang_MMT_Type *mmt;
   User_Mark_Type *um;
   char *s;

   if (NULL == (mmt = pop_valid_user_mark (&um)))
     return "";

   s = um->b->name;

   SLang_free_mmt (mmt);
   return s;
}

/*}}}*/

static int
user_mark_bin_op_result (int op, SLtype a, SLtype b,
			 SLtype *c)
{
   (void) a; (void) b;
   switch (op)
     {
      default:
	return 0;

      case SLANG_GT:
      case SLANG_GE:
      case SLANG_LT:
      case SLANG_LE:
      case SLANG_EQ:
      case SLANG_NE:
	*c = SLANG_INT_TYPE;
	break;
     }
   return 1;
}

static int
user_mark_bin_op (int op,
		  SLtype a_type, VOID_STAR ap, unsigned int na,
		  SLtype b_type, VOID_STAR bp, unsigned int nb,
		  VOID_STAR cp)
{
   int *ic;
   unsigned int n, n_max;
   unsigned int da, db;
   SLang_MMT_Type **a, **b;

   (void) a_type;
   (void) b_type;

   if (na == 1) da = 0; else da = 1;
   if (nb == 1) db = 0; else db = 1;

   if (na > nb) n_max = na; else n_max = nb;

   a = (SLang_MMT_Type **) ap;
   b = (SLang_MMT_Type **) bp;

   ic = (int *) cp;

   for (n = 0; n < n_max; n++)
     {
	User_Mark_Type *ua, *ub;
	Buffer *ba, *bb;
	int pa, pb;
	unsigned int la, lb;

	ub = NULL;
	ba = bb = NULL;
	pa = pb = 0;
	la = lb = 0;

	if ((*a != NULL)
	    && (NULL != (ua = (User_Mark_Type *) SLang_object_from_mmt (*a))))
	  {
	     la = ua->m.n;
	     pa = ua->m.point;
	     ba = ua->b;
	  }

	if ((*b != NULL)
	    && (NULL != (ub = (User_Mark_Type *) SLang_object_from_mmt (*b))))
	  {
	     lb = ub->m.n;
	     pb = ub->m.point;
	     bb = ub->b;
	  }

	if ((ba == NULL) || (bb == NULL))
	  {
	     ic[n] = 0;
	     a += da;
	     b += db;
	     continue;
	  }

	switch (op)
	  {
	   case SLANG_GT:
	     ic[n] = ((la > lb)
		      || ((la == lb) && (pa > pb)));
	     break;

	   case SLANG_GE:
	     ic[n] = ((la > lb)
		      || ((la == lb) && (pa >= pb)));
	     break;

	   case SLANG_LT:
	     ic[n] = ((la < lb)
		      || ((la == lb) && (pa < pb)));
	     break;

	   case SLANG_LE:
	     ic[n] = ((la < lb)
		      || ((la == lb) && (pa <= pb)));
	     break;

	   case SLANG_EQ:
	     ic[n] = ((ba == bb) && (la == lb) && (pa == pb));
	     break;

	   case SLANG_NE:
	     ic[n] = ((ba != bb) || (la != lb) || (pa != pb));
	     break;

	   default:
	     return 0;
	  }

	a += da;
	b += db;
     }

   return 1;
}

/*}}}*/

#if JED_HAS_LINE_MARKS
void jed_create_line_mark (int *color) /*{{{*/
{
   SLang_MMT_Type *mmt;
   User_Mark_Type *um;

   if (NULL == (mmt = jed_make_user_object_mark ()))
     return;

   um = (User_Mark_Type *) SLang_object_from_mmt (mmt);
   um->m.flags |= JED_LINE_MARK | (*color & MARK_COLOR_MASK);
   if (-1 == SLang_push_mmt (mmt))
     SLang_free_mmt (mmt);
}

/*}}}*/
#endif

#ifndef SLFUTURE_CONST
# define SLFUTURE_CONST
#endif

static int user_mark_sget (SLtype type, SLFUTURE_CONST char *name)
{
   SLang_MMT_Type *mmt;
   User_Mark_Type *um;
   int status;
   Buffer *buf;

   (void) type;

   if (NULL == (mmt = pop_valid_user_mark (&um)))
     return -1;

   buf = um->b;

   status = -1;
   if (0 == strcmp (name, "buffer_name"))
     status = SLang_push_string (buf->name);
   else
     SLang_verror (SL_NOT_IMPLEMENTED,
		   "Mark_Type.%s is invalid", name);

   SLang_free_mmt (mmt);
   return status;
}

int register_jed_classes (void) /*{{{*/
{
   SLang_Class_Type *cl;

   cl = SLclass_allocate_class ("Mark_Type");
   if (cl == NULL) return -1;
   (void) SLclass_set_destroy_function (cl, free_user_mark);

   (void) SLclass_set_sget_function (cl, user_mark_sget);

   if (-1 == SLclass_register_class (cl, JED_MARK_TYPE, sizeof (User_Mark_Type), SLANG_CLASS_TYPE_MMT))
     return -1;

   if (-1 == SLclass_add_binary_op (JED_MARK_TYPE, JED_MARK_TYPE, user_mark_bin_op, user_mark_bin_op_result))
     return -1;

   return 0;
}

/*}}}*/

void jed_widen_whole_buffer (Buffer *b) /*{{{*/
{
   while (b->narrow != NULL) widen_buffer (b);
}

/*}}}*/

#if JED_HAS_SAVE_NARROW
static void restore_saved_narrow (void) /*{{{*/
{
   Mark *beg, *end;

   Jed_Save_Narrow_Type *save_narrow;

   if (NULL == (save_narrow = CBuf->save_narrow))
     return;

   push_spot ();
   /* remove current restriction */
   jed_widen_whole_buffer (CBuf);

   beg = save_narrow->beg;
   end = save_narrow->end;

   while (beg != NULL)
     {
	jed_goto_mark (beg);
	jed_push_mark ();
	jed_goto_mark (end);
	if (end->flags & NARROW_REGION_MARK)
	  narrow_to_region ();
	else narrow_to_lines ();

	beg = beg->next;
	end = end->next;
     }
   pop_spot ();
}

/*}}}*/
static void free_mark_chain (Mark *m) /*{{{*/
{
   Mark *next;

   while (m != NULL)
     {
	next = m->next;
	SLfree ((char *)m);
	m = next;
     }
}

/*}}}*/
void jed_free_saved_narrow (Buffer *b) /*{{{*/
{
   Jed_Save_Narrow_Type *save_narrow;

   save_narrow = b->save_narrow;
   if (save_narrow == NULL) return;

   b->save_narrow = save_narrow->next;

   free_mark_chain (save_narrow->beg);
   free_mark_chain (save_narrow->end);
   SLfree ((char *)save_narrow);
}

/*}}}*/
void jed_push_narrow (void) /*{{{*/
{
   Jed_Save_Narrow_Type *save_narrow;

   if (NULL == (save_narrow = (Jed_Save_Narrow_Type *) jed_malloc0 (sizeof (Jed_Save_Narrow_Type))))
     {
	exit_error ("push_narrow: malloc error.", 0);
     }
   save_narrow->beg = save_narrow->end = NULL;
   save_narrow->next = CBuf->save_narrow;
   CBuf->save_narrow = save_narrow;

   push_spot ();
   while (CBuf->narrow != NULL)
     {
	Mark *m;

	bob ();
	m = create_mark (0);
	m->next = save_narrow->beg;
	save_narrow->beg = m;

	eob ();
	m = create_mark (CBuf->narrow->is_region ? NARROW_REGION_MARK : 0);
	m->next = save_narrow->end;
	save_narrow->end = m;

	widen_buffer (CBuf);
     }

   restore_saved_narrow ();

   pop_spot ();
}

/*}}}*/
void jed_pop_narrow (void) /*{{{*/
{
   restore_saved_narrow ();
   jed_free_saved_narrow (CBuf);
}

/*}}}*/
#endif

int jed_count_narrows (void) /*{{{*/
{
   int n = 0;
   Narrow_Type *nt = CBuf->narrow;

   while (nt != NULL)
     {
	n++;
	nt = nt->next;
     }
   return n;
}

/*}}}*/

unsigned int jed_count_lines_in_region (void)
{
   Mark *m;
   unsigned int n0, n1;

   m = CBuf->marks;
   if (m == NULL)
     return 0;

   n0 = m->n - CBuf->nup;
   n1 = LineNum;

   if (n0 > n1)
     return 1 + (n0 - n1);

   return 1 + (n1 - n0);
}

