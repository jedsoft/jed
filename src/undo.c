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
#include "buffer.h"
#include "undo.h"
#include "ins.h"
#include "line.h"
#include "misc.h"
#include "screen.h"

#define LAST_UNDO CBuf->undo->Last_Undo
#define FIRST_UNDO CBuf->undo->First_Undo
#define UNDO_RING CBuf->undo->Undo_Ring

#ifdef UNDO_HAS_REDO
# define CURRENT_UNDO CBuf->undo->Current_Undo
#endif

static int Undo_In_Progress = 0;
int Undo_Buf_Unch_Flag;	       /* 1 if buffer prev not modified */

#ifdef UNDO_HAS_REDO
#define DONT_RECORD_UNDO  (!(CBuf->flags & UNDO_ENABLED)\
			   || (CBuf->undo == NULL))
#else
#define DONT_RECORD_UNDO  (!(CBuf->flags & UNDO_ENABLED)\
			   || (CBuf->undo == NULL) || Undo_In_Progress)
#endif

#define UNDO_BD_FLAG 0x8000
#define UNDO_UNCHANGED_FLAG 0x4000

#ifdef UNDO_HAS_REDO
# define IS_UNDO_BD (CURRENT_UNDO->type & UNDO_BD_FLAG)
#else
# define IS_UNDO_BD (LAST_UNDO->type & UNDO_BD_FLAG)
#endif

static void prepare_next_undo(void) /*{{{*/
{
   LAST_UNDO++;
   if (LAST_UNDO >= UNDO_RING + MAX_UNDOS) LAST_UNDO = UNDO_RING;
   if (LAST_UNDO == FIRST_UNDO)
     {
	FIRST_UNDO++;
#ifdef UNDO_HAS_REDO
	if (FIRST_UNDO >= UNDO_RING + MAX_UNDOS) FIRST_UNDO = UNDO_RING;
#endif
     }
#ifdef UNDO_HAS_REDO
   if (LAST_UNDO == CURRENT_UNDO)      /*  undo-ring overflow will trash  */
     CURRENT_UNDO = NULL;	       /*  further undos  */
#else
   if (FIRST_UNDO >= UNDO_RING + MAX_UNDOS) FIRST_UNDO = UNDO_RING;
#endif

   LAST_UNDO->type = 0;
   LAST_UNDO->misc = 0;
}

/*}}}*/

#ifdef UNDO_HAS_REDO
/*  Returns True if there is still  undo info to be processed. */
#define MORE_UNDO_INFO (CURRENT_UNDO && (CURRENT_UNDO->type))
#endif

void record_deletion (unsigned char *p, int n) /*{{{*/
{
   int misc;
   unsigned char *pb;

   if (DONT_RECORD_UNDO || (n == 0)) return;

   while (1)
     {
	if (((LAST_UNDO->type & 0xFF) == 0)
	    || ((LAST_UNDO->type & CDELETE)
		&& (LAST_UNDO->linenum == LineNum + CBuf->nup)
		&& (LAST_UNDO->point == Point)))
	  {
	     if (LAST_UNDO->type != 0) misc = LAST_UNDO->misc; else misc = 0;
	     pb = LAST_UNDO->buf + misc;
	     while ((misc < 8) && n)
	       {
		  *pb++ = *p++;
		  misc++;
		  n--;
	       }
	     LAST_UNDO->misc = misc;
	     LAST_UNDO->type |= CDELETE;
	     LAST_UNDO->linenum = LineNum + CBuf->nup;
	     LAST_UNDO->point = Point;
	     if (Undo_Buf_Unch_Flag) LAST_UNDO->type |= UNDO_UNCHANGED_FLAG;
	     Undo_Buf_Unch_Flag = 0;
	  }

	if (n == 0)
	  break;

	prepare_next_undo();
     }

   if (Undo_Buf_Unch_Flag) LAST_UNDO->type |= UNDO_UNCHANGED_FLAG;
#ifdef UNDO_HAS_REDO
   set_current_undo ();
#endif
}

/*}}}*/

