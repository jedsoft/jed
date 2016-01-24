/* Copyright (c) 1992-2010 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#define HAS_PALETTE_CODE 0

#include "config.h"
#include "jed-feat.h"

#include <windows.h>
#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#include <process.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <assert.h>
#include <io.h>
#include <errno.h>

#include "display.h"
#include "buffer.h"
#include "sysdep.h"
#include "screen.h"
#include "keymap.h"
#include "hooks.h"
#include "ins.h"
#include "ledit.h"
#include "misc.h"
#include "cmds.h"
#include "sig.h"
#include "win32.h"
#include "colors.h"
/* #include "resource.h" */  /* Where is it? */

#if JED_HAS_SUBPROCESSES
# include "jprocess.h"
#endif

#ifdef __WIN32__
# define HAS_WHEEL_MOUSE_SUPPORT	1
#else
# define HAS_WHEEL_MOUSE_SUPPORT	0
#endif

#define MSW_STRING_TYPE   1
#define MSW_INT_TYPE      2
#define MSW_COLOR_TYPE    3

#define MAX_STRING_SIZE	  128
#define MAX_KEYS          128

#define KEY_SHIFT         1
#define KEY_CONTROL       2
#define KEY_ALT	 	  4

int NumLock_Is_Gold = 0;
int PC_Alt_Char = 27;
int PC_Fn_Char = 0;

/* Last Charset used: keep track of input language changes */
static DWORD JedCharSet = DEFAULT_CHARSET;
static DWORD JedACP = CP_ACP;

#define SPACE_CHAR        (32 | (JNORMAL_COLOR << 8))

#define MAX_MENU_ID       256

#if !defined(_MSC_VER) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
extern HINSTANCE _hInstance;
#else
HINSTANCE _hInstance;
#endif

#ifdef __WIN32__
HINSTANCE _hPrev;
#else
extern HINSTANCE _hPrev;
#endif

/* #define CURRENT_BG_COLOR JColors[JNORMAL_COLOR].hbrBG */
#define CURRENT_BG_COLOR This_Window.current_color->hbrBG

#if HAS_WHEEL_MOUSE_SUPPORT

#ifndef WM_MOUSEWHEEL
# define WM_MOUSEWHEEL (WM_MOUSELAST+1)  /* message that will be supported by the OS */
#endif

/* Details of the mystery mouse wheel window */

#define MOUSEZ_CLASSNAME  "MouseZ"            /* wheel window class */
#define MOUSEZ_TITLE      "Magellan MSWHEEL"  /* wheel window title */

#define MSH_WHEELMODULE_CLASS (MOUSEZ_CLASSNAME)
#define MSH_WHEELMODULE_TITLE (MOUSEZ_TITLE)
#define MSH_MOUSEWHEEL     "MSWHEEL_ROLLMSG"
#define MSH_WHEELSUPPORT   "MSH_WHEELSUPPORT_MSG"  /* name of msg to send to query for wheel support */
#define MSH_SCROLL_LINES   "MSH_SCROLL_LINES_MSG"

/* Details of the mouse wheel functionality - not all currently used by Wjed */

typedef struct
{
   HWND hwnd;
   UINT uiMshMsgMouseWheel,
        uiMshMsg3DSupport,
        uiMshMsgScrollLines;
   INT  iScrollLines;
   BOOL fActive;
}
MS_Wheel_Type;

static MS_Wheel_Type Wheel_Mouse;

#endif                                 /* HAS_WHEEL_MOUSE_SUPPORT */

typedef struct
{
   char  *name;
   int    type;
   char  *dflt;
   void  *buf;
}
Msw_Ini_Type;

typedef struct
{
   COLORREF bg;
   COLORREF fg;
   HBRUSH   hbrBG;
}
Color_Type;

typedef struct
{
   char *name;
   int r, g, b;
}
Color_Value_Type;

typedef struct
{
   char        title[MAX_STRING_SIZE];
   HWND        w;
   HDC         hdc;
   int         ndc;

   char        font_name[MAX_STRING_SIZE];
   int         font_height;
   int         font_bold;
   int         font_width;
   HFONT       font;

   int         x;
   int         y;
   int         height;
   int         width;

   int         scroll_r1, scroll_r2;        /* scrolling region */
   int         cursor_row, cursor_col;      /* row column of cursor (1,1) origin */
   int         vis_curs_row, vis_curs_col;  /* position of VISIBLE cursor */
   int         cursor_showing;

   Color_Type *current_color;

   int         focus;
}
MSWindow_Type;

LRESULT CALLBACK JEDWndProc(HWND, UINT, WPARAM, LPARAM);

static void msw_cleanup(void);
static void msw_get_defaults(void);
static void msw_get_default(Msw_Ini_Type *t);
static void get_dc(void);
static void release_dc(void);
static void init_application(void);
static void init_instance(void);
void process_message(void);
static void show_cursor(void);
static void hide_cursor(void);
static void msw_normal_video(void);
static void set_window_name(char *s);
static void msw_define_color(char *, int *, int *, int *);
static void msw_select_font(char *, int *, int *);

static void get_menubar(void);
static void destroy_menubar(void);
static void create_popup_menu(void);
static void destroy_menu(void);
static void append_menu_item(void);
static void append_popup_menu(void);
static void append_separator(void);
static void insert_menu_item(void);
static void insert_menu_item_pos(void);
static void insert_popup_menu(void);
static void insert_popup_menu_pos(void);
static void insert_separator(void);
static void insert_separator_pos(void);
static void delete_menu_item(void);
static void delete_menu_item_pos(void);
static void get_menu_state(void);
static void get_menu_state_pos(void);
static void get_popup_menu(void);
static void check_menu_item(void);
static void check_menu_item_pos(void);
static void enable_menu_item(void);
static void enable_menu_item_pos(void);
static void redraw_menubar(void);

static void set_init_popup_callback(void);
static void msw_help(void);

static HINSTANCE hPrevInst;
static HINSTANCE hInstance;
static char *szJedSection = "WJED";

static int MSW_Screen_Cols;
static int MSW_Screen_Rows;
static int MSW_Term_Cannot_Scroll = 0;
static int MSW_Term_Cannot_Insert = 0;
static int MSW_Use_Ansi_Colors = 1;
static int MSW_Ignore_Beep = 3;
static int Performing_Update;

static MSWindow_Type This_Window;
static Color_Type JColors[JMAX_COLORS];

static char *InitPopup_Callback;
static char *Menu_Callbacks[MAX_MENU_ID];

#if HAS_PALETTE_CODE
static LPLOGPALETTE The_Lplgpl = NULL;
static HPALETTE The_Hpalette = NULL;
#endif

static char Font_Name[MAX_STRING_SIZE];
static int Font_Height;
static int Font_Bold;

static Msw_Ini_Type Msw_Ini_List[] =
{
     { "Font",         MSW_STRING_TYPE,   "fixed",         Font_Name },
     { "FontHeight",   MSW_INT_TYPE,      "12",            &Font_Height },
     { "FontBold",     MSW_INT_TYPE,      "-1",            &Font_Bold },
     { "Background",   MSW_COLOR_TYPE,    "0,0,0",         &JColors[JNORMAL_COLOR].bg },
     { "Foreground",   MSW_COLOR_TYPE,    "192,192,192",   &JColors[JNORMAL_COLOR].fg },
     { "StatusBG",     MSW_COLOR_TYPE,    "0,0,128",       &JColors[JSTATUS_COLOR].bg },
     { "StatusFG",     MSW_COLOR_TYPE,    "255,255,0",     &JColors[JSTATUS_COLOR].fg },
     { "RegionBG",     MSW_COLOR_TYPE,    "255,0,255",     &JColors[JREGION_COLOR].bg },
     { "RegionFG",     MSW_COLOR_TYPE,    "255,255,255",   &JColors[JREGION_COLOR].fg },
     { "OperatorBG",   MSW_COLOR_TYPE,    "0,0,0",         &JColors[JOP_COLOR].bg },
     { "OperatorFG",   MSW_COLOR_TYPE,    "255,255,255",   &JColors[JOP_COLOR].fg },
     { "NumberBG",     MSW_COLOR_TYPE,    "0,0,0",         &JColors[JNUM_COLOR].bg },
     { "NumberFG",     MSW_COLOR_TYPE,    "0,0,192",       &JColors[JNUM_COLOR].fg },
     { "StringBG",     MSW_COLOR_TYPE,    "0,0,0",         &JColors[JSTR_COLOR].bg },
     { "StringFG",     MSW_COLOR_TYPE,    "0,192,255",     &JColors[JSTR_COLOR].fg },
     { "CommentBG",    MSW_COLOR_TYPE,    "0,0,0",         &JColors[JCOM_COLOR].bg },
     { "CommentFG",    MSW_COLOR_TYPE,    "0,128,0",       &JColors[JCOM_COLOR].fg },
     { "KeywordBG",    MSW_COLOR_TYPE,    "0,0,0",         &JColors[JKEY_COLOR].bg },
     { "KeywordFG",    MSW_COLOR_TYPE,    "255,255,255",   &JColors[JKEY_COLOR].fg },
     { "Keyword1BG",   MSW_COLOR_TYPE,    "0,0,0",         &JColors[JKEY_COLOR + 1].bg },
     { "Keyword1FG",   MSW_COLOR_TYPE,    "255,255,255",   &JColors[JKEY_COLOR + 1].fg },
     { "DelimiterBG",  MSW_COLOR_TYPE,    "0,0,0",         &JColors[JDELIM_COLOR].bg },
     { "DelimiterFG",  MSW_COLOR_TYPE,    "255,255,255",   &JColors[JDELIM_COLOR].fg },
     { "PreprocessBG", MSW_COLOR_TYPE,    "0,0,0",         &JColors[JPREPROC_COLOR].bg },
     { "PreprocessFG", MSW_COLOR_TYPE,    "0,255,0",       &JColors[JPREPROC_COLOR].fg },
     { "MessageBG",    MSW_COLOR_TYPE,	  "0,0,0",         &JColors[JMESSAGE_COLOR].bg},
     { "MessageFG",    MSW_COLOR_TYPE,	  "255,255,0",     &JColors[JMESSAGE_COLOR].fg},
     { "ErrorFG",      MSW_COLOR_TYPE,	  "255,0,0",       &JColors[JERROR_COLOR].fg},
     { "ErrorBG",      MSW_COLOR_TYPE,	  "0,0,0",         &JColors[JERROR_COLOR].bg},
     { "MenuFG",       MSW_COLOR_TYPE,	  "0,0,0",         &JColors[JMENU_COLOR].fg},
     { "MenuBG",       MSW_COLOR_TYPE,	  "0,255,255",     &JColors[JMENU_COLOR].bg},
     {"CursorFG",      MSW_COLOR_TYPE,	  "0,255,0",       &JColors[JCURSOR_COLOR].fg},
     {"CursorBG",      MSW_COLOR_TYPE,	  "255,0,0",       &JColors[JCURSOR_COLOR].bg},
     {"DollarFG",      MSW_COLOR_TYPE,	  "0,0,255",       &JColors[JDOLLAR_COLOR].fg},
     {"DollarBG",      MSW_COLOR_TYPE,	  "0,0,0",         &JColors[JDOLLAR_COLOR].bg},
     {"CursorOvrFG",   MSW_COLOR_TYPE,	  "255,0,0",	   &JColors[JCURSOROVR_COLOR].fg},
     {"CursorOvrBG",   MSW_COLOR_TYPE,    "0,255,0",	   &JColors[JCURSOROVR_COLOR].bg},
     { "Title",        MSW_STRING_TYPE,   "WJED",          This_Window.title },
     { "X",            MSW_INT_TYPE,      "0",             &This_Window.x },
     { "Y",            MSW_INT_TYPE,      "0",             &This_Window.y },
     { "Width",        MSW_INT_TYPE,      "700",           &This_Window.width },
     { "Height",       MSW_INT_TYPE,      "500",           &This_Window.height },
     { NULL,           0,                 NULL,            NULL }
};

