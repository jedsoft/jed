/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <windows.h>
#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#include <process.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
# include <direct.h>
#elif defined(__BORLANDC__)
# include <dir.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <time.h>

#include <assert.h>
#include <io.h>
#include <errno.h>

#include "display.h"
#include "buffer.h"
#include "sysdep.h"
#include "screen.h"
#include "keymap.h"
#include "hooks.h"
#include "ins.h"
#include "ledit.h"
#include "misc.h"
#include "cmds.h"
#include "sig.h"
#include "win32.h"
#if JED_HAS_SUBPROCESSES
# include "jprocess.h"
#endif

#define KEY_SHIFT         1
#define KEY_CONTROL       2
#define KEY_ALT	 	  4
#define KEY_ALT_GR 	  8

int NumLock_Is_Gold = 0;
int PC_Alt_Char = 27;
int PC_Fn_Char = 0;

#if SLANG_VERSION < 20000
# define SLw32_Hstdin _SLw32_Hstdin
extern HANDLE _SLw32_Hstdin;
#endif

static unsigned char f_keys[4][12] =
{
     { 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 133, 134 },
     { 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 135, 136 },
     { 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 137, 138 },
     { 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 139, 140 }
};

static unsigned char small_keypad_keys[4][13] =
{
     { 'G', 'H', 'I', 0, 'K', 0, 'M', 0, 'O', 'P', 'Q', 'R', 'S' },   /* normal */
     { '0', '1', '2', 0, '3', 0, '4', 0, '5', '6', '7', '8', '9' },   /* shift */
     { 'w', 141, 132, 0, 's', 0, 't', 0, 'u', 145, 'v', 146, 147 },   /* ctrl */
     { 151, 152, 153, 0, 155, 0, 157, 0, 159, 160, 161, 162, 163 }   /* alt */
};

static unsigned char num_keypad_keys[4][13] =
{
     { 'w', 'x', 'y', 0, 't', 'u', 'v', 0, 'q', 'r', 's', 'p', 'n' },
     { '0', '1', '2', 0, '3',  0 , '4', 0, '5', '6', '7', '8', '9' },
     { 'w', 141, 132, 0, 's', 143, 't', 0, 'u', 145, 'v', 146, 147 },
     { 'w', 'x', 'y', 0, 't', 'u', 'v', 0, 'q', 'r', 's', 'p', 'n' }
};

static void _putkey(unsigned char c)
{
   buffer_keystring ((char *)&c, 1);
}

