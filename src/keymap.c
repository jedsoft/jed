/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"
/*{{{ Include Files */

#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#include <string.h>
#include <setjmp.h>

#include "buffer.h"
#include "cmds.h"
#include "paste.h"
#include "screen.h"
#include "window.h"
#include "text.h"
#include "ledit.h"
#include "keymap.h"
#include "sysdep.h"
#include "misc.h"
#include "display.h"
#include "file.h"
#include "undo.h"
#include "ins.h"
#include "hooks.h"
#include "replace.h"
#include "indent.h"
#if JED_HAS_MENUS
# include "menu.h"
#endif

#ifdef IBMPC_SYSTEM
# include "doskeys.h"
#endif

/*}}}*/

void (*X_Define_Keys_Hook)(SLKeyMap_List_Type *);
int (*X_Set_Abort_Char_Hook) (unsigned char);
unsigned char Jed_Abort_Char = 7;		       /* ^G */

typedef struct /*{{{*/
{
   jmp_buf b;
}

/*}}}*/
jmp_buf_struct;

jmp_buf_struct Jump_Buffer, *Jump_Buffer_Ptr;

FVOID_STAR Last_Key_Function;
FVOID_STAR Jed_This_Key_Function;

int *Repeat_Factor;                    /* repeats a key sequence if non null */

char *Read_This_Character = NULL;      /* alternate keyboard buffer */

char Jed_Key_Buffer[SLANG_MAX_KEYMAP_KEY_SEQ + 1];
char Key_Buffer[SLANG_MAX_KEYMAP_KEY_SEQ + 1];
char *Key_Bufferp = Key_Buffer;
static int Last_Key_Buffer_Len;

int Jed_Max_Hits = 300;


int jed_beep (void) /*{{{*/
{
   tt_beep ();
   flush_output ();
   return 1;
}

/*}}}*/

static int redraw (void)
{
   jed_redraw_screen (0);
   return 0;
}

#if HAS_MOUSE
#ifndef IBMPC_SYSTEM
static int xterm_mouse_cmd (void)
{
   int b, id;
   static int last_down = JMOUSE_BUTTON_1;
   JMouse_Type m;

   b = my_getkey ();
   m.x = (unsigned char) my_getkey () - 32;
   m.y = (unsigned char) my_getkey () - 32;
   
   b -= 32;

   if ((b & 3) == 3)
     {
	m.type = JMOUSE_UP;
	m.button = last_down;
	m.state = last_down;
     }
   else
     {
	m.type = JMOUSE_DOWN;
	if ((b & 3) == 0)
	  m.button = JMOUSE_BUTTON_1;
	else if (b & 1)
	  m.button = JMOUSE_BUTTON_2;
	else if (b & 2)
	  m.button = JMOUSE_BUTTON_3;
	m.state = 0;
     }
   last_down = m.button;

   if (-1 == (id = jed_mouse_add_event (&m)))
     return -1;
   
   ungetkey (&id);
   jed_mouse_cmd ();
   return 1;
}
#endif
#endif				       /* HAS_MOUSE */

SLKeymap_Function_Type Jed_Functions[] = /*{{{*/
{
   /* Put common cases here to speedup lookup */
     {"self_insert_cmd", ins_char_cmd},
     {"next_char_cmd", next_char_cmd},
     {"next_line_cmd", next_line_cmd},
     {"previous_char_cmd", previous_char_cmd},
     {"previous_line_cmd", previous_line_cmd},
     {"delete_char_cmd", delete_char_cmd},
     {"beg_of_line", bol},
     {"eol_cmd", eol_cmd},
     {"beg_of_buffer", bob},
     {"end_of_buffer", eob},
     {"newline_and_indent", newline_and_indent},
     {"newline", newline_cmd},
     {"page_down", pagedown_cmd},
     {"page_up", pageup_cmd},
     {"backward_delete_char", backward_delete_char_cmd},
     {"backward_delete_char_untabify", backward_delete_char_untabify},
     {"beep", jed_beep},
     {"begin_macro", begin_keyboard_macro},
     {"center_line", center_line},
     {"copy_region", copy_to_pastebuffer},
#if (defined(__MSDOS__) && !defined(__WIN32__)) || (defined(__QNX__) && defined(__WATCOMC__))
     {"coreleft", show_memory},
#endif
     {"delete_window", delete_window},
     {"digit_arg", digit_arg},
/*      {"double_line", double_line}, */
     {"end_macro", end_keyboard_macro},
     {"enlarge_window", enlarge_window},
     {"evaluate_cmd", evaluate_cmd},
     {"execute_macro", execute_keyboard_macro},
     {"exchange",exchange_point_mark},
     {"exit_jed", jed_exit_jed_cmd},
     {"exit_mini", exit_minibuffer},
     {"find_file", find_file},
     {"format_paragraph", text_format_paragraph},
     {"goto_match", goto_match},
     {"indent_line", indent_line},
     {"insert_file", insert_file_cmd},
#ifdef HAS_MOUSE
     {"mouse_cmd", jed_mouse_cmd},
#endif
     {"kbd_quit", kbd_quit},
     {"kill_buffer", kill_buffer},
     {"kill_line", kill_line},
     {"kill_region", kill_region},
     {"macro_query", macro_query},
     {"mark_spot", mark_spot},
     {"mini_complete", mini_complete},
     {"narrow", narrow_to_lines},
     {"narrow_paragraph", narrow_paragraph},
     {"narrow_to_region", narrow_to_region},
     {"one_window", one_window},
     {"other_window", other_window},
     {"pop_spot", pop_spot},
     {"quoted_insert", quoted_insert},
     {"redraw", redraw},
      /* {"replace", replace}, */
     {"save_some_buffers", save_some_buffers},
     {"scroll_left", jed_scroll_left_cmd},
     {"scroll_right", jed_scroll_right_cmd},
       /* {"search_backward", search_backward_cmd},
	  {"search_forward", search_forward_cmd}, */
     {"set_mark_cmd", set_mark_cmd},
     {"split_window", split_window},
     {"switch_to_buffer", get_buffer},
     {"sys_spawn_cmd", sys_spawn_cmd},
     {"text_smart_quote", text_smart_quote},
/*       {"transpose_lines", transpose_lines}, */
     {"trim_whitespace", jed_trim_whitespace},
     {"undo", undo},
     {"widen", widen},
     {"widen_region", widen_region},
     {"write_buffer", jed_save_buffer_as_cmd},
     {"yank", yank},
#if JED_HAS_MENUS
     {"select_menubar", jed_select_menu_bar},
#endif
     {(char *) NULL, NULL}
};