static Color_Value_Type Msw_Std_Color[] =
{
     {"black", 0, 0, 0},
     {"blue", 0, 0, 192},
     {"green", 0, 128, 0},
     {"cyan", 0, 192, 192},
     {"red", 192, 0, 0},
     {"magenta", 192, 0, 192},
     {"lightgray", 192, 192, 192},
     {"gray", 128, 128, 128},
     {"brightblue", 0, 0, 255},
     {"brightred", 255, 0, 0},
     {"brightgreen", 0, 255, 0},
     {"brightcyan", 0, 255, 255},
     {"brightmagenta", 255, 0, 255},
     {"yellow", 255, 255, 0},
     {"white", 255, 255, 255},
     {"brown", 110, 74, 32},
     {NULL, 0, 0, 0}
};

#define S SLANG_STRING_TYPE
#define I SLANG_INT_TYPE
static SLang_Intrin_Fun_Type Jed_WinGUI_Table[] =
{
   MAKE_INTRINSIC_SII("w32_select_font", msw_select_font, VOID_TYPE),
   MAKE_INTRINSIC_S("x_set_window_name", set_window_name, VOID_TYPE),
   MAKE_INTRINSIC_4("w32_define_color", msw_define_color, VOID_TYPE, S,I,I,I),
   /*Prototype: Void msw_define_color(char *, int, int, int);*/
   MAKE_INTRINSIC("w32_get_menubar", get_menubar, VOID_TYPE, 0),
   /* Prototype: Integer get_menubar(Void);
    * Returns integer which is handle of menubar. If there is no
    * menubar, it creates it.
    * To show menubar, call `redraw_menu'
    */
   MAKE_INTRINSIC("w32_destroy_menubar", destroy_menubar, VOID_TYPE, 0),
   /* Prototype: Void destroy_menubar(Void);
    * Destroys menubar
    */
   MAKE_INTRINSIC("w32_create_popup_menu", create_popup_menu, VOID_TYPE, 0),
   /* Prototype: Integer create_popup_menu(Void);
    * Creates empty popup menu and returns integer value which is
    * it's handle. If popup is not appended to another menu, it must
    * destroyed after use.
    */
   MAKE_INTRINSIC("w32_destroy_menu", destroy_menu, VOID_TYPE, 0),
   /* Prototype: Void destroy_menu(Integer hmenu);
    * Destroys menu and all it's popup menus.
    * Note: Do not destroy menubar with this function
    *       (use `destroy_menubar')
    */
   MAKE_INTRINSIC("w32_append_menu_item", append_menu_item, VOID_TYPE, 0),
   /* Prototype: Void append_menu_item(Integer hmenu, String name, Integer id, String callback);
    * Appends menu item with name 'name' and identifier 'id' at the end
    * of 'hmenu'. When item is selected, the 'callback' will be executed.
    * Callback can be intrinsic or internal function.
    */
   MAKE_INTRINSIC("w32_append_popup_menu", append_popup_menu, VOID_TYPE, 0),
   /* Prototype: Void append_popop_menu(Integer hmenu, String name, Integer popup);
    * Appends popup menu with name 'name' and handle 'popup' at the end
    * of 'hmenu'
    */
   MAKE_INTRINSIC("w32_append_separator", append_separator, VOID_TYPE, 0),
   /* Prototype: Void append_separator(Integer hmenu);
    * Appends menu separator at the end of 'hmenu'
    */
   MAKE_INTRINSIC("w32_insert_menu_item", insert_menu_item, VOID_TYPE, 0),
   /* Prototype: Void insert_menu_item(Integer hmenu, Integer id, String name, Integer idNew, String callback);
    * Inserts menu item with name 'name' and identifier 'idNew' before
    * menu item with identifier 'id'.
    * When item is selected, the 'callback' will be executed.
    * Callback can be intrinsic or internal function.
    */
   MAKE_INTRINSIC("w32_insert_menu_item_pos", insert_menu_item_pos, VOID_TYPE, 0),
   /* Prototype: Void insert_menu_item_pos(Integer hmenu, Integer pos, String name, Integer idNew, String callback);
    * Inserts menu item with name 'name' and identifier 'idNew' before
    * menu item with zero-based position 'pos' in 'hmenu'.
    * When item is selected, the 'callback' will be executed.
    * Callback can be intrinsic or internal function.
    */
   MAKE_INTRINSIC("w32_insert_popup_menu", insert_popup_menu, VOID_TYPE, 0),
   /* Prototype: Void insert_popup_menu(Integer hmenu, Integer id, String name, Integer popup);
    * Inserts popup menu with name 'name' and handle 'popup' before
    * menu item with identifier 'id'
    */
   MAKE_INTRINSIC("w32_insert_popup_menu_pos", insert_popup_menu_pos, VOID_TYPE, 0),
   /* Prototype: Void insert_popup_menu_pos(Integer hmenu, Integer pos, String name, Integer popup);
    * Inserts popup menu with name 'name' and handle 'popup' before
    * menu item with zero-based position 'pos' in 'hmenu'
    */
   MAKE_INTRINSIC("w32_insert_separator", insert_separator, VOID_TYPE, 0),
   /* Prototype: Void insert_separator(Integer hmenu, Integer id);
    * Inserts menu separator before menu item with identifier 'id'
    */
   MAKE_INTRINSIC("w32_insert_separator_pos", insert_separator_pos, VOID_TYPE, 0),
   /* Prototype: Void insert_separator_pos(Integer hmenu, Integer pos);
    * Inserts menu separator before menu item with zero-based position 'pos'
    */
   MAKE_INTRINSIC("w32_delete_menu_item", delete_menu_item, VOID_TYPE, 0),
   /* Prototype: Void delete_menu_item(Integer hmenu, Integer id);
    * Deletes menu item with identifier id from menu with handle 'hmenu'
    */
   MAKE_INTRINSIC("w32_delete_menu_item_pos", delete_menu_item_pos, VOID_TYPE, 0),
   /* Prototype: Void delete_menu_item_pos(Integer hmenu, Integer pos);
    * Deletes menu item at zero-based position 'pos' from menu 'hmenu'
    */
   MAKE_INTRINSIC("w32_get_menu_state", get_menu_state, VOID_TYPE, 0),
   /* Prototype: Integer get_menu_state(Integer hmenu, Integer id);
    * Gets state of menu item with identifier 'id'
    * <return value> & 1 == 1 if menu item is enabled
    * <return value> & 2 == 1 if menu item is checked
    */
   MAKE_INTRINSIC("w32_get_menu_state_pos", get_menu_state_pos, VOID_TYPE, 0),
   /* Prototype: Integer get_menu_state(Integer hmenu, Integer pos);
    * Gets state of menu item at zero-based position 'pos'
    * <return value> & 1 == 1 if menu item is enabled
    * <return value> & 2 == 1 if menu item is checked
    */
   MAKE_INTRINSIC("w32_get_popup_menu", get_popup_menu, VOID_TYPE, 0),
   /* Prototype: Void get_popup_menu(Integer hmenu, Integer pos);
    * Returns handle of popup menu at zero-based position 'pos'
    * If return value is 0, there is no popup at the position.
    */
   MAKE_INTRINSIC("w32_check_menu_item", check_menu_item, VOID_TYPE, 0),
   /* Prototype: Void check_menu_item(Integer hmenu, Integer id, Integer flag);
    * This functions changes check state of menu item. If flag is nonzero,
    * it checks menu item, otherwise it unchecks it
    */
   MAKE_INTRINSIC("w32_check_menu_item_pos", check_menu_item_pos, VOID_TYPE, 0),
   /* Prototype: Void check_menu_item(Integer hmenu, Integer pos, Integer flag);
    * This functions changes check state of menu item. If flag is nonzero,
    * it checks menu item, otherwise it unchecks it
    */
   MAKE_INTRINSIC("w32_enable_menu_item", enable_menu_item, VOID_TYPE, 0),
   /* Prototype: Void enable_menu_item(Integer hmenu, Integer id, Integer flag);
    * This functions enable or disable menu item. If flag is nonzero, the
    * menu item will be enabled, otherwise it'll be disabled.
    */
   MAKE_INTRINSIC("w32_enable_menu_item_pos", enable_menu_item_pos, VOID_TYPE, 0),
   /* Prototype: Void enable_menu_item_pos(Integer hmenu, Integer pos, Integer flag);
    * This functions enable or disable menu item. If flag is nonzero, the
    * menu item will be enabled, otherwise it'll be disabled.
    */
   MAKE_INTRINSIC("w32_redraw_menubar", redraw_menubar, VOID_TYPE, 0),
   /* Prototype: Void redraw_menubar(Void);
    * Redraws menubar. This functions should be called if menubar is changed
    */

   MAKE_INTRINSIC("w32_set_init_popup_callback", set_init_popup_callback, VOID_TYPE, 0),
   /* Prototype: Void set_init_popup_callback(String callback);
    * Executes callback before menu poppup was popped up.
    */

   MAKE_INTRINSIC("msw_help", msw_help, VOID_TYPE, 0),
   /* Prototype: Void msw_help(String filename, String keword, Integer Partial_Keys);
    * Starts Windows Help with 'filename' help file. If 'keyword' is not null
    * string shows topic with specified keyword. If 'Partial_Keys' != 0
    * shows Search dialog if there is more than one help topic beginnig with
    * 'keyword'
    */

   MAKE_INTRINSIC(NULL, NULL, 0, 0)
};
#undef S
#undef I

static int wjed_has_unicode(void)
{
   static int unicode = -1;

   if (unicode == -1)
     unicode = SLsmg_is_utf8_mode();
   return unicode;
}

static void _putkey (WCHAR wc)
{
   if (wjed_has_unicode ())
     {
        unsigned char buf[SLUTF8_MAX_MBLEN+1], *b;
        b = SLutf8_encode((SLwchar_Type)wc, buf, SLUTF8_MAX_MBLEN);
        buffer_keystring ((char *)buf, b-buf);
     }
   else
     {
        /* 0xE0 is prefix for cursor keys, but also &agrave; on ANSI
         * charset. The charmap has 0xE0 0xE0 to &agrave; */
	unsigned char ch = (unsigned char) wc;
        if (ch == 0xE0) buffer_keystring ((char *)&ch, 1);
        buffer_keystring ((char *)&ch, 1);
     }
}

static void send_key_sequence (unsigned int n, unsigned char c1, unsigned char c2,
			       unsigned char c3, unsigned char c4)
{
   unsigned char buf[4];

   if (n > 4)
     exit_error ("n>4 in send_key_sequence", 0);

   buf[0] = c1;
   buf[1] = c2;
   buf[2] = c3;
   buf[3] = c4;
   buffer_keystring((char *)buf, n);
}

/* Getting defaults from INI file */
static COLORREF msw_get_color(char *s, char *dflt)
{

   if (s[0] >= '0' && s[0] <= '9')
     {
	long r, g, b;
	char *sptr, *endptr;

	sptr = s;
	r = strtol(sptr, &endptr, 0);
	while ((*endptr == ' ') || (*endptr == '\t')) endptr++; /* skipping whitespace */
	if ((sptr == endptr) || (*endptr++ != ',')) return msw_get_color(dflt, dflt);
	sptr = endptr;

	g = strtol(sptr, &endptr, 0);
	while ((*endptr == ' ') || (*endptr == '\t')) endptr++; /* skipping whitespace */

	if ((sptr == endptr) || (*endptr++ != ',')) return msw_get_color(dflt, dflt);
	sptr = endptr;

	b = strtol(sptr, &endptr, 0);
	while ((*endptr == ' ') || (*endptr == '\t')) endptr++; /* skipping whitespace */
	if ((sptr == endptr) || (*endptr++ != 0)) return msw_get_color(dflt, dflt);

#if HAS_PALETTE_CODE
	return PALETTERGB((BYTE)r, (BYTE)g, (BYTE)b);
#else
	return RGB((BYTE)r, (BYTE)g, (BYTE)b);
#endif
     }
   else
     {
	char buf[50];
	GetProfileString(szJedSection, s, dflt, buf, sizeof(buf));
	if (buf[0] >='0' && buf[0] <= '9')
	  return msw_get_color(buf, dflt);
	else
	  return msw_get_color(dflt, dflt);
     }
}

