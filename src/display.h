/* Copyright (c) 1992, 1998, 2000, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#if SLANG_VERSION < 20000
# define JX_SETXXX_RETURN_TYPE void
# define JX_SETXXX_RETURN_VAL
#else
# define JX_SETXXX_RETURN_TYPE int
# define JX_SETXXX_RETURN_VAL 0
#endif

/* This is the main hook that must be set at compile-time. */
extern void (*tt_get_terminfo)(void);

/* These hooks should be set during run-time by the tt_get_terminfo hook */
extern void (*tt_beep)(void);
extern void (*tt_write_string)(char *);
extern void (*tt_get_screen_size)(int *, int *);
extern JX_SETXXX_RETURN_TYPE (*tt_set_color)(int, char *, char *, char *);
extern JX_SETXXX_RETURN_TYPE (*tt_set_mono) (int, char *, SLtt_Char_Type);

#ifndef IBMPC_SYSTEM
extern void (*tt_wide_width)(void);
extern void (*tt_narrow_width)(void);
extern void (*tt_enable_cursor_keys)(void);
extern void (*tt_set_term_vtxxx)(int *);
# if SLANG_VERSION < 20000
extern void (*tt_set_color_esc)(int, char *);
# endif
#endif

extern int *tt_Ignore_Beep;
extern int *tt_Use_Ansi_Colors;
extern int *tt_Term_Cannot_Scroll;
extern int *tt_Term_Cannot_Insert;

#ifndef IBMPC_SYSTEM
extern int *tt_Blink_Mode;
#endif

extern void flush_output (void);
