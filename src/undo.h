#ifndef DAVIS_UNDO_H_
#define DAVIS_UNDO_H_
/* Copyright (c) 1992-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#if defined (__MSDOS_16BIT__) || defined (__os2_16__)
/* size is something like 2000 x (16) = 32K */
#define MAX_UNDOS 2000
#else
#define UNDO_HAS_REDO
/* 24 bytes * 10K = 240K */
#define MAX_UNDOS 10000
#endif

typedef struct
{
   unsigned short type;		       /* type of damage */
   unsigned int linenum;	       /* where damage was */
   int point;			       /*  */
   int misc;			       /* misc information */
   unsigned char buf[8];	       /* buffer for chars */
} Undo_Object_Type;

typedef struct Undo_Type
{
   Undo_Object_Type *Last_Undo;
   Undo_Object_Type *First_Undo;
#ifdef UNDO_HAS_REDO
   Undo_Object_Type *Current_Undo;
#endif
   Undo_Object_Type Undo_Ring[MAX_UNDOS];
} Undo_Type;

void record_deletion(unsigned char *, int);
extern void record_insertion(int);
extern void record_newline_insertion(void);
extern int undo(void);
extern void create_undo_ring(void);
extern int Undo_Buf_Unch_Flag;	       /* 1 if buffer prev not modified */

#ifdef UNDO_HAS_REDO
extern void set_current_undo(void);
extern void update_undo_unchanged(void);
#endif

extern void unmark_undo_boundary (Buffer *);

extern void jed_undo_record_position (void);

#endif