static void msw_get_default(Msw_Ini_Type *t)
{
   char s[MAX_STRING_SIZE];

   GetProfileString(szJedSection, t->name, t->dflt, s, sizeof(s));

   switch (t->type)
     {
      case MSW_STRING_TYPE:
	strncpy((char *)t->buf, s, MAX_STRING_SIZE);
	((char *)t->buf)[MAX_STRING_SIZE-1] = 0;
	break;

      case MSW_INT_TYPE:
	*(int *)t->buf = atoi(s);
	break;

      case MSW_COLOR_TYPE:
	*(COLORREF *)t->buf = msw_get_color(s, t->dflt);
	break;
     }

}

static void msw_get_defaults(void)
{
   int i = 0;
   char buf[128];

   /* Check for standard color names in INI file.
    * Add they if cannot find. */
   while (Msw_Std_Color[i].name != NULL)
     {
	if (!GetProfileString(szJedSection, Msw_Std_Color[i].name, "", buf, sizeof(buf)))
	  msw_define_color(Msw_Std_Color[i].name, &Msw_Std_Color[i].r, &Msw_Std_Color[i].g, &Msw_Std_Color[i].b);

	i++;
     }

   i = 0;
   while (Msw_Ini_List[i].name != NULL)
     msw_get_default(&Msw_Ini_List[i++]);
}

static void get_dc(void)
{

   if (!This_Window.ndc)
     {
	This_Window.hdc = GetDC(This_Window.w);
#if HAS_PALETTE_CODE
	SelectPalette (This_Window.hdc, The_Hpalette, FALSE);
#endif
     }
   This_Window.ndc++;
}

static void release_dc(void)
{
   assert(This_Window.ndc);

   if (This_Window.ndc == 1) ReleaseDC(This_Window.w, This_Window.hdc);
   This_Window.ndc--;
}

void process_message(void)
{
   MSG msg;

   if (
#ifdef _MSC_VER
       GetMessage(&msg, NULL, 0, 0)
#else
       PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)
#endif
       )
     {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
     }
}

static void init_application(void)
{
   WNDCLASS wc;

   wc.style = 0;
   wc.lpfnWndProc = JEDWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = hInstance;
   wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
   wc.hbrBackground = NULL;
   wc.lpszMenuName = NULL;
   wc.lpszClassName = "wjed";

   RegisterClass(&wc);
}

static int select_font(char *fontname, int fontheight, int fontbold)
{
   TEXTMETRIC tm;
   HFONT font;
   int height, weight;

   get_dc();
   SetMapMode(This_Window.hdc, MM_TEXT);

   if (fontbold < 0)
     weight = FW_DONTCARE;
   else
     weight = fontbold > 0 ? FW_BOLD : FW_NORMAL;

   height = -MulDiv(fontheight, GetDeviceCaps(This_Window.hdc, LOGPIXELSY), 72);

   font = CreateFont(height, 0, 0, 0,
                     weight, 0, 0, 0,
                     JedCharSet, 0, 0, 0, FIXED_PITCH,
                     fontname);

   if (font == NULL)
     {
	font = CreateFont(height, 0, 0, 0,
			  weight, 0, 0, 0,
			  DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH,
			  fontname);
     }

   if (font == NULL)
     {
	/* This should never happen as we've told CreateFont to use the default */
	/* character set if the requested one isn't available. */
	jed_verror ("Unable to create instance of '%s' font", fontname);
	release_dc ();
	return -1;
     }

   if(This_Window.font != NULL)
     DeleteObject(This_Window.font);

   This_Window.font        = font;
   This_Window.font_bold   = fontbold;
   This_Window.font_height = fontheight;

   strncpy(This_Window.font_name, fontname, MAX_STRING_SIZE-1);
   This_Window.font_name[MAX_STRING_SIZE-1] = '\0';

   /* retrieve font metrics (width and base) */

   SelectObject(This_Window.hdc, This_Window.font);
   GetTextMetrics(This_Window.hdc, &tm);
   This_Window.font_width = tm.tmAveCharWidth;
   This_Window.font_height = tm.tmHeight;

   release_dc();
   return 0;
}

static void init_instance (void)
{
   int i;
   TEXTMETRIC tm;

   This_Window.hdc = NULL;
   This_Window.ndc = 0;

   for (i = 0; i < MAX_MENU_ID; i++) Menu_Callbacks[i] = NULL;
   InitPopup_Callback = NULL;

   msw_get_defaults();

/* Apparantly this is wrong */
   /* if (This_Window.font_bold < 0) */
   /*   font_weight = FW_DONTCARE; */
   /* else */
   /*   font_weight = (This_Window.font_bold > 0) ? FW_BOLD : FW_NORMAL; */

   /* This_Window.font = CreateFont(-This_Window.font_height, 0, 0, 0, */
   /* 				 font_weight, */
   /* 				 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, */
   /* 				 This_Window.font_name); */
#if !HAS_PALETTE_CODE
   for (i = 0; i < JMAX_COLORS; i++)
     JColors[i].hbrBG = CreateSolidBrush(JColors[i].bg);
#endif

   msw_normal_video();

   /* creating window */
   This_Window.w = CreateWindow("wjed", This_Window.title, WS_OVERLAPPEDWINDOW, This_Window.x,
				This_Window.y, This_Window.width,This_Window.height, NULL, NULL,
				hInstance, NULL);
   get_dc();
#if HAS_PALETTE_CODE
   if ((RC_PALETTE & GetDeviceCaps (This_Window.hdc, RASTERCAPS)) &&
       (NULL != (The_Lplgpl = malloc (sizeof(*The_Lplgpl) - sizeof(The_Lplgpl->palPalEntry)
				      + 2*JMAX_COLORS * sizeof(PALETTEENTRY)))))
     {
	The_Lplgpl->palVersion = 0x300;
	The_Lplgpl->palNumEntries = JMAX_COLORS*2;
	for (i = 0; i < JMAX_COLORS; i++)
	  {
	     The_Lplgpl->palPalEntry[i*2].peRed = GetRValue (JColors[i].fg);
	     The_Lplgpl->palPalEntry[i*2].peGreen = GetGValue (JColors[i].fg);
	     The_Lplgpl->palPalEntry[i*2].peBlue = GetBValue (JColors[i].fg);
	     The_Lplgpl->palPalEntry[i*2].peFlags = PC_NOCOLLAPSE;

	     The_Lplgpl->palPalEntry[i*2+1].peRed = GetRValue (JColors[i].bg);
	     The_Lplgpl->palPalEntry[i*2+1].peGreen = GetGValue (JColors[i].bg);
	     The_Lplgpl->palPalEntry[i*2+1].peBlue = GetBValue (JColors[i].bg);
	     The_Lplgpl->palPalEntry[i*2+1].peFlags = PC_NOCOLLAPSE;
	  }
	The_Hpalette = CreatePalette (The_Lplgpl);
	if (The_Hpalette)
	  {
	     SelectPalette (This_Window.hdc, The_Hpalette, FALSE);
	     RealizePalette (This_Window.hdc);
	  }
     }
   /* ReleaseDC (This_Window.w, This_Window.hdc); */

   for (i = 0; i < JMAX_COLORS; i++)
     JColors[i].hbrBG = CreateSolidBrush(JColors[i].bg);
#endif				       /* HAS_PALETTE_CODE */

   /* creating fonts, brushes etc */
   (void) select_font (Font_Name, Font_Height, Font_Bold);

#ifdef _MSC_VER
   SetTimer (This_Window.w, 43, 100, NULL);
#endif

   /* function that will be called when jed exits (deletes fonts, brushes etc.) */
   atexit(msw_cleanup);

   /* retrieve font metrics (width and base) */
   /* get_dc(); */

   SelectObject(This_Window.hdc, This_Window.font);
   GetTextMetrics(This_Window.hdc, &tm);
   This_Window.font_width = tm.tmAveCharWidth;
   This_Window.font_height = tm.tmHeight;
   release_dc();

   ShowWindow(This_Window.w, SW_SHOW);
   UpdateWindow(This_Window.w);

#if !defined(__WIN32__) || !JED_HAS_SUBPROCESSES
   SetTimer(This_Window.w, 42, 30000, NULL);     /* used for updating display time */
#endif
   JedACP = GetACP();
}

static int msw_init_term (void)
{
   if (Batch) return 0;

   hInstance = _hInstance;
   hPrevInst = _hPrev;

   if (!hPrevInst) init_application();

   init_instance();
   return 0;
}

static void copy_rect(int x1, int y1, int x2, int y2, int x3, int y3)
{
   int dx, dy;
   RECT rcSrc;

   dx = (x3 - x1) * This_Window.font_width;
   dy = (y3 - y1) * This_Window.font_height;

   SetRect(&rcSrc, x1 * This_Window.font_width, y1 * This_Window.font_height, x2 * This_Window.font_width, y2 * This_Window.font_height);

   ScrollWindow(This_Window.w, dx, dy, &rcSrc, NULL);

#ifdef __WIN32__
   UpdateWindow(This_Window.w);
#endif
}

static void blank_rect(int x1, int y1, int x2, int y2)
{
   char blanks[256];

   memset (blanks, ' ', sizeof (blanks));

   if (This_Window.cursor_showing) hide_cursor();

   get_dc ();
   SelectObject(This_Window.hdc, This_Window.font);
   SetTextColor(This_Window.hdc, This_Window.current_color->fg);
   SetBkColor(This_Window.hdc, This_Window.current_color->bg);

   while (y1 < y2)
     {
	int n = x2 - x1;
	int xx = x1;

	while (n > 0)
	  {
	     int dn;
	     if (n > (int) sizeof (blanks))
	       dn = sizeof (blanks);
	     else
	       dn = n;

	     (void) TextOut(This_Window.hdc, xx * This_Window.font_width,
			    y1 * This_Window.font_height, blanks, dn);
	     n -= dn;
	     xx += dn;
	  }
	y1++;
     }
   release_dc ();
#if 0
   RECT rc;

   SetRect(&rc, x1 * This_Window.font_width, y1 * This_Window.font_height, x2 * This_Window.font_width, y2 * This_Window.font_height);
   get_dc();
   FillRect(This_Window.hdc, &rc, CURRENT_BG_COLOR);
   release_dc();
#endif
}

static void msw_reverse_video(int color)
{
   if ((color < 0) || (color >= JMAX_COLORS))
     return;

   This_Window.current_color = &JColors[color];
}

static int smg_read_at(int row, int col, SLsmg_Char_Type *s, unsigned int n)
{
   int saverow, savecol;
   unsigned int rc;

   saverow = SLsmg_get_row();
   savecol = SLsmg_get_column();
   SLsmg_gotorc(row, col);
   rc = SLsmg_read_raw (s, n);
   SLsmg_gotorc(saverow, savecol);
   return rc;
}

static void _tt_writeW(WCHAR *s, int n, int color)
{
   if (color < 0 || color > JMAX_COLORS)
     color = 0;

   get_dc();
   SelectObject(This_Window.hdc, This_Window.font);
   SetTextColor(This_Window.hdc, JColors[color].fg);
   SetBkColor(This_Window.hdc, JColors[color].bg);
   TextOutW(This_Window.hdc, This_Window.cursor_col * This_Window.font_width, This_Window.cursor_row * This_Window.font_height, s, n);
   This_Window.cursor_col += n;

   if (This_Window.cursor_col >= MSW_Screen_Cols) This_Window.cursor_col = MSW_Screen_Cols - 1;
   release_dc();
}

# define LEAD_OFFSET      (0xD800 - (0x10000 >> 10))
static int decode_utf32 (unsigned int u, WCHAR *buf)
{
   if (u < 0xFFFF)
     {
	*buf = u;
	return 1;
     }
    else
      {
	 *(buf)   = LEAD_OFFSET + (u >> 10);
	 *(buf+1) = 0xDC00 + (u & 0x3FF);
	 return 2;
      }
}

/* SLsmg_Char_Type decode/compare: the char internals changed for SLang2 */
static int decode_smgchar(SLsmg_Char_Type *s, WCHAR *buf)
{
   if (s->nchars > 0)
     return decode_utf32(s->wchars[0], buf);

   *buf = (WCHAR)' ';
   return 1;
}

