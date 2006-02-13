#ifndef _JED_SCREEN_H_
#define _JED_SCREEN_H_
/* Copyright (c) 1992, 1998, 2000, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "window.h"

/* JED screen management routines */
  
extern int Screen_Row;		       /* cursor row */
extern int Screen_Col;		       /* cursor col */
extern int Cursor_Motion;	       /* cursor movement only */

extern void recenter(int *);
extern int window_line(void);
extern void scroll_down(int, int, int);
extern int scroll_up(int, int, int);

extern void update(Line *, int, int, int);
extern void jed_init_display (void);
extern void jed_reset_display(void);
extern void point_cursor(int);
extern void point_column(int);
extern int calculate_column(void);

extern void register_change(int);
extern void touch_window(void);
extern int cursor_visible(void);
extern Line *find_top(void);
extern Line *jed_find_top_to_recenter (Line *);
extern void do_dialog(char *);
extern void jed_fillmem(char *, char, int);
extern int Goal_Column;
extern int User_Prefers_Line_Numbers;
extern int Wants_Attributes;
extern int Term_Supports_Color;
extern int Wants_Syntax_Highlight;
extern int Display_Time;
extern int Want_Eob;
extern void set_status_format(char *, int *);

extern void init_syntax_highlight (void);
extern void write_syntax_highlight (int, Line *, unsigned int);
extern int Mode_Has_Syntax_Highlight;
extern int Wants_HScroll;
extern int Mini_Ghost;
extern void jed_redraw_screen(int);
extern void define_top_screen_line(char *);
extern void jed_resize_display (void);
extern volatile int Jed_Resize_Pending;

extern int jed_compute_effective_length (unsigned char *, unsigned char *);
extern int jed_find_line_on_screen (Line *, int);
extern int jed_get_screen_size (int *, int *);

#if JED_HAS_DISPLAY_TABLE
extern unsigned char Output_Display_Table[256];
#endif

#if JED_HAS_LINE_ATTRIBUTES
extern Line *jed_find_non_hidden_line (Line *);
#endif

extern int Jed_Version_Number;
extern char *Jed_Version_String;

extern int Jed_Dollar;
extern int Jed_Num_Screen_Rows;
extern int Jed_Num_Screen_Cols;
extern int Jed_Highlight_Trailing_WS;
extern char *MiniBuf_Get_Response_String;
extern int Jed_Simulate_Graphic_Chars;

extern int Jed_Display_Initialized;
#endif
/* #ifdef _JED_SCREEN_H_ */