int undo (void) /*{{{*/
{
   int line;
   CHECK_READ_ONLY
   if (!(CBuf->flags & UNDO_ENABLED))
     {
	msg_error("Undo not enabled for this buffer.");
	return(0);
     }
   else if ((CBuf->undo == NULL)
#ifdef UNDO_HAS_REDO
	    || (0 == MORE_UNDO_INFO)
#else
	    || (LAST_UNDO->type == 0)
#endif
	    )
     {
	msg_error("No more undo information.");
	return(0);
     }
   Undo_In_Progress = 1;

   do
     {
	int undo_type;
#ifdef UNDO_HAS_REDO
	line = (int) CURRENT_UNDO->linenum;
	undo_type = CURRENT_UNDO->type & 0xFF;
#else
	line = (int) LAST_UNDO->linenum;
	undo_type = LAST_UNDO->type & 0xFF;
#endif
	if ((line <= (int) CBuf->nup)
	    || ((unsigned int) line >= CBuf->nup + Max_LineNum))
	  {
	     if (((unsigned int) line != CBuf->nup + Max_LineNum)
		 || (undo_type == NLINSERT))
	       {
		  msg_error("Next undo lies outside visible buffer.");
		  break;
	       }
	  }
	line -= CBuf->nup;
	goto_line(&line);
#ifdef UNDO_HAS_REDO
	Point = CURRENT_UNDO->point;
#else
	Point = LAST_UNDO->point;
#endif
	if (Point > CLine->len)
	  {
	     Point = 0;
	     msg_error ("Internal Error in undo: Point > CLine->len");
	     break;
	  }

	switch (undo_type)
	  {
#ifdef UNDO_HAS_REDO
	   case CDELETE: (void) jed_insert_nbytes (CURRENT_UNDO->buf, CURRENT_UNDO->misc);
	     /* Point = CURRENT_UNDO->point; */
#else
	   case CDELETE: (void) jed_insert_nbytes (LAST_UNDO->buf, LAST_UNDO->misc);
	     /* Point = LAST_UNDO->point; */
#endif
	     break;

#ifdef UNDO_HAS_REDO
	   case CINSERT: (void) jed_del_nbytes (CURRENT_UNDO->misc);
#else
	   case CINSERT: (void) jed_del_nbytes (LAST_UNDO->misc);
#endif
	     break;

	   case UNDO_POSITION:
	     break;

	   case NLINSERT: jed_del_wchar (); break;

	   default: return(1);
	  }

#ifdef UNDO_HAS_REDO
	if (CURRENT_UNDO == NULL) break;
	/*  no more undo info after overflow */

	/*  above calls to ins_chars/deln/del may overflow the undo-ring */
	if (CURRENT_UNDO->type & UNDO_UNCHANGED_FLAG)
	  {
	     mark_buffer_modified (CBuf, 0, 1);
	  }

	if (CURRENT_UNDO == FIRST_UNDO)
	  {
	     CURRENT_UNDO = NULL ;
	     break ;			/*  no more undo info  */
	  }

	CURRENT_UNDO--;
	if (CURRENT_UNDO < UNDO_RING) CURRENT_UNDO = UNDO_RING + MAX_UNDOS - 1;
#else
	if (LAST_UNDO->type & UNDO_UNCHANGED_FLAG)
	  {
	     mark_buffer_modified (CBuf, 0, 1);
	  }

	if (LAST_UNDO == FIRST_UNDO) LAST_UNDO->type = 0;
	else
	  {
	     LAST_UNDO--;
	     if (LAST_UNDO < UNDO_RING) LAST_UNDO = UNDO_RING + MAX_UNDOS - 1;
	  }
#endif
     }
#ifdef UNDO_HAS_REDO
   while ((!IS_UNDO_BD) && (CURRENT_UNDO->type));
#else
   while ((!IS_UNDO_BD) && (LAST_UNDO->type));
#endif

   message("Undo!");
   Undo_In_Progress = 0;
   return(1);
}

/*}}}*/