# define SLSMGCHAR_EQUAL(o, n) \
   ( ((o)->nchars == (n)->nchars) \
	&& ((o)->color  == (n)->color) \
	&& (0 == memcmp((o)->wchars, (n)->wchars, (n)->nchars * sizeof(SLwchar_Type))))
# define SLSMGCHAR_SET_CHAR(sc, _char) do { (sc).nchars = 1; (sc).wchars[0] = (_char); } while (0)
# define SLSMGCHAR_SET_COLOR(sc, _color) (sc).color = (_color)

static void msw_write_smgchars(SLsmg_Char_Type *s, SLsmg_Char_Type *smax)
{
   WCHAR buf[512];
   WCHAR *b, *bmax;
   int oldcolor, color;

   b = buf;
   bmax = b + sizeof (buf)/sizeof(buf[0]);

   hide_cursor();

   oldcolor = (SLSMG_EXTRACT_COLOR(*s) & SLSMG_COLOR_MASK);
   color = 0;
   get_dc();
   for (; s < smax; s++)
     {
        color = (SLSMG_EXTRACT_COLOR(*s) & SLSMG_COLOR_MASK);
        if (oldcolor != color ||
            b >= bmax-1) /* always leave room for 1 more 1 char (for surrogates) */
          {
             _tt_writeW(buf, b - buf, oldcolor);
             b = buf;
             msw_reverse_video(color);
             oldcolor = color;
          }
        b += decode_smgchar(s, b);
   }
   if (b != buf)
     _tt_writeW(buf, b - buf, color);

   release_dc();
}

static void msw_write_smgchar(SLsmg_Char_Type *s)
{
   WCHAR buf[2];
   int l;

   get_dc();
   l = decode_smgchar(s, buf);
   _tt_writeW (buf, l, SLSMG_EXTRACT_COLOR(*s) & SLSMG_COLOR_MASK);
   release_dc();
}

static void hide_cursor(void)
{
   SLsmg_Char_Type sc;
   int col = This_Window.vis_curs_col, row = This_Window.vis_curs_row;
   int savecur;

   if (This_Window.cursor_showing == 0)
     return;
   This_Window.cursor_showing = 0;

   if (smg_read_at(row, col, &sc, 1) == 0)
     {
	SLSMGCHAR_SET_CHAR(sc, ' ');
	SLSMGCHAR_SET_COLOR(sc, JNORMAL_COLOR);
     }
   savecur = This_Window.cursor_col;
   msw_write_smgchar(&sc);
   This_Window.cursor_col = savecur;
}

static void show_cursor(void)
{
   SLsmg_Char_Type sc;
   int color;

   if (This_Window.cursor_showing) hide_cursor();

   This_Window.cursor_showing = 1;
   This_Window.vis_curs_row = This_Window.cursor_row;
   This_Window.vis_curs_col = This_Window.cursor_col;

   if (CBuf && CBuf->flags & OVERWRITE_MODE)
     color = JCURSOROVR_COLOR;
   else
     color = JCURSOR_COLOR;

   if (This_Window.focus)
     {
	int savecur;
	if (smg_read_at(This_Window.cursor_row, This_Window.cursor_col, &sc, 1) == 0)
	  SLSMGCHAR_SET_CHAR(sc, ' ');
	SLSMGCHAR_SET_COLOR(sc, color);
        savecur = This_Window.cursor_col;
        msw_write_smgchar(&sc);
        This_Window.cursor_col = savecur;
     }
   else
     {
	RECT rc;
	rc.left = This_Window.cursor_col * This_Window.font_width;
	rc.top = This_Window.cursor_row * This_Window.font_height;
	rc.right = rc.left + This_Window.font_width;
	rc.bottom = rc.top + This_Window.font_height;
	get_dc();
	FrameRect(This_Window.hdc, &rc, JColors[color].hbrBG);
	release_dc();
     }
}

void sys_suspend(void)
{
   ShowWindow(This_Window.w, SW_MINIMIZE);
}

void flush_output (void)
{
   fflush (stdout);
}

/* Hooks */
static void msw_update_open (void)
{
   hide_cursor ();
   Performing_Update = 1;
}

static void msw_update_close (void)
{
   Performing_Update = 0;
   if (JWindow->trashed) return;
   show_cursor ();
}

static int msw_init_slang (void)
{
   if ((-1 == SLadd_intrin_fun_table(Jed_WinGUI_Table, "WINGUI"))
       || (-1 == SLdefine_for_ifdef ("MOUSE")))
     return -1;

   return 0;
}

static void msw_suspend (void)
{
   WINDOWPLACEMENT wndpl;

   GetWindowPlacement(This_Window.w, &wndpl);

   if (wndpl.showCmd == SW_MINIMIZE)
     ShowWindow(This_Window.w, SW_NORMAL);
   else
     ShowWindow(This_Window.w, SW_MINIMIZE);
}

static void msw_define_xkeys (SLKeyMap_List_Type *map)
{
#if 0
   /* This is set by doskeys.c */
   SLkm_define_key ("^[Ow", (FVOID_STAR) bob, map);
   SLkm_define_key ("^[Oq", (FVOID_STAR) eob, map);
#endif
   SLkm_define_key ("\xE0\xE0", (FVOID_STAR) ins_char_cmd, map);
}

/* This routine is called from S-Lang inner interpreter.  It serves
 as a poor mans version of an interrupt 9 handler */
