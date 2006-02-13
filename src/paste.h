/* Copyright (c) 1992, 1998, 2000, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

extern int set_mark_cmd(void);
extern int push_spot(void);
extern int jed_push_mark (void);
extern int jed_goto_mark(Mark *);
extern int jed_pop_mark (int);
extern int mark_spot(void);
extern int pop_spot(void);
extern int widen_buffer(Buffer *);
extern int widen(void);
extern int widen_region (void);
extern int narrow_to_region (void);
extern int narrow_to_lines (void);
extern void jed_widen_whole_buffer (Buffer *);
extern void jed_init_mark (Mark *, unsigned int);
extern void jed_init_mark_for_buffer (Mark *, Buffer *, unsigned int);
extern int jed_init_mark_for_line (Mark *, Line *, unsigned int);
extern void jed_copy_mark (Mark *dest, Mark *src);

#if JED_HAS_SAVE_NARROW
extern void jed_free_saved_narrow (Buffer *);
extern void jed_push_narrow (void);
extern void jed_pop_narrow (void);
#endif
extern int jed_count_narrows (void);
extern int exchange_point_mark(void);
extern int yank(void);
extern int check_region(int *);
extern int copy_region_to_buffer(Buffer *);
extern int delete_region(void);
extern int copy_to_pastebuffer(void);
extern int kill_region(void);
extern int insert_rectangle(void);
extern int kill_rectangle(void);
extern int blank_rectangle(void);
extern int open_rectangle(void);
extern int copy_rectangle(void);

extern void goto_user_mark (void);
extern void create_user_mark (void);
extern void free_user_marks (Buffer *);
extern void move_user_mark (void);
extern int jed_is_user_mark_in_narrow (void);
extern int jed_move_user_object_mark (SLang_MMT_Type *);
extern SLang_MMT_Type *jed_make_user_object_mark (void);
extern char *user_mark_buffer (void);
extern int register_jed_classes (void);

#if JED_HAS_LINE_MARKS
extern void jed_create_line_mark (int *);
#endif

extern unsigned int jed_count_lines_in_region (void);
extern Buffer *Paste_Buffer;

