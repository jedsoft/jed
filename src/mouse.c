/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2022 John E. Davis
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
#include <slang.h>

#ifdef HAS_MOUSE

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

/*}}}*/

int (*JMouse_Event_Hook)(void);
void (*JMouse_Hide_Mouse_Hook) (int);

/*{{{ Static Global Variables */

static SLang_Name_Type *Jed_Default_Mouse_Down_Hook;
static SLang_Name_Type *Jed_Mouse_Status_Down_Hook;
static SLang_Name_Type *Jed_Default_Mouse_Up_Hook;
static SLang_Name_Type *Jed_Mouse_Status_Up_Hook;
static SLang_Name_Type *Jed_Default_Mouse_Drag_Hook;
static SLang_Name_Type *Jed_Mouse_Status_Drag_Hook;

#if JED_HAS_MULTICLICK
static SLang_Name_Type *Jed_Mouse_Status_2Click_Hook;
static SLang_Name_Type *Jed_Mouse_Status_3Click_Hook;
static SLang_Name_Type *Jed_Default_Mouse_2Click_Hook;
static SLang_Name_Type *Jed_Default_Mouse_3Click_Hook;
#endif

static Window_Type *Use_This_Window;
static Window_Type *Save_Window;
static unsigned int Down_Mask;

#ifdef IBMPC_SYSTEM
# define JED_MOUSE_MAX_QUEUE_SIZE 8
#else
# define JED_MOUSE_MAX_QUEUE_SIZE 64
#endif
static JMouse_Type Mouse_Queue [JED_MOUSE_MAX_QUEUE_SIZE];
static JMouse_Type Current_Event;

static int Mouse_Queue_Hint;

static unsigned char Mouse_Button_Map [6] =
{
   JMOUSE_BUTTON_1,
   JMOUSE_BUTTON_2,
   JMOUSE_BUTTON_3,
   JMOUSE_BUTTON_4,
   JMOUSE_BUTTON_5,
   JMOUSE_BUTTON_6
};

/*}}}*/

#ifndef USE_GPM_MOUSE
# if defined(__MSDOS__)
#  if !defined(MSWINDOWS)
#   include "pcmouse.c"
#  endif
# else
#  ifdef __os2__
#   include "os2mouse.c"
#  else
int (*X_Open_Mouse_Hook)(void);
void (*X_Close_Mouse_Hook)(void);
#  endif /* os2 */
# endif /* ibmpc */
#endif /* NOT USE_GPM_MOUSE */

#if JED_HAS_MENUS
static int Hack_Hack = 1;
static Window_Type *Non_Existent_Menu_Window = (Window_Type *)&Hack_Hack;
#endif
#define BUFFER_STATUS_LINE	1
#define MENU_BAR_STATUS		2

/* returns button number from map */
static unsigned char m2b (int m)
{
   switch (m)
     {
      default:
      case 0x01: return 0;
      case 0x02: return 1;
      case 0x04: return 2;
      case 0x08: return 3;
      case 0x10: return 4;
      case 0x20: return 5;
     }
}

/* *ap and *bp must have one of values in set (1, 2, 4, 8, 16, 32, 64).  */
void jed_map_mouse_buttons (int *ap, int *bp) /*{{{*/
{
   unsigned char a, b;

   a = m2b (*ap);
   b = m2b (*bp);

   if ((a > 5) || (b > 5)) return;

   Mouse_Button_Map [a] = b << 1;
}

/*}}}*/

int jed_mouse_add_event (JMouse_Type *ev) /*{{{*/
{
   int save = Mouse_Queue_Hint;
   /* Search the queue looking for an empty slot. */
   while (Mouse_Queue_Hint < JED_MOUSE_MAX_QUEUE_SIZE)
     {
	if (Mouse_Queue[Mouse_Queue_Hint].type == 0)
	  {
	     Mouse_Queue [Mouse_Queue_Hint] = *ev;
	     return Mouse_Queue_Hint;
	  }
	Mouse_Queue_Hint++;
     }

   Mouse_Queue_Hint = save;
   while (Mouse_Queue_Hint > 0)
     {
	Mouse_Queue_Hint--;
	if (Mouse_Queue[Mouse_Queue_Hint].type == 0)
	  {
	     Mouse_Queue [Mouse_Queue_Hint] = *ev;
	     return Mouse_Queue_Hint;
	  }
     }

   if (Input_Buffer_Len == 0)
     {
	jed_flush_mouse_queue ();
	Mouse_Queue [0] = *ev;
	return 0;
     }

   return -1;
}