static void msw_check_kbd(void)
{
   MSG msg;

   if (Batch) return;

   while (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
     process_message();
}

/* Terminal functions */
static void msw_goto_rc(int r, int c)
{
   get_dc();
   if (This_Window.cursor_showing) hide_cursor();

   r += This_Window.scroll_r1;
   if (r >= MSW_Screen_Rows) r = MSW_Screen_Rows - 1;
   if (c >= MSW_Screen_Cols) c = MSW_Screen_Cols - 1;

   This_Window.cursor_row = r;
   This_Window.cursor_col = c;

   if (!Performing_Update) show_cursor();
   release_dc();
}

static void msw_del_eol(void)
{
   blank_rect(This_Window.cursor_col, This_Window.cursor_row, MSW_Screen_Cols, This_Window.cursor_row + 1);
}

static void msw_delete_nlines(int n)
{
   int r1, r2;
   r1 = This_Window.cursor_row;
   r2 = This_Window.scroll_r2;

   if (r1 <= r2 - n) copy_rect(0, r1 + n, MSW_Screen_Cols, r2 + 1, 0, r1);
   r2++;
   blank_rect(0, r2 - n, MSW_Screen_Cols, r2);
}

#if 0				       /* UNUSED */
static void msw_delete_char(void)
{
   copy_rect(This_Window.cursor_col + 1, This_Window.cursor_row, MSW_Screen_Cols, This_Window.cursor_row + 1,
	     This_Window.cursor_col, This_Window.cursor_row);
}

static void msw_erase_line(void)
{
   blank_rect(0, This_Window.cursor_row, MSW_Screen_Cols, This_Window.cursor_row + 1);
}

static void msw_begin_insert(void)
{
   hide_cursor();
   copy_rect(This_Window.cursor_col, This_Window.cursor_row, MSW_Screen_Cols - 1, This_Window.cursor_row + 1,
	     This_Window.cursor_col + 1, This_Window.cursor_row);
}

static void msw_end_insert(void)
{
}

/* This function is called assuming that cursor is in correct position */
static void msw_putchar(char ch)
{
   if (ch == '\b')
     {
	ch = ' ';
	if (This_Window.cursor_col == 0) return;
	This_Window.cursor_col--;
     }

   get_dc();
   if (Rev_Vid_Flag != JNORMAL_COLOR) msw_normal_video();
   tt_write(&ch, 1);
   show_cursor ();
   release_dc();
}

#endif				       /* UNUSED */

static void msw_normal_video(void)
{
   msw_reverse_video (JNORMAL_COLOR);
}

static void msw_cls(void)
{
   RECT rc;

   get_dc();
   GetClientRect(This_Window.w, &rc);
   FillRect(This_Window.hdc, &rc, CURRENT_BG_COLOR);
   release_dc();
   /* Unfortunately the background color produced by FillRect is slightly different
    * from the desired background.  So patch it up with blank_rect.
    */
   blank_rect (0, 0, MSW_Screen_Cols, MSW_Screen_Rows);
}

static void msw_beep(void)
{
   if (MSW_Ignore_Beep & 0x1) MessageBeep(0);

   if (MSW_Ignore_Beep & 0x2)
     {
	RECT rc;

	get_dc();
	GetClientRect(This_Window.w, &rc);
	InvertRect(This_Window.hdc, &rc);
	release_dc();

	InvalidateRect(This_Window.w, NULL, TRUE);
#ifdef __WIN32__
	UpdateWindow(This_Window.w);
#endif
     }
}

static void msw_reverse_index(int n)
{
   int r1, r2;

   r1 = This_Window.scroll_r1;
   r2 = This_Window.scroll_r2;

   if (r2 >= r1 + n) copy_rect(0, r1, MSW_Screen_Cols, r2 - n + 1, 0, r1 + n);

   blank_rect(0, r1, MSW_Screen_Cols, r1 + n);
}

static void msw_write_string (char *s)
{
   get_dc();
   /* tt_write(s, strlen(s)); */
   if (!Performing_Update) show_cursor ();
   release_dc();
}

static void msw_smart_puts(SLsmg_Char_Type *neww, SLsmg_Char_Type *oldd, int len, int row)
{
   int col;

   col = 0;
   while ((col < len) && SLSMGCHAR_EQUAL(&neww[col], &oldd[col]))
     col++;

   if (col < len)
     {
	msw_goto_rc (row, col);
	msw_write_smgchars(&neww[col], &neww[len]);
     }
}

static void msw_set_scroll_region(int r1, int r2)
{
   This_Window.scroll_r1 = r1;
   This_Window.scroll_r2 = r2;
}

static void msw_reset_scroll_region (void)
{
   msw_set_scroll_region (0, MSW_Screen_Cols - 1);
}

static int msw_reset_video (void)
{
   msw_reset_scroll_region ();
   msw_goto_rc (0, 0);
   msw_normal_video ();
   return 0;
}

static int msw_init_video (void)
{
   msw_reset_video ();
   if ((MSW_Screen_Rows == 0) || (MSW_Screen_Cols == 0))
     {
	MSW_Screen_Cols = 80;
	MSW_Screen_Rows = 24;
     }
   return 0;
}

static int msw_flush_output (void)
{
   return 0;
}

static char *convert_color (char *c, char *buf)
{
   unsigned long ul;

   if ((*c != '#')
       || (7 != strlen (c)))   /* e.g., #RRGGBB */
     return c;

   if (1 != sscanf (c+1, "%lX", &ul))
     return c;

   if (ul > 0xFFFFFFUL)
     return c;

   sprintf (buf, "%ld,%ld,%ld", (ul >> 16)&0xFF, (ul >> 8)&0xFF, ul & 0xFF);
   return buf;
}

static JX_SETXXX_RETURN_TYPE msw_set_color (int i, char *what, char *fg, char *bg)
{
   int r, g, b;
   char buf[64];
   char fg_buf[64];
   char bg_buf[64];
   HBRUSH bgbr;

   if ((i < 0) || (i > JMAX_COLORS))
     return JX_SETXXX_RETURN_VAL;

   (void) what;

   fg = convert_color (fg, fg_buf);
   bg = convert_color (bg, bg_buf);

   r = GetRValue(JColors[i].fg);
   g = GetGValue(JColors[i].fg);
   b = GetBValue(JColors[i].fg);
   sprintf(buf, "%d,%d,%d", r, g, b);
   JColors[i].fg = msw_get_color(fg, buf);

   r = GetRValue(JColors[i].bg);
   g = GetGValue(JColors[i].bg);
   b = GetBValue(JColors[i].bg);
   sprintf(buf, "%d,%d,%d", r, g, b);
   JColors[i].bg = msw_get_color(bg, buf);

#if HAS_PALETTE_CODE
   if (The_Hpalette)
     {
	The_Lplgpl->palPalEntry[i*2].peRed = GetRValue (JColors[i].fg);
	The_Lplgpl->palPalEntry[i*2].peGreen = GetGValue (JColors[i].fg);
	The_Lplgpl->palPalEntry[i*2].peBlue = GetBValue (JColors[i].fg);
	The_Lplgpl->palPalEntry[i*2].peFlags = PC_NOCOLLAPSE;
	The_Lplgpl->palPalEntry[i*2+1].peRed = GetRValue (JColors[i].bg);
	The_Lplgpl->palPalEntry[i*2+1].peGreen = GetGValue (JColors[i].bg);
	The_Lplgpl->palPalEntry[i*2+1].peBlue = GetBValue (JColors[i].bg);
	The_Lplgpl->palPalEntry[i*2+1].peFlags = PC_NOCOLLAPSE;
	SetPaletteEntries (The_Hpalette, i*2, 2, The_Lplgpl->palPalEntry+i*2);
	get_dc();
	SelectPalette (This_Window.hdc, The_Hpalette, FALSE);
	RealizePalette (This_Window.hdc);
	release_dc();
     }
#endif
   bgbr = CreateSolidBrush(JColors[i].bg);
   if (bgbr != 0)
     {
	DeleteObject(JColors[i].hbrBG);
	JColors[i].hbrBG = bgbr;
     }

   SLsmg_touch_screen ();

   return JX_SETXXX_RETURN_VAL;

   /* InvalidateRect(This_Window.w, NULL, FALSE); */
}

static void cover_exposed_area (int x, int y, int width, int height)
{
   SLsmg_Char_Type *s;
   int row, save_row, save_col, max_col, max_row, col;
   int width_chars, len;

   Performing_Update++;
   hide_cursor ();
   save_row = This_Window.cursor_row;
   save_col = This_Window.cursor_col;
   col = x / This_Window.font_width;
   row = y / This_Window.font_height;

   width_chars = 2 + width / This_Window.font_width;
   max_col = col + width_chars;
   max_row = 2 + row + height / This_Window.font_height;
   if (max_col > MSW_Screen_Cols) max_col = MSW_Screen_Cols;
   if (max_row > MSW_Screen_Rows) max_row = MSW_Screen_Rows;

   if (NULL == (s = (SLsmg_Char_Type *)SLmalloc(width_chars*sizeof(*s))))
     goto done;

   while (row < max_row)
     {
	msw_goto_rc (row, col);
        len = smg_read_at(row, col, s, width_chars);
        msw_write_smgchars(s, s + len);
	row++;
     }
   SLfree((char *)s);
   msw_goto_rc (save_row, save_col);

   done:
   Performing_Update--;

   show_cursor ();
}

static void push_mouse_event(int button, int x, int y, int state, int type)
{
   unsigned int s = 0;
   int col, row;
   static int last_button;
   static JMouse_Type jm;
#if JED_HAS_MULTICLICK
   static long last_press_time;
   static unsigned int clicks = 0;
   static long MultiClick_Time = -1;
#endif
   int id;

   col = 1 + x / This_Window.font_width;
   row = 1 + y / This_Window.font_height;

   if ((type == JMOUSE_DRAG)
       && (col == jm.x) && (row == jm.y))
     return;

   if (button == 0)
     button = last_button;
   else
     last_button = button;

#if JED_HAS_MULTICLICK
   if (type == JMOUSE_DOWN)
     {
	long the_time = GetTickCount();
	if (MultiClick_Time == -1)
	  MultiClick_Time = GetDoubleClickTime();
	if ((last_button == button)
	    && (the_time - last_press_time < MultiClick_Time))
	  {
	     clicks++;
	     if (clicks == 2)
	       type = JMOUSE_DOUBLE_CLICK;
	     else
	       type = JMOUSE_TRIPLE_CLICK;
	  }
	else
	  {
	     clicks = 1;
	     last_button = button;
	  }
	last_press_time = the_time;
     }
   else if ((clicks > 1) && (last_button == button))
     {
	/* Last was a multi-click.  Ignore this event. */
	type = JMOUSE_IGNORE_EVENT;
     }
#endif

   jm.button = button;

   if ((type == JMOUSE_UP)
       && (0 == (jm.state & jm.button)))
     {
	/* Make sure this button was down. */
	return;
     }

   if (state & MK_LBUTTON) s |= JMOUSE_BUTTON_1;
   if (state & MK_MBUTTON) s |= JMOUSE_BUTTON_2;
   if (state & MK_RBUTTON) s |= JMOUSE_BUTTON_3;
   if (state & MK_CONTROL) s |= JMOUSE_CTRL;
   if (state & MK_SHIFT) s |= JMOUSE_SHIFT;

   jm.state = s;
   jm.x = col;
   jm.y = row;
   jm.type = type;

   if (-1 == (id = jed_mouse_add_event (&jm)))
     {
	msg_error ("Failed!");
	return;
     }

   send_key_sequence(3, 033, 0, id, 0);
}

static void push_wm_mouse_event (int button, int x, int y, int state, int type)
{
   switch (button)
     {
      case WM_LBUTTONUP:
      case WM_LBUTTONDOWN:
	if (!(GetKeyState(VK_MENU) & 0x8000))
	  {
	     button = JMOUSE_BUTTON_1;
	     break;
	  }
	/* drop */
      case WM_MBUTTONUP:
      case WM_MBUTTONDOWN:
	button = JMOUSE_BUTTON_2;
	break;

      case WM_RBUTTONUP:
      case WM_RBUTTONDOWN:
	button = JMOUSE_BUTTON_3;
	break;

      default:
	return;
     }

   push_mouse_event (button, x, y, state, type);
}

static void msw_define_color(char *color_name, int *pr, int *pg, int *pb)
{
   char buf[30];

   sprintf(buf, "%d,%d,%d", *pr, *pg, *pb);
   WriteProfileString(szJedSection, color_name, buf);
}

static void msw_select_font (char *fontname, int *height, int *bold)
{
   if (0 == select_font (fontname, *height, *bold))
     {
	jed_init_display ();
	jed_redraw_screen (1);
	/* update font data, As I need it if the user changes Language */
	strcpy(Font_Name, fontname);
	Font_Height = *height;
	Font_Bold = *bold;
	return;
     }
   jed_verror ("Unable to allocate font %s", fontname);
}

static void set_window_name (char *s)
{
   if (Batch) return;
   strcpy(This_Window.title, s);
   SetWindowText(This_Window.w, s);
}

static void msw_cleanup(void)
{
   int i;
   char buf[10];
   RECT rc;

   GetWindowRect(This_Window.w, &rc);
   sprintf(buf, "%d", (int) rc.left);
   WriteProfileString(szJedSection, "X", buf);
   sprintf(buf, "%d", (int) rc.top);
   WriteProfileString(szJedSection, "Y", buf);
   sprintf(buf, "%d", (int) (rc.right - rc.left));
   WriteProfileString(szJedSection, "Width", buf);
   sprintf(buf, "%d", (int) (rc.bottom - rc.top));
   WriteProfileString(szJedSection, "Height", buf);
   KillTimer(This_Window.w, 42);
#ifdef _MSC_VER
   KillTimer (This_Window.w, 43);
#endif

   if (This_Window.w) DestroyWindow(This_Window.w);
   DeleteObject(This_Window.font);
   for(i = 0; i < JMAX_COLORS; i++)
     if (JColors[i].hbrBG) DeleteObject(JColors[i].hbrBG);
}

#if !defined(__WIN32__) || !JED_HAS_SUBPROCESSES
int sys_input_pending(int *tsecs, int unused)
{
   DWORD t = GetTickCount() + *tsecs * 100L;

   (void) unused;

   while ((!Input_Buffer_Len) && (GetTickCount() < t)) process_message();

   return Input_Buffer_Len != 0;
}

unsigned char sys_getkey(void)
{
   int n;

   while (!SLKeyBoard_Quit && !Input_Buffer_Len) process_message ();

   if (SLKeyBoard_Quit)
     {
	SLKeyBoard_Quit = 0;
	flush_input ();
	return Jed_Abort_Char;
     }

   n = my_getkey();

   SLKeyBoard_Quit = 0;

   return n;
}
#else
int sys_input_pending(int *tsecs, int all)
{
   DWORD ret = 0;
   int i, n;

   if ((all >= 0) && (Input_Buffer_Len || Batch)) return (Input_Buffer_Len);

   if (all < 0)
     {
	/* Note: Input_Events[0] is only used by console jed. */
	ret = WaitForMultipleObjects(Num_Subprocesses, Input_Events+1, FALSE,
				     *tsecs * 100);
     }
   else
     {
	DWORD t;
	long rtime = *tsecs * 100L;

	msw_check_kbd ();
	while ((rtime > 0) && !Input_Buffer_Len)
	  {
	     t = GetTickCount();
	     ret = MsgWaitForMultipleObjects(Num_Subprocesses, Input_Events+1,
					     FALSE, rtime, QS_ALLINPUT);

	     if ((DWORD)(WAIT_OBJECT_0 + Num_Subprocesses) != ret) break;
	     msw_check_kbd ();
	     rtime -= GetTickCount() - t;
	  }

	if ((rtime < 0) || Input_Buffer_Len)
	  return Input_Buffer_Len;
     }

   if (WAIT_TIMEOUT == ret) return 0;

   n = 0;
   i = 1;   /* First input event is not used by wjed.  Start at 1. */
   while (i <= Num_Subprocesses)
     {
	 /* Check if current subprocess has input */
	if (WAIT_TIMEOUT != WaitForSingleObject(Input_Events[i], 0))
	  {
	     read_process_input (i);
	     n++;
	  }
	i++;
     }
   if (all < 0) return n;
   else return 0;
}

unsigned char sys_getkey(void)
{
   int n = 450;

   /* sleep for 45 second and try again */
   while (!SLKeyBoard_Quit && !sys_input_pending(&n, Num_Subprocesses))
     {
	 /* update status line incase user is displaying time */
	if (SLKeyBoard_Quit) break;
	JWindow->trashed = 1;
	update((Line *) NULL, 0, 1, 0);
     }

   if (SLKeyBoard_Quit)
     {
	SLKeyBoard_Quit = 0;
	flush_input ();
	return Jed_Abort_Char;
     }

   return my_getkey();
}

#endif

static int Ignore_Wchar_Message;

static unsigned char f_keys[4][12] =
{
     { 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 133, 134 },
     { 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 135, 136 },
     { 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 137, 138 },
     { 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 139, 140 }
};

static unsigned char small_keypad_keys[4][13] =
{
     { 'G', 'H', 'I', 0, 'K', 0, 'M', 0, 'O', 'P', 'Q', 'R', 'S' },   /* normal */
     { '0', '1', '2', 0, '3', 0, '4', 0, '5', '6', '7', '8', '9' },   /* shift */
     { 'w', 141, 132, 0, 's', 0, 't', 0, 'u', 145, 'v', 146, 147 },   /* ctrl */
     { 151, 152, 153, 0, 155, 0, 157, 0, 159, 160, 161, 162, 163 }   /* alt */
};

static unsigned char num_keypad_keys[4][13] =
{
     { 'w', 'x', 'y', 0, 't', 'u', 'v', 0, 'q', 'r', 's', 'p', 'n' },
     { '0', '1', '2', 0, '3',  0 , '4', 0, '5', '6', '7', '8', '9' },
     { 'w', 141, 132, 0, 's', 143, 't', 0, 'u', 145, 'v', 146, 147 },
     { 'w', 'x', 'y', 0, 't', 'u', 'v', 0, 'q', 'r', 's', 'p', 'n' }
};

static LRESULT CALLBACK process_key_down (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   unsigned int key_state = 0;
   unsigned int scan;
   unsigned char prefix, c1;
   int i, state;

   if (GetKeyState(VK_CONTROL) & 0x8000) key_state |= KEY_CONTROL;
   if (GetKeyState(VK_SHIFT) & 0x8000) key_state |= KEY_SHIFT;
   if (GetKeyState(VK_MENU) & 0x8000) key_state |= KEY_ALT;

   Ignore_Wchar_Message = 0;

   scan = (unsigned int) ((lParam >> 16) & 0x1FF);

   switch (scan)
     {
      default: return DefWindowProc(hWnd, msg, wParam, lParam);

      case 0x00E:		       /* backspace */
	/* Ctrl-BS  --> ALT-@
	 * Shift-BS --> ALT-?
	 * BS       --> ^?
	 */
	if ((key_state & KEY_ALT) == 0)
	  {
	     if ((key_state & KEY_CONTROL) != 0)
	       send_key_sequence(2, PC_Alt_Char, '@', 0, 0);
	     else if ((key_state & KEY_SHIFT) != 0)
	       send_key_sequence(2, PC_Alt_Char, '?', 0, 0);
	     else
	       send_key_sequence(1, 127, 0, 0, 0);
	  }
	Ignore_Wchar_Message = 1;
	return 0;

      case 0x039: 		       /* space */
	if (key_state & KEY_CONTROL)
	  {
	     Ignore_Wchar_Message = 1;
	     buffer_keystring ("\000\003", 2);
	  }
	return 0;

      case 0x003:		       /* 2 key */
	if (((key_state & KEY_ALT) == 0)
	    && (key_state & KEY_CONTROL))
	  {
	     Ignore_Wchar_Message = 1;
	     buffer_keystring ("\000\003", 2);
	     return 0;
	  }
	return DefWindowProc(hWnd, msg, wParam, lParam);

      case 0x00C:		       /* -/_ key */
	if (((key_state & KEY_ALT) == 0)
	    && (key_state & KEY_CONTROL))
	  {
	     Ignore_Wchar_Message = 1;
	     send_key_sequence(1, 31, 0, 0, 0); /* ^_ */
	     return 0;
	  }
	return DefWindowProc(hWnd, msg, wParam, lParam);

      /* PF1 and PF2 codes */
      /* case 0x0045: */
      case 0x0145:
	if (NumLock_Is_Gold)
	  {
#if 0				       /* does not work on xp */
	     BYTE keyState[256];
	     /* toggle the caps-lock state */
	     GetKeyboardState(keyState);
	     keyState[VK_NUMLOCK] = 0;/*keyState[VK_NUMLOCK]^1;*/
	     SetKeyboardState(keyState);
#endif

	     buffer_keystring ("\033OP", 3);
	     Ignore_Wchar_Message = 1 ;
	     return 0 ;
	  }
	return DefWindowProc(hWnd, msg, wParam, lParam);

      case 0x0135:
	c1 = 'Q'; break ;

      case 0xE02F:
	c1 = 'Q'; break;	       /* KEYPAD SLASH */

      case 0x037:		       /* KEYPAD STAR */
	c1 = 'R';
	break;

      case 0x04A:		       /* KEYPAD MINUS */
	c1 = 'S';
	break;

      case 0x04E:		       /* KEYPAD PLUS */
	c1 = 'm';
	break;

      case 0x047:            /* KEYPAD HOME */
      case 0x048:            /* KEYPAD UP */
      case 0x049:            /* KEYPAD PGUP */
      case 0x04B:            /* KEYPAD LEFT */
      case 0x04C:            /* KEYPAD 5 */
      case 0x04D:            /* KEYPAD RIGHT */
      case 0x04F:            /* KEYPAD END */
      case 0x050:            /* KEYPAD DOWN */
      case 0x051:            /* KEYPAD PGDN */
      case 0x053:            /* KEYPAD DEL */
      case 0x052:            /* KEYPAD INSERT */
	if (!NumLock_Is_Gold && (GetKeyState(VK_NUMLOCK) & 0x0001))
          return DefWindowProc(hWnd, msg, wParam, lParam);

 	state = 0;
 	if (key_state & KEY_SHIFT) state = 1;
 	if (key_state & KEY_CONTROL) state = 2;
 	if (key_state & KEY_ALT) state = 3;

 	if (key_state & (KEY_CONTROL | KEY_ALT))
 	  {
	     Ignore_Wchar_Message = 1;
	     send_key_sequence(2, 0, num_keypad_keys[state][scan - 0x47], 0, 0);
 	     return 0;
 	  }
 	else
 	  c1 = num_keypad_keys[state][scan - 0x47];
 	break;

      case 0x11C:            /* KEYPAD ENTER */
	if (key_state & KEY_ALT)
	  {
	     send_key_sequence(2, 0, 166, 0, 0);
	     Ignore_Wchar_Message = 1;
	     return 0;
 	  }
 	else
 	  {
 	     c1 = 'M';
 	     break;
 	  }

      case 0x147: 		       /* home */
      case 0x148:		       /* UP */
      case 0x149:		       /* PGUP */
      case 0x14B:		       /* LEFT */
      case 0x14D:		       /* RIGHT */
      case 0x14F:		       /* END */
      case 0x150:		       /* DOWN */
      case 0x151:		       /* PGDN */
      case 0x153:		       /* DEL */
      case 0x152:		       /* INSERT */
	prefix = 0xE0;
	state = 0;
	if (key_state & KEY_SHIFT) state = 1;
	if (key_state & KEY_CONTROL) state = 2;
	if (key_state & KEY_ALT)
	  {
	     prefix = 0;
	     state = 3;
	  }

	send_key_sequence(2, prefix, small_keypad_keys[state][scan - 0x147], 0, 0);
	Ignore_Wchar_Message = 1;
	return 0;

      case 0x3b:		       /* F1 */
      case 0x3c:
      case 0x3d:
      case 0x3e:
      case 0x3f:
      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43:
      case 0x44:
      case 0x57:
      case 0x58:		       /* F12 */
	i = scan - 0x3b;
	if (i > 9) i -= 0x12;

	state = 0;
	if (key_state & KEY_SHIFT) state = 1;
	if (key_state & KEY_CONTROL) state = 2;
	if (key_state & KEY_ALT)
	  {
	     if (i == 3)		       /* Alt-F4 */
	       return DefWindowProc(hWnd, msg, wParam, lParam);
	     else
	       state = 3;
	  }

	send_key_sequence(2, PC_Fn_Char, f_keys[state][i], 0, 0);
	Ignore_Wchar_Message = 1;
	return 0;
     }

   send_key_sequence(3, 033, 'O', c1, 0);
   Ignore_Wchar_Message = 1;
   return 0;
}

static int load_dropped_files(HDROP hDrop, HWND hwnd)
{
   char szFilename[JED_MAX_PATH_LEN];
   int iNumFiles;

   (void) hwnd;

   iNumFiles = DragQueryFile (hDrop, 0xFFFFFFFF, NULL, JED_MAX_PATH_LEN-1);

   while(--iNumFiles>=0)
     {
	DragQueryFile(hDrop, iNumFiles, szFilename, JED_MAX_PATH_LEN-1);
	(void) find_file_in_window(szFilename);
	window_buffer(CBuf);
	jed_redraw_screen (1);
     }
   DragFinish(hDrop);

   return 0;
}

LRESULT CALLBACK JEDWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static int mouse_button_down;
   PAINTSTRUCT ps;
   RECT rc;

   switch (msg)
     {
      case WM_INPUTLANGCHANGE:
	  {
	     CHARSETINFO csi;

	     JedCharSet = wParam;
	     TranslateCharsetInfo((DWORD *)wParam, &csi, TCI_SRCCHARSET);
	     JedACP = csi.ciACP;
	     (void) msw_select_font (Font_Name, &Font_Height, &Font_Bold);
	  }
	return 0;

      case WM_CREATE:
	This_Window.w = hWnd;
	DragAcceptFiles (hWnd, TRUE);
	mouse_button_down = 0;
	return 0;

      case WM_DROPFILES:
	(void) load_dropped_files ((HANDLE) wParam, hWnd);
	return 0;

      case WM_SIZE:
	if (wParam != SIZE_MINIMIZED)
	  {
	     This_Window.width = LOWORD(lParam);
	     This_Window.height = HIWORD(lParam);
	     jed_init_display ();
	     jed_redraw_screen (1);
	  }
	break;

      case WM_PAINT:
	BeginPaint(This_Window.w, &ps);
#if HAS_PALETTE_CODE
	if (NULL != The_Hpalette)
	  SelectPalette (ps.hdc, The_Hpalette, FALSE);
#endif
	This_Window.hdc = ps.hdc;
	This_Window.ndc = 1;
	cover_exposed_area(ps.rcPaint.left, ps.rcPaint.top,
			   ps.rcPaint.right - ps.rcPaint.left,
			   ps.rcPaint.bottom - ps.rcPaint.top);
	EndPaint(This_Window.w, &ps);
	This_Window.hdc = NULL;
	This_Window.ndc = 0;
	break;

#if HAS_PALETTE_CODE
      case WM_PALETTECHANGED:
	if (wParam != This_Window.w && The_Hpalette != NULL)
	  {
	     get_dc();
	     UpdateColors (This_Window.hdc);
	     release_dc();
	  }
	break;

      case WM_QUERYNEWPALETTE:
	if (The_Hpalette != NULL)
	  {
	     get_dc();
	     RealizePalette (This_Window.hdc);
	     release_dc();
	  }
	break;
#endif

      case WM_ERASEBKGND:
	GetClientRect(This_Window.w, &rc);
	FillRect((HDC)wParam, &rc, CURRENT_BG_COLOR);
	break;

      case WM_SYSCHAR:
	_putkey(PC_Alt_Char);
	if (PC_Alt_Char != 0)
	  _putkey (wParam);
	else
	  _putkey((lParam >> 16) & 0x1FF);/* was:	_putkey(wParam); */

	return MAKELONG(0, 1);

      case WM_CHAR:
	if (Ignore_Wchar_Message == 0)
	  {
	     if ((int) wParam == (int) Jed_Abort_Char)
	       {
		  if (Ignore_User_Abort == 0) SLang_set_error(USER_BREAK);
		  SLKeyBoard_Quit = 1;
	       }
	     /* PS 28Apr98: handle shiftTAB same as DOS */
	     if (wParam == 0x009)
	       {
		  if (GetKeyState(VK_SHIFT)<0)
		    {
		       send_key_sequence(2, 0, 'o' & 31, 0, 0);
		       Ignore_Wchar_Message = 1;
		       break;
		    }
	       }
	     if (!wjed_has_unicode())
	       _putkey(wParam);
	     else
	       {
		  static char dbcsbuf[3] = "";
		  WCHAR buf[10];
		  int i, buflen = 10;
		  int dbcsl;

		  /* Here I get ANSI characters, but _putkey() wants UTF-16. */
		  if (!dbcsbuf[0] && IsDBCSLeadByte((unsigned char)wParam)) {
		     dbcsbuf[0] = (char)wParam;
		     break;
		  }
		  dbcsl = 1;
		  if (dbcsbuf[0])
		    {
		       dbcsbuf[1] = (unsigned char) wParam;
		       dbcsl = 2;
		    }
		  else
		    {
		       dbcsbuf[0] = (unsigned char) wParam;
		    }
		  dbcsbuf[2] = 0;
		  buflen = MultiByteToWideChar(JedACP, 0, dbcsbuf, dbcsl, buf, buflen);
		  for (i = 0; i < buflen; i++)
		    _putkey(buf[i]);
		  dbcsbuf[0] = 0;
	       }
	     break;
	  }
	return DefWindowProc(hWnd, msg, wParam, lParam);

      case WM_SYSKEYDOWN:
      case WM_KEYDOWN:
	return process_key_down (hWnd, msg, wParam, lParam);

      case WM_KEYUP:
	Ignore_Wchar_Message = 0;
	break;

      case WM_SYSKEYUP:
	Ignore_Wchar_Message = 0;
	return DefWindowProc(hWnd, msg, wParam, lParam);

      case WM_LBUTTONUP:
      case WM_MBUTTONUP:
      case WM_RBUTTONUP:

	if (!mouse_button_down) break;

	push_wm_mouse_event(msg, LOWORD(lParam), HIWORD(lParam), wParam, JMOUSE_UP);
	mouse_button_down--;
	break;

      case WM_LBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
	mouse_button_down++;
	push_wm_mouse_event(msg, LOWORD(lParam), HIWORD(lParam), wParam, JMOUSE_DOWN);
	break;

      case WM_MOUSEMOVE:
	if (wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON))
	  push_mouse_event(0, LOWORD(lParam), HIWORD(lParam), wParam, JMOUSE_DRAG);
	break;

      case WM_SETFOCUS:
      case WM_KILLFOCUS:
	hide_cursor();
	This_Window.focus = (msg == WM_SETFOCUS);
	show_cursor();
	break;

#if 0
	/* This does not quite work when coming back from a shell */
      case WM_ACTIVATE:
	if ((wParam != WA_INACTIVE) && (CBuf != NULL)) check_buffers();
	break;
#endif
      case WM_CLOSE:
	jed_exit_jed(1);
	break;

#if !defined(__WIN32__) || !JED_HAS_SUBPROCESSES
      case WM_TIMER:
	if ((wParam == 42) && Display_Time)
	  {
	     JWindow->trashed = 1;
	     update((Line *) NULL, 0, 1, 0);
	  }
	return 0;
#endif

#if defined(_MSC_VER) && JED_HAS_SUBPROCESSES
      case WM_TIMER:
	/*
	 * just stop the GetMessage call...
	 */
        return 0;
#endif

      case WM_COMMAND:
	if (wParam < MAX_MENU_ID)
	  {
	     int force = 1;
	     if (Menu_Callbacks[wParam])
	       {
		  if (is_internal(Menu_Callbacks[wParam]))
		    {
		       char buf[512];
		       SLsnprintf(buf, sizeof (buf), ". \"%s\" call", Menu_Callbacks[wParam]);
		       SLang_load_string (buf);
		    }
		  else
		    if (SLang_execute_function(Menu_Callbacks[wParam]) == 0) msg_error(Menu_Callbacks[wParam]);
	       }

	     update_cmd(&force);
	  }
	return 0;

      case WM_INITMENUPOPUP:
	SLang_push_integer(wParam);
	if (InitPopup_Callback != NULL)
	  if (SLang_execute_function(InitPopup_Callback) == 0)
	    msg_error(InitPopup_Callback);
	return 0;

      default:

#if HAS_WHEEL_MOUSE_SUPPORT
      /* Check for a mouse wheel movement message
       * and translate it into a fake keystroke. This has now been
       * tested on Win95,98,NT4,2000
       */
        if ((Wheel_Mouse.fActive
             && (msg == Wheel_Mouse.uiMshMsgMouseWheel))
            || (msg == WM_MOUSEWHEEL))
          {
	     /* Under X, the wheel mouse produces JMOUSE_BUTTON_4 and
 	      * JMOUSE_BUTTON_5 events.  jed/lib/mouse.sl maps JMOUSE_BUTTON_4
 	      * events to upward movement and JMOUSE_BUTTON_5 to downward
 	      * movement in the buffer.
 	      */
	     if ((int) wParam < 0)
	       push_mouse_event (JMOUSE_BUTTON_5, LOWORD(lParam), HIWORD(lParam), wParam, JMOUSE_DOWN);
	     else
	       push_mouse_event (JMOUSE_BUTTON_4, LOWORD(lParam), HIWORD(lParam), wParam, JMOUSE_DOWN);

 	     return 0;
          }
#endif
        return DefWindowProc(hWnd, msg, wParam, lParam);
     }

   return 0;
}

