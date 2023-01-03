/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#include <dos.h>

#if JED_HAS_MULTICLICK
#include <sys/types.h>
#include <time.h>

static int JX_MultiClick_Time = 5;     /* 5/10 of sec */
#endif

#ifdef __GO32__
# include <go32.h>
# include <sys/farptr.h>
#endif

static int Mouse_X, Mouse_Y;

static void generate_press (int x, int y, int type, int but) /*{{{*/
{
   char buf[3];
   unsigned char b, s;
   JMouse_Type jm;
   int id;
#if JED_HAS_MULTICLICK
   clock_t t;
   static clock_t last_press_time;
   static unsigned int clicks;
   static unsigned int last_button;

   t = clock ();

   t = t / (CLOCKS_PER_SEC / 10);	       /* clocks per 1/10 sec */

   if (type == JMOUSE_DOWN)
     {
	if (((int)last_button == but)
	    && (last_press_time + JX_MultiClick_Time > t))
	  {
	     clicks++;
	     if (clicks == 2)
	       type = JMOUSE_DOUBLE_CLICK;
	     else
	       type = JMOUSE_TRIPLE_CLICK;
	  }
	else
	  {
	     clicks = 1;
	     last_button = but;
	  }
	last_press_time = t;
     }
   else if ((clicks > 1) && ((int)last_button == but))
     {
	/* Last was a multi-click.  Ignore this event. */
	type = JMOUSE_IGNORE_EVENT;
     }
#endif

   Mouse_X = jm.x = x;
   Mouse_Y = jm.y = y;

   b = but & 0xF;
   s = but >> 4;

   if (b == 1) jm.button = JMOUSE_BUTTON_1;	       /* left */
   else if (b == 2) jm.button = JMOUSE_BUTTON_3;    /* right */
   else jm.button = JMOUSE_BUTTON_2;		       /* middle */

   if (s & 0x8) 		       /* alt key--- use as middle */
     {
	jm.button = JMOUSE_BUTTON_2;
     }

   if (s & 0x3)	jm.state = JMOUSE_SHIFT;
   else if (s & 0x4) jm.state = JMOUSE_CTRL;
   else jm.state = 0;

   jm.type = type;

   if (-1 == (id = jed_mouse_add_event (&jm)))
     return;

   buf[0] = 27; buf[1] = 0; buf[2] = (char) id;

   ungetkey_string (buf, 3);
}

/*}}}*/

/* Mouse routines for the ibmpc */
static int Mouse_Hidden = 1;
static int Last_Mouse_Hidden = 1;

static void show_mouse (int show) /*{{{*/
{
   union REGS r;

   if (show)
     {
	if (Mouse_Hidden == 0) return;
	r.x.ax = 1;
     }
   else
     {
	if (Mouse_Hidden) return;
	r.x.ax = 2;
     }
   int86 (0x33, &r, &r);
   Mouse_Hidden = !show;
}

/*}}}*/

static void pc_close_mouse () /*{{{*/
{
   show_mouse (0);
}

/*}}}*/

static int pc_get_mouse_event (void) /*{{{*/
{
   union REGS r;
   static int last_press;
   short x, y;

   /* return 0; */
   if (last_press)
     {
	show_mouse (1);
	r.x.ax = 6;
	r.x.bx = (last_press & 0xF) - 1;
	int86 (0x33, &r, &r);
	if (r.x.bx)
	  {
	     /* truncate to short because DJGPP has junk in 32 bit registers*/
	     x = r.x.cx;
	     y = r.x.dx;
	     x = x / 8 + 1;
	     y = y / 8 + 1;
	     generate_press (x, y, JMOUSE_UP, last_press);
	     last_press = 0;
	     return 1;
	  }
     }

   r.x.ax = 3;
   int86 (0x33, &r, &r);
   x = r.x.cx;
   y = r.x.dx;

#if 0
   /* Check motion counters */
   r.x.ax = 0xB;
   int86 (0x33, &r, &r);
#endif
   if (last_press)
     {
#if 0
	y += (short) (int) (r.x.dx);
#endif

	x = x / 8 + 1;
	y = y / 8 + 1;

	if ((Mouse_X == x) && (Mouse_Y == y)) return 0;
	generate_press (x, y, JMOUSE_DRAG, last_press);
	return 1;
     }

   x = x / 8 + 1;
   y = y / 8 + 1;

   /* It looks like we are looking for a press. */
   if ((Mouse_X != x) || (Mouse_Y != y))
     {
	/* mouse moved so show it. */
	show_mouse (1);
     }
   Mouse_X = x;
   Mouse_Y = y;
   if (r.x.bx)
     {
	if (r.x.bx & 1) last_press = 1;   /* left */
	else if (r.x.bx & 2) last_press = 2;   /* right */
	else if (r.x.bx & 4) last_press = 3;	       /* middle */
	else return 0;

	/* Find shift key status */
#ifdef __GO32__
	r.h.ah = 0x12;
	int86 (0x16, &r, &r);
	last_press |= (r.x.ax & 0xF) << 4;
#else
	last_press |= (*(unsigned char far *) 0x417) << 4;
#endif
	generate_press (x, y, JMOUSE_DOWN, last_press);
	return 1;
     }
   return 0;
}