/*}}}*/

SLKeyMap_List_Type *Global_Map;

int kbd_quit(void) /*{{{*/
{
   int sle = SLang_get_error ();
   SLKeyBoard_Quit = 1;
#if JED_HAS_MENUS
   if (Jed_Menus_Active)
     jed_exit_menu_bar ();
#endif
   SLang_set_error (SL_USER_BREAK);
   msg_error("Quit!");
   if (Ignore_User_Abort) 
     SLang_set_error (sle);
#ifdef UNDO_HAS_REDO
   set_current_undo ();
#endif
   return(1);
}

/*}}}*/

void init_keymaps(void) /*{{{*/
{
   int ch;
   char simple[2];
   simple[1] = 0;
   
   if (NULL == (Global_Map = SLang_create_keymap ("global", NULL)))
     exit_error ("Malloc error creating keymap.", 0);
   
   Global_Map->functions = Jed_Functions;
   
   /* This breaks under some DEC ALPHA compilers (scary!) */
#ifndef __DECC
   for (ch = ' '; ch < 256; ch++)
     {
	simple[0] = (char) ch;
	SLkm_define_key (simple, (FVOID_STAR) ins_char_cmd, Global_Map);
     }
#else
   ch = ' ';
   while (1)
     {
	simple[0] = (char) ch;
	SLkm_define_key (simple, (FVOID_STAR) ins_char_cmd, Global_Map);
	ch = ch + 1;
	if (ch == 256) break;
     }
#endif
   
   SLkm_define_key ("^[1", (FVOID_STAR) digit_arg, Global_Map);
   SLkm_define_key ("^[2", (FVOID_STAR) digit_arg, Global_Map);
   SLkm_define_key ("^[3", (FVOID_STAR) digit_arg, Global_Map);
   SLkm_define_key ("^[4", (FVOID_STAR) digit_arg, Global_Map);
   SLkm_define_key ("^[5", (FVOID_STAR) digit_arg, Global_Map);
   SLkm_define_key ("^[6", (FVOID_STAR) digit_arg, Global_Map);
   SLkm_define_key ("^[7", (FVOID_STAR) digit_arg, Global_Map);
   SLkm_define_key ("^[8", (FVOID_STAR) digit_arg, Global_Map);
   SLkm_define_key ("^[9", (FVOID_STAR) digit_arg, Global_Map);
   SLkm_define_key ("^[0", (FVOID_STAR) digit_arg, Global_Map);
   
#ifdef IBMPC_SYSTEM
   SLkm_define_key (PC_DEL, (FVOID_STAR) delete_char_cmd, Global_Map);
   SLkm_define_key (PC_DEL1, (FVOID_STAR) delete_char_cmd, Global_Map);
   SLkm_define_key (PC_NULL, (FVOID_STAR) set_mark_cmd, Global_Map);
   SLkm_define_key (PC_LT, (FVOID_STAR) previous_char_cmd, Global_Map);
   SLkm_define_key (PC_LT1, (FVOID_STAR) previous_char_cmd, Global_Map);
   SLkm_define_key (PC_UP, (FVOID_STAR) previous_line_cmd, Global_Map);
   SLkm_define_key (PC_UP1, (FVOID_STAR) previous_line_cmd, Global_Map);
   SLkm_define_key (PC_DN, (FVOID_STAR) next_line_cmd, Global_Map);
   SLkm_define_key (PC_DN1, (FVOID_STAR) next_line_cmd, Global_Map);
   SLkm_define_key (PC_RT, (FVOID_STAR) next_char_cmd, Global_Map);
   SLkm_define_key (PC_RT1, (FVOID_STAR) next_char_cmd, Global_Map);
   SLkm_define_key (PC_PGUP, (FVOID_STAR) pageup_cmd, Global_Map);
   SLkm_define_key (PC_PGUP1, (FVOID_STAR) pageup_cmd, Global_Map);
   SLkm_define_key (PC_PGDN, (FVOID_STAR) pagedown_cmd, Global_Map);
   SLkm_define_key (PC_PGDN1, (FVOID_STAR) pagedown_cmd, Global_Map);
   SLkm_define_key (PC_HOME, (FVOID_STAR) bol, Global_Map);
   SLkm_define_key (PC_HOME1, (FVOID_STAR) bol, Global_Map);
   SLkm_define_key (PC_END, (FVOID_STAR) eol_cmd, Global_Map);
   SLkm_define_key (PC_END1, (FVOID_STAR) eol_cmd, Global_Map);
   
   /* Now special keypad stuff */
   SLkm_define_key (PC_ENTER, (FVOID_STAR) newline_cmd, Global_Map);
   
   /* wordperfect type stuff */
   SLkm_define_key (PC_F1, (FVOID_STAR) kbd_quit, Global_Map);
   /* SLkm_define_key (PC_F2, (FVOID_STAR) search_forward_cmd, Global_Map);
    SLkm_define_key (PC_SHIFT_F2, (FVOID_STAR) search_backward_cmd, Global_Map); */
   SLkm_define_key (PC_F4, (FVOID_STAR) indent_line, Global_Map);
   SLkm_define_key (PC_ALT_F5, (FVOID_STAR) set_mark_cmd, Global_Map);
   SLkm_define_key (PC_SHIFT_F4, (FVOID_STAR) narrow_paragraph, Global_Map);
   SLkm_define_key (PC_SHIFT_F6, (FVOID_STAR) center_line, Global_Map);
   SLkm_define_key (PC_F7, (FVOID_STAR) jed_exit_jed_cmd, Global_Map);
#else
   
   /* give vtxxx arrow keys */
   SLkm_define_key ("^[[D", (FVOID_STAR) previous_char_cmd, Global_Map);
   SLkm_define_key ("^[OD", (FVOID_STAR) previous_char_cmd, Global_Map);
   SLkm_define_key ("^[[A", (FVOID_STAR) previous_line_cmd, Global_Map);
   SLkm_define_key ("^[OA", (FVOID_STAR) previous_line_cmd, Global_Map);
   SLkm_define_key ("^[[B", (FVOID_STAR) next_line_cmd, Global_Map);
   SLkm_define_key ("^[OB", (FVOID_STAR) next_line_cmd, Global_Map);
   SLkm_define_key ("^[[C", (FVOID_STAR) next_char_cmd, Global_Map);
   SLkm_define_key ("^[OC", (FVOID_STAR) next_char_cmd, Global_Map);
   SLkm_define_key ("^[[6~", (FVOID_STAR) pagedown_cmd, Global_Map);
   SLkm_define_key ("^[[5~", (FVOID_STAR) pageup_cmd, Global_Map);
   SLkm_define_key ("^K^[[C", (FVOID_STAR) jed_scroll_left_cmd, Global_Map);
   SLkm_define_key ("^K^[[D", (FVOID_STAR) jed_scroll_right_cmd, Global_Map);
   SLkm_define_key ("^K^[[A", (FVOID_STAR) bob, Global_Map);
   SLkm_define_key ("^K^[[B", (FVOID_STAR)eob, Global_Map);
#if HAS_MOUSE
   SLkm_define_key ("\033[M", (FVOID_STAR)xterm_mouse_cmd, Global_Map);
#endif
#ifdef sun
   SLkm_define_key ("\033[216z", (FVOID_STAR) pageup_cmd, Global_Map);
   SLkm_define_key ("\033[222z", (FVOID_STAR) pagedown_cmd, Global_Map);
   SLkm_define_key ("\033[214z", (FVOID_STAR) bol, Global_Map);
   SLkm_define_key ("\033[220z", (FVOID_STAR) eol_cmd, Global_Map);
#endif
#endif   
   
   /* SLkm_define_key ("'", (FVOID_STAR) text_smart_quote, Global_Map);
   SLkm_define_key ("\"", (FVOID_STAR) text_smart_quote, Global_Map); */
   SLkm_define_key ("^_", (FVOID_STAR) undo, Global_Map);
   SLkm_define_key ("^?", (FVOID_STAR) backward_delete_char_untabify, Global_Map);
#ifndef IBMPC_SYSTEM
   SLkm_define_key ("^H", (FVOID_STAR) backward_delete_char_untabify, Global_Map);
#endif
   SLkm_define_key ("^B", (FVOID_STAR) bol, Global_Map);
   SLkm_define_key ("^D", (FVOID_STAR) pagedown_cmd, Global_Map);
   SLkm_define_key ("^E", (FVOID_STAR) eol_cmd, Global_Map);
/*
 SLkm_define_key ("^FB", (FVOID_STAR) search_backward_cmd, Global_Map);
 SLkm_define_key ("^FF", (FVOID_STAR) search_forward_cmd, Global_Map);
*/
   SLkm_define_key ("^G", (FVOID_STAR) kbd_quit, Global_Map);
   SLkm_define_key ("^I", (FVOID_STAR) indent_line, Global_Map);
   SLkm_define_key ("^K(", (FVOID_STAR) begin_keyboard_macro, Global_Map);
   SLkm_define_key ("^K)", (FVOID_STAR) end_keyboard_macro, Global_Map);
   SLkm_define_key ("^KD", (FVOID_STAR) evaluate_cmd, Global_Map);
   SLkm_define_key ("^KE", (FVOID_STAR) jed_exit_jed_cmd, Global_Map);
   SLkm_define_key ("^KG", (FVOID_STAR) find_file, Global_Map);
   SLkm_define_key ("^KK", (FVOID_STAR) copy_to_pastebuffer, Global_Map);
   SLkm_define_key ("^KM", (FVOID_STAR) mark_spot, Global_Map);
   SLkm_define_key ("^KX", (FVOID_STAR) execute_keyboard_macro, Global_Map);
   SLkm_define_key ("^K^B", (FVOID_STAR) set_mark_cmd, Global_Map);
   SLkm_define_key ("^K^I", (FVOID_STAR) insert_file_cmd, Global_Map);
/*     SLang_define_key1("^K^L", (VOID_STAR) double_line, SLKEY_F_INTRINSIC, Global_Map); */
   SLkm_define_key ("^K^M", (FVOID_STAR) pop_spot, Global_Map);
   SLkm_define_key ("^K^P", (FVOID_STAR) yank, Global_Map);
/*     SLang_define_key1("^K^R", (VOID_STAR) replace, SLKEY_F_INTRINSIC, Global_Map); */
   SLkm_define_key ("^K^V", (FVOID_STAR) kill_region, Global_Map);
   SLkm_define_key ("^K^W", (FVOID_STAR) jed_save_buffer_as_cmd, Global_Map);
   SLkm_define_key ("^L", (FVOID_STAR) kill_line, Global_Map);
   SLkm_define_key ("^M", (FVOID_STAR) newline_and_indent, Global_Map);
   SLkm_define_key ("^R", (FVOID_STAR) redraw, Global_Map);
   SLkm_define_key ("^U", (FVOID_STAR) pageup_cmd, Global_Map);
   SLkm_define_key ("^V", (FVOID_STAR) delete_char_cmd, Global_Map);
   SLkm_define_key ("^W0", (FVOID_STAR) delete_window, Global_Map);
   SLkm_define_key ("^W1", (FVOID_STAR) one_window, Global_Map);
   SLkm_define_key ("^W2", (FVOID_STAR) split_window, Global_Map);
   SLkm_define_key ("^WO", (FVOID_STAR) other_window, Global_Map);
   SLkm_define_key ("^XB", (FVOID_STAR) get_buffer, Global_Map);
   SLkm_define_key ("^XK", (FVOID_STAR) kill_buffer, Global_Map);
   SLkm_define_key ("^XN", (FVOID_STAR) narrow_to_region, Global_Map);
   SLkm_define_key ("^XQ", (FVOID_STAR) macro_query, Global_Map);
   SLkm_define_key ("^XS", (FVOID_STAR) save_some_buffers, Global_Map);
   SLkm_define_key ("^XW", (FVOID_STAR) widen_region, Global_Map);
   SLkm_define_key ("^X^", (FVOID_STAR) enlarge_window, Global_Map);
/*     SLang_define_key1("^X^T", (VOID_STAR) transpose_lines, SLKEY_F_INTRINSIC, Global_Map); */
   SLkm_define_key ("^X^[", (FVOID_STAR) evaluate_cmd, Global_Map);
   SLkm_define_key ("^X^X", (FVOID_STAR) exchange_point_mark, Global_Map);
   SLkm_define_key ("^Z", (FVOID_STAR) sys_spawn_cmd, Global_Map);
   SLkm_define_key ("^[<", (FVOID_STAR) bob, Global_Map);
   SLkm_define_key ("^[>", (FVOID_STAR) eob, Global_Map);
   SLkm_define_key ("^[N", (FVOID_STAR) narrow_paragraph, Global_Map);
   SLkm_define_key ("^[Q", (FVOID_STAR) text_format_paragraph, Global_Map);
   SLkm_define_key ("^[S", (FVOID_STAR) center_line, Global_Map);
   SLkm_define_key ("^[X", (FVOID_STAR) evaluate_cmd, Global_Map);
   SLkm_define_key ("^[\\", (FVOID_STAR) jed_trim_whitespace, Global_Map);
   SLkm_define_key ("^\\", (FVOID_STAR) goto_match, Global_Map);
   SLkm_define_key ("`", (FVOID_STAR) quoted_insert, Global_Map);
   
   if (X_Define_Keys_Hook != NULL)  (*X_Define_Keys_Hook)(Global_Map);
}

