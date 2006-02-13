/* Copyright (c) 1992, 1998, 2000, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef __JED_WINDOW_H_
#define __JED_WINDOW_H_
#include "buffer.h"

typedef struct Window_Type
{
   struct Window_Type *next;
   int sx, sy;			       /* column/row of window start in display (from 0) */
   int rows;			       /* number of rows (does not include status line) */
   int width;
   int hscroll_column;		       /* numbered from 1 */
   Buffer *buffer;
   Mark beg;			       /* buffer position of top corner */
   Mark mark;			       /* last cursor pos in window before switch */
   int trashed;			       /* true if one or more lines in window were changed */
   int flags;			       /* Note that trashed should be a bit here */
}
Window_Type;

extern Window_Type *JWindow;

extern Window_Type *create_window(int, int, int, int, int);
extern void touch_screen_for_buffer(Buffer *);
extern void touch_window_hard(Window_Type *, int);


extern int is_line_visible (int);
extern int split_window(void);
extern int other_window(void);
extern int one_window(void);
extern int delete_window(void);
extern int enlarge_window(void);
extern void window_buffer(Buffer *);

/* jed_update_window_sizes must be called before Jed_Num_Screen_Rows/Cols
 * has been set.
 */
extern void jed_update_window_sizes (int, int);
extern int buffer_visible(Buffer *);
extern int Top_Window_SY;

extern void jed_scroll_left (int);
extern void jed_scroll_right (int);
extern void jed_set_scroll_column (int);
extern int jed_get_scroll_column (void);

extern int jed_num_windows (void);

extern int jed_init_window_intrinsics (void);


#if JED_HAS_SUBPROCESSES
extern void move_window_marks (int);
#endif

#endif

