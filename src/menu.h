/* Copyright (c) 2007-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#ifndef _JED_MENU_H_
#define _JED_MENU_H_	1

typedef struct _Menu_Bar_Type Menu_Bar_Type;

extern int jed_init_menus (void);
extern int jed_redraw_menus (void);
extern int jed_select_menu_bar (void);
extern int jed_exit_menu_bar (void);
extern void jed_delete_menu_bar (Menu_Bar_Type *);

extern int jed_menu_handle_mouse (unsigned int, int, int, int, int);

extern SLang_Key_Type *jed_menu_do_key (void);
extern int Jed_Menus_Active;
extern void jed_notify_menu_buffer_changed (void);
#endif