/*}}}*/

static JMouse_Type *get_mouse_event (unsigned int *type) /*{{{*/
{
   int queue_pos;
   JMouse_Type *jm;

   /* Keyboard input should be ready.  If it is not, get out. */
   if (0 == input_pending (&Number_Zero))
     return NULL;

   queue_pos = jed_getkey ();

   if ((queue_pos >= JED_MOUSE_MAX_QUEUE_SIZE)
       || (queue_pos < 0))
     return NULL;

   jm = Mouse_Queue + queue_pos;
   /* 1, 2, 4, 8,..., 32 --> Mouse_Button_Map of 0, 1, 2, ... 5 */
   jm->button = Mouse_Button_Map [m2b(jm->button)];
   *type = jm->type;
   jm->type = 0;

   Mouse_Queue_Hint = queue_pos;

   return jm;
}

/*}}}*/

void jed_mouse_get_event_info (void) /*{{{*/
{
   SLang_push_integer (Current_Event.x);
   SLang_push_integer (Current_Event.y);
   SLang_push_integer (Current_Event.state);
}

/*}}}*/

void jed_flush_mouse_queue (void) /*{{{*/
{
   unsigned int i;

   for (i = 0; i < JED_MOUSE_MAX_QUEUE_SIZE; i++)
     Mouse_Queue[i].type = 0;
   Mouse_Queue_Hint = 0;
}

/*}}}*/

static int window_exists (Window_Type *win) /*{{{*/
{
   Window_Type *w;

   w = JWindow;
   do
     {
	if (w == win) return 1;
	w = w->next;
     }
   while (w != JWindow);
   return 0;
}

/*}}}*/

static int switch_to_event_window (int x, int y, int *linep, int *colp, int *status) /*{{{*/
{
   Window_Type *w;
   int delta_y;
   static int last_status;

   *status = 0;

   if (Use_This_Window != NULL)
     {
#if JED_HAS_MENUS
	if (Use_This_Window == Non_Existent_Menu_Window)
	  {
	     *status = last_status = MENU_BAR_STATUS;
	     *colp = x;
	     *linep = y;
	     return 0;
	  }
#endif
	if (0 == window_exists (Use_This_Window))
	  {
	     last_status = 0;
	     return -1;
	  }
	*status = last_status;
     }
   else
     {
#if JED_HAS_MENUS
	if (Jed_Menus_Active
	    || ((y == 1) && (Top_Window_SY != 0)))
	  {
	     Use_This_Window = Non_Existent_Menu_Window;
	     *colp = x;
	     *linep = y;
	     *status = last_status = MENU_BAR_STATUS;
	     return 0;
	  }
#endif
	w = JWindow;
	do
	  {
	     int sy, bot;

	     sy = w->sy;
	     bot = sy + w->rows + 1;

	     if ((y >= sy) && (y < bot))
	       {
		  Use_This_Window = w;
		  break;
	       }
	     if (y == bot)
	       {
		  Use_This_Window = w;
		  *status = BUFFER_STATUS_LINE;
		  break;
	       }
	     w = w->next;
	  }
	while (w != JWindow);
     }

   if (Use_This_Window == NULL)
     {
	last_status = 0;
	return -1;
     }

   while (JWindow != Use_This_Window)
     other_window ();

   last_status = *status;
   if (last_status)
     {
	*colp = x;
	return 0;
     }

   /* What line does y correspond ??? */
   delta_y = y - (Use_This_Window->sy + window_line ());
   y = LineNum;
#if JED_HAS_LINE_ATTRIBUTES
   if (delta_y == 0)
     {
	Line *l = CLine;
	while ((l != NULL)
	       && (l->flags & JED_LINE_HIDDEN))
	  {
	     y--;
	     l = l->prev;
	  }
     }
   else if (delta_y > 0)
     {
	Line *l = CLine;
	while (delta_y && (l != NULL))
	  {
	     if (0 == (l->flags & JED_LINE_HIDDEN))
	       delta_y--;
	     l = l->next;
	     y++;
	  }

	while ((l != NULL) && (l->flags & JED_LINE_HIDDEN))
	  {
	     l = l->next;
	     y++;
	  }
     }
   else if (delta_y < 0)
     {
	Line *l = CLine;
	while (delta_y && (l != NULL))
	  {
	     if (0 == (l->flags & JED_LINE_HIDDEN))
	       delta_y++;
	     l = l->prev;
	     y--;
	  }

	while ((l != NULL) && (l->flags & JED_LINE_HIDDEN))
	  {
	     l = l->prev;
	     y--;
	  }
     }
#endif

   *linep = y + delta_y;
   *colp = JWindow->hscroll_column + x - 1;
   return 0;
}

