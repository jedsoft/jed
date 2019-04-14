/* Copyright (c) 1992-2019 John E. Davis
 *
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifdef HAS_MOUSE

#define INCL_ERRORS
#define INCL_SUB
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#include "config.h"
#include "jed-feat.h"

#include <os2.h>

#include <process.h>

static void show_mouse (int show);

static int Mouse_X, Mouse_Y;

#if __os2_16__
typedef USHORT APIRET;
#define DosSetPriority DosSetPrty
#endif

#define THREADSTACKSIZE      32768
static TID Os2_Mouse_ThreadID = (TID) 0;

static int Mouse_Hidden = 1;
static int Last_Mouse_Hidden = 1;

static HMOU Mouse_Handle = -1;

#define	MOUSE_BN1 (MOUSE_MOTION_WITH_BN1_DOWN | MOUSE_BN1_DOWN)
#define	MOUSE_BN2 (MOUSE_MOTION_WITH_BN2_DOWN | MOUSE_BN2_DOWN)
#define	MOUSE_BN3 (MOUSE_MOTION_WITH_BN3_DOWN | MOUSE_BN3_DOWN)

#define	MOUSE_NOMOVE  (MOUSE_BN1_DOWN | MOUSE_BN2_DOWN | MOUSE_BN3_DOWN)
#define	MOUSE_DRAGGED (MOUSE_MOTION_WITH_BN1_DOWN | MOUSE_MOTION_WITH_BN2_DOWN | MOUSE_MOTION_WITH_BN3_DOWN)
#define	MOUSE_MOVED   (MOUSE_MOTION | MOUSE_DRAGGED)

#define	MouseNoMove(event) (MOUSE_NOMOVE & event.fs)
#define	MouseMoved(event)  (MOUSE_MOVED & event.fs)
#define MouseDragged(event) (MOUSE_DRAGGED & event.fs)
#define	MouseButtonPressed(event,button) (event.fs & button)
#define	MouseButtons(event) (event.fs &	(MOUSE_BN1 | MOUSE_BN2 | MOUSE_BN3))

static int buttonDown [3] = {0, 0, 0};
static int buttonMask [3];
USHORT noButtons;

static void os2_add_event (int x, int y, int type, int but)
{
   char buf[4];
   int b, s, id;
   JMouse_Type jm;

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

static void os2_mouse_handler (void *Args)
{
   MOUEVENTINFO event_info;
   APIRET Error;
   USHORT wait_flag;
   int press, i;
   short x, y;
   KBDINFO kbd_info;

   (void) Args;
   wait_flag = MOU_NOWAIT;

   DosSetPriority( PRTYS_THREAD, PRTYC_NOCHANGE, PRTYD_MINIMUM, 0 ) ;

   while (Mouse_Handle) {

      Error = MouReadEventQue (&event_info, &wait_flag, Mouse_Handle);
      if ((Error != NO_ERROR) || !event_info.time) {
	DosSleep (5);
        continue;
      }

      x = event_info.col + 1;
      y = event_info.row + 1;

      if (MouseMoved (event_info) && ((x != Mouse_X) || (y != Mouse_Y)))
	{
	   /* mouse moved so show it. */
	   show_mouse (1);
	}

      for(i = 0; i < noButtons; i++) {
	 if(MouseButtonPressed (event_info, buttonMask[i])) {
	    if (!buttonDown [i]) {
	       /* I don't think that we can find shift key status in OS/2, since we
		* are reading the keyboard in another thread */
	       kbd_info.cb = sizeof (kbd_info);
	       KbdGetStatus (&kbd_info, 0);
	       press = (i + 1) | ((kbd_info.fsState & 0xF) << 4);
	       os2_add_event (x, y, JMOUSE_DOWN, press);
	       buttonDown [i] = 1;
	       continue;
	    } else {         /* button already down */
	       if (MouseDragged (event_info))
		 {
		    if ((x == Mouse_X) && (y == Mouse_Y)) continue;
		    os2_add_event (x, y, JMOUSE_DRAG, i + 1);
		    continue;
		 }
	    }

	 } else {
	    if (buttonDown [i]) {
	       os2_add_event (x, y, JMOUSE_UP, i + 1);
	       buttonDown [i] = 0;
	       continue;
	    }
	 }
      }
   }
   Mouse_Handle = -1;
   _endthread ();
}