/*}}}*/

char Last_Kbd_Command_String[32];
char Current_Kbd_Command_String[32];

void set_current_kbd_command (char *s) /*{{{*/
{
   strncpy (Current_Kbd_Command_String, s, 31);
   Current_Kbd_Command_String[31] = 0;
}

/*}}}*/

static void update_jed_keybuffer (void) /*{{{*/
{
   Last_Key_Buffer_Len = (int) (Key_Bufferp - Key_Buffer);
   if (Last_Key_Buffer_Len == 1)
     {
	*Jed_Key_Buffer = *Key_Buffer;
	*(Jed_Key_Buffer + 1) = 0;
     }
   else
     {
        *Key_Bufferp = 0;
	strcpy (Jed_Key_Buffer, Key_Buffer);
     }
   
   Key_Bufferp = Key_Buffer;
}

/*}}}*/

static int do_macro_string (char *str, int repeat)
{
   static int macro_depth;
   char *rtc_save;
   int ret = 0;

   if (macro_depth > 10)
     {
	macro_depth = 0;
	msg_error ("Possible runaway macro aborted.");
	return -1;
     }
	     
   macro_depth++;
   rtc_save = Read_This_Character;
	     
   while (repeat > 0)
     {
	repeat--;

	Read_This_Character = str;
   
	while ((SLang_get_error () == 0)
	       && (SLKeyBoard_Quit == 0)
	       && (Read_This_Character != NULL))
	  jed_do_key();
	
	if (Read_This_Character != NULL)
	  {
	     ret = -1;
	     break;
	  }
     }

   Read_This_Character = rtc_save;
   macro_depth--;

   return ret;
}