#if defined(__x86_64__)
static int pop_hmenu (HMENU *h)
{
   long long i;
   if (-1 == SLang_pop_long_long (&i))
     return -1;

   *h = (HMENU) i;
   return 0;
}

static int push_hmenu (HMENU h)
{
   return SLang_push_long_long ((long long)h);
}

#else
static int pop_hmenu (HMENU *h)
{
   long i;
   if (-1 == SLang_pop_long (&i))
     return -1;

   *h = (HMENU) i;
   return 0;
}

static int push_hmenu (HMENU h)
{
   return SLang_push_long ((long) h);
}
#endif

void get_menubar()
{
   HMENU hmenu = GetMenu(This_Window.w);

   if (!hmenu)
     {
	hmenu = CreateMenu();
	SetMenu(This_Window.w, hmenu);
	DrawMenuBar(This_Window.w);
     }
   (void) push_hmenu (hmenu);
}

void destroy_menubar()
{
   int i;
   HMENU hmenu = GetMenu(This_Window.w);

   if (hmenu)
     {
	SetMenu(This_Window.w, NULL);
	DestroyMenu(hmenu);

     }

   for(i = 0; i < MAX_MENU_ID; i++) Menu_Callbacks[i] = NULL;
}

void create_popup_menu()
{
   HMENU hmenu = CreatePopupMenu();

   (void) push_hmenu (hmenu);
}