static void process_key_event(KEY_EVENT_RECORD *key)
{
   unsigned int key_state = 0;
   unsigned int scan;
   unsigned char prefix, c1 = 0;
   int i, state;
   DWORD d = key->dwControlKeyState;

   if (!key->bKeyDown) return;
   if (d & LEFT_ALT_PRESSED) key_state |= KEY_ALT;
   if (d & RIGHT_ALT_PRESSED) key_state |= KEY_ALT_GR;
   if (d & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) key_state |= KEY_CONTROL;
   if (d & SHIFT_PRESSED) key_state |= KEY_SHIFT;
   
   scan = key->wVirtualScanCode;
   if (d & ENHANCED_KEY) 
     scan |= 0x100;

   switch (scan)
     {
      case 0x00F:		       /* TAB */
	if (key_state & KEY_SHIFT)
	  {
	     /* Same as DOS */
	     buffer_keystring ("\000\017", 2);
	     return;
	  }
	break;

      case 0x00E:		       /* backspace */
	_putkey (127);
	return;

      case 0x039: 		       /* space */
	if (key_state & KEY_CONTROL)
	  {
	     buffer_keystring ("\000\003", 2);
	     return;
	  }
	break;

      case 0x003:		       /* 2 key */
	if (((key_state & KEY_ALT) == 0)
	    && (key_state & KEY_CONTROL))
	  {
	     buffer_keystring ("\000\003", 2);
	     return;
	  }
	break;

      case 0x007:		       /* 6 key */
	if (((key_state & KEY_ALT) == 0)
	    && (key_state & KEY_CONTROL))
	  {
	     _putkey (30);	       /* Ctrl-^ */
	     return;
	  }
	break;

      case 0x00C:		       /* -/_ key */
	if (((key_state & KEY_ALT) == 0)
	    && (key_state & KEY_CONTROL))
	  {
	     _putkey (31);	       /* ^_ */
	     return;
	  }
	break;

	/* PF1 and PF2 codes */
      case 0x0045:		       /* NUMLOCK */
      case 0x0145:
	if (NumLock_Is_Gold)
	  {
#if 0/* this does not appear to work on xp */
	     BYTE keyState[256];
	     /* toggle the caps-lock state */
	     GetKeyboardState(keyState);
	     keyState[VK_NUMLOCK] = keyState[VK_NUMLOCK]^1;
	     SetKeyboardState(keyState);
#endif
	     buffer_keystring ("\033OP", 3);
	     return;
	  }
	break;

      case 0x0135:
	c1 = 'Q'; break ;

      case 0xE02F:
	c1 = 'Q'; break;	       /* KEYPAD SLASH */

      case 0x037:		       /* KEYPAD STAR */
	c1 = 'R';
	break;

      case 0x04A:		       /* KEYPAD MINUS */
	c1 = 'S';
	break;

      case 0x04E:		       /* KEYPAD PLUS */
	c1 = 'm';
	break;

      case 0x047:            /* KEYPAD HOME */
      case 0x048:            /* KEYPAD UP */
      case 0x049:            /* KEYPAD PGUP */
      case 0x04B:            /* KEYPAD LEFT */
      case 0x04C:            /* KEYPAD 5 */
      case 0x04D:            /* KEYPAD RIGHT */
      case 0x04F:            /* KEYPAD END */
      case 0x050:            /* KEYPAD DOWN */
      case 0x051:            /* KEYPAD PGDN */
      case 0x053:            /* KEYPAD DEL */
      case 0x052:            /* KEYPAD INSERT */
	
	if ((NumLock_Is_Gold == 0)
	    && (d & NUMLOCK_ON))
	  break;

 	state = 0;
 	if (key_state & KEY_SHIFT) state = 1;
 	if (key_state & KEY_CONTROL) state = 2;
 	if (key_state & KEY_ALT) state = 3;

 	if (key_state & (KEY_CONTROL | KEY_ALT))
	  {
 	     _putkey (0);
 	     _putkey (num_keypad_keys[state][scan - 0x47]);
 	     return;
	  }
 	else
 	  c1 = num_keypad_keys[state][scan - 0x47];
 	break;

      case 0x11C:            /* KEYPAD ENTER */
	if (key_state & KEY_ALT)
	  {
 	     _putkey(0);
 	     _putkey(166);
	     return;
	  }
	else
	  {
 	     c1 = 'M';
 	     break;
	  }

      case 0x147: 		       /* home */
      case 0x148:		       /* UP */
      case 0x149:		       /* PGUP */
      case 0x14B:		       /* LEFT */
      case 0x14D:		       /* RIGHT */
      case 0x14F:		       /* END */
      case 0x150:		       /* DOWN */
      case 0x151:		       /* PGDN */
      case 0x153:		       /* DEL */
      case 0x152:		       /* INSERT */
	prefix = 0xE0;
	state = 0;
	if (key_state & KEY_SHIFT) state = 1;
	if (key_state & KEY_CONTROL) state = 2;
	if (key_state & KEY_ALT)
	  {
	     prefix = 0;
	     state = 3;
	  }

	_putkey (prefix);
	_putkey (small_keypad_keys[state][scan - 0x147]);
	return;

      case 0x3b:		       /* F1 */
      case 0x3c:
      case 0x3d:
      case 0x3e:
      case 0x3f:
      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43:
      case 0x44:
      case 0x57:
      case 0x58:		       /* F12 */
	i = scan - 0x3b;
	if (i > 9) i -= 0x12;

	state = 0;
	if (key_state & KEY_SHIFT) state = 1;
	if (key_state & KEY_CONTROL) state = 2;
	if (key_state & KEY_ALT)
#if 0
	  if (i == 3)		       /* Alt-F4 */
	    return DefWindowProc(hWnd, msg, wParam, lParam);
	else
#endif
	  state = 3;

	_putkey(PC_Fn_Char);
	_putkey(f_keys[state][i]);
	return;

#if 0
      case 0x145:		       /* NumLock */
	c1 = 'P';
	break;
#endif
     }

   if (c1)
     {
	buffer_keystring ("\033O", 2);
	_putkey (c1);
	return;
     }

#if 0 /* defined(__MINGW32__) */
   c1 = key->AsciiChar;
#else
   c1 = key->uChar.AsciiChar;
#endif
   if (c1 != 0)
     {
	if (c1 == (char) Jed_Abort_Char)
	  {
	     if (Ignore_User_Abort == 0) SLang_set_error (USER_BREAK);
	     SLKeyBoard_Quit = 1;
	  }
	if (key_state & KEY_ALT) _putkey(PC_Alt_Char);
	_putkey(c1);
	return;
     }
#if 0
   if (1)
     {
	static FILE *fp;
	
	if (fp == NULL)
	  fp = fopen ("key.log", "w");
	
	if (fp != NULL)
	  {
	     fprintf (fp, "Key: scan=0x%X\n", scan);
	     fflush (fp);
	  }
     }
#endif
}


