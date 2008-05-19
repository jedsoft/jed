/* This is the interface to the GPM mouse under Linux */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#include <signal.h>
#include <slang.h>

#include <gpm.h>

#include "jdmacros.h"

#include "buffer.h"
#include "sysdep.h"
#include "keymap.h"
#include "misc.h"
#include "paste.h"
#include "screen.h"
#include "ledit.h"
#include "ins.h"
#include "display.h"
#include "hooks.h"

#define MOD_SHIFT 1
#define MOD_CTRL 4

static int Mouse_Showing;
static int MouseX, MouseY;
static int Suspend_Mouse_Events;

#if 0
/* This is needed to get the link to work with gpm. */
int wgetch (void)
{
   return 0;
}
int *stdscr;
#endif

static void draw_mouse (void)
{
   Gpm_Event event;
   
   if (MouseX < 1)
     MouseX = 1;
   else if (MouseX >= Jed_Num_Screen_Cols)
     MouseX = Jed_Num_Screen_Cols;

   if (MouseY < 1)
     MouseY = 1;
   else if (MouseY >= Jed_Num_Screen_Rows)
     MouseY = Jed_Num_Screen_Rows;

   event.x = MouseX;
   event.y = MouseY;
	
   GPM_DRAWPOINTER(&event);
   Mouse_Showing = 1;
}


static int mouse_handler_2 (void)
{
   int b = 0, nbuttons;
   Gpm_Event event;
   unsigned char buf[3];
   JMouse_Type jm;
   
   if (Gpm_GetEvent (&event) <= 0) return -1;
   if (Suspend_Mouse_Events) 
     {
	if (Suspend_Mouse_Events == -1) 
	  Suspend_Mouse_Events = 0;
	return -1;
     }
   
   MouseX += event.dx;
   MouseY += event.dy;
   
   draw_mouse ();
   
   if (event.type & GPM_MOVE) return 0;
   
   nbuttons = 0;
   if (event.buttons & GPM_B_LEFT)
     {
	b = JMOUSE_BUTTON_1;
	nbuttons++;
     }
   if (event.buttons & GPM_B_MIDDLE)
     {
	b = JMOUSE_BUTTON_2;
	nbuttons++;
     }
   if (event.buttons & GPM_B_RIGHT)
     {
	b = JMOUSE_BUTTON_3;
	nbuttons++;
     }
   
   if (nbuttons != 1) return 0;

   jm.button = b;
   
   if ((event.type & GPM_DOUBLE)
       && (event.type & GPM_DOWN)) jm.type = JMOUSE_DOUBLE_CLICK;
   else if ((event.type & GPM_TRIPLE)
	    && (event.type & GPM_DOWN)) jm.type = JMOUSE_TRIPLE_CLICK;
   else if (event.type & GPM_DRAG) jm.type = JMOUSE_DRAG;
   else if (event.type & GPM_DOWN) jm.type = JMOUSE_DOWN;
   else if (event.type & GPM_UP) jm.type = JMOUSE_UP;
   else return 0;
   
   if (event.modifiers & MOD_SHIFT)
     {
	jm.state = JMOUSE_SHIFT;
     }
   else if (event.modifiers & MOD_CTRL)
     {
	jm.state = JMOUSE_CTRL;
     }
   else jm.state = 0;
   
   jm.x = MouseX;
   jm.y = MouseY;
   
   b = jed_mouse_add_event (&jm);
   if (b == -1) return 0;
   
   buf[0] = 27; buf[1] = 0; buf[2] = (char) b;
   ungetkey_string ((char *)buf, 3);
   return 1;
}


static int Warp_Pending;
static void close_update (void)
{
   if (Warp_Pending || Mouse_Showing)
     {
	if (Warp_Pending)
	  {
	     MouseX = Screen_Col;
	     MouseY = Screen_Row;
	  }
	draw_mouse ();
	Warp_Pending = 0;
     }
   else Mouse_Showing = 0;
}

static void warp_pointer (void)
{
   Warp_Pending = 1;
}

static char *CutBuffer;
static int CutBuffer_Len;

static int insert_cutbuffer (void)
{
   CHECK_READ_ONLY
   if (CutBuffer == NULL) return 0;
   if (CutBuffer_Len) jed_insert_nbytes ((unsigned char *) CutBuffer, CutBuffer_Len);
   return CutBuffer_Len;
}

static void region_to_cutbuffer (void)
{
   int nbytes;
   
   if (CutBuffer != NULL)
     {
	SLfree (CutBuffer);
     }
   
   CutBuffer = make_buffer_substring(&nbytes);
   CutBuffer_Len = nbytes;
}

static void close_mouse (void)
{
   JMouse_Hide_Mouse_Hook = NULL;
   X_Update_Close_Hook = NULL;

   if (JMouse_Event_Hook == NULL) return;
   JMouse_Event_Hook = NULL;
   
   Gpm_Close ();
}

static void disable_mouse (void)
{
   reset_tty ();		       /* closes mouse */
   X_Open_Mouse_Hook = NULL;
   X_Close_Mouse_Hook = NULL;
   init_tty ();
}

static SLang_Intrin_Fun_Type gpm_mouse_table[] =
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
   MAKE_INTRINSIC("gpm_disable_mouse", disable_mouse, VOID_TYPE, 0),
   SLANG_END_INTRIN_FUN_TABLE
};

static void hide_mouse (int show)
{
   Mouse_Showing = show;
}

static int open_mouse (void)
{
   static int not_first_time;
   Gpm_Connect conn;
   char *term;
   SLSig_Fun_Type *sigtstp_fun;
   int status;

   /* Unbelievable.  If we are running in an Xterm, gpm will turn on mouse
    * reporting.  Nice huh? NOT!!!
    */
   term = getenv ("TERM");
   if ((term != NULL)
       && (!strncmp (term, "xterm", 5) || !strncmp (term, "rxvt", 4)))
     return -1;

   /* Another libgpm annoyance.  gpm installs a signal handler for SIGTSTP.
    * I am going to un-install it.
    */
   sigtstp_fun = SLsignal (SIGTSTP, (SLSig_Fun_Type *)SIG_DFL);

   conn.eventMask = ~0;
   conn.defaultMask = 0;
   conn.maxMod = MOD_CTRL | MOD_SHIFT;
   conn.minMod = 0;

   Mouse_Showing = 0;
   Suspend_Mouse_Events = -1;
   MouseX = Jed_Num_Screen_Cols / 2;
   MouseY = Jed_Num_Screen_Rows / 2;
   
   status = Gpm_Open (&conn, 0);
   /* Uninstall the gpm signal handler */
   (void) SLsignal (SIGTSTP, sigtstp_fun);

   if (status == -1)
     return -1;

   if (not_first_time == 0)
     {
	if (-1 == SLadd_intrin_fun_table (gpm_mouse_table, "MOUSE"))
	  return -1;
	
	not_first_time = 1;
     }
   
   JMouse_Event_Hook = mouse_handler_2;
   X_Update_Close_Hook = close_update;
   JMouse_Hide_Mouse_Hook = hide_mouse;
   return gpm_fd;
}

   
int (*X_Open_Mouse_Hook)(void) = open_mouse;
void (*X_Close_Mouse_Hook)(void) = close_mouse;

   