/*}}}*/

static void move_mouse (int x, int y) /*{{{*/
{
   union REGS r;
   r.x.ax = 4;
   r.x.cx = 8 * (x - 1);
   r.x.dx = 8 * (y - 1);
   int86 (0x33, &r, &r);
}

/*}}}*/

static void pc_update_open_hook (void) /*{{{*/
{
   Last_Mouse_Hidden = Mouse_Hidden;
   if (Mouse_Hidden == 0) show_mouse (0);
}

/*}}}*/

static int Warp_Pending;
static void pc_update_close_hook (void) /*{{{*/
{
   if (Last_Mouse_Hidden == 0) show_mouse (1);
   if (Warp_Pending) move_mouse (Screen_Col, Screen_Row);
   Warp_Pending = 0;
}

/*}}}*/

static void warp_pointer (void) /*{{{*/
{
   Warp_Pending = 1;
}

/*}}}*/

static char *CutBuffer;
static int CutBuffer_Len;

static int insert_cutbuffer (void) /*{{{*/
{
   CHECK_READ_ONLY
   if (CutBuffer == NULL) return 0;
   if (CutBuffer_Len) jed_insert_nbytes ((unsigned char *) CutBuffer, CutBuffer_Len);
   return CutBuffer_Len;
}

/*}}}*/

static void region_to_cutbuffer (void) /*{{{*/
{
   int nbytes;

   if (CutBuffer != NULL)
     {
	SLfree (CutBuffer);
     }

   CutBuffer = make_buffer_substring(&nbytes);
   CutBuffer_Len = nbytes;
}

/*}}}*/

static SLang_Intrin_Fun_Type gpm_mouse_table[] = /*{{{*/
{
   MAKE_INTRINSIC("x_warp_pointer", warp_pointer, VOID_TYPE, 0),
   MAKE_INTRINSIC("x_insert_cutbuffer", insert_cutbuffer, INT_TYPE, 0),
   /* Prototype: Integer x_insert_cut_buffer ();
    * Inserts cutbuffer into the current buffer and returns the number
    * of characters inserted.
    */
   MAKE_INTRINSIC("x_copy_region_to_cutbuffer", region_to_cutbuffer, VOID_TYPE, 0),
   /* Prototype: Void x_copy_region_to_cutbuffer();
    */
   SLANG_END_TABLE
};

/*}}}*/

void (*X_Close_Mouse_Hook)(void);
static int pc_open_mouse (void) /*{{{*/
{
   union REGS r;
   int rows, cols, scanlines;
   static int not_first_time;

   X_Close_Mouse_Hook = NULL;
   JMouse_Event_Hook = NULL;
   JMouse_Hide_Mouse_Hook = NULL;
   X_Update_Open_Hook = NULL;
   X_Update_Close_Hook = NULL;
#ifndef __GO32__
   if (getvect (0x33) == NULL) return -1;
#endif
   /* Now see if the mouse is present */
   r.x.ax = 0x21;
   int86 (0x33, &r, &r);
   if (r.x.ax == 0x21) return -1;

   jed_get_screen_size (&rows, &cols);

#ifdef __GO32__
   scanlines = _farpeekw (_go32_conventional_mem_selector (), 0x485);
#else
   scanlines = *(int *) 0x485;
#endif
   if (scanlines <= 0) scanlines = 8;

   r.x.ax = 7;
   r.x.cx = 0;
   r.x.dx = 8 * cols - 1;
   int86 (0x33, &r, &r);

   r.x.ax = 8;
   r.x.cx = 0;
   r.x.dx = scanlines * rows - 1;
   int86 (0x33, &r, &r);

   move_mouse (cols / 2 + 1, rows / 2 + 1);

   if (not_first_time == 0)
     {
	if (-1 == SLadd_intrin_fun_table (gpm_mouse_table, "MOUSE"))
	  return -1;

	not_first_time = 1;
     }

   JMouse_Hide_Mouse_Hook = show_mouse;
   X_Close_Mouse_Hook = pc_close_mouse;
   JMouse_Event_Hook = pc_get_mouse_event;
   X_Update_Open_Hook = pc_update_open_hook;
   X_Update_Close_Hook = pc_update_close_hook;
   return 1;
}

/*}}}*/

int (*X_Open_Mouse_Hook)(void) = pc_open_mouse;