static void process_mouse_event(MOUSE_EVENT_RECORD *mevent)
{
   unsigned int s = 0;
   int col, row, type, button, b;
   static int last_button;
   static JMouse_Type jm;
   static DWORD last_button_state;
   DWORD state;
#if 0 /* JED_HAS_MULTICLICK */
   static DWORD last_press_time;
   static unsigned int clicks = 0;
   static unsigned long MultiClick_Time = -1;
#endif
   int id;

   col = mevent->dwMousePosition.X + 1;
   row = mevent->dwMousePosition.Y + 1;

   switch (mevent->dwEventFlags)
     {
      case 0:
#if JED_HAS_MULTICLICK
      case DOUBLE_CLICK:
#endif
	b = mevent->dwButtonState ^ last_button_state;
	if (!b) return;
	switch (b)
	  {
	   case FROM_LEFT_1ST_BUTTON_PRESSED:
	     if (!(mevent->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))
	       {
		  button = JMOUSE_BUTTON_1;
		  break;
	       }
	     /* drop */
	   case FROM_LEFT_2ND_BUTTON_PRESSED:
	     button = JMOUSE_BUTTON_2;
	     break;

	   case RIGHTMOST_BUTTON_PRESSED:
	     button = JMOUSE_BUTTON_3;
	     break;

	   default:
	     return;
	  }

	jm.button = button;
#if JED_HAS_MULTICLICK
	if (mevent->dwEventFlags == DOUBLE_CLICK)
	  type = JMOUSE_DOUBLE_CLICK;
	else
#endif
	  if (mevent->dwButtonState & b)
	    type = JMOUSE_DOWN;
	else
	  type = JMOUSE_UP;

	break;

      case MOUSE_MOVED:
	button = 0;
	if ((col == jm.x) && (row == jm.y)) return;
	type = JMOUSE_DRAG;
	break;

      default:
	return;
     }

   if (button == 0)
     button = last_button;
   else
     last_button = button;

#if 0 /* JED_HAS_MULTICLICK */
   if (type == JMOUSE_DOWN)
     {
	DWORD the_time = GetTickCount();
	if (MultiClick_Time == -1)
	  MultiClick_Time = GetDoubleClickTime();
	if ((last_button == button)
	    && (the_time - last_press_time < MultiClick_Time))
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
	     last_button = button;
	  }
	last_press_time = the_time;
     }
   else if ((clicks > 1) && (last_button == button))
     {
	/* Last was a multi-click.  Ignore this event. */
	type = JMOUSE_IGNORE_EVENT;
     }
#endif

   if ((type == JMOUSE_UP) && (0 == (jm.state & jm.button)))
     {
	return;
     }

   state = mevent->dwButtonState;
   if (state & FROM_LEFT_1ST_BUTTON_PRESSED) s |= JMOUSE_BUTTON_1;
   if (state & FROM_LEFT_2ND_BUTTON_PRESSED) s |= JMOUSE_BUTTON_2;
   if (state & RIGHTMOST_BUTTON_PRESSED) s |= JMOUSE_BUTTON_3;
   state = mevent->dwControlKeyState;
   if (state & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) s |= JMOUSE_CTRL;
   if (state & SHIFT_PRESSED) s |= JMOUSE_SHIFT;

   jm.state = s;
   jm.x = col;
   jm.y = row;
   jm.type = type;

   last_button_state = mevent->dwButtonState;
   if (-1 == (id = jed_mouse_add_event (&jm)))
     {
	msg_error ("Failed!");
	return;
     }

   buffer_keystring ("\033\000", 2);
   _putkey(id);
}

static void process_resize_records (WINDOW_BUFFER_SIZE_RECORD *w)
{
   int c, r;

   c = w->dwSize.X;
   r = w->dwSize.Y;
   if ((r != SLtt_Screen_Rows) || (c != SLtt_Screen_Cols))
     {
	jed_init_display ();
	jed_redraw_screen (1);
     }
}

