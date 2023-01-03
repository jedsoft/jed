/* Copyright (c) 1992-2023 John E. Davis
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
#define MINIBUFFER_WINDOW	0x01
   int flags;			       /* Note that trashed should be a bit here */

   void *private_data;		       /* used by the callback functions */
}
Window_Type;

/* The mini window exists at all times.  When active, it is part of the
 * window list.  When inactive, it is not part of the list.
 */
extern Window_Type *JMiniWindow;
extern Window_Type *JWindow;

#define IN_MINI_WINDOW		(JWindow->flags & MINIBUFFER_WINDOW)

extern Window_Type *jed_create_minibuffer_window (void);

/* Window callbacks for GUIs.  If any of these return -1, it is regarded as
 * a fatal error, and jed will exit.
 */
extern int (*jed_new_window_cb) (Window_Type *);
extern void (*jed_free_window_cb) (Window_Type *);
extern int (*jed_create_mini_window_cb) (Window_Type *);
extern int (*jed_leave_window_cb) (Window_Type *);
extern int (*jed_enter_window_cb) (Window_Type *);
extern int (*jed_split_window_cb) (Window_Type *oldwin, Window_Type *newwin);
extern int (*jed_window_geom_change_cb) (void);

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