static int key_interpret (SLang_Key_Type *key) /*{{{*/
{
   char *str;
   int ret;
   
   strcpy (Last_Kbd_Command_String, Current_Kbd_Command_String);
   Jed_This_Key_Function = key->f.f;

   switch (key->type)
     {
      case SLKEY_F_INTRINSIC:
	set_current_kbd_command (Jed_Key_Buffer);
	ret = (key->f.f) ();
	break;
	
      case SLKEY_F_INTERPRET:
	str = key->f.s;
	set_current_kbd_command (str);

	if (*str == ' ')
	  {
	     /* string to insert */
	     (void) jed_insert_string(str + 1);
	  }
	else if (*str == '@')
	  {
	     int repeat;

	     if (Repeat_Factor != NULL)
	       {
		  repeat = *Repeat_Factor;
		  Repeat_Factor = NULL;
	       }
	     else repeat = 1;

	     (void) do_macro_string (str + 1, repeat);
	  }
	else if ((*str == '.')
		 || !SLang_execute_function(str))
	  SLang_load_string(str);

	ret = 1;
	break;

#ifdef SLKEY_F_SLANG
      case SLKEY_F_SLANG:
	ret = SLexecute_function (key->f.slang_fun);
	break;
#endif

      default:
	SLang_verror (SL_NOT_IMPLEMENTED, "Keymap key-type %d is not supported", (int)key->type);
	return -1;
     }

   Last_Key_Function = Jed_This_Key_Function;
   return(ret);
}

/*}}}*/

static char *find_function_string (FVOID_STAR f) /*{{{*/
{
   SLKeymap_Function_Type *fp;
   
   if (f == (FVOID_STAR) ins_char_cmd) return "self_insert_cmd";
   
   fp = Jed_Functions;
   while ((fp != NULL) && (fp->name != NULL))
     {
	if ((FVOID_STAR) fp->f == f) return (char *) fp->name;
	fp++;
     }
   return NULL;
}

/*}}}*/

static SLFUTURE_CONST char *lookup_key_function_string (SLang_Key_Type *key)
{
   if (key == NULL)
     return NULL;

   switch (key->type)
     {
      case SLKEY_F_INTRINSIC:
	return find_function_string (key->f.f);
	
      case SLKEY_F_INTERPRET:
	return key->f.s;
	
#ifdef SLKEY_F_SLANG
      case SLKEY_F_SLANG:
	return key->f.slang_fun->name;
#endif
      default:
	return "";
     }
}


