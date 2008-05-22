/* Copyright (c) 1992, 1998, 2000, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

extern Buffer *The_MiniBuffer;

extern int jed_ns_load_file (char *, char *);
extern int find_file_in_window(char *);
extern int find_file(void);
extern int get_buffer(void);
extern int evaluate_cmd(void);
extern int kill_buffer(void);
extern int jed_save_buffer_as_cmd (void);
extern int jed_save_buffer_cmd (void);
extern int jed_save_buffer_as (char *);
extern int jed_save_buffer_to_file (char *, char *);
extern int search_forward_cmd(void);
extern int global_setkey_cmd(void);
extern int search_backward_cmd(void);
extern int insert_file_cmd(void);
extern void init_minibuffer(void);
extern int exit_minibuffer(void);
extern int select_minibuffer(void);
extern int mini_complete(void);
extern void buffer_substring(void);
extern char *make_line_string(char *, unsigned int);
extern int set_buffer(char *);
extern char *pop_to_buffer(char *);
extern int init_jed_intrinsics(void);
#if defined (__unix__) || defined (__os2__) || defined(__WIN32__)
extern int shell_command(char *);
extern int pipe_region(char *);
#endif

extern void load_buffer(void);
extern void update_cmd(int *);
extern void update_sans_update_hook_cmd (int *);
extern void copy_region_cmd(char *);
extern void jed_setup_minibuffer_keymap (void);
#ifndef IBMPC_SYSTEM
# if defined(__QNX__)
   extern int show_memory(void);
# endif
extern void screen_w80(void);
extern void screen_w132(void);
#else
extern int show_memory(void);
#endif
extern int markp(void);
extern int dup_mark(void);
extern void mini_read(char *, char *, char *);
extern void send_string_to_term(char *);
extern void make_buffer_list(void);
extern char *make_buffer_substring(int *);
extern int expand_wildcards(char *);
extern int bufferp(char *);
extern int kill_buffer_cmd(char *);
extern int get_doc_string(char *, char *);
extern int insert_buffer_name(char *);
extern int find_file_cmd(char *);
extern void read_object_with_completion(char *, char *, char *, int *);
extern void switch_to_buffer_cmd(char *);
extern char *what_buffer(void);
extern void set_tab(int *);
extern char *command_line_argv(int *);
extern int MiniBuffer_Active;
extern FILE *jed_open_slang_file(char *, char *);
extern char *Completion_Buffer;
extern void set_expansion_hook (char *);

extern int jed_vget_y_n (char *, char *);
extern int jed_get_y_n (char *);
extern int jed_get_mini_response (char *);
extern int jed_get_yes_no( char *);

extern int Jed_Load_Quietly;