static int os2_get_mouse_event (void)
{
   return 0;
}

/* Mouse routines for OS/2 */

static void show_mouse (int show)
{
   NOPTRRECT PtrRect;

   if (show)
     {
	if (Mouse_Hidden == 0) return;
	MouDrawPtr (Mouse_Handle);
     }
   else
     {
	if (Mouse_Hidden) return;
	PtrRect.row = 0;
	PtrRect.col = 0;
	PtrRect.cRow = Jed_Num_Screen_Rows - 1;
	PtrRect.cCol = Jed_Num_Screen_Cols - 1;
	MouRemovePtr (&PtrRect, Mouse_Handle);
     }
   Mouse_Hidden = !show;
}

static void os2_close_mouse ()
{
   MouClose (Mouse_Handle);
   Mouse_Handle = 0;
#if defined(__os2_16__)
   while (!Mouse_Handle) DosSleep(0);
#else
   DosWaitThread (&Os2_Mouse_ThreadID, DCWW_WAIT);
#endif
   Os2_Mouse_ThreadID = 0;
}

static void move_mouse (int x, int y)
{
   PTRLOC ptr_loc;
   ptr_loc.row = y - 1;
   ptr_loc.col = x - 1;
   MouSetPtrPos (&ptr_loc, Mouse_Handle);
}

static void os2_update_open_hook (void)
{
   Last_Mouse_Hidden = Mouse_Hidden;
   if (Mouse_Hidden == 0) show_mouse (0);
}

static int Warp_Pending;
static void os2_update_close_hook (void)
{
   if (Last_Mouse_Hidden == 0) show_mouse (1);
   if (Warp_Pending) move_mouse (Screen_Col - 1, Screen_Row - 1);
   Warp_Pending = 0;
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

static SLang_Intrin_Fun_Type os2_mouse_table[] =
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

void (*X_Close_Mouse_Hook)(void);

static int os2_open_mouse ()
{
   static int not_first_time = 0;
   USHORT Error;

   X_Close_Mouse_Hook = NULL;
   JMouse_Event_Hook = NULL;
   JMouse_Hide_Mouse_Hook = NULL;
   X_Update_Open_Hook = NULL;
   X_Update_Close_Hook = NULL;

   /* Now open the device */

   Error = MouOpen (NULL, &Mouse_Handle);
   if (Error)
      return -1;

   Mouse_X = Jed_Num_Screen_Cols / 2 + 1;
   Mouse_Y = Jed_Num_Screen_Rows / 2 + 1;

   MouDrawPtr (Mouse_Handle);
   MouGetNumButtons (&noButtons, Mouse_Handle);
   if (noButtons == 3)
     buttonMask [2] = MOUSE_BN3;

   buttonMask [0] = MOUSE_BN1;
   buttonMask [1] = MOUSE_BN2;

   MouFlushQue(Mouse_Handle);		/* flush mouse queue		*/

   move_mouse (Mouse_X, Mouse_Y);
   if (not_first_time == 0)
     {
	if (-1 == SLadd_intrin_fun_table (os2_mouse_table, "MOUSE"))
	  return -1;

	not_first_time = 1;
     }

#if defined __BORLANDC__
   Os2_Mouse_ThreadID = _beginthread (os2_mouse_handler, THREADSTACKSIZE, NULL);
#else
   Os2_Mouse_ThreadID = _beginthread (os2_mouse_handler, NULL, THREADSTACKSIZE, NULL);
#endif

   JMouse_Hide_Mouse_Hook = show_mouse;
   X_Close_Mouse_Hook = os2_close_mouse;
/* I am not sure if I should set this */
   JMouse_Event_Hook = os2_get_mouse_event;
   X_Update_Open_Hook = os2_update_open_hook;
   X_Update_Close_Hook = os2_update_close_hook;
   return 1;
}

int (*X_Open_Mouse_Hook)(void) = os2_open_mouse;

#endif