void destroy_menu()
{
   HMENU hmenu;
   int count, i;
   int id;

   if (0 == pop_hmenu (&hmenu))
     {
	count = GetMenuItemCount(hmenu);
	for(i = 0; i < count; i++)
	  {
	     id = GetMenuItemID(hmenu, i);
	     if (id > 0 && Menu_Callbacks[id] != NULL)
	       {
		  SLfree(Menu_Callbacks[id]);
		  Menu_Callbacks[id] = NULL;
	       }
	  }

	if (!DestroyMenu(hmenu)) msg_error("Cannot destroy menu");
     }
}

void append_menu_item()
{
   HMENU hmenu;
   char *name = NULL;
   char *callback = NULL;
   int   id;

   if (!SLpop_string(&callback) &&
       !SLang_pop_integer(&id) &&
       !SLpop_string(&name) &&
       !pop_hmenu (&hmenu))
     {
	if ((id < 0) || (id >= MAX_MENU_ID))
	  msg_error("Id is out of range.");
	else
	  {
	     if (!AppendMenu(hmenu, MF_STRING, id, name))
	       msg_error("Cannot append menu item");
	     else
	       {
		  if (Menu_Callbacks[id] != NULL)
		    {
		       jed_verror ("Id %d is already used.", id);
		    }
		  else
		    {
		       Menu_Callbacks[id] = callback;
		       callback = NULL;
		    }
	       }
	  }
     }

   if (name != NULL) SLfree(name);
   if (callback != NULL) SLfree(callback);
}

void append_popup_menu()
{
   HMENU hmenu;
   char *name = NULL;
   HMENU popup;

   if (!pop_hmenu(&popup) &&
       !SLpop_string (&name) &&
       !pop_hmenu(&hmenu))
     {
	if (!AppendMenu(hmenu, MF_STRING | MF_POPUP, (UINT_PTR)popup, name))
	  msg_error("Cannot append popup menu");

     }
   if (name != NULL) SLfree(name);
}

void append_separator()
{
   HMENU hmenu;

   if (!pop_hmenu (&hmenu))
     {
	if (!AppendMenu(hmenu, MF_STRING | MF_SEPARATOR, 0, 0))
	  msg_error("Cannot append separator");
     }
}

void insert_menu_item()
{
   HMENU hmenu;
   int   id;
   char *name = NULL;
   int   idNew;
   char *callback = NULL;

   if (!SLpop_string(&callback) &&
       !SLang_pop_integer(&idNew) &&
       !SLpop_string(&name) &&
       !SLang_pop_integer(&id) &&
       !pop_hmenu(&hmenu))
     {
	if ((idNew < 0) || (idNew >= MAX_MENU_ID))
	  msg_error("Id is out of range");
	else
	  {
	     if (!InsertMenu(hmenu, id, MF_STRING | MF_BYCOMMAND, idNew, name))
	       msg_error("Cannot insert menu item");
	     else
	       {
		  if (Menu_Callbacks[idNew] != NULL)
		    {
		       jed_verror ("Id %d is already used.", idNew);
		    }
		  else
		    {
		       Menu_Callbacks[idNew] = callback;
		       callback = NULL;
		    }
	       }
	  }
     }
   if (name != NULL) SLfree(name);
   if (callback != NULL) SLfree(callback);
}

void insert_popup_menu()
{
   HMENU hmenu;
   int   id;
   char *name = NULL;
   HMENU popup;

   if (!pop_hmenu(&popup) &&
       !SLpop_string (&name) &&
       !SLang_pop_integer(&id) &&
       !pop_hmenu(&hmenu))
     {
	if (!InsertMenu(hmenu, id, MF_STRING | MF_POPUP | MF_BYCOMMAND, (UINT_PTR)popup, name))
	  msg_error("Cannot insert popup menu");

     }
   if (name != NULL) SLfree (name);
}

void insert_separator()
{
   HMENU hmenu;
   int   id;

   if (!SLang_pop_integer(&id) &&
       !pop_hmenu (&hmenu))
     {
	if (!InsertMenu(hmenu, id, MF_STRING | MF_SEPARATOR | MF_BYCOMMAND, 0, 0))
	  msg_error("Cannot insert separator");
     }
}

void insert_menu_item_pos()
{
   HMENU hmenu;
   int   pos;
   char *name = NULL;
   int   idNew;
   char *callback = NULL;

   if (!SLpop_string(&callback) &&
       !SLang_pop_integer(&idNew) &&
       !SLpop_string(&name) &&
       !SLang_pop_integer(&pos) &&
       !pop_hmenu(&hmenu))
     {
	if ((idNew < 0) || (idNew >= MAX_MENU_ID))
	  msg_error("Id is out of range.");
	else
	  {
	     if (!InsertMenu(hmenu, pos, MF_STRING | MF_BYPOSITION, idNew, name))
	       msg_error("Cannot insert menu item");
	     else
	       {
		  if (Menu_Callbacks[idNew] != NULL)
		    {
		       jed_verror ("Id %d is already used.", idNew);
		    }
		  else
		    {
		       Menu_Callbacks[idNew] = callback;
		       callback = NULL;
		    }
	       }
	  }
     }
   if (name != NULL) SLfree(name);
   if (callback != NULL) SLfree(callback);
}

void insert_popup_menu_pos()
{
   HMENU hmenu;
   int   pos;
   char *name = NULL;
   HMENU popup;

   if (!pop_hmenu(&popup) &&
       !SLpop_string(&name) &&
       !SLang_pop_integer(&pos) &&
       !pop_hmenu(&hmenu))
     {
	if (!InsertMenu(hmenu, pos, MF_STRING | MF_POPUP | MF_BYPOSITION, (UINT_PTR)popup, name))
	  msg_error("Cannot insert popup menu");

     }
   if (name != NULL) SLfree(name);
}

void insert_separator_pos()
{
   HMENU hmenu;
   int   pos;

   if (!SLang_pop_integer(&pos) &&
       !pop_hmenu(&hmenu))
     {
	if (!InsertMenu(hmenu, pos, MF_STRING | MF_SEPARATOR | MF_BYPOSITION, 0, 0))
	  msg_error("Cannot insert separator");
     }
}

void delete_menu_item()
{
   HMENU hmenu;
   int   id;

   if (!SLang_pop_integer(&id) &&
       !pop_hmenu(&hmenu))
     {
	if ((id < 0) || (id >= MAX_MENU_ID))
	  {
	     msg_error("Id is out of range.");
	     return;
	  }

	if (!DeleteMenu(hmenu, id, MF_BYCOMMAND))
	  msg_error("Cannot delete menu");
	else
	  {
	     if (Menu_Callbacks[id])
	       {
		  SLfree(Menu_Callbacks[id]);
		  Menu_Callbacks[id] = NULL;
	       }
	  }
     }
}

void delete_menu_item_pos()
{
   HMENU hmenu, popup;
   int   pos;
   int   id;

   if (!SLang_pop_integer(&pos) &&
       !pop_hmenu(&hmenu))
     {
	id = GetMenuItemID(hmenu, pos);
	if ((id > 0) && (Menu_Callbacks[id] != NULL))
	  {
	     SLfree(Menu_Callbacks[id]);
	     Menu_Callbacks[id] = NULL;
	  }
	else
	  if (id == -1)
	  {
	     popup = GetSubMenu(hmenu, pos);
	     (void) push_hmenu (popup);
	     destroy_menu();
	  }

	if (!DeleteMenu(hmenu, pos, MF_BYPOSITION))
	  msg_error("Cannot delete menu");
     }
}

void get_menu_state()
{
   HMENU hmenu;
   int   id;
   UINT  mstate;
   int   state;

   if (!SLang_pop_integer(&id) &&
       !pop_hmenu(&hmenu))
     {
	if (-1 == (int)(mstate = GetMenuState(hmenu, id, MF_BYCOMMAND)))
	  msg_error("Cannot get menu state");
	else
	  {
	     state = 0;
	     if (mstate & MF_ENABLED) state = 1;
	     if (mstate & MF_CHECKED) state |= 2;

	     (void)SLang_push_integer(state);
	  }
     }
}