static void process_console_records(void)
{
   INPUT_RECORD record;
   DWORD bytesRead;
   DWORD n = 0;

   if (FALSE == GetNumberOfConsoleInputEvents(SLw32_Hstdin, &n))
     return;

   while (n > 0)
     {
	ReadConsoleInput(SLw32_Hstdin, &record, 1, &bytesRead);
	switch (record.EventType)
	  {
	   case KEY_EVENT:
	     process_key_event(&record.Event.KeyEvent);
	     break;

	   case MOUSE_EVENT:
	     process_mouse_event(&record.Event.MouseEvent);
	     break;

	   case WINDOW_BUFFER_SIZE_EVENT:
	     process_resize_records(&record.Event.WindowBufferSizeEvent);
	     break;
	  }
	n--;
     }
}

#ifdef JED_HAS_SUBPROCESSES
int sys_input_pending (int *tsecs, int all)
{
   DWORD ret = 0;
   int i, n;

   if ((all >= 0) && (Input_Buffer_Len || Batch)) return Input_Buffer_Len;

   if (all < 0)
     {
	ret = WaitForMultipleObjects(Num_Subprocesses,
				     Input_Events + 1, FALSE, *tsecs * 100);
     }
   else
     {
	long rtime, t;

	rtime = *tsecs * 100L;
	
	process_console_records ();
	while ((rtime > 0) && !Input_Buffer_Len)	
	  {
	     t = GetTickCount ();

	     ret = WaitForMultipleObjects(Num_Subprocesses + 1,
					  Input_Events, FALSE, rtime);

	     if (ret != WAIT_OBJECT_0) break;

	     process_console_records();
	     rtime -= GetTickCount() - t;
	  }

	if ((rtime < 0) || Input_Buffer_Len)
	  return Input_Buffer_Len;
     }

   if (WAIT_TIMEOUT == ret) return 0;

   i = n = 1;
   while (i <= Num_Subprocesses)
     {
	if (WAIT_TIMEOUT != WaitForSingleObject(Input_Events[i], 0))
	  {
	     read_process_input(i - 1);
	     n++;
	  }
	i++;
     }

   if (all < 0)
     return n;
   else
     return 0;
}
#else
int sys_input_pending (int *tsecs, int unused)
{
   DWORD d;
   long rtime, t;

   if (Input_Buffer_Len) return Input_Buffer_Len;
   rtime = *tsecs * 100L;
   do
     {
	t = GetTickCount ();

	d = WaitForSingleObject(SLw32_Hstdin, rtime);
	if (d == WAIT_TIMEOUT) return 0;
	process_console_records();
	rtime -= GetTickCount() - t;
     }
   while ((rtime > 0) && !Input_Buffer_Len);

   return Input_Buffer_Len;
}
#endif

unsigned char sys_getkey (void)
{
   int n = 450;
#ifdef JED_HAS_SUBPROCESSES
   int all = Num_Subprocesses;
#else
   int all = 0;
#endif

   while (1)
     {
	if (SLKeyBoard_Quit) break;

	/* sleep for 45 second and try again */
	if (sys_input_pending(&n, all) > 0)
	  break;

	if (SLKeyBoard_Quit) break;

	/* update status line incase user is displaying time */
	if (Display_Time || all)
	  {
	     JWindow->trashed = 1;
	     update((Line *) NULL, all, 1, 0);
	  }
     }

   if (SLKeyBoard_Quit)
     {
	SLKeyBoard_Quit = 0;
	flush_input ();
	return Jed_Abort_Char;
     }

   return (unsigned char) my_getkey();
}

void sys_suspend(void)
{
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   char *shell;
   
   shell = getenv ("COMSPEC");
   if (shell == NULL)
     shell = "CMD";

   memset ((char *) &si, 0, sizeof (STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = STARTF_USESTDHANDLES;
   si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
   si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
   si.hStdError = GetStdHandle (STD_ERROR_HANDLE);

   if (FALSE == CreateProcess (NULL, shell, NULL, NULL, TRUE, 0, NULL, NULL,
			       &si, &pi))
     {
	jed_verror ("Command shell '%s' failed to run\n", shell);
     }

   WaitForSingleObject (pi.hProcess, INFINITE);
   CloseHandle (pi.hProcess);
   CloseHandle (pi.hThread);
}

int jed_init_w32_support (void)
{
   SLang_Interrupt = process_console_records;
   return w32_init_subprocess_support (0);
}