static int do_key (void) /*{{{*/
{
   SLang_Key_Type *key;
   int repeat;
   
   if (SLang_Key_TimeOut_Flag == 0)
     {
	Key_Bufferp = Key_Buffer;
	*Key_Bufferp = 0;
     }

#if JED_HAS_MENUS
   if (Jed_Menus_Active)
     {
	key = jed_menu_do_key ();
	update_jed_keybuffer ();
     }
   else
#endif
     {
	key = SLang_do_key (CBuf->keymap, jed_getkey);
	update_jed_keybuffer ();
	if ((IN_MINI_WINDOW == 0)
	    && (key != NULL)
	    && jed_hook_exists ("_jed_before_key_hooks"))
	  {
	     (void) jed_va_run_hooks ("_jed_before_key_hooks", JED_HOOKS_RUN_ALL, 
				      1, lookup_key_function_string (key));
	  }
     }
   
   if ((key != NULL) && (key->f.f != NULL))
     {
	if (Repeat_Factor == NULL) return key_interpret (key);
	repeat = *Repeat_Factor;
	Suspend_Screen_Update = 1;
	
	/* some routines may use the repeat factor as a prefix argument */
	while (repeat > 0)
	  {
	     repeat--;

	     if (SLKeyBoard_Quit || SLang_get_error () || (Repeat_Factor == NULL)) 
	       break;

	     key_interpret (key);
	  }
	Repeat_Factor = NULL;
	
	return(1);
     }
   else if (!Executing_Keyboard_Macro && !SLKeyBoard_Quit)
     {
	jed_beep ();
	flush_input ();
     }
   if (SLKeyBoard_Quit) kbd_quit();
   return(0);
}

/*}}}*/

int jed_do_key (void)
{
   int ret = do_key ();
   if (SLang_get_error ())
     {
	jed_abort_keyboard_macro ();
	Read_This_Character = NULL;
	Repeat_Factor = NULL;
     }
   return ret;
}



void do_jed(void) /*{{{*/
{
   char *mode;
   Buffer *tthis;

   tthis = CBuf;
   mode = tthis->mode_string;

   /* Mark Undo boundary now because this might not be valid later */
   mark_undo_boundary (tthis);

   Repeat_Factor = NULL;
   Replace_Preserve_Case = 0;
   if (jed_do_key()) JWindow->trashed = 1;
   
    /* internal editing commands may have selected a different buffer
     * so put it back to the one associated with the window.
     */
   if (CBuf != JWindow->buffer)
     {
	char *name;
	if (buffer_exists(JWindow->buffer))
	  name = JWindow->buffer->name; 
	else name = "*scratch*";
	switch_to_buffer_cmd(name);
     }

#if JED_HAS_MENUS
   if ((tthis != CBuf) || (CBuf->mode_string != mode))
     jed_notify_menu_buffer_changed ();
#endif
}

/*}}}*/

static int run_switch_active_buffer_hooks (char **bnamep)
{
   char *buffer_name = *bnamep;

   if (buffer_name == CBuf->name)
     return 0;

   if (buffer_name == NULL)
     (void) jed_va_run_hooks ("_jed_switch_active_buffer_hooks", JED_HOOKS_RUN_ALL, 1, "");
   else
     {
	(void) jed_va_run_hooks ("_jed_switch_active_buffer_hooks", JED_HOOKS_RUN_ALL, 1, buffer_name);
	SLang_free_slstring (buffer_name);
     }
   
   /* This should not fail because CBuf->name is an slstring, and if it
    * does fail, it is not important.
    */
   *bnamep = SLang_create_slstring (CBuf->name);
   return 0;
}


void jed (void) /*{{{*/
{
   /* This routine is called from main.  It calls the startup-hooks before
    * entering the main editing loop.
    */
   char *buffer_name;

   if (SLang_get_error ())
     {
	update (NULL, 1, 1, 0);
	SLang_restart (1);
	SLang_set_error (0);
	jed_sleep (2);
     }

#if JED_HAS_MENUS
   jed_notify_menu_buffer_changed ();
#endif

   buffer_name = NULL;
   (void) run_switch_active_buffer_hooks (&buffer_name);
   (void) jed_va_run_hooks ("_jed_startup_hooks", JED_HOOKS_RUN_ALL, 0);
   (void) run_switch_active_buffer_hooks (&buffer_name);
   
   Jump_Buffer_Ptr = &Jump_Buffer;
   
   if (setjmp(Jump_Buffer.b) != 0)
     {
	SLang_restart(1);   /* just in case */
     }
   
   if (CBuf != JWindow->buffer)
     {
	switch_to_buffer(JWindow->buffer);
	window_buffer(CBuf);
     }

#if JED_HAS_MENUS
   jed_notify_menu_buffer_changed ();
#endif

   JWindow->trashed = 1;
   update((Line *) NULL, 0, 0, 1);
   Read_This_Character = NULL;

   while(1)
     {
	int had_err = 0;
	Suspend_Screen_Update = 0;
	
	do_jed();
	if (SLang_get_error ())
	  {
	     update (NULL, 1, 0, 0);
	     SLang_restart (1);
	     SLang_set_error (0);
	     had_err = 1;
	  }

	if (!SLKeyBoard_Quit && (CBuf->flags & BUFFER_MODIFIED)
	    && (!Cursor_Motion)) CBuf->hits += 1;
	
	SLKeyBoard_Quit = 0;

	if (CBuf->hits > Jed_Max_Hits)
	  {
	     auto_save_buffer(CBuf);
	     check_buffers();   /* check files on disk to see if they are recent */
	  }
	(void) run_switch_active_buffer_hooks (&buffer_name);
	if (Jed_Menus_Active == 0)
	  (void) jed_va_run_hooks ("_jed_after_key_hooks", JED_HOOKS_RUN_ALL, 0);

	if ((Last_Key_Function != (FVOID_STAR) ins_char_cmd)
	    || (JWindow->trashed) || (JWindow != JWindow->next)
	    || Suspend_Screen_Update ||
	    (Mini_Ghost) || (*Message_Buffer) || (*Error_Buffer))
	  {
	     update((Line *) NULL, 0, had_err, 1);
	  }
     }
}

/*}}}*/

