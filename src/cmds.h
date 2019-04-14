/* Copyright (c) 1992-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
extern int jed_buffer_visible (char *b);
extern int newline(void);
extern int newline_cmd(void);
extern int previous_char_cmd(void);
extern int delete_char_cmd(void);
extern int backward_delete_char_cmd(void);
extern int backward_delete_char_untabify(void);
extern int previous_line_cmd(void);
extern int next_line_cmd(void);
extern int next_char_cmd(void);
extern int kill_line(void);
extern void insert_line(Line *line);
extern int double_line(void);
extern int kill_eol_cmd(void);
extern int kill_line_cmd(void);
extern int jed_trim_whitespace(void);
extern int indent_line(void);
extern int transpose_lines(void);
extern int newline_and_indent(void);
extern int eol_cmd(void);
extern int sys_spawn_cmd(void);
extern int jed_spawn_fg_process (int (*)(VOID_STAR), VOID_STAR);
extern int ins_char_cmd(void);
extern int jed_exit_jed(int);
extern int jed_quit_jed(int);
extern int jed_exit_jed_cmd(void);
extern int save_some_buffers(void);
extern int pagedown_cmd(void);
extern int pageup_cmd(void);
extern int jed_scroll_left_cmd (void);
extern int jed_scroll_right_cmd (void);
extern int quoted_insert(void);
extern void indent_to(int);
extern int goto_column1(int *);
extern void goto_column(int *);
extern void goto_top_of_window (void);
extern int goto_bottom_of_window (void);
extern void insert_whitespace(int *);
extern unsigned char *get_current_indent(int *);
extern unsigned char *jed_skip_whitespace (void);
extern unsigned char *jed_bskip_whitespace (void);
extern int jed_looking_at(char *);

extern int Blink_Flag;
extern int Indented_Text_Mode;
extern int Kill_Line_Feature;
extern int Jed_Secure_Mode;
extern int Goal_Column;
extern int Jed_Tab_Default;
extern int Jed_Wrap_Default;
extern int Jed_Case_Search_Default;
extern int Jed_Suspension_Not_Allowed;
extern int Jed_Use_Tabs_Default;
#if 0
extern int new_find_matching (int *, int *);
#endif