/*}}}*/

static int do_function (SLang_Name_Type *fun, int line, int col, int button, int shift) /*{{{*/
{
   int ret;

   if ((-1 == SLang_start_arg_list ())
       || (-1 == SLang_push_integer (line))
       || (-1 == SLang_push_integer (col))
       || (-1 == SLang_push_integer (button))
       || (-1 == SLang_push_integer (shift))
       || (-1 == SLang_end_arg_list ()))
     return 1;

   if ((-1 == SLexecute_function (fun))
       || (-1 == SLang_pop_integer (&ret)))
     ret = 1;

   return ret;
}

/*}}}*/

static void switch_to_window (Window_Type *w) /*{{{*/
{
   if (window_exists (w))
     {
	while (JWindow != w) other_window ();
     }
}

/*}}}*/

static int do_mouse_cmd (JMouse_Type *jmouse, unsigned int type) /*{{{*/
{
   int button, shift;
   int linenum = -1, column, is_status;
   int ret;
   SLang_Name_Type *fun, *default_fun;
   Jed_Buffer_Hook_Type *h = CBuf->buffer_hooks;

   button = jmouse->button;

   /* Consistency check */
   if (type == JMOUSE_DOWN)
     {
	if (Down_Mask & button)
	  {
	     Down_Mask = 0;
	     return 0;
	  }
	Save_Window = JWindow;
     }
   else if ((0 == (Down_Mask & button))
	    && (type != JMOUSE_DOUBLE_CLICK)
	    && (type != JMOUSE_TRIPLE_CLICK))
     {
	Down_Mask = 0;
	return 0;
     }

   if (Down_Mask == 0)
     Use_This_Window = NULL;

   if (-1 == switch_to_event_window (jmouse->x, jmouse->y,
				     &linenum, &column, &is_status))
     {
	Down_Mask = 0;
	Use_This_Window = NULL;
	return 0;
     }

   default_fun = NULL;
   fun = NULL;

   switch (type)
     {
      case JMOUSE_DRAG:
	if (is_status) fun = Jed_Mouse_Status_Drag_Hook;
	else
	  {
	     if (h != NULL) fun = h->mouse_drag_hook;
	     default_fun = Jed_Default_Mouse_Drag_Hook;
	  }
	break;
#if JED_HAS_MULTICLICK
      case JMOUSE_DOUBLE_CLICK:
	if (is_status) fun = Jed_Mouse_Status_2Click_Hook;
	else
	  {
	     if (h != NULL) fun = h->mouse_2click_hook;
	     default_fun = Jed_Default_Mouse_2Click_Hook;
	  }
	break;

      case JMOUSE_TRIPLE_CLICK:
	if (is_status) fun = Jed_Mouse_Status_3Click_Hook;
	else
	  {
	     if (h != NULL) fun = h->mouse_3click_hook;
	     default_fun = Jed_Default_Mouse_3Click_Hook;
	  }
	break;
#endif
      case JMOUSE_DOWN:
	Down_Mask |= button;
	if (is_status) fun = Jed_Mouse_Status_Down_Hook;
	else
	  {
	     if (h != NULL) fun = h->mouse_down_hook;
	     default_fun = Jed_Default_Mouse_Down_Hook;
	  }
	break;

      case JMOUSE_UP:
	Down_Mask &= ~button;

	if (is_status) fun = Jed_Mouse_Status_Up_Hook;
	else
	  {
	     if (h != NULL) fun = h->mouse_up_hook;
	     default_fun = Jed_Default_Mouse_Up_Hook;
	  }
	break;

      default:
	flush_input ();
	fun = NULL;
     }

   if (jmouse->state & JMOUSE_SHIFT)
     shift = 1;
   else if (jmouse->state & JMOUSE_CTRL)
     shift = 2;
   else shift = 0;

#if JED_HAS_MENUS
   if (is_status == MENU_BAR_STATUS)
     return jed_menu_handle_mouse (type, column, linenum, button, shift);
#endif

   ret = -1;
   if (fun != NULL)
     {
	ret = do_function (fun, linenum, column, button, shift);
     }

   if ((ret == -1) && (default_fun != NULL))
     {
	ret = do_function (default_fun, linenum, column, button, shift);
     }

   if ((type == JMOUSE_UP)
       && (Down_Mask == 0)
       && (ret <= 0))
     {
	switch_to_window (Save_Window);
	Save_Window = NULL;
     }

   return ret;
}