int digit_arg(void) /*{{{*/
{
   static int repeat;
   char buf[20];
   int key;
   unsigned int i;
   
   i = 0;
   buf[i++] = (char) SLang_Last_Key_Char;
   
   /* After jed_do_key (what called this), Key_Bufferp is reset.  However, I want 
    * to keep it for echoing subsequent characters.  I restore its previous 
    * value so that echoing will continue. 
    */
   
   Key_Bufferp = Key_Buffer + Last_Key_Buffer_Len;
   
   SLang_Key_TimeOut_Flag = 1;
   while (i < sizeof(buf)-1)
     {
	buf[i] = 0;
	key = jed_getkey();
	if ((key < '0') || (key > '9')) break;
	buf[i++] = (char) key;
     }
   buf[i] = 0;
   repeat = atoi(buf);
   Repeat_Factor = &repeat;
   if (Executing_Keyboard_Macro) Macro_Buffer_Ptr--;
   else
     {
	ungetkey (&key);
	if (Defining_Keyboard_Macro) Macro_Buffer_Ptr--;
     }
   
   if (Key_Bufferp != Key_Buffer) Key_Bufferp--;
   
   /* Key_Timeout is still active and is only reset after this call. */
   jed_do_key();
   return(1);
}

/*}}}*/

static int which_key (char *f) /*{{{*/
{
   int num = 0, i;
   SLang_Key_Type *key, *key_root;
   FVOID_STAR fp;
   unsigned char type;
   unsigned char buf[5];
   
   if (NULL == (fp = (FVOID_STAR) SLang_find_key_function(f, CBuf->keymap)))
     type = SLKEY_F_INTERPRET;
   else type = SLKEY_F_INTRINSIC;
   
   i = 256;
   key_root = CBuf->keymap->keymap;
   while (i--)
     {
	key = key_root->next;
	if ((key == NULL) && (type == key_root->type) &&
	    (((type == SLKEY_F_INTERPRET) && (!strcmp((char *) f, key_root->f.s)))
	     || ((type == SLKEY_F_INTRINSIC) && (fp == key_root->f.f))))
	  {
	     buf[0] = 2;
	     buf[1] = 256 - 1 - i;
	     buf[2] = 0;
	     SLang_push_string(SLang_make_keystring(buf));
	     num++;
	  }
	
	while (key != NULL)
	  {
	     if ((key->type == type) &&
		 (((type == SLKEY_F_INTERPRET) && (!strcmp((char *) f, key->f.s)))
		  || ((type == SLKEY_F_INTRINSIC) && (fp == key->f.f))))
	       {
		  SLang_push_string(SLang_make_keystring(key->str));
		  num++;
	       }
	     key = key->next;
	  }
	key_root++;
     }
   return(num);
}

/*}}}*/


static void dump_this_binding (SLang_Key_Type *key) /*{{{*/
{
   unsigned char ch, *s;
   char *str,  ctrl[2];
   SLFUTURE_CONST char *fun;
   int n, len;
   
   s = key->str;
   
   ctrl[0] = '^';
   len = *s++ - 1;;
   
   while (len-- > 0)
     {
	n = 1;
        ch = *s++;
	if (ch == 127)
	  {
	     str = "DEL"; n = 3;
	  }
	else if (ch > ' ') str = (char *) &ch;
	else if (ch == 27)
	  {
	     str = "ESC";
	     n = 3;
	  }
	else if (ch == ' ')
	  {
	     str = "SPACE";
	     n = 5;
	  }
	else if (ch == '\t')
	  {
	     str = "TAB"; n = 3;
	  }
	else
	  {
	     str = ctrl;
	     *(str + 1) = ch + '@';
	     n = 2;
	  }
	(void) jed_quick_insert((unsigned char *) str, n);
	(void) jed_insert_byte (' ');
     }
   (void) jed_insert_wchar_n_times('\t', 3);
   
   fun = lookup_key_function_string (key);
   
   if (fun == NULL) fun = "** Unknown **";
   (void) jed_insert_string (fun);
   (void) jed_insert_newline ();
}

/*}}}*/

static void dump_bindings(char *map) /*{{{*/
{
   int i;
   SLang_Key_Type *next, *key_root;
   SLKeyMap_List_Type *kml;

   CHECK_READ_ONLY_VOID
   
   if (NULL == (kml = SLang_find_keymap(map)))
     {
	msg_error("Keymap undefined.");
	return;
     }
   key_root = kml->keymap;
   
   for (i = 0; i < 256; i++)
     {
	next = key_root->next;
	if (next != NULL)
	  {
	     while (next != NULL)
	       {
		  dump_this_binding (next);
		  next = next->next;
	       }
	  }
	else if (key_root->f.f != NULL) dump_this_binding (key_root);
	key_root++;
     }
}

/*}}}*/

static void use_keymap(char *name) /*{{{*/
{
   SLKeyMap_List_Type *map;
   
   if ((name == NULL) || (*name == 0)
       || (NULL == (map = SLang_find_keymap(name))))
     {
	msg_error("Unknown keymap.");
     }
   else CBuf->keymap = map;
}

/*}}}*/

static SLFUTURE_CONST char *what_keymap (void) /*{{{*/
{
   return CBuf->keymap->name;
}

/*}}}*/

void jed_set_abort_char (int *c) /*{{{*/
{
   char str[2];
   char ch = (char) *c;
   SLkeymap_Type *km;

   if (X_Set_Abort_Char_Hook != NULL)
     (void) (*X_Set_Abort_Char_Hook) ((unsigned char) ch);

   Jed_Abort_Char = (int) ch;

   str[0] = ch; str[1] = 0;
   km = SLKeyMap_List_Root;
   while (km != NULL)
     {
	SLang_undefine_key(str, km);
	SLkm_define_key (str, (FVOID_STAR) kbd_quit, km);
	km = km->next;
     }
}

/*}}}*/

static SLKeymap_Function_Type *Flist_Context;
static int Flist_Context_Len;

static char *Slang_Functions[JED_MAX_ADD_COMPLETIONS];
static char **Slang_Flist_Context;