void record_insertion(int n) /*{{{*/
{
   if (DONT_RECORD_UNDO || !n) return;

   if ((Undo_Buf_Unch_Flag) && (LAST_UNDO->type))
     {
	prepare_next_undo ();
     }

   if (LAST_UNDO->type == 0)
     {
	LAST_UNDO->misc = n;
	LAST_UNDO->point = Point;
     }
   else if ((LAST_UNDO->type & CINSERT) && (LAST_UNDO->linenum == LineNum + CBuf->nup)
	    && (LAST_UNDO->point + LAST_UNDO->misc == Point)
	    && (LAST_UNDO->misc <= 32))
     {
	LAST_UNDO->misc += n;
     }
   else
     {
	prepare_next_undo();
	LAST_UNDO->point = Point;
	LAST_UNDO->misc = n;
     }

   LAST_UNDO->type |= CINSERT;
   if (Undo_Buf_Unch_Flag) LAST_UNDO->type |= UNDO_UNCHANGED_FLAG;
   LAST_UNDO->linenum = LineNum + CBuf->nup;
#ifdef UNDO_HAS_REDO
   set_current_undo ();
#endif
}

/*}}}*/

void record_newline_insertion() /*{{{*/
{
   if (DONT_RECORD_UNDO) return;
   if (LAST_UNDO->type != 0) prepare_next_undo();
   if (Undo_Buf_Unch_Flag) LAST_UNDO->type |= UNDO_UNCHANGED_FLAG;
   LAST_UNDO->point = Point;
   LAST_UNDO->misc = 0;
   LAST_UNDO->type |= NLINSERT;
   LAST_UNDO->linenum = LineNum + CBuf->nup;
#ifdef UNDO_HAS_REDO
   set_current_undo ();
#endif
}

/*}}}*/

void jed_undo_record_position (void)
{
   if (DONT_RECORD_UNDO) return;
   if (LAST_UNDO->type != 0) prepare_next_undo();
   if (Undo_Buf_Unch_Flag) LAST_UNDO->type |= UNDO_UNCHANGED_FLAG;
   LAST_UNDO->point = Point;
   LAST_UNDO->misc = 0;
   LAST_UNDO->type |= UNDO_POSITION;
   LAST_UNDO->linenum = LineNum + CBuf->nup;
#ifdef UNDO_HAS_REDO
   set_current_undo ();
#endif
}

void delete_undo_ring(Buffer *b) /*{{{*/
{
   SLfree ((char *)b->undo);
}

/*}}}*/

void create_undo_ring() /*{{{*/
{
   Undo_Type *ur;
   Undo_Object_Type *uo;
   int n;

   if (NULL == (ur = (Undo_Type *) SLmalloc (sizeof(Undo_Type))))
     {
	msg_error("Unable to malloc space for undo!");
	return;
     }
   CBuf->undo = ur;
   uo = ur->Undo_Ring;
   ur->Last_Undo = ur->First_Undo = uo;
#ifdef UNDO_HAS_REDO
   ur->Current_Undo = NULL;
#endif

   n = MAX_UNDOS;
   while (n--)
     {
	uo->type = 0;
	uo++;
     }
}

/*}}}*/

void mark_undo_boundary (Buffer *b) /*{{{*/
{
   Buffer *s = CBuf;

   CBuf = b;

   if (!DONT_RECORD_UNDO && (LAST_UNDO->type != 0))
     {
	LAST_UNDO->type |= UNDO_BD_FLAG;
     }
   CBuf = s;
}

/*}}}*/

void unmark_undo_boundary (Buffer *b) /*{{{*/
{
#if 0
   (void) b;
#else
   Buffer *s = CBuf;

   CBuf = b;

   if (!DONT_RECORD_UNDO && (LAST_UNDO->type != 0))
     {
	LAST_UNDO->type &= ~UNDO_BD_FLAG;
     }
   CBuf = s;
#endif
}

/*}}}*/

#ifdef UNDO_HAS_REDO
void set_current_undo()		/*  Called when text is inserted, deleted,  */ /*{{{*/
{				/*  or the abort key is pressed.  */
   if ((!Undo_In_Progress) && CBuf->undo)
     {
	CURRENT_UNDO = LAST_UNDO ;
     }
}

/*}}}*/

void update_undo_unchanged (void) /*{{{*/
/* Update unchange flags after a buffer save.  The reason for this is
 * that after a save, undoing on the buffer does not affect the disk file.
 */
{
   Undo_Object_Type *uo;
   int n;

   if (DONT_RECORD_UNDO) return;

   uo = UNDO_RING ;
   n = MAX_UNDOS;
   while (n--)
     {
	uo->type &= ~UNDO_UNCHANGED_FLAG;
	uo++;
     }

   CURRENT_UNDO = LAST_UNDO ;
}

/*}}}*/
#endif

