/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#include "buffer.h"
#include "display.h"
#include "hooks.h"

void (*tt_beep)(void);
void (*tt_write_string)(SLFUTURE_CONST char *);
void (*tt_get_screen_size)(int *, int *);
JX_SETXXX_RETURN_TYPE (*tt_set_color)(int, SLFUTURE_CONST char *, SLFUTURE_CONST char *, SLFUTURE_CONST char *);
JX_SETXXX_RETURN_TYPE (*tt_set_mono) (int, SLFUTURE_CONST char *, SLtt_Char_Type);

#ifndef IBMPC_SYSTEM
void (*tt_wide_width)(void);
void (*tt_narrow_width)(void);
void (*tt_enable_cursor_keys)(void);
void (*tt_set_term_vtxxx)(int *);
#endif

int *tt_Ignore_Beep;
int *tt_Use_Ansi_Colors;
int *tt_Term_Cannot_Scroll;
int *tt_Term_Cannot_Insert;
#ifndef IBMPC_SYSTEM
int *tt_Blink_Mode;
#endif

void flush_output (void)
{
   SLtt_flush_output ();
}

static void get_screen_size (int *r, int *c)
{
   SLtt_get_screen_size ();
   *r = SLtt_Screen_Rows;
   *c = SLtt_Screen_Cols;
}

static void get_terminfo (void)
{
   /* Placed here for windows dll support.  Apparantly the windows equivalent
    * of the run-time linker cannot perform the proper relocations.
    */
   tt_beep		= SLtt_beep;
   tt_write_string	= SLtt_write_string;
   tt_get_screen_size	= get_screen_size;
   tt_set_color		= SLtt_set_color;
   tt_set_mono		= SLtt_set_mono;

#ifndef IBMPC_SYSTEM
   tt_wide_width		= SLtt_wide_width;
   tt_narrow_width		= SLtt_narrow_width;
   tt_enable_cursor_keys	= SLtt_enable_cursor_keys;
   tt_set_term_vtxxx		= SLtt_set_term_vtxxx;
#endif
   tt_Ignore_Beep       	= &SLtt_Ignore_Beep;
   tt_Use_Ansi_Colors   	= &SLtt_Use_Ansi_Colors;
   tt_Term_Cannot_Scroll	= &SLtt_Term_Cannot_Scroll;
   tt_Term_Cannot_Insert	= &SLtt_Term_Cannot_Insert;
#ifndef IBMPC_SYSTEM
   SLtt_Blink_Mode = 0;
   tt_Blink_Mode		= &SLtt_Blink_Mode;
#endif

#ifdef REAL_UNIX_SYSTEM
   SLtt_Force_Keypad_Init = 1;
#endif
   if (Batch == 0)
     SLtt_get_terminfo ();
}

void (*tt_get_terminfo)(void)	= get_terminfo;
int (*X_Argc_Argv_Hook)(int, char **) = NULL;