int next_function_list(char *buf) /*{{{*/
{
   char *name;
   char **max;
   
   /* Convert '-' to '_' */
   
   name = buf;
   while (*name != 0)
     {
	if (*name == '-') *name = '_';
	name++;
     }
   
   while (1)
     {
	SLKeymap_Function_Type *tthis = Flist_Context;
	SLFUTURE_CONST char *tname = tthis->name;
	if (tname == NULL) break;
	Flist_Context++;
	if (!Flist_Context_Len || !strncmp(buf, tname, Flist_Context_Len))
	  {
	     strcpy(buf, tname);
	     return(1);
	  }
     }
   
   max = Slang_Functions + JED_MAX_ADD_COMPLETIONS;
   while (Slang_Flist_Context < max)
     {
	name = *Slang_Flist_Context;
	if (name == NULL) return(0);
#if 0
	name++;  /* skip past hash mark */
#endif
	Slang_Flist_Context++;
	if (!Flist_Context_Len || !strncmp(buf, name, Flist_Context_Len))
	  {
	     strcpy(buf, name);
	     return(1);
	  }
     }
   return(0);
}

/*}}}*/

int open_function_list(char *buf) /*{{{*/
{
   Flist_Context = Jed_Functions;
   Slang_Flist_Context = Slang_Functions;
   Flist_Context_Len = strlen(buf);
   return next_function_list(buf);
}

/*}}}*/

int add_to_completion(char *name) /*{{{*/
{
   static char **last = Slang_Functions;

   if (last == Slang_Functions + JED_MAX_ADD_COMPLETIONS)
     return 0;

   if (0 == SLang_is_defined (name))
     {
	jed_verror ("add_completion: %s undefined", name);
	return -1;
     }

   name = SLang_create_slstring (name);
   if (name == NULL)
     return -1;

   *last++ = name;
   return 0;
}

/*}}}*/

int is_internal(char *f) /*{{{*/
{
   if (NULL == SLang_find_key_function(f, CBuf->keymap)) return(0);
   return 1;
}

/*}}}*/

static void copy_keymap_cmd (char *dest, char *src)
{
   SLKeyMap_List_Type *ksrc;

   ksrc = SLang_find_keymap (src);
   if (ksrc == NULL)
     {
	jed_verror ("Keymap \"%s\" does not exist",  src);
	return;
     }
   if (NULL != SLang_find_keymap (dest))
     {
	jed_verror ("Keymap \"%s\" already exists", dest);
	return;
     }

   if (NULL == SLang_create_keymap (dest, ksrc))
     jed_verror ("Unable to create keymap \"%s\"", dest);
}

void jed_call_cmd (char *str)
{
   SLKeyMap_List_Type *km;
   int (*fp)(void);

   if (*str == ' ')
     {
	jed_insert_string (str + 1);
	return;
     }
   
   if (*str == '@')
     {
	(void) do_macro_string (str + 1, 1);
	return;
     }
   
   km = CBuf->keymap;
   if (NULL == (fp = (int (*)(void)) (SLang_find_key_function(str, km))))
     jed_verror ("Internal function %s does not exist", str);
   else (void) (*fp)();
}

typedef struct
{
   unsigned char *key;
   unsigned int len;
   unsigned int num_read;
}
Getkey_Callback_Data_Type;

static Getkey_Callback_Data_Type Getkey_Callback_Data;

static int get_key_function_callback (void)
{
   unsigned int i = Getkey_Callback_Data.num_read;
   
   if (i == Getkey_Callback_Data.len)
     return SLANG_GETKEY_ERROR;
   
   Getkey_Callback_Data.num_read += 1;
   return (int) Getkey_Callback_Data.key[i];
}

   
static int push_key_binding (unsigned char *keystr)
{
   SLang_Key_Type *key;
   char *fun;
   int type;
#ifdef SLKEY_F_SLANG
   SLang_Name_Type *ref = NULL;
#endif

   SLang_Key_TimeOut_Flag = 0;

   if (keystr == NULL)
     {	
	Key_Bufferp = Key_Buffer;
	key = SLang_do_key (CBuf->keymap, jed_getkey);
	update_jed_keybuffer ();
	if (key == NULL)
	  flush_input ();

	if (SLKeyBoard_Quit || (SLang_get_error () == USER_BREAK))
	  {
	     SLang_set_error (0);
	     SLKeyBoard_Quit = 0;
	  }
     }
   else
     {
	Getkey_Callback_Data.len = (unsigned int) keystr[0];
	Getkey_Callback_Data.key = keystr + 1;
	Getkey_Callback_Data.num_read = 0;
	key = SLang_do_key (CBuf->keymap, get_key_function_callback);
     }
   
   fun = NULL;
   type = -1;

   if (key != NULL)
     switch (key->type)
       {
	case SLKEY_F_INTRINSIC:
	  type = 1;
	  fun = find_function_string (key->f.f);
	  break;
	  
	case SLKEY_F_INTERPRET:
	  fun = key->f.s;
	  type = 0;
	  if (fun[0] == '@') type = 2;
	  else if (fun[0] == ' ') type = 3;
	  break;
	  
#ifdef SLKEY_F_SLANG
	case SLKEY_F_SLANG:
	  type = 4;
	  ref = key->f.slang_fun;
	  break;
#endif
     }
   
   if (-1 == SLang_push_integer (type))
     return -1;

#ifdef SLKEY_F_SLANG
   if (type == 4)
     type = SLang_push_function (ref);
   else
#endif
     type = SLang_push_string (fun);
   
   return type;
}

/*}}}*/
   
static void get_key_binding (void)
{
   unsigned char *s = NULL;

   if (SLang_Num_Function_Args == 1)
     {
	char *str;

	if (-1 == SLang_pop_slstring (&str))
	  return;

	s = (unsigned char *) SLang_process_keystring (str);
	SLang_free_slstring (str);

	if (s == NULL)
	  return;
     }

   (void) push_key_binding (s);
}
   
/* This is weird, Ultrix cc will not compile if set_key comes before unset_key */
static void unset_key (char *key) /*{{{*/
{
   if (*key)
     SLang_undefine_key(key, Global_Map);
}

/*}}}*/

static void unset_key_in_keymap(char *key, char *map) /*{{{*/
{
   SLKeyMap_List_Type *kmap;

   if (NULL == (kmap = SLang_find_keymap(map)))
     {
	jed_verror ("Unknown keymap: %s", map);
	return;
     }
   
   if (*key) SLang_undefine_key(key, kmap);
}

/*}}}*/