void get_menu_state_pos()
{
   HMENU hmenu;
   int   pos;
   UINT  mstate;
   int   state;

   if (!SLang_pop_integer(&pos) &&
       !pop_hmenu(&hmenu))
     {
	if (-1 == (int)(mstate = GetMenuState(hmenu, pos, MF_BYPOSITION)))
	  msg_error("Cannot get menu state");
	else
	  {
	     state = 0;
	     if (mstate & MF_GRAYED) state = 1;
	     if (mstate & MF_CHECKED) state |= 2;

	     (void)SLang_push_integer(state);
	  }
     }
}

void get_popup_menu()
{
   HMENU hmenu;
   int   pos;
   HMENU popup;

   if (!SLang_pop_integer(&pos) &&
       !pop_hmenu(&hmenu))
     {
	popup = GetSubMenu(hmenu, pos);
	(void) push_hmenu (popup);
     }
}

void check_menu_item()
{
   HMENU hmenu;
   int   id;
   int   flag;

   if (!SLang_pop_integer(&flag) &&
       !SLang_pop_integer(&id) &&
       !pop_hmenu(&hmenu))
     {
	if (-1 == (int) CheckMenuItem(hmenu, id, MF_BYCOMMAND | (flag)?MF_CHECKED:MF_UNCHECKED))
	  msg_error("Menu item does not exist");
     }
}

void check_menu_item_pos()
{
   HMENU hmenu;
   int   pos;
   int   flag;

   if (!SLang_pop_integer(&flag) &&
       !SLang_pop_integer(&pos) &&
       !pop_hmenu(&hmenu))
     {
	if (!CheckMenuItem(hmenu, pos, MF_BYPOSITION | (flag)?MF_CHECKED:MF_UNCHECKED))
	  msg_error("Menu item does not exist");
     }
}

void enable_menu_item()
{
   HMENU hmenu;
   int   id;
   int   flag;

   if (!SLang_pop_integer(&flag) &&
       !SLang_pop_integer(&id) &&
       !pop_hmenu(&hmenu))
     {
	if (-1 == EnableMenuItem(hmenu, id,
				 MF_BYCOMMAND | ((flag)?MF_ENABLED:MF_DISABLED)))
	  msg_error("Menu item does not exist");
     }
}

void enable_menu_item_pos()
{
   HMENU hmenu;
   int   pos;
   int   flag;

   if (!SLang_pop_integer(&flag) &&
       !SLang_pop_integer(&pos) &&
       !pop_hmenu(&hmenu))
     {
	if (-1 == EnableMenuItem(hmenu, pos, MF_BYPOSITION | (flag)?MF_ENABLED:MF_DISABLED))
	  msg_error("Menu item does not exist");
     }
}

void redraw_menubar()
{
   DrawMenuBar(This_Window.w);
}

void set_init_popup_callback()
{
   char *callback;

   if (SLpop_string(&callback))
     return;

   if (InitPopup_Callback != NULL)
     SLfree(InitPopup_Callback);

   InitPopup_Callback = callback;
}

void msw_help (void)
{
   char *file = NULL;
   char *keyword = NULL;
   int   partial_keyword;
   UINT  help_type;

   if (!SLang_pop_integer(&partial_keyword) &&
       !SLpop_string (&keyword) &&
       !SLpop_string(&file))
     {
	ULONG_PTR p;

	help_type = (partial_keyword) ? HELP_PARTIALKEY : HELP_KEY;

	p = (ULONG_PTR)keyword;
	if (*keyword == '\0')
	  {
	     help_type = HELP_CONTENTS;
	     p = 0;
	  }

	if (!WinHelp(This_Window.w, file, help_type, p))
	  {
	     msg_error("Help file not found.");
	  }

     }
   if (keyword != NULL) SLfree(keyword);
   if (file != NULL) SLfree(file);
}

static void msw_get_display_size (int *rows, int *cols)
{
   *cols = MSW_Screen_Cols = This_Window.width / This_Window.font_width;
   *rows = MSW_Screen_Rows = This_Window.height / This_Window.font_height;
}

static JX_SETXXX_RETURN_TYPE msw_set_mono (int i, char *what , SLtt_Char_Type unused)
{
   (void)i;
   (void)what;
   (void)unused;
   return JX_SETXXX_RETURN_VAL;
}

int jed_init_w32_support (void)
{
   return w32_init_subprocess_support (1);
}

#if HAS_WHEEL_MOUSE_SUPPORT

/* Initialise mouse wheel handling functionality */

static int init_wheel_mouse (void)
{
   /* Find the mystery mouse window */

   Wheel_Mouse.hwnd = FindWindow(MSH_WHEELMODULE_CLASS, MSH_WHEELMODULE_TITLE);

   /* Register messages to determine mousey information */

   Wheel_Mouse.uiMshMsgMouseWheel  = RegisterWindowMessage(MSH_MOUSEWHEEL);
   Wheel_Mouse.uiMshMsg3DSupport   = RegisterWindowMessage(MSH_WHEELSUPPORT);
   Wheel_Mouse.uiMshMsgScrollLines = RegisterWindowMessage(MSH_SCROLL_LINES);

   /* If we have a wheel enquiry message, ask about the presence of a wheel */

   if (Wheel_Mouse.uiMshMsg3DSupport)
     Wheel_Mouse.fActive = (BOOL)SendMessage(Wheel_Mouse.hwnd, Wheel_Mouse.uiMshMsg3DSupport, 0, 0);
   else
     Wheel_Mouse.fActive = FALSE;

   /* If we have a scroll line enquiry message ask about that */

   if (Wheel_Mouse.uiMshMsgScrollLines)
     Wheel_Mouse.iScrollLines = (int)SendMessage(Wheel_Mouse.hwnd, Wheel_Mouse.uiMshMsgScrollLines, 0, 0);
   else
     Wheel_Mouse.iScrollLines = 3;

   return 0;
}
#endif                                 /* HAS_WHEEL_MOUSE_SUPPORT */

#if defined(__BORLANDC__) || defined(__WIN32__) || defined(__VC__)
extern int main(int, char **);

int PASCAL WinMain(HINSTANCE inst, HINSTANCE pinst, LPSTR lpszCmdLine, int nCmdShow)
{
   char **argv;
   int  argc;
   int count;
   char *pt;
   int ret;
   char *command_line;

   /* add 8 for "wjed", the separating space, and a terminating '\0' */
   if (NULL == (command_line = SLMALLOC (strlen(lpszCmdLine) + 6)))
     return 0;

#ifdef _MSC_VER
   _hInstance = inst;
#else
   (void) inst;
#endif
   (void) nCmdShow;

#if HAS_WHEEL_MOUSE_SUPPORT
   (void) init_wheel_mouse ();
#endif

   strcpy(command_line, "wjed ");
   strcat(command_line, lpszCmdLine);
   _hPrev = pinst;

   /* Handle quoted filenames containing space characters.  It is a pity
    * that the application has to do this.
    */
   while ( (*command_line != '\0') && (*command_line == ' '))
     command_line++;		       /* start on 1st non-space */

   pt = command_line;
   count = 0;
   while ( *pt != '\0' )
     {
	count++;			       /* this is an argument */
#ifdef __WIN32__
      /* If the first character of the argument is a double quote,
       * search for a matching double-quote to end the argument,
       * otherwise, find the next space
       */
	if (*pt == '"')
	  {
	     pt++;		       /* Skip the initial quote */
	     while ((*pt != '\0')
		    && (*pt != '"'))
	       pt++;
	     if (*pt != '\0')	       /* Skip the end quote */
	       pt++;
	  }
	else
#endif
	  while ((*pt != '\0') && (*pt != ' '))
	    pt++;			       /* advance until a space */

	while (*pt == ' ')
	  pt++;			       /* advance until a non-space */
     }

   argv = (char **) SLMALLOC( (count+3) * sizeof(char *) );
   if (argv == NULL )
     return 0;			       /* malloc error */

   argc = 0;
   pt = command_line;
   while ((argc < count) && (*pt != '\0'))
     {
	argv[ argc ] = pt;
	argc++;
#ifdef __WIN32__
      /* If the first character of the argument is a double quote,
       * search for a matching double-quote to end the argument,
       * otherwise, find the next space
       */
	if (*pt == '"')
	  {
	     pt++;		       /* Skip the initial quote */
	     while ((*pt != '\0')
		    && (*pt != '"'))
	       pt++;
	     if (*pt != '\0')	       /* Skip the end quote */
	       pt++;
	  }
	else
#endif
	while ( *pt != '\0' && *pt != ' ' )
	  pt++;			       /* advance until a space */
	if ( *pt != '\0' )
	  *(pt++) = '\0';		       /* parse argument here */
	while ( *pt == ' ')
	  pt++;			       /* advance until a non-space */
     }
   argv [ argc ] = (char *) NULL;      /* NULL terminated list */

   ret = main(argc, argv);

   SLfree(command_line);
   return ret;
}

#endif				       /* __BORLANDC__ || __VC__ */

void (*tt_beep)(void);
void (*tt_write_string)(char *);
JX_SETXXX_RETURN_TYPE (*tt_set_color)(int, char *, char *, char *);
JX_SETXXX_RETURN_TYPE (*tt_set_mono) (int, char *, SLtt_Char_Type);
void (*tt_get_screen_size)(int *, int *);
int *tt_Ignore_Beep;
int *tt_Use_Ansi_Colors;
int *tt_Term_Cannot_Scroll;
int *tt_Term_Cannot_Insert;

static void init_tt_hooks (void)
{
   tt_beep		= msw_beep;
   tt_write_string	= msw_write_string;
   tt_get_screen_size	= msw_get_display_size;
   tt_set_color		= msw_set_color;
   tt_set_mono		= msw_set_mono;

   tt_Ignore_Beep  	= &MSW_Ignore_Beep;
   tt_Use_Ansi_Colors  	= &MSW_Use_Ansi_Colors;
   tt_Term_Cannot_Scroll= &MSW_Term_Cannot_Scroll;
   tt_Term_Cannot_Insert= &MSW_Term_Cannot_Insert;
}

static void msw_get_terminfo (void)
{
   SLsmg_Term_Type tt;

   init_tt_hooks ();

   MSW_Screen_Cols = 80;
   MSW_Screen_Rows = 24;

   (void) jed_add_init_slang_hook (msw_init_slang);

   /* Set this so that main will not try to read from stdin.  It is quite
    * likely that this is started from a menu or something.
    */
   Stdin_Is_TTY = -1;

   /* init hooks */
   X_Update_Open_Hook = msw_update_open;
   X_Update_Close_Hook = msw_update_close;
   X_Suspend_Hook = msw_suspend;
   /*   X_Argc_Argv_Hook = X_eval_command_line; */
   X_Define_Keys_Hook = msw_define_xkeys;
   SLang_Interrupt = msw_check_kbd;
   X_Init_Term_Hook = msw_init_term;

   if (Batch)
     return;

   memset ((char *) &tt, 0, sizeof (SLsmg_Term_Type));

   tt.tt_normal_video = msw_normal_video;
   tt.tt_set_scroll_region = msw_set_scroll_region;
   tt.tt_goto_rc = msw_goto_rc;
   tt.tt_reverse_index = msw_reverse_index;
   tt.tt_reset_scroll_region = msw_reset_scroll_region;
   tt.tt_delete_nlines = msw_delete_nlines;
   tt.tt_cls = msw_cls;
   tt.tt_del_eol = msw_del_eol;
   tt.tt_smart_puts = msw_smart_puts;
   tt.tt_flush_output = msw_flush_output;
   tt.tt_reset_video = msw_reset_video;
   tt.tt_init_video = msw_init_video;

   tt.tt_screen_rows = &MSW_Screen_Rows;
   tt.tt_screen_cols = &MSW_Screen_Cols;
   tt.tt_term_cannot_scroll = &MSW_Term_Cannot_Scroll;

   SLsmg_set_terminal_info (&tt);
}

void (*tt_get_terminfo)(void) = msw_get_terminfo;
int (*X_Argc_Argv_Hook)(int, char **) = NULL;