/*}}}*/

int jed_mouse_cmd (void) /*{{{*/
{
   unsigned int type;
   JMouse_Type *jmouse;

   if ((NULL == (jmouse = get_mouse_event (&type)))
       || (type == JMOUSE_IGNORE_EVENT))
     return 0;

   Current_Event = *jmouse;

#if JED_HAS_MULTICLICK
   if ((-1 == do_mouse_cmd (jmouse, type))
       && ((type == JMOUSE_DOUBLE_CLICK) || (type == JMOUSE_TRIPLE_CLICK)))
     {
	/* Event multiclick event not handled so simulated click. */
	(void) do_mouse_cmd (&Current_Event, JMOUSE_DOWN);
	(void) do_mouse_cmd (&Current_Event, JMOUSE_UP);
     }
#else
   (void) do_mouse_cmd (jmouse, type);
#endif
   return 1;
}

/*}}}*/

void jed_set_current_mouse_window (void) /*{{{*/
{
   switch_to_window (Save_Window);
}

/*}}}*/

void jed_set_default_mouse_hook (char *hook_name, char *function) /*{{{*/
{
   SLang_Name_Type *f;

   f = SLang_get_function (function);

   if (!strcmp ("mouse_drag", hook_name))
     Jed_Default_Mouse_Drag_Hook = f;
   else if (!strcmp ("mouse_up", hook_name))
     Jed_Default_Mouse_Up_Hook = f;
   else if (!strcmp ("mouse_down", hook_name))
     Jed_Default_Mouse_Down_Hook = f;
   else if (!strcmp ("mouse_status_up", hook_name))
     Jed_Mouse_Status_Up_Hook = f;
   else if (!strcmp ("mouse_status_down", hook_name))
     Jed_Mouse_Status_Down_Hook = f;
   else if (!strcmp ("mouse_status_drag", hook_name))
     Jed_Mouse_Status_Drag_Hook = f;
   else if (!strcmp ("mouse_status_drag", hook_name))
     Jed_Mouse_Status_Drag_Hook = f;
#if JED_HAS_MULTICLICK
   else if (!strcmp ("mouse_2click", hook_name))
     Jed_Default_Mouse_2Click_Hook = f;
   else if (!strcmp ("mouse_3click", hook_name))
     Jed_Default_Mouse_3Click_Hook = f;
#endif
}

/*}}}*/

#endif				       /* HAS_MOUSE */