static void set_key_in_keymap_1 (SLKeyMap_List_Type *kmap)
{
   char *key, *func;

   if (-1 == SLang_pop_slstring (&key))
     return;
#ifdef SLKEY_F_SLANG
   if (SLang_peek_at_stack () == SLANG_REF_TYPE)
     {
	SLang_Name_Type *nt = SLang_pop_function ();
	if (nt == NULL)
	  {
	     SLang_free_slstring (key);
	     return;
	  }
	if (-1 == SLkm_define_slkey (key, nt, kmap))
	  {
	     SLang_free_function (nt);
	     SLang_free_slstring (key);
	     return;
	  }
	SLang_free_slstring (key);
	return;
     }
#endif
   if (-1 == SLang_pop_slstring (&func))
     {
	SLang_free_slstring (key);
	return;
     }

   if (*key)
     (void) SLang_define_key(key, func, kmap);
   
   SLang_free_slstring (func);
   SLang_free_slstring (key);
}

static void set_key_in_keymap (void)
{
   char *keymap;
   SLKeyMap_List_Type *kmap;

   if (-1 == SLang_pop_slstring (&keymap))
     return;

   if (NULL == (kmap = SLang_find_keymap(keymap)))
     jed_verror ("Unknown keymap: %s", keymap);
   else
     set_key_in_keymap_1 (kmap);

   SLang_free_slstring (keymap);
}

/*}}}*/

static void set_key (void)
{
   set_key_in_keymap_1 (Global_Map);
}


/*}}}*/

static int keymap_p(char *name) /*{{{*/
{
   return ! (NULL == SLang_find_keymap(name));
}

/*}}}*/


static void make_keymap_cmd (char *km)
{
   copy_keymap_cmd (km, "global");
}

static void get_keymaps (void)
{
   int j, n;
   char **keymaps;
   SLang_Array_Type *at;
   SLkeymap_Type *km;

   n = 0;
   km = SLKeyMap_List_Root;
   while (km != NULL)
     {
	n++;
	km = km->next;
     }

   if (NULL == (at = SLang_create_array (SLANG_STRING_TYPE, 0, NULL, &n, 1)))
     return;

   keymaps = (char **)at->data;
   j = 0;
   km = SLKeyMap_List_Root;
   while (km != NULL)
     {
	if (j == n)
	  break;		       /* keymap list changing?? */

	if (NULL == (keymaps[j] = SLang_create_slstring (km->name)))
	  {
	     SLang_free_array (at);
	     return;
	  }
	km = km->next;
	j++;
     }
   SLang_push_array (at, 1);
}

static void getkey_wchar_intrin (void)
{
   int ch;
   SLwchar_Type wch;
   
   if (-1 == jed_getkey_wchar (&wch))
     {
	ch = (int) wch;
	(void) SLang_push_integer (-ch);
	return;
     }
   (void) SLclass_push_long_obj (SLANG_LONG_TYPE, (long)wch);
}

static void ungetkey_wchar_intrin (long *chp)
{
   if (*chp < 0)
     {
	int ch = -(int)*chp;
	ungetkey (&ch);
	return;
     }
   jed_ungetkey_wchar ((SLwchar_Type)*chp);
}

static SLang_Intrin_Fun_Type Keymap_Intrinsics [] = 
{
   MAKE_INTRINSIC_0("setkey", set_key, VOID_TYPE),
   MAKE_INTRINSIC_0("definekey", set_key_in_keymap, VOID_TYPE),
   MAKE_INTRINSIC_S("unsetkey", unset_key, VOID_TYPE),
   MAKE_INTRINSIC("what_keymap", what_keymap, STRING_TYPE, 0),
   MAKE_INTRINSIC_0("get_keymaps", get_keymaps, VOID_TYPE),
   MAKE_INTRINSIC_S("keymap_p",  keymap_p, INT_TYPE),
   MAKE_INTRINSIC("get_key_binding", get_key_binding, VOID_TYPE, 0),
   MAKE_INTRINSIC_S("dump_bindings", dump_bindings, VOID_TYPE),
   MAKE_INTRINSIC_SS("undefinekey", unset_key_in_keymap, VOID_TYPE),
   MAKE_INTRINSIC_S("which_key", which_key, INT_TYPE),
   MAKE_INTRINSIC_S("make_keymap", make_keymap_cmd, VOID_TYPE),
   MAKE_INTRINSIC_SS("copy_keymap", copy_keymap_cmd, VOID_TYPE),
   MAKE_INTRINSIC_S("use_keymap",  use_keymap, VOID_TYPE),
   MAKE_INTRINSIC("_getkey", jed_getkey, INT_TYPE, 0),
   MAKE_INTRINSIC_I("_ungetkey", ungetkey, VOID_TYPE),
   MAKE_INTRINSIC_0("getkey", getkey_wchar_intrin, VOID_TYPE),
   MAKE_INTRINSIC_1("ungetkey", ungetkey_wchar_intrin, VOID_TYPE, SLANG_LONG_TYPE),
#ifdef REAL_UNIX_SYSTEM
   MAKE_INTRINSIC_1("set_default_key_wait_time", jed_set_default_key_wait_time, SLANG_INT_TYPE, SLANG_INT_TYPE),
#endif
   SLANG_END_INTRIN_FUN_TABLE
};

static char *Key_Buffer_Ptr = Jed_Key_Buffer;
static char *Last_Kbd_Command_Str_Ptr = Last_Kbd_Command_String;
static char *Current_Kbd_Command_Str_Ptr = Current_Kbd_Command_String;

static SLang_Intrin_Var_Type Keymap_Variables [] =
{
   MAKE_VARIABLE("LASTKEY", &Key_Buffer_Ptr, STRING_TYPE, 1),
   MAKE_VARIABLE("LAST_KBD_COMMAND", &Last_Kbd_Command_Str_Ptr, STRING_TYPE, 1),
   MAKE_VARIABLE("CURRENT_KBD_COMMAND", &Current_Kbd_Command_Str_Ptr, STRING_TYPE, 1),
   MAKE_VARIABLE(NULL, NULL, 0, 0)
};

int jed_init_keymap_intrinsics (void)
{
   if ((-1 == SLadd_intrin_fun_table (Keymap_Intrinsics, NULL))
       || (-1 == SLadd_intrin_var_table (Keymap_Variables, NULL)))
     return -1;
   
   return 0;
}
