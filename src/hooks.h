/* Copyright (c) 1992-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
extern int (*X_Read_Hook) (void);
extern int (*X_Input_Pending_Hook) (void);
extern void (*X_Suspend_Hook)(void);
extern int (*X_Argc_Argv_Hook)(int, char **);
#if 0
extern int (*X_Init_SLang_Hook)(void);
#endif
extern int jed_add_init_slang_hook (int (*)(void));

extern int (*X_Init_Term_Hook) (void);
extern void (*X_Reset_Term_Hook) (void);
extern void (*X_Update_Open_Hook)(void);      /* hooks called when starting */
extern void (*X_Update_Close_Hook)(void);     /* and finishing update */
extern void (*X_Define_Keys_Hook) (SLKeyMap_List_Type *);
extern int (*X_Set_Abort_Char_Hook) (unsigned char);

#ifdef HAS_MOUSE
extern int (*X_Open_Mouse_Hook)(void);
extern void (*X_Close_Mouse_Hook)(void);
extern int (*JMouse_Event_Hook)(void);
extern void (*JMouse_Hide_Mouse_Hook)(int);
#endif

#define JED_HOOKS_RUN_ALL		1
#define JED_HOOKS_RUN_UNTIL_0		2
#define JED_HOOKS_RUN_UNTIL_NON_0	3
extern int jed_init_user_hooks (void);
extern int jed_va_run_hooks (char *name, int method, unsigned int nargs, ...);
extern int jed_hook_exists (char *);

