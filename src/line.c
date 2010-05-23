/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2010 John E. Davis
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
#include "ins.h"
#include "misc.h"
#include "paste.h"
#include "undo.h"
#include "line.h"

/* breaks line at current point leaving point at end of current line */

int split_line (void) /*{{{*/
{
    int size;

   if (CLine == NULL)
     {
	exit_error("split_line: Null Line", 1);
     }
   size = CLine->len - Point;

   if (NULL == make_line(size + 1))
     {
	exit_error("Allocation Failure in split_line", 0);
     }

   SLMEMCPY((char *) CLine->data, (char *) (CLine->prev->data + Point), size);
   CLine->len = size;
   CLine = CLine->prev;  LineNum--;
   CLine->len = Point;
   remake_line(Point + 1);
   /* now update the marks */
   jed_update_marks(NLINSERT, 1);
   record_newline_insertion();
   return(0);
}

/*}}}*/

void splice_line (void) /*{{{*/
{
   int n1, n2;
   int save_point;
#if JED_HAS_LINE_ATTRIBUTES
   unsigned int flags;
#endif

   if (CLine->next == NULL)
     {
	exit_error("splice line: next line is Null", 1);
     }
   push_spot();
   n1 = CLine->len;
   n2 = CLine->next->len;

#ifdef KEEP_SPACE_INFO
   if (n1 + n2 > CLine->space)
#endif
     remake_line(n1 + n2 + 1);

   SLMEMCPY((char *) (CLine->data + Point), (char *) CLine->next->data, n2);
   CLine->len = n1 + n2;

   save_point = Point;
   (void) jed_down (1);
   Point = save_point;

   jed_update_marks(NLDELETE, 1);
#if JED_HAS_LINE_ATTRIBUTES
   flags = CLine->flags;
#endif
   delete_line();
   pop_spot();

#if JED_HAS_LINE_ATTRIBUTES
   if (n1 == 0)
     CLine->flags = flags;
#endif
}

/*}}}*/

