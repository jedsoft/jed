/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#include "config.h"
#include "jed-feat.h"

#define USE_NEW_META_CODE	1
#define HAS_IBM_NUMLOCK_CODE	0
#define SKIP_LOCALE_CODE	0
#ifndef HAVE_SETLOCALE
# define X_LOCALE		1
#endif

#define XJED_USE_COMPOUND_TEXT	0

/*{{{ Include Files */

#ifndef VMS
# include <X11/Xlib.h>
# include <X11/Xutil.h>
/* #include <X11/Xos.h> */
# include <X11/Xatom.h>
# include <X11/keysym.h>
# include <X11/cursorfont.h>
# include <X11/Intrinsic.h>
# if XJED_HAS_XRENDERFONT
#  include <X11/Xft/Xft.h>
# endif
# if !SKIP_LOCALE_CODE
#  if XtSpecificationRelease >= 6
#   define XJED_USE_R6IM
#   include <X11/Xlocale.h>
#  endif
# endif
#else
# include <decw$include/Xlib.h>
# include <decw$include/Xutil.h>
/* #include <decw$include/Xos.h> */
# include <decw$include/Xatom.h>
# include <decw$include/keysym.h>
# include <decw$include/cursorfont.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include <slang.h>

#include "jdmacros.h"

/* #include "xterm.h"*/
#include "buffer.h"
#include "display.h"
#include "sysdep.h"
#include "screen.h"
#include "keymap.h"
#include "hooks.h"
#include "ins.h"
#include "ledit.h"
#include "misc.h"
#include "cmds.h"
#include "sig.h"
#include "file.h"
#include "vterm.h"
#include "colors.h"

/* Fixes some X headers... */
#ifndef Button6Mask
# define Button6Mask (Button5Mask<<1)
#endif

/*}}}*/

/* X_HAVE_UTF8_STRING is defined in X11.h */
#if (SLANG_VERSION >= 20000) && defined(X_HAVE_UTF8_STRING)
# define USE_XUTF8_CODE		1
#else
# define USE_XUTF8_CODE		0
#endif

/*{{{ Static Global Variables */

static int JX_Screen_Cols;
static int JX_Screen_Rows;
static int JX_Term_Cannot_Scroll = 0;
static int JX_Term_Cannot_Insert = 0;
static int JX_Use_Ansi_Colors = 1;
static int JX_Ignore_Beep = 3;
static int JX_Blink_Mode = 1;
static int JX_MultiClick_Time = 500;   /* milliseconds */

static int JX_MetaMask = Mod1Mask;

#ifdef XJED_USE_R6IM
static char *R6IM_Input_Method = NULL;
static XIMStyle R6IM_Input_Style = 0;
static char *R6IM_Preedit_Type = "Root";
static XIC R6IM_Xic;
static void i18init(void);
static XPoint R6IM_Spot;
static XVaNestedList R6IM_Preedit_Attr;
static void move_input_position (void);
static void set_geometry (XEvent *, XIMStyle, char *);
#endif

static char *The_Xserver_Vendor;

/* static int Current_Color; */
static int X_Alt_Char = 27;
static KeySym X_Last_Keysym;
static int X_Warp_Pending = 0;

typedef struct /*{{{*/
{
   GC gc;
   unsigned long fg, bg;
   char *fg_name;
   char *bg_name;
   int dirty;
}
/*}}}*/
GC_Info_Type;

typedef struct /*{{{*/
{
   Window w;
   Atom wm_del_win;		       /* delete window protocol */

   int height, width;
   int border;			       /* inside border */
   int o_border;		       /* outside border */
   Colormap color_map;

   /* font information */
   XFontStruct *font;
   char *font_name;
   int font_height, font_width, font_base;
   int is_dual_font;		       /* if both single and double width */
#if XJED_HAS_XRENDERFONT
   XftFont *xftfont;
   XftDraw *xftdraw;
   double face_size;
#endif    

   /* GC info */
   GC_Info_Type *text_gc;
   GC_Info_Type *current_gc;

   int vis_curs_row, vis_curs_col;     /* position of VISIBLE cursor */

   /* flags */
   int cursor_showing;		       /* true if widow has cursor showing */
   int focus;			       /* true if window has focus */
   int window_mapped;		       /* true if window is mapped */

   /* Window tty parameters */
   int insert_mode;		       /* true if inserting */
   int scroll_r1,  scroll_r2;	       /* scrolling region */
   int cursor_row, cursor_col;	       /* row column of cursor (0, 0) origin */

   int visible;			       /* from visibilitynotify */
   Cursor mouse;
}
/*}}}*/
JXWindow_Type;

static JXWindow_Type XWin_Buf;
static JXWindow_Type *XWin;

static Display *This_XDisplay;
static Window This_XWindow;
static int This_XScreen;
static int Performing_Update;
static int Check_Buffers_Pending;
static int No_XEvents;		       /* if true, do nothing */

typedef struct /*{{{*/
{
   char *name;
   char *name1;
   int type;
   char *value;
   char **dflt;
}

/*}}}*/
XWindow_Arg_Type;

#define XJED_CLASS	"XTerm"
static char *This_App_Name = "xjed";
static char *This_App_Title = "XJed";
/* #define Default_Geometry "80x24+0-0" */
#define Default_Geometry "80x24"
static char *This_Geometry = NULL;
static char *This_Font_Name = "fixed";
#if XJED_HAS_XRENDERFONT
static char *This_Face_Size = "                           ";
#endif
static char *This_Border_Width_Name = "0";
static char *This_Internal_Border_Name = "0";
static char *This_MFG = "green";
static char *This_MBG = "white";
static char *Iconic = NULL;

static Atom Xjed_Prop;
static Atom Compound_Text_Atom;
static Atom UTF8_String_Atom;
static Atom Text_Atom;
static Atom Targets_Atom;

static XEvent Current_Event;
static char *Selection_Send_Data = NULL;
static int receive_selection (XEvent *);
static int send_selection (XEvent *);

static GC_Info_Type Default_GC_Info[JMAX_COLORS] = /*{{{*/
{
     {NULL, 0, 0, "black", "white",0},     /* NORMAL */
     {NULL, 0, 0, "green", "red",0},       /* CURSOR */
     {NULL, 0, 0, "default", "skyblue",0},   /* STATUS */
     {NULL, 0, 0, "default", "magenta",0},      /* REGION */
     {NULL, 0, 0, "default", "skyblue",0},      /* MENU */
     {NULL, 0, 0, "default", "default",0},     /* operator */
     {NULL, 0, 0, "green", "default",0},     /* numbers */
     {NULL, 0, 0, "blue", "default",0},      /* strings */
     {NULL, 0, 0, "default", "gray",0},      /* comments */
     {NULL, 0, 0, "default", "default",0},      /* delimeters */
     {NULL, 0, 0, "magenta", "default",0},      /* preprocess */
     {NULL, 0, 0, "blue", "default",0},      /* message */
     {NULL, 0, 0, "red", "default",0},      /* error */
     {NULL, 0, 0, "magenta", "default",0},      /* dollar */
     {NULL, 0, 0, "red", "default",0},       /* keyword */
     {NULL, 0, 0, "green", "default",0},       /* keyword1 */
     {NULL, 0, 0, "red", "default",0},       /* keyword2 */
     {NULL, 0, 0, "green", "default",0}       /* ... fold mark */
};

/*}}}*/

/* cheat a little, use VOID_TYPE for boolean arguments */
static XWindow_Arg_Type X_Arg_List[] = /*{{{*/
{
   /* These MUST be in this order!!! */
#define XARG_DISPLAY	0
     {"Display",		"d", 	STRING_TYPE,	NULL,	NULL},
#define XARG_NAME	1
     {"Name",		NULL,	STRING_TYPE,	NULL,	&This_App_Name},
#define XARG_GEOMETRY	2
     {"Geometry",		NULL, 	STRING_TYPE,	NULL,	&This_Geometry},

#define XARG_START	2
     /* Note: it's good to look for
      * `font', `background', `foreground'
      * instead of
      * `Font', `background', `foreground'
      * so that XTerm names can be used
      * (resource vs. class names)
      *
      * also, change order of names a little?
      */
     {"font",		"fn",	STRING_TYPE,	NULL,	&This_Font_Name},
     {"fgMouse",		"mfg", 	STRING_TYPE,	NULL,	&This_MFG},
     {"bgMouse",		"mbg", 	STRING_TYPE,	NULL,	&This_MBG},
     {"background",	"bg", 	STRING_TYPE,	NULL,	&Default_GC_Info[JNORMAL_COLOR].bg_name},
     {"foreground",	"fg",	STRING_TYPE,	NULL,	&Default_GC_Info[JNORMAL_COLOR].fg_name},
     {"fgStatus",	"sfg", 	STRING_TYPE,	NULL,	&Default_GC_Info[JSTATUS_COLOR].fg_name},
     {"bgStatus",	"sbg", 	STRING_TYPE,	NULL,	&Default_GC_Info[JSTATUS_COLOR].bg_name},
     {"fgRegion",	"rfg", 	STRING_TYPE,	NULL,	&Default_GC_Info[JREGION_COLOR].fg_name},
     {"bgRegion",	"rbg", 	STRING_TYPE,	NULL,	&Default_GC_Info[JREGION_COLOR].bg_name},
     {"fgCursor",	"cfg", 	STRING_TYPE,	NULL,	&Default_GC_Info[JCURSOR_COLOR].fg_name},
     {"bgCursor",	"cbg", 	STRING_TYPE,	NULL,	&Default_GC_Info[JCURSOR_COLOR].bg_name},
     {"fgCursorOvr",	"cofg", STRING_TYPE,	NULL,	&Default_GC_Info[JCURSOROVR_COLOR].fg_name},
     {"bgCursorOvr",	"cobg", STRING_TYPE,	NULL,	&Default_GC_Info[JCURSOROVR_COLOR].bg_name},
     {"fgMenu",		"fgm", 	STRING_TYPE,	NULL,	&Default_GC_Info[JMENU_COLOR].fg_name},
     {"bgMenu",		"bgm", 	STRING_TYPE,	NULL,	&Default_GC_Info[JMENU_COLOR].bg_name},
     {"fgOperator",	"fgop",	STRING_TYPE,	NULL,	&Default_GC_Info[JOP_COLOR].fg_name},
     {"bgOperator",	"bgop",	STRING_TYPE,	NULL,	&Default_GC_Info[JOP_COLOR].bg_name},
     {"fgNumber",		"fgnm",	STRING_TYPE,	NULL,	&Default_GC_Info[JNUM_COLOR].fg_name},
     {"bgNumber",		"bgnm",	STRING_TYPE,	NULL,	&Default_GC_Info[JNUM_COLOR].bg_name},
     {"fgString",		"fgst",	STRING_TYPE,	NULL,	&Default_GC_Info[JSTR_COLOR].fg_name},
     {"bgString",		"bgst",	STRING_TYPE,	NULL,	&Default_GC_Info[JSTR_COLOR].bg_name},
     {"fgComments",	"fgco",	STRING_TYPE,	NULL,	&Default_GC_Info[JCOM_COLOR].fg_name},
     {"bgComments",	"bgco",	STRING_TYPE,	NULL,	&Default_GC_Info[JCOM_COLOR].bg_name},
     {"fgKeyword",	"fgkw",	STRING_TYPE,	NULL,	&Default_GC_Info[JKEY_COLOR].fg_name},
     {"bgKeyword",	"bgkw",	STRING_TYPE,	NULL,	&Default_GC_Info[JKEY_COLOR].bg_name},
     {"fgKeyword1",	"fgkw1",STRING_TYPE,	NULL,	&Default_GC_Info[JKEY_COLOR + 1].fg_name},
     {"bgKeyword1",	"bgkw1",STRING_TYPE,	NULL,	&Default_GC_Info[JKEY_COLOR + 1].bg_name},
     {"fgKeyword2",	"fgkw2",STRING_TYPE,	NULL,	&Default_GC_Info[JKEY_COLOR + 2].fg_name},
     {"bgKeyword2",	"bgkw2",STRING_TYPE,	NULL,	&Default_GC_Info[JKEY_COLOR + 2].bg_name},
     {"fgDelimiter",	"fgde",	STRING_TYPE,	NULL,	&Default_GC_Info[JDELIM_COLOR].fg_name},
     {"bgDelimiter",	"bgde",	STRING_TYPE,	NULL,	&Default_GC_Info[JDELIM_COLOR].bg_name},
     {"fgPreprocess",	"fgpr",	STRING_TYPE,	NULL,	&Default_GC_Info[JPREPROC_COLOR].fg_name},
     {"bgPreprocess",	"bgpr",	STRING_TYPE,	NULL,	&Default_GC_Info[JPREPROC_COLOR].bg_name},
     {"bgMessage",	"bgms",	STRING_TYPE,	NULL,	&Default_GC_Info[JMESSAGE_COLOR].bg_name},
     {"fgMessage",	"fgms",	STRING_TYPE,	NULL,	&Default_GC_Info[JMESSAGE_COLOR].fg_name},
     {"bgError",		"bger",	STRING_TYPE,	NULL,	&Default_GC_Info[JERROR_COLOR].bg_name},
     {"fgError",		"fger",	STRING_TYPE,	NULL,	&Default_GC_Info[JERROR_COLOR].fg_name},
     {"fgDots",		"fgdt",	STRING_TYPE,	NULL,	&Default_GC_Info[JDOTS_COLOR].bg_name},
     {"bgDots",		"bgdt",	STRING_TYPE,	NULL,	&Default_GC_Info[JDOTS_COLOR].bg_name},

     {"BorderWidth",	"bw", 	STRING_TYPE,	NULL,	&This_Border_Width_Name},
     {"title",		NULL,	STRING_TYPE,	NULL,	&This_App_Title},
     {"BorderColor",	"bd", 	STRING_TYPE,	NULL,	NULL},
     {"Iconic",		"ic",	VOID_TYPE,	NULL,	&Iconic},
     {"xrm",		NULL,	STRING_TYPE,	NULL,	NULL},
     {"internalBorder", "ib",	STRING_TYPE,	NULL,	&This_Internal_Border_Name},
#ifdef XJED_USE_R6IM
     {"inputMethod",      "im",   STRING_TYPE,    NULL,   &R6IM_Input_Method},
     {"inputStyle",       "is",   STRING_TYPE,    NULL,   &R6IM_Preedit_Type},
#endif
#if XJED_HAS_XRENDERFONT
     {"facesize",	"fs",	STRING_TYPE,	NULL,	&This_Face_Size},
#endif	
     {NULL,		NULL,	0,		NULL,	NULL}
};

/*}}}*/

/*}}}*/

#if XJED_HAS_XRENDERFONT
static XftColor *Pixel2XftColor (Pixel pixel)
{
# define CACHE_SIZE  40
   static struct 
     {
	XftColor        color;
	unsigned long use;
     }
   cache[CACHE_SIZE];
   static unsigned long use;
   unsigned int i;
   unsigned long oldest, oldestuse;
   XColor color;
    
   oldestuse = (unsigned long) -1L;
   oldest = 0;
   for (i = 0; i < CACHE_SIZE; i++)
     {
	if (cache[i].use)
	  {	  
	     if (cache[i].color.pixel == pixel)
	       {
		  cache[i].use = ++use;
		  return &cache[i].color;
	       }
	  }
	
	if (cache[i].use < oldestuse)
	  {
	     oldestuse = cache[i].use;
	     oldest = i;
	  }
    }

   i = oldest;
   color.pixel = pixel;
   XQueryColor (This_XDisplay, XWin->color_map, &color);
   cache[i].color.color.red = color.red;
   cache[i].color.color.green = color.green;
   cache[i].color.color.blue = color.blue;
   cache[i].color.color.alpha = 0xffff;
   cache[i].color.pixel = pixel;
   cache[i].use = ++use;
   return &cache[i].color;
}
#endif

#if SLANG_VERSION >= 20000
/* conversion tools */
/* wchars to XChar2b for X drawing */
static XChar2b *wchars_to_XChar2b(SLwchar_Type *w, unsigned int nchars)
{
   unsigned int i;
   XChar2b *x = (XChar2b *)SLmalloc(nchars*sizeof(XChar2b));

   if (x == NULL)
     return NULL;

   for (i = 0; i < nchars; i++) 
     {
	SLwchar_Type w_i = w[i];
	if (w_i > 0xFFFF)
	  w_i = '?';
	x[i].byte2 = (unsigned char)(w_i & 0xFF);
	x[i].byte1 = (unsigned char)((w_i >> 8 )& 0xFF);
     }

   return x;
}

static SLwchar_Type *bytes_to_wchars (unsigned char *s, unsigned int nchars)
{
   unsigned int i;
   
   SLwchar_Type *w = (SLwchar_Type *)SLmalloc(nchars*sizeof(SLwchar_Type));   
   if (w == NULL)
     return NULL;

   for (i = 0; i < nchars; i++) 
     w[i] = s[i];

   return w;
}

static SLwchar_Type *utf8nt_to_wchars(unsigned char *s, unsigned int len, unsigned int *ncharsp)
{
   SLwchar_Type *w;
   unsigned int i, nchars;
   unsigned char *smax;

   nchars = SLutf8_strlen(s, 0);
   if (NULL == (w = (SLwchar_Type *)SLmalloc(nchars*sizeof(SLwchar_Type))))
     {
	*ncharsp = 0;
	return NULL;
     }

   smax = s + len;
   for (i = 0; i < nchars; i++)
     {
	unsigned int n;
	if (SLutf8_decode(s, smax, &w[i], &n) == NULL)
	  w[i] = '?';
	s += n;
     }

   *ncharsp = nchars;
   return w;
}

# define SLSMGCHAR_EQUAL(o, n) \
   (((o)->nchars == (n)->nchars) \
       && ((o)->color == (n)->color) \
       && (0 == memcmp((o)->wchars, (n)->wchars, (n)->nchars * sizeof(SLwchar_Type))))

# define SLSMGCHAR_SET_CHAR(sc, _char) \
   do { (sc).nchars = 1; (sc).wchars[0] = (_char); } while (0)
# define SLSMGCHAR_SET_COLOR(sc, _color) \
   (sc).color = (_color)
# define SLSMG_COUNT_CHARS(sc) ((sc).nchars)

#else
/* SLang1 versions */
# define SLSMGCHAR_EQUAL(o, n) ((o) == (n))

# ifdef SLSMG_HLINE_CHAR_UNICODE 
/* Grrr.... hack for the slang1-utf8 version hacked by RedHat and SuSE... */
#  define SLSMGCHAR_SET_CHAR(sc, _char) (sc) = ((sc) & 0xFF000000) + (_char)
#  define SLSMGCHAR_SET_COLOR(sc, _color) (sc) = ((sc) & 0xFF) + ((_color)<<24)
# else
#  define SLSMGCHAR_SET_CHAR(sc, _char) (sc) = ((sc) & 0xFF00) + (_char)
#  define SLSMGCHAR_SET_COLOR(sc, _color) (sc) = ((sc) & 0xFF) + ((_color)<<8)
# endif
# define SLSMG_COUNT_CHARS(sc) (1)
/* These exist in SLang 2 include, but not on SLang 1. Define here to allow
 * using the same code 
 */
# define SLSMG_COLOR_MASK 0xFF
# define SLwchar_Type char

#endif				       /* SLANG_VERSION >= 20000 */

/* This function does the low-level drawing.
 * It can draw the new text, but setting 'overimpose' to 1 it draws the string 
 * over the existing text (used for unicode combining characters).
 * It can use Xft (if configured), or plain X functions.
 */

static int xdraw (GC_Info_Type *gc, int row, int col, SLwchar_Type *w, int nchars, int overimpose)
{
   int b = XWin->border;
   int x = col * XWin->font_width;
   int y = row * XWin->font_height;

#if XJED_HAS_XRENDERFONT
   if ((XWin->face_size > 0) && (XWin->xftdraw != NULL))
     {
	/* if (!XWin->xftdraw) */
	/*   XWin->xftdraw = XftDrawCreate(This_XDisplay, This_XWindow, DefaultVisual(This_XDisplay, This_XScreen), XWin->color_map); */

	if (overimpose == 0)
	  XftDrawRect (XWin->xftdraw, Pixel2XftColor(gc->bg), x + b, y,
		       nchars * XWin->font_width, XWin->font_height);

# if SLANG_VERSION >= 20000
	XftDrawString32 (XWin->xftdraw, Pixel2XftColor(gc->fg), XWin->xftfont,
			x + b, y + b + XWin->font_base, w, nchars);
# else
	XftDrawString8 (XWin->xftdraw, Pixel2XftColor(gc->fg), XWin->xftfont,
			x + b, y + b + XWin->font_base, (unsigned char *)w, nchars);
# endif
	return 0;
     }
#endif				       /* XJED_HAS_XRENDERFONT */

#if SLANG_VERSION >= 20000
     {
	XChar2b *d = wchars_to_XChar2b (w, nchars);

	if (d == NULL)
	  return -1;

	if (overimpose)
	  XDrawString16(This_XDisplay, This_XWindow, gc->gc,
			x + b, y + b + XWin->font_base, d, nchars);
	else
	  XDrawImageString16(This_XDisplay, This_XWindow, gc->gc, 
			     x + b, y + b + XWin->font_base, d, nchars);
	SLfree((char *)d);
	return 0;
     }
#else
   /* Standard X, SLang 1 */
     {
	if (overimpose)
	  XDrawString(This_XDisplay, This_XWindow, gc->gc,
		      x + b, y + b + XWin->font_base, w, nchars);
	else	     
	  XDrawImageString(This_XDisplay, This_XWindow, gc->gc,
			   x + b, y + b + XWin->font_base, w, nchars);
	return 0;
     }
#endif
}

/* Get 'n' characters from SLSMG screen, at position ('row', 'col'). */
static unsigned int smg_read_at(int row, int col, SLsmg_Char_Type *s, unsigned int n)
{
   int saverow, savecol;
   unsigned int rc;

   saverow = SLsmg_get_row ();
   savecol = SLsmg_get_column ();
   SLsmg_gotorc(row, col);
   rc = SLsmg_read_raw (s, n);
   SLsmg_gotorc (saverow, savecol);
   return rc; 
}

/* Write to screen a single SLsmg_Char, handling combining characters 
 * (X doesn't seem to handle these...) to position (row, col).
 * This function doesn't touch the cursor position.
 */
static void JX_write_smgchar (int row, int col, SLsmg_Char_Type *s)
{
   int color = SLSMG_EXTRACT_COLOR(*s) & SLSMG_COLOR_MASK;
   
   if ((color >= JMAX_COLORS) || (color < 0))
     color = 0;

#if SLANG_VERSION >= 20000
   if (s->nchars > 0)
     (void) xdraw (XWin->text_gc + color, row, col, s->wchars, 1, 0);

   if (Jed_UTF8_Mode)
     {
	unsigned int i;
	for (i = 1; i < s->nchars; i++)
	  (void) xdraw(XWin->text_gc+color, row, col, &s->wchars[i], 1, 1);
     }
#else
     {
	SLwchar_Type ch = SLSMG_EXTRACT_CHAR(*s);
	(void) xdraw(XWin->text_gc+color, row, col, &ch, 1, 0);
     }
#endif
}

/* Write to screen a row of SLsmg_Chars, handling combining characters
 * (X doesn't seem to handle these...) to position (row, col).
 * This function doesn't touch the cursor position.
 */

static void JX_write_smgchars(int row, int col, SLsmg_Char_Type *s, SLsmg_Char_Type *smax)
{
   SLwchar_Type *b, *bend, buf[512];
   int oldcolor, color;
   SLsmg_Char_Type *s0;
   int is_dual_font;

   b = buf;
   bend = buf + 510;

   oldcolor = (SLSMG_EXTRACT_COLOR(*s) & SLSMG_COLOR_MASK);
   if ((oldcolor < 0) || (oldcolor >= JMAX_COLORS))
     oldcolor = 0;
   
   is_dual_font = XWin->is_dual_font;
   s0 = s;
   while (s < smax)
     {
        color = (SLSMG_EXTRACT_COLOR(*s) & SLSMG_COLOR_MASK);
	if ((color < 0) || (color >= JMAX_COLORS))
	  color = 0;

	if (oldcolor != color		   /* Color changed. */
	    || (b >= bend)		   /* Space finished */
	    || SLSMG_COUNT_CHARS(*s) > 1)  /* a combining character */
          {
	     (void) xdraw(XWin->text_gc+oldcolor, row, col, buf, b-buf, 0);
	     col += (int)(s-s0);
	     s0 = s;
	     b = buf;
             oldcolor = color;
          }
#if SLANG_VERSION >= 20000
	if (s->nchars > 1)
	  {
	     /* this cell has combining characters */
	     JX_write_smgchar(row, col, s);
	     col++;
	     s0 = s + 1;
	  }
	else if (s->nchars == 0)
	  {
	     /* SLsmg thinks this is a double width character, but the font has no such characters */
	     if (is_dual_font == 0)
	       *b++ = ' ';
	  }
	else
#endif
	  *b++ = SLSMG_EXTRACT_CHAR(*s);
	s++;
     }
   if (b != buf)
     (void) xdraw(XWin->text_gc+color, row, col, buf, b-buf, 0);
}

static void hide_cursor (void) /*{{{*/
{
   SLsmg_Char_Type sc;

   if (No_XEvents
       || (XWin->cursor_showing == 0))
     return;

   XWin->cursor_showing = 0;
   if (0 == smg_read_at (XWin->vis_curs_row, XWin->vis_curs_col, &sc, 1))
     {
	SLSMGCHAR_SET_CHAR(sc, ' ');
	SLSMGCHAR_SET_COLOR(sc, JNORMAL_COLOR);
     }

   JX_write_smgchar(XWin->vis_curs_row, XWin->vis_curs_col, &sc);
}
/*}}}*/

static void copy_rect(int x1, int y1, int x2, int y2, int x3, int y3) /*{{{*/
{
   int w, h;

   if (No_XEvents || (XWin->window_mapped == 0))
     return;

   w = (x2 - x1) * XWin->font_width;
   h = (y2 - y1) * XWin->font_height;

   if ((w <= 0) || (h <= 0)) return;

   x3 = XWin->border + x3 * XWin->font_width;
   x1 = XWin->border + x1 * XWin->font_width;
   y3 = XWin->border + y3 * XWin->font_height;
   y1 = XWin->border + y1 * XWin->font_height;
   hide_cursor ();
   XCopyArea (This_XDisplay, This_XWindow, This_XWindow, XWin->current_gc->gc,
	      x1, y1, w, h, x3, y3);
}

/*}}}*/

static void blank_rect (int x1,  int y1, int x2, int y2) /*{{{*/
{
   int w, h;

   if (No_XEvents || (XWin->window_mapped == 0)) return;

   w = (x2 - x1) * XWin->font_width;
   h = (y2 - y1) * XWin->font_height;

   if ((w <= 0) || (h <= 0)) return;

   x1 = XWin->border + x1 * XWin->font_width;
   y1 = XWin->border + y1 * XWin->font_height;
   hide_cursor ();
   XClearArea (This_XDisplay, This_XWindow, x1, y1, w, h, 0);
}

/*}}}*/

static void JX_set_scroll_region(int r1, int r2) /*{{{*/
{
   XWin->scroll_r1 = r1;
   XWin->scroll_r2 = r2;
   /* vterm_set_scroll_region (r1, r2); */
}

/*}}}*/

static void JX_reset_scroll_region (void) /*{{{*/
{
   JX_set_scroll_region (0, JX_Screen_Rows - 1);
}

/*}}}*/

static void show_cursor (void) /*{{{*/
{
   SLsmg_Char_Type sc;
   int row, col, b;
   int color;
   GC_Info_Type *gc_info;
   GC gc;
   XGCValues gcv;

   if (No_XEvents)
     return;

   if (XWin->cursor_showing) hide_cursor ();

   XWin->cursor_showing = 1;
   row = XWin->vis_curs_row = XWin->cursor_row;
   col = XWin->vis_curs_col = XWin->cursor_col;
   b = XWin->border;

   if ((CBuf != NULL) && (CBuf->flags & OVERWRITE_MODE))
     color = JCURSOROVR_COLOR;
   else
     color = JCURSOR_COLOR;

   gc_info = &XWin->text_gc[color];
   gc = gc_info->gc;

   if (XWin->focus)
     {
	/* restore the modified GC */
	if (gc_info->dirty)
	  {
	     gc_info->dirty = 0;
	     gcv.foreground = gc_info->fg;
	     gcv.background = gc_info->bg;
	     XChangeGC(This_XDisplay, gc, GCForeground | GCBackground, &gcv);
	  }
	if (smg_read_at(row, col, &sc, 1) == 0)
	  SLSMGCHAR_SET_CHAR(sc, ' ');
	SLSMGCHAR_SET_COLOR(sc, color);
	JX_write_smgchar(row, col, &sc);
     }
   else
     {
	gc_info->dirty = 1;
	gcv.foreground = gc_info->bg;
	gcv.background = gc_info->fg;
	XChangeGC(This_XDisplay, gc, GCForeground | GCBackground, &gcv);

	XDrawRectangle(This_XDisplay, This_XWindow,
		       gc,
		       col * XWin->font_width + b,
		       row * XWin->font_height + b,
		       XWin->font_width - 1,
		       XWin->font_height - 1);
     }

   XFlush(This_XDisplay);
}

/*}}}*/

static void toggle_cursor (int on) /*{{{*/
{
   if (on)
     {
	if (XWin->focus) return;
	XWin->focus = 1;
     }
   else
     {
	if (XWin->focus == 0) return;
	XWin->focus = 0;
     }
   show_cursor ();
}

/*}}}*/


/* This routine assumes that cursor is in the correct location.  The
 * cursor is placed at the end of the string.  Even if we are unable to
 * write the string, make sure that the cursor is moved as if we did
 * the write. The main reason for this is that our X cursor must track
 * the vterm cursor so that the display gets updated properly.
 * Otherwise, smart_puts will call forward_cursor and then write to the
 * virtual display, and get that wrong because forward_cursor assumes
 * that the XWin cursor is correct.
 */

static void JX_write_string (char *s) /*{{{*/
{
   unsigned int nchars;
   SLwchar_Type *w;
   unsigned int nbytes = strlen(s);
#if SLANG_VERSION >= 20000   
   
   if (Jed_UTF8_Mode)	  
     w = utf8nt_to_wchars((unsigned char *)s, nbytes, &nchars);
   else
     {	
	w = bytes_to_wchars((unsigned char *)s, nbytes);
	nchars = nbytes;
     }
   if (w == NULL)
     goto write_done;

#else
   nchars = nbytes;
   w = s;
#endif
   
   if ((No_XEvents == 0) && XWin->window_mapped)
     {
	hide_cursor ();
	(void) xdraw(XWin->current_gc, XWin->cursor_row, XWin->cursor_col, w, nchars, 0);
     }
#if SLANG_VERSION >= 20000   
   SLfree((char *)w);
   write_done:
#endif

   XWin->cursor_col += nchars;
   if (XWin->cursor_col >= JX_Screen_Cols) 
     XWin->cursor_col = JX_Screen_Cols - 1;
   if (!Performing_Update)
     show_cursor ();
}

static void JX_goto_rc(int r, int c) /*{{{*/
{
   if (XWin == NULL) return;
   if (XWin->cursor_showing) hide_cursor ();
   if (r >= JX_Screen_Rows) r = JX_Screen_Rows - 1;
   if (c >= JX_Screen_Cols) c = JX_Screen_Cols - 1;
   XWin->cursor_row = r + XWin->scroll_r1;
   XWin->cursor_col = c;
   /* vterm_goto_rc (r, c); */
   if (Performing_Update) return;
   show_cursor ();
}

/*}}}*/

/* Must respect scrolling region */
static void JX_delete_nlines(int n) /*{{{*/
{
   int r1, r2;

   /* vterm_delete_nlines (n); */

   if (No_XEvents || (XWin->window_mapped == 0))
     return;

   r1 = XWin->cursor_row;
   r2 = XWin->scroll_r2;

   if (r1 <= r2 - n) copy_rect(0, r1 + n, JX_Screen_Cols, r2 + 1,
			       0, r1);

   r2++;
   blank_rect(0, r2 - n, JX_Screen_Cols, r2);
}

/*}}}*/

static void JX_reverse_index(int n) /*{{{*/
{
   int r1, r2;

   /* vterm_reverse_index (n); */

   if (No_XEvents || (XWin->window_mapped == 0))
     return;

   r1 = XWin->scroll_r1;
   r2 = XWin->scroll_r2;

   if (r2 >= r1 + n) copy_rect(0, r1, JX_Screen_Cols, r2 - n + 1,
			       0, r1 + n);

   blank_rect(0, r1, JX_Screen_Cols, r1 + n);
}

/*}}}*/

static void JX_beep(void) /*{{{*/
{
   GC gc;
   XGCValues gcv;

   if (No_XEvents) return;
   flush_input();
   if (JX_Ignore_Beep & 0x1) XBell (This_XDisplay, 50);

   /* visible bell */

   if (JX_Ignore_Beep & 0x2)
     {
	gc = XCreateGC(This_XDisplay, This_XWindow, 0, &gcv);

        XSetState(This_XDisplay, gc,
		  WhitePixel (This_XDisplay, This_XScreen),
                  BlackPixel(This_XDisplay, This_XScreen),
		  GXinvert, AllPlanes);

        XFillRectangle (This_XDisplay, This_XWindow, gc,
                        0, 0,
			XWin->font_width * JX_Screen_Cols,
			XWin->font_height * JX_Screen_Rows);

        XFlush (This_XDisplay);

	/* I attempted to put a pause in here but it was too slow. */

        XFillRectangle (This_XDisplay, This_XWindow, gc,
                        0, 0,
			XWin->font_width * JX_Screen_Cols,
			XWin->font_height * JX_Screen_Rows);

	XFreeGC(This_XDisplay, gc);
     }
   XFlush (This_XDisplay);
}

/*}}}*/

static void JX_del_eol(void) /*{{{*/
{
   /* vterm_del_eol (); */

   if (No_XEvents || (XWin->window_mapped == 0))
     return;

   blank_rect(XWin->cursor_col, XWin->cursor_row, JX_Screen_Cols, XWin->cursor_row + 1);
}

/*}}}*/

static void JX_reverse_video(int color) /*{{{*/
{
   if ((color < 0) || (color >= JMAX_COLORS))
     return;

   if (XWin == NULL) return;
   /* Current_Color = color; */
   XWin->current_gc = XWin->text_gc + color;
   /* vterm_reverse_video (color); */
}

/*}}}*/

static void JX_normal_video(void) /*{{{*/
{
   JX_reverse_video (JNORMAL_COLOR);
}

/*}}}*/

static void JX_smart_puts(SLsmg_Char_Type *neww, SLsmg_Char_Type *oldd, int len, int row)
{
   int col;

   /* Skip equal chars at the beginning */
   col = 0;
   while ((col < len) && SLSMGCHAR_EQUAL(&neww[col], &oldd[col]))
     col++;
   
   if (col < len) 
     {
	hide_cursor();
	JX_write_smgchars(row, col, neww+col, neww+len);
	JX_goto_rc (row, len);
     }
}

/*}}}*/

static void cover_exposed_area (int x, int y, int width, int height, int count) /*{{{*/
{
   SLsmg_Char_Type *s;
   int row, max_col, max_row, col;
   int width_chars, len;

   Performing_Update++;
   /* VTerm_Suspend_Update++; */
   hide_cursor ();
   col = (x - XWin->border) / XWin->font_width;
   row = (y - XWin->border) / XWin->font_height;

   width_chars = 2 + width / XWin->font_width;
   max_col = col + width_chars;
   max_row = 2 + row + height / XWin->font_height;
   if (max_col > JX_Screen_Cols) max_col = JX_Screen_Cols;
   if (max_row > JX_Screen_Rows) max_row = JX_Screen_Rows;

   if (NULL != (s = (SLsmg_Char_Type *)SLmalloc(width_chars*sizeof(SLsmg_Char_Type))))
     {
	while (row < max_row)
	  {
	     len = smg_read_at(row, col, s, width_chars);
	     JX_write_smgchars(row, col, s, s + len);
	     row++;
	  }
	SLfree ((char *)s);
     }
   Performing_Update--;
   if (count == 0) show_cursor ();
}

/*}}}*/

#include "xkeys.c"

/* Return 1 if event is listed in the switch or zero otherwise.  The switch
 * events are considered harmless--- that is, processing them does not really
 * interfere with internal JED state (redisplay, etc...).  More bluntly,
 * harmless means that the events can be processesed while checking for
 * pending input.
 */
static int Debug_Xjed = 0;
static int x_handle_harmless_events (XEvent *report) /*{{{*/
{
   switch (report->type)
     {
      case EnterNotify:
	toggle_cursor(report->xcrossing.focus);
	break;

      case LeaveNotify:
	/* toggle_cursor(0); */
	break;

      case UnmapNotify:
	XWin->window_mapped = 0;
	break;
      case MapNotify:
	XWin->window_mapped = 1;
	break;

      case FocusIn:
	toggle_cursor(1);
#ifdef XJED_USE_R6IM
	if (NULL != R6IM_Xic)
	  XSetICFocus (R6IM_Xic);
#endif
	Check_Buffers_Pending = 1;
	break;

      case FocusOut:
	toggle_cursor(0);
#ifdef XJED_USE_R6IM
	if (NULL != R6IM_Xic)
	  XUnsetICFocus (R6IM_Xic);
#endif
	break;

      case VisibilityNotify: XWin->visible = report->xvisibility.state;
	break;

      case GraphicsExpose:
	cover_exposed_area (report->xgraphicsexpose.x,
			    report->xgraphicsexpose.y,
			    report->xgraphicsexpose.width,
			    report->xgraphicsexpose.height,
			    report->xgraphicsexpose.count);
	break;

      case ConfigureNotify:
	if ((report->xconfigure.height == XWin->height) 
	    && (report->xconfigure.width == XWin->width))
	  break;

	XWin->width = report->xconfigure.width;
	XWin->height = report->xconfigure.height;
	jed_init_display ();
	jed_redraw_screen (1);
	break;

      case Expose:
	cover_exposed_area (report->xexpose.x,
			    report->xexpose.y,
			    report->xexpose.width,
			    report->xexpose.height,
			    report->xexpose.count);
	break;

      case ReparentNotify:
      case NoExpose:
	break;

      case KeyPress:
	/* Just look for Modifier key presses */
#ifdef XJED_USE_R6IM
	if (R6IM_Xic && (R6IM_Input_Style & XIMPreeditPosition))
	  move_input_position ();
#endif
	return IsModifierKey (XLookupKeysym (&report->xkey, 0));

      case SelectionNotify:
	(void) receive_selection (report);
	if (Performing_Update == 0)
	  update_cmd (&Number_One);
	break;

      default:
	if (Debug_Xjed)
	  fprintf(stderr, "harmless: %d\n", report->type);
	return 0;
     }
   return 1;
}

/*}}}*/

static void fill_jmouse (JMouse_Type *jmouse, /*{{{*/
			 unsigned char type, int x, int y, unsigned long t,
			 unsigned int button, unsigned int state)
{
   unsigned char s;
#if JED_HAS_MULTICLICK
   static unsigned long last_press_time;
   static unsigned int clicks;
   static unsigned int last_button;

   if (type == JMOUSE_DOWN)
     {
	if ((last_button == button)
	    && (last_press_time + JX_MultiClick_Time > t))
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
	last_press_time = t;
     }
   else if ((clicks > 1) && (last_button == button))
     {
	/* Last was a multi-click.  Ignore this event. */
	type = JMOUSE_IGNORE_EVENT;
     }
#endif
   jmouse->type = type;
   jmouse->x = 1 + (x - XWin->border) / XWin->font_width;

   if (y < XWin->border) jmouse->y = 0;
   else jmouse->y = 1 + (y - XWin->border) / XWin->font_height;

   if (button == Button1) jmouse->button = JMOUSE_BUTTON_1;
   else if (button == Button2) jmouse->button = JMOUSE_BUTTON_2;
   else if (button == Button3) jmouse->button = JMOUSE_BUTTON_3;
   else if (button == Button4) jmouse->button = JMOUSE_BUTTON_4;
   else if (button == Button5) jmouse->button = JMOUSE_BUTTON_5;
   else jmouse->button = JMOUSE_BUTTON_6;

   s = 0;
   if (state & Button1Mask) s |= JMOUSE_BUTTON_1;
   if (state & Button2Mask) s |= JMOUSE_BUTTON_2;
   if (state & Button3Mask) s |= JMOUSE_BUTTON_3;
   if (state & Button4Mask) s |= JMOUSE_BUTTON_4;
   if (state & Button5Mask) s |= JMOUSE_BUTTON_5;
   if (state & Button6Mask) s |= JMOUSE_BUTTON_5;
   if (state & ShiftMask) s |= JMOUSE_SHIFT;
   if (state & ControlMask) s |= JMOUSE_CTRL;

   jmouse->state = s;
}

/*}}}*/

#define MOUSE_DRAG_THRESHOLD 3

/* if force is true, wait for an event.  If force is false, only
 *  process events that exist.  This will return either when there
 *  are no more events or a key/mouse event is processed returning
 *  1 in the process */
static int X_process_events (int force, char *buf, unsigned int buflen,
			     int *n_chars) /*{{{*/
{
   XEvent report;
   JMouse_Type jmouse;
   int ch1;
   int block_expose = 0;
   char *bufp;
   KeySym ks = 0;
   int esc = 27;

   Window root, child;
   int posx, posy, rootx, rooty;
   unsigned int keys_buttons;
   int last_x, last_y;
   static int last_event, last_motion_down_button;
   static unsigned int motion_state;
#if MOUSE_DRAG_THRESHOLD
   static int button_press_x, button_press_y;
#endif
   int width, height;

   while (force || XPending(This_XDisplay))
     {
	XNextEvent(This_XDisplay, &report);
	Current_Event = report;

	switch (report.type)
	  {
	   case ClientMessage:
	     if ((report.xclient.format == 32) &&
		 ((Atom) report.xclient.data.l[0] == XWin->wm_del_win))
	       jed_exit_jed (1);
	     break;

	   case MotionNotify:

	     /* Make sure we get the last event of this type */
	     while (XCheckMaskEvent (This_XDisplay, ButtonMotionMask, &report))
	       ;

	     if (!XQueryPointer(This_XDisplay, report.xmotion.window,
				&root, &child, &rootx, &rooty, &posx, &posy,
				&keys_buttons)) break;

	     /* This will ensure that modifier keys are not pressed while
	        we are in motion. */

	     if ((last_event == MotionNotify)
		 && (motion_state != keys_buttons))
	       break;

	     motion_state = keys_buttons;

	     memset ((char *) &jmouse, 0, sizeof (jmouse));
	     last_x = jmouse.x;
	     last_y = jmouse.y;

	     fill_jmouse (&jmouse, JMOUSE_DRAG,
			  posx, posy, report.xmotion.time,
			  last_motion_down_button, keys_buttons);

#if MOUSE_DRAG_THRESHOLD
	     if ((abs(button_press_x - posx) <= MOUSE_DRAG_THRESHOLD)
		 && (abs(button_press_y - posy) <= MOUSE_DRAG_THRESHOLD))
	       break;
#endif	     
	     if ((last_x == jmouse.x) && (last_y == jmouse.y)) break;
	     if (-1 == (ch1 = jed_mouse_add_event (&jmouse)))
	       break;		       /* queue full */

	     /* return ESC ^@ */
	     *buf++ = esc; *buf++ = 0; *buf++ = ch1;
	     *n_chars = 3;

	     last_event = MotionNotify;
	     return 1;

	   case Expose:
	     if (block_expose == 0) cover_exposed_area (report.xexpose.x,
							report.xexpose.y,
							report.xexpose.width,
							report.xexpose.height,
							report.xexpose.count);
	     else
	       {
		  if (report.xexpose.count == 0)
		    {
		       jed_redraw_screen (1);
		       block_expose = 0;
		    }
	       }
	     break;

	   case ConfigureNotify:
	     width = report.xconfigure.width;
	     height = report.xconfigure.height;
	     if ((width != XWin->width) ||
		 (height != XWin->height))
	       {
		  XWin->width = width;
		  XWin->height = height;
		  jed_init_display ();
		  jed_redraw_screen (0);
		  block_expose = -1;
	       }
#ifdef XJED_USE_R6IM
	     if (R6IM_Input_Style & XIMPreeditArea)
	       {
		  set_geometry (&report, XIMPreeditArea, XNPreeditAttributes);
		  set_geometry (&report, XIMStatusArea, XNStatusAttributes);
	       }
#endif
	     break;

	   case ButtonPress:
	     /* Prohibit dragging more than one button at a time. */
	     if (last_event == MotionNotify) break;
	     /* drop */

	   case ButtonRelease:
	     if ((last_event == MotionNotify) &&
		 (report.xbutton.button != (unsigned int) last_motion_down_button))
	       break;

#if MOUSE_DRAG_THRESHOLD
	     if (report.type == ButtonPress)
	       {
		  button_press_x = report.xbutton.x;
		  button_press_y = report.xbutton.y;
	       }
	     else
	       {
		  button_press_x = 0;
		  button_press_y = 0;
	       }
#endif
	     last_event = 0;

	     fill_jmouse (&jmouse,
			  ((report.type == ButtonRelease) ? JMOUSE_UP : JMOUSE_DOWN),
			  report.xbutton.x, report.xbutton.y, report.xbutton.time,
			  report.xbutton.button, report.xbutton.state);

	     if (-1 == (ch1 = jed_mouse_add_event (&jmouse)))
	       break;		       /* queue full */

	     if ((report.type == ButtonPress)
		 && (0 == (report.xbutton.state
			   & (Button1Mask|Button2Mask|Button3Mask
			      |Button4Mask|Button5Mask|Button6Mask))))
	       last_motion_down_button = report.xbutton.button;

	     /* ESC ^@ is a  mouse prefix */
	     *buf++ = esc; *buf++ = 0; *buf++ = ch1;
	     *n_chars = 3;

	     return 1;

#if HAS_IBM_NUMLOCK_CODE
	   case KeyRelease:
	     if ((report.xkey.keycode != 98)
# if 0
		 || (0 != strcmp (The_Xserver_Vendor,
				  "International Business Machines"))
# endif
		 )
	       {
		  (void) x_handle_harmless_events (&report);
		  break;
	       }
	     /* Drop */
#endif
	   case KeyPress:
	     bufp = buf;
#ifndef XJED_USE_R6IM
	     *n_chars = XLookupString(&report.xkey, buf, buflen, &ks, NULL);
#else
	     if (!XFilterEvent (&report, report.xkey.window))
	       {
		  Status status_return;
		  if (R6IM_Xic != NULL)
		    {
#if USE_XUTF8_CODE
		       if (Jed_UTF8_Mode)
			 *n_chars = Xutf8LookupString (R6IM_Xic, &report.xkey, buf, buflen,
						       &ks, &status_return);
		       else
#endif
			 
			 *n_chars = XmbLookupString (R6IM_Xic, &report.xkey, buf, buflen,
						     &ks, &status_return);
		    }
		  else
		    *n_chars = XLookupString(&report.xkey, buf, buflen, &ks, NULL);

	       }
	     else
	       *n_chars = 0;
#endif
	     ks = ks & 0xFFFF;
	     X_Last_Keysym = ks;
#if USE_NEW_META_CODE
	     if ((*n_chars == 0) && (ks > 0x00FF) && (ks < 0x0FFF))
	       {
		  *n_chars = 1;
		  buf[0] = ks & 0x00FF ;
	       }
#endif
	     
	     bufp = (char *)map_keysym_to_keyseq (ks, report.xkey.state & (ShiftMask|ControlMask));
	     if (bufp != NULL)
	       {
		  *n_chars = (unsigned char) *bufp++;
#if USE_NEW_META_CODE
		  if (report.xkey.state & JX_MetaMask)
		    {
		       buf[0] = X_Alt_Char;
		       SLMEMCPY (buf + 1, bufp, *n_chars);
		       *n_chars += 1;
		    }
		  else
#endif
		    SLMEMCPY(buf, bufp, *n_chars);
	       }
	     else if (*n_chars == 1)
	       {
		  if (bufp == NULL)
		    bufp = buf;

		  if (report.xkey.state & JX_MetaMask)
		    {
		       ch1 = *bufp;
#if 0
		       /* Only do this on alphabetic characters.  This
			* is because, e.g., german keyboards use 'Alt-{'
			* to generate the '{' character
			*/
		       if (isalnum (ch1) && (ch1 < 0x80))
			 {
#endif
			    if (X_Alt_Char <= 0) *buf |= 0x80;
			    else
			      {
				 *bufp++ = (unsigned char) X_Alt_Char;
				 *bufp = (unsigned char) ch1;
				 *n_chars = 2;
			      }
#if 0
			 }
#endif
		    }
		  else if (report.xkey.state & ControlMask)
		    {
		       if (*buf == ' ') *buf = 0;
		       else if (*buf == '-') *buf = 0x1F;
		    }
	       }

	     if (*n_chars == 0) break;
	     return 1;

	   case SelectionNotify:
	     (void) receive_selection (&report);
	     break;
	   case SelectionClear:
	     SLfree (Selection_Send_Data);   /* NULL ok */
	     Selection_Send_Data = NULL;
	     break;
	   case SelectionRequest:
	     (void) send_selection (&report);
	     break;

	   default:
	     (void) x_handle_harmless_events (&report);
	  }
     }
   return 0;
}

/*}}}*/

static int X_read_key (void) /*{{{*/
{
   int nread;
   char buf[64];
   (void) X_process_events (1, buf, sizeof (buf), &nread);
   if (nread > 1) ungetkey_string(buf + 1, nread - 1);
   return (int) *buf;
}

/*}}}*/

static int X_input_pending (void) /*{{{*/
{
   XEvent ev;
   int n;

   if (No_XEvents) return 0;

   n = XPending (This_XDisplay);
   if (!n) return (0);

   /* I need some way of getting only kbd events. */
   while (n--)
     {
	XPeekEvent(This_XDisplay, &ev);
	if (0 == x_handle_harmless_events (&ev)) return 1;
	XNextEvent(This_XDisplay, &ev);
     }
   return 0;
}

/*}}}*/

static void JX_get_display_size (int *rows, int *cols) /*{{{*/
{
   JX_Screen_Cols = (XWin->width - XWin->border) / XWin->font_width;
   JX_Screen_Rows = (XWin->height - XWin->border) / XWin->font_height;
   *cols = JX_Screen_Cols;
   *rows = JX_Screen_Rows;
}

/*}}}*/

static void JX_set_term_vtxxx (int *n) /*{{{*/
{
   (void) n;
}

/*}}}*/

static void JX_narrow_width (void) /*{{{*/
{
}

/*}}}*/

static void  JX_wide_width (void) /*{{{*/
{
}

/*}}}*/

static void JX_enable_cursor_keys(void) /*{{{*/
{
}

/*}}}*/

static void JX_cls(void) /*{{{*/
{
   /* vterm_cls (); */

   if (No_XEvents) return;
   if (XWin->window_mapped == 0) return;
   XClearWindow(This_XDisplay, This_XWindow);
}

/*}}}*/

/* This routine is called from S-Lang inner interpreter.  It serves
   as a poor mans version of an interrupt 9 handler */
static void xjed_check_kbd(void) /*{{{*/
{
   char buf[64];
   int n;
   register char *b, *bmax;

   if (Batch || No_XEvents) return;
   while (XPending(This_XDisplay))
     {
	if (X_process_events (0, buf, sizeof (buf), &n) == 0) continue;

	b = buf; bmax = b + n;
	while (b < bmax)
	  {
	     if (*b == (char) Jed_Abort_Char)
	       {
		  if (Ignore_User_Abort == 0)
		    SLang_set_error (USER_BREAK);
		  if (b != buf) buffer_keystring (buf, (int) (b - buf));
		  SLKeyBoard_Quit = 1;
		  break;
	       }
	     b++;
	  }
	if (!SLKeyBoard_Quit) buffer_keystring (buf, n);
     }
}

/*}}}*/

static void xjed_suspend (void) /*{{{*/
{
   if (No_XEvents) return;
   if (XWin->focus)
     {
	/* XIconifyWindow (This_XDisplay, XWin->w, This_XScreen); */
	if (XWin->visible == VisibilityUnobscured) XLowerWindow (This_XDisplay, This_XWindow);
	else XRaiseWindow (This_XDisplay, This_XWindow);
     }
   else
     {
	/* The window doesn't have focus which means that this was most
	 * likely called by pressing Ctrl-Z from another window.
	 */
	fprintf (stderr, "jed stopping\n");
#ifdef SIGSTOP
	kill (0, SIGSTOP);
#endif
	/* sys_suspend (); */
     }
}

/*}}}*/

static void x_toggle_visibility (void) /*{{{*/
{
   int hide_win = (XWin->visible == VisibilityUnobscured);

   if ((SLang_Num_Function_Args == 1)
       && (-1 == SLang_pop_integer (&hide_win)))
     return;

   if (hide_win)
     /* XIconifyWindow (This_XDisplay, XWin->w, This_XScreen); */
     XLowerWindow (This_XDisplay, This_XWindow);
   else
     XRaiseWindow (This_XDisplay, This_XWindow);
}
/*}}}*/


static int get_font_width (XFontStruct *f, int *wp, int *is_dualp)
{
   int w0, w1;

   *is_dualp = 0;
   if (f->min_bounds.width == f->max_bounds.width)
     {
	*wp = f->max_bounds.width;
	return 0;
     }

   /* Simple heristic */
   w0 = XTextWidth (f, "M", 1);
   w1 = XTextWidth (f, "l", 1);
   if (w0 != w1)
     (void) fprintf (stderr, "This font does not appear to be single-width.  Expect rendering problems.\n");
   
#if SLANG_VERSION >= 20000
   if (Jed_UTF8_Mode
       && (f->min_bounds.width * 2 == f->max_bounds.width))
     {
	*wp = f->min_bounds.width;
	*is_dualp = 1;
	return 0;
     }
#endif
   
   *wp = f->max_bounds.width;
   return 0;
}

static int load_font (char *font) /*{{{*/
{
   static XFontStruct *xfont;

#if XJED_HAS_XRENDERFONT
   if (XWin->face_size > 0)
     {
	/* the user wants xrender */
	XWin->xftfont = XftFontOpen(This_XDisplay, This_XScreen, 
				    XFT_FAMILY, XftTypeString, font, 
				    XFT_SIZE, XftTypeDouble, XWin->face_size,
				    XFT_SPACING, XftTypeInteger, XFT_MONO,
				    NULL);
	if (XWin->xftfont == NULL) return -1;
	XWin->font_name = font;
	XWin->font_height = XWin->xftfont->ascent + XWin->xftfont->descent;
	XWin->font_width = XWin->xftfont->max_advance_width;
	XWin->font_base = XWin->xftfont->ascent;
	return 0;
     }
#endif       
   xfont = XLoadQueryFont(This_XDisplay, font);
   if (xfont == NULL) return -1;
   XWin->font = xfont;
   XWin->font_name = font;
   XWin->font_height = xfont->ascent + xfont->descent;
   XWin->font_base = xfont->ascent;
   (void) get_font_width (xfont, &XWin->font_width, &XWin->is_dual_font);
   if (XWin->font_width <= 0)
     {
	fprintf (stderr, "Font width for %s is <= 0\n", font);
	return -1;
     }
   return 0;
}

/*}}}*/

static void get_xdefaults (void) /*{{{*/
{
   XWindow_Arg_Type *xargs = X_Arg_List + XARG_START;  /* skip display, name, etc */

   while (xargs->name != NULL)
     {
	if (xargs->dflt == NULL)
	  {
	     xargs++;
	     continue;
	  }

	if ((xargs->type != VOID_TYPE)
	    && (xargs->value == NULL))
	  {
	     static char *class_names[] =
	       {
		  "UXjed", "UXJed", "uxjed", NULL
	       };	     
	     char *p, *cn;
	     char **cnp;

	     /* Note that strings returned by XGetDefault are owned by
	      * Xlib and should not be modified or freed by the client.
	      * However, the solaris folks apparantly are not aware of this
	      * and the following two function calls produce memory leaks.
	      * Sigh.
	      */
	     p = NULL;
#if SLANG_VERSION >= 20000
	     if (Jed_UTF8_Mode)
	       {
		  cnp = class_names;
		  while (NULL != (cn = *cnp++))
		    {
		       p = XGetDefault (This_XDisplay, cn, xargs->name);
		       if (p != NULL)
			 break;
		    }
	       }
#endif
	     if (p == NULL)
	       {
		  cnp = class_names;
		  while (NULL != (cn = *cnp++))
		    {
		       cn++;	       /* Skip leading U */
		       p = XGetDefault (This_XDisplay, cn, xargs->name);
		       if (p != NULL)
			 break;
		    }
	       }
	     if (p == NULL)
	       {
#if SLANG_VERSION >= 20000
		  if (Jed_UTF8_Mode)
		    p = XGetDefault (This_XDisplay, "UXTerm", xargs->name);
#endif
		  if (p == NULL)
		    p = XGetDefault (This_XDisplay, "XTerm", xargs->name);
	       }

	     if (p != NULL)
	       xargs->value = p;
	  }

	if (xargs->value != NULL)
	  *xargs->dflt = xargs->value;

	xargs++;
     }
}

/*}}}*/

static void set_window_name (char *s) /*{{{*/
{
   if (Batch) return;
   XStoreName (This_XDisplay, XWin->w, s);
}

/*}}}*/

static void set_icon_name (char *s) /*{{{*/
{
   if (Batch) return;
   XSetIconName(This_XDisplay, XWin->w, s);
}

/*}}}*/

#if 0
static void set_wm_hints (JXWindow_Type *w, int xpos,  int ypos, unsigned long orflags) /*{{{*/
{
   XSizeHints h;
   XWMHints h1;
   XClassHint ch;

   ch.res_name = "xjed";
   ch.res_class = "XJed";

   h.width_inc = w->font_width;
   h.height_inc = w->font_height;
   h.min_width = 5 * w->font_width + w->border;
   h.min_height = 5 * w->font_height + w->border;
   h.base_height = 0;
   h.base_width = 0;
   h.x = xpos; h.y = ypos;
   h.height = w->height;
   h.width = w->width;

   h.flags = PMinSize | PResizeInc | PBaseSize;
   h.flags |= orflags;

   XSetWMNormalHints(This_XDisplay, w->w, &h);

   /* This bit allows me to track the focus.  It is not at all clear from
      the documentation. */
   h1.input = 1;
   h1.flags = InputHint;
   XSetWMHints(This_XDisplay, w->w, &h1);
# if 0
   XSetClassHint(This_XDisplay, w->w, &ch);
# endif
}

/*}}}*/
#endif

static int alloc_color(char* color_name, XColor* color_info)
{
   XColor exact_info;

   if (XAllocNamedColor(This_XDisplay, XWin->color_map, color_name, color_info,
			&exact_info))
     return color_info->pixel;

   if (0 == strncmp (color_name, "bright", 6))
     color_name += 6;

   if (XAllocNamedColor(This_XDisplay, XWin->color_map, color_name, color_info,
			&exact_info))
     return color_info->pixel;

   fprintf(stderr, "Can't allocate color %s\n", color_name);
   return -1;
}

/* This parses the colors in the XWin structure and setting
   defaults to fg, bg upon failure of either one */
static void setup_ith_color (int i) /*{{{*/
{
   XColor xcol;
   int fg, bg;

   if (!Term_Supports_Color)
     return;

   fg = alloc_color(XWin->text_gc[i].fg_name, &xcol);
   bg = alloc_color(XWin->text_gc[i].bg_name, &xcol);

   if ((fg < 0) || (bg < 0))
     return;

   XWin->text_gc[i].fg = fg;
   XWin->text_gc[i].bg = bg;
}

/*}}}*/

/* This is used to set the colors in the Win structure and if f is non-zero,
 * the previous definitions are freed.  f is 0 when the colors correspond to the
 * default. */

static void x_set_color_free (int i, char *fgcolor, char *bgcolor, int do_free) /*{{{*/
{
   char *save_fg, *save_bg, *fg, *bg;

   if ((*fgcolor == 0) || !strcmp (fgcolor, "default"))
     fgcolor = XWin->text_gc[0].fg_name;

   if ((*bgcolor == 0) || !strcmp (bgcolor, "default"))
     bgcolor = XWin->text_gc[0].bg_name;

   if (NULL == (fg = SLmalloc(strlen(fgcolor) + 1)))
     return;

   strcpy (fg, fgcolor);
   if (NULL == (bg = SLmalloc (strlen(bgcolor) + 1)))
     {
	SLfree (fg);
	return;
     }
   strcpy (bg, bgcolor);

   save_fg = XWin->text_gc[i].fg_name;
   XWin->text_gc[i].fg_name = fg;
   save_bg = XWin->text_gc[i].bg_name;
   XWin->text_gc[i].bg_name = bg;

   setup_ith_color (i);
   if (do_free)
     {
	if (save_fg != NULL) SLfree (save_fg);
	if (save_bg != NULL) SLfree (save_bg);
     }
}

/*}}}*/

static int setup_and_parse_colors (void) /*{{{*/
{
   unsigned long fg, bg, tmp;
   char *fg_name, *bg_name;

   int i;
   GC_Info_Type *d;

   /* Check to see if this is a color display */

   bg = WhitePixel (This_XDisplay, This_XScreen);
   fg = BlackPixel (This_XDisplay, This_XScreen);

   fg_name = Default_GC_Info[0].fg_name;
   bg_name = Default_GC_Info[0].bg_name;

   XWin->color_map = DefaultColormap (This_XDisplay, This_XScreen);
#if 0
   if (XWin->color_map == NULL)
     {
	fprintf (stderr, "Unable to get a colormap\n");
	exit (1);
     }
#endif
   if (DisplayCells (This_XDisplay, This_XScreen) > 2)
     {
	Term_Supports_Color = 1;
     }
   else Term_Supports_Color = 0;

   for (i = 0; i < JMAX_COLORS; i++)
     {
	d = Default_GC_Info + i;
	/* The assumption here is that ALL colors beyond JNORMAL_COLOR (0)
	 * take reversed fg, bgs.  I really ought to have flags if this is
	 * not the case. */
	d->fg = fg;
	d->bg = bg;
	if (d->fg_name == NULL) d->fg_name = fg_name;
	if (d->bg_name == NULL) d->bg_name = bg_name;

	if (i == JNORMAL_COLOR)
	  {
	     char *tmp_name;
	     tmp = fg; fg = bg; bg = tmp;
	     tmp_name = fg_name; fg_name = bg_name; bg_name = tmp_name;
	  }

	x_set_color_free (i, d->fg_name, d->bg_name, 0);
     }
   
   return 0;
}

/*}}}*/

static void set_mouse_color (char *fgc, char *bgc) /*{{{*/
{
   XColor xfg, xbg;

   if (0 == Term_Supports_Color)
     return;

   if (alloc_color(fgc, &xfg) < 0)
     return;
   if (alloc_color(bgc, &xbg) < 0)
     return;

   XRecolorCursor (This_XDisplay, XWin->mouse, &xfg, &xbg);
}

/*}}}*/

static void create_needed_gcs (void) /*{{{*/
{
   int i;
   XGCValues xgcv;

#if XJED_HAS_XRENDERFONT
   if (XWin->face_size == 0)
#endif
     xgcv.font = XWin->font->fid;

   for (i = 0; i < JMAX_COLORS; i++)
     {
	xgcv.foreground = XWin->text_gc[i].fg;
	xgcv.background = XWin->text_gc[i].bg;

#if XJED_HAS_XRENDERFONT
	if (XWin->face_size > 0)
	  {
	     XWin->text_gc[i].gc = XCreateGC(This_XDisplay, This_XWindow,
					     GCForeground | GCBackground /*| GCFont*/, /* XFT */
					     &xgcv);
	  }
	else
#endif
	  XWin->text_gc[i].gc = XCreateGC(This_XDisplay, This_XWindow,
					  GCForeground | GCBackground | GCFont,
					  &xgcv);
     }
}

/*}}}*/

static Window create_XWindow (JXWindow_Type *win) /*{{{*/
{
   int bdr, x, y, flags;
   unsigned int width, height;
   XSizeHints sizehint;
   XClassHint xcls;
   XWMHints wmhint;
   long key_event_type;

   bdr = atoi(This_Border_Width_Name);

   if (This_Geometry == NULL)
     This_Geometry = Default_Geometry;

   sizehint.flags = 0;
   flags = XParseGeometry (This_Geometry, &x, &y, &width, &height);
   if (flags & WidthValue)
     {
	sizehint.width = width;
	sizehint.flags |= USSize;
     }
   else
     {
	width = JX_Screen_Cols;
     }
   if (flags & HeightValue)
     {
	sizehint.height = height;
	sizehint.flags |= USSize;
     }
   else
     {
	height = JX_Screen_Rows;
     }

   win->height  	= height * win->font_height + 2 * win->border;
   win->width		= width  * win->font_width  + 2 * win->border;

   sizehint.height	= win->height;
   sizehint.width	= win->width;
   sizehint.width_inc	= win->font_width;
   sizehint.height_inc	= win->font_height;
   sizehint.min_width	= 5 * win->font_width  + win->border;
   sizehint.min_height	= 5 * win->font_height + win->border;
   sizehint.base_height	= 0;
   sizehint.base_width	= 0;

   if (flags & XValue)
     {
	if (flags & XNegative)
	  {
	     x += (DisplayWidth (This_XDisplay, This_XScreen)
		   - sizehint.width - 2 * win->border);
	     sizehint.win_gravity = NorthEastGravity;
	  }
	sizehint.x = x;
	sizehint.flags |= USPosition;
     }
   else	x = 0;

   if (flags & YValue)
     {
	if (flags & YNegative)
	  {
	     y += (DisplayHeight (This_XDisplay, This_XScreen)
		   - sizehint.height - 2 * win->border);
	     if ((flags&XValue) && (flags&XNegative))
	       sizehint.win_gravity = SouthEastGravity;
	     else
	       sizehint.win_gravity = SouthWestGravity;
	  }
	sizehint.y = y;
	sizehint.flags |= USPosition;
     }
   else	y = 0;

   sizehint.flags |= (PMinSize | PResizeInc | PBaseSize);

   /* create and display window */
   win->w = XCreateSimpleWindow(This_XDisplay,
				RootWindow(This_XDisplay, This_XScreen),
				x, y,     /* xpos, ypos */
				win->width,     /* width, height */
				win->height,     /* width, height */
				bdr,	       /* border width */
				win->text_gc[JNORMAL_COLOR].fg,
				win->text_gc[JNORMAL_COLOR].bg
				);

   xcls.res_name = This_App_Name;
   xcls.res_class = XJED_CLASS;

   wmhint.input = True;		/* track the focus */
   if (Iconic != NULL)
     wmhint.initial_state = IconicState;
   else
     wmhint.initial_state = NormalState;

   wmhint.flags = InputHint | StateHint;

   wmhint.window_group = win->w;
   wmhint.flags |= WindowGroupHint;

   XSetWMProperties (This_XDisplay, win->w, NULL, NULL, NULL, 0,
                     &sizehint, &wmhint, &xcls);

   /* Enable the delete window protocol */
   win->wm_del_win = XInternAtom (This_XDisplay, "WM_DELETE_WINDOW", False);
   XSetWMProtocols (This_XDisplay, win->w, &win->wm_del_win, 1);

#if XJED_SET_WM_COMMAND
   XSetCommand(This_XDisplay, win->w, _Jed_Startup_Argv, _Jed_Startup_Argc);
#endif
   
   if (NULL == (The_Xserver_Vendor = XServerVendor (This_XDisplay)))
     The_Xserver_Vendor = "";

   key_event_type = KeyPressMask;

#if HAS_IBM_NUMLOCK_CODE
   if ((0 == strcmp (The_Xserver_Vendor, "International Business Machines"))
       || 0 == strcmp (The_Xserver_Vendor, "Hewlett-Packard Company"))
     key_event_type |= KeyReleaseMask;
#endif

   /* select event types */
   XSelectInput(This_XDisplay, win->w,
		(ExposureMask | key_event_type
		 | ButtonPressMask | ButtonReleaseMask
		 | StructureNotifyMask
		 | PointerMotionHintMask | ButtonMotionMask
		 | EnterWindowMask
		 /* | LeaveWindowMask */
		 | FocusChangeMask
		 | VisibilityChangeMask)
		);

   if (XWin->mouse) XDefineCursor(This_XDisplay, win->w, XWin->mouse);
   return win->w;
}

/*}}}*/

static int x_err_handler (Display *d, XErrorEvent *ev) /*{{{*/
{
   char errmsg[256];
   No_XEvents = 1;
   XGetErrorText (d, ev->error_code, errmsg, 255);
   exit_error (errmsg, 0);
   return 1;
}

/*}}}*/

static int x_ioerr_handler (Display *d) /*{{{*/
{
   No_XEvents = 1;
   exit_error("XWindows IO error", 0);
   return d == NULL;  /* just use d to avoid a warning */
}

/*}}}*/

static int open_Xdisplay (void) /*{{{*/
{
   char dname [256];
   char *n;

   n = X_Arg_List[XARG_DISPLAY].value;
   if (n != NULL)
     {
	strncpy (dname, X_Arg_List[XARG_DISPLAY].value, sizeof (dname)-10);
	dname[sizeof(dname)-10] = 0;
	n = dname;
	while (*n && (*n != ':')) n++;
	if (*n == 0) strcpy(n, ":0.0");
	n = dname;
     }

   XSetIOErrorHandler (x_ioerr_handler);
   
   if ( (This_XDisplay = XOpenDisplay(n)) == NULL )
     return 0;

   XSetErrorHandler (x_err_handler);
   return 1;
}


/* returns socket descriptor */
static int init_Xdisplay (void) /*{{{*/
{
#ifdef XJED_USE_R6IM
   setlocale(LC_ALL, "");
#endif

   if (X_Arg_List[XARG_NAME].value != NULL)
     {
	This_App_Name = X_Arg_List[XARG_NAME].value;
     }

   XWin = &XWin_Buf;
   memset ((char *)XWin, 0, sizeof (JXWindow_Type));

   get_xdefaults ();
   XWin->border = atoi(This_Internal_Border_Name);
   if (XWin->border < 0)
     XWin->border = 0;

   XWin->font_name = This_Font_Name;
#if XJED_HAS_XRENDERFONT
   /* if a parameter to -fs was supplied, we assume the user wants XFT */
   if (strlen(This_Face_Size))
     {
	if ((XWin->face_size = atof(This_Face_Size))<=0)
	  /* we couldn't convert the value, or a negative value was specified */
	  XWin->face_size = 0;
     }
   else
     /* there was no -fs, so we don't do anything */
     XWin->face_size = 0;

    /* make sure that XWin->xftdraw is null in any case */
   XWin->xftdraw = NULL;
#endif

   This_XScreen = DefaultScreen(This_XDisplay);

   if (-1 == load_font(XWin->font_name))
     {
	(void) fprintf( stderr, "xjed: cannot load font %s, using fixed.\n", XWin->font_name);
	if (-1 == load_font("fixed"))
	  {
	     (void) fprintf( stderr, "xjed: cannot load fixed font.\n");
	     exit (1);
	  }
     }

   XWin->text_gc = Default_GC_Info;
   if (-1 == setup_and_parse_colors ())		       /* This allocs and parses colors */
     exit (1);

   XWin->mouse = XCreateFontCursor (This_XDisplay, XC_xterm);
   set_mouse_color (This_MFG, This_MBG);

   This_XWindow = create_XWindow(XWin);

   set_window_name (This_App_Title);
   set_icon_name (This_App_Name);

   /* GCs and their colors */
   create_needed_gcs ();		       /* This uses info from previous call */

   XWin->current_gc = XWin->text_gc + JNORMAL_COLOR;

#if XJED_HAS_XRENDERFONT
   /* we only XSetFont() if we're NOT using renderfont */
   if (XWin->face_size != 0)
     {
	XWin->xftdraw = XftDrawCreate(This_XDisplay, This_XWindow, DefaultVisual(This_XDisplay, This_XScreen), XWin->color_map);
	if (NULL == XWin->xftdraw)
	  {
	     fprintf (stderr, "xjed: XftDrawCreate failed\n");
	     exit (1);
	  }
     }
   else
#endif
     XSetFont (This_XDisplay, XWin->current_gc->gc, XWin->font->fid);

   /* display window */

   XMapWindow(This_XDisplay, This_XWindow);

#ifdef XJED_USE_R6IM
   i18init ();
#endif
   Compound_Text_Atom = XInternAtom(This_XDisplay, "COMPOUND_TEXT", False);
   Text_Atom = XInternAtom(This_XDisplay, "TEXT", False);
   UTF8_String_Atom = XInternAtom (This_XDisplay, "UTF8_STRING", False);
   Targets_Atom = XInternAtom (This_XDisplay, "TARGETS", False);
   Xjed_Prop = XInternAtom(This_XDisplay, "XJED_PROPERTY_TEXT", False);
   return ConnectionNumber (This_XDisplay);
}

/*}}}*/

static void reset_Xdisplay (void) /*{{{*/
{
   if (This_XDisplay != NULL) XCloseDisplay(This_XDisplay);
}

/*}}}*/

#define UPCSE(x)  (((x) <= 'z') && ((x) >= 'a') ? (x) - 32 : (x))
static int myXstrcmp(char *a, char *b) /*{{{*/
{
   register char cha, chb;
   /* do simple comparison */

   cha = *a++;  chb = *b++;
   if ((cha != chb) && (UPCSE(cha) != UPCSE(chb))) return 0;
   while ((cha = *a++), (chb = *b++), (cha && chb) != 0)
     {
	if (cha != chb) return 0;
     }

   return (cha == chb);
}

/*}}}*/

#define STREQS(a, b) myXstrcmp(a, b)
static int X_eval_command_line (int argc, char **argv) /*{{{*/
{
   char *arg;
   int i;
   XWindow_Arg_Type *opt;

   for (i = 1; i < argc; i++)
     {
	arg = argv[i];
	if (*arg != '-') break;
	if (0 == strcmp ("--debug-xjed", arg))
	  {
	     Debug_Xjed = 1;
	     continue;
	  }

	arg++;
	opt = X_Arg_List;
	while (opt->name != NULL)
	  {
	     if (STREQS(opt->name, arg)
		 || ((opt->name1 != NULL) && STREQS(opt->name1, arg))) break;
	     opt++;
	  }

	if (opt->name == NULL) break;

	if (opt->type == VOID_TYPE) opt->value = "on";
	else if (i + 1 < argc)
	  {
	     i++;
	     opt->value = argv[i];
	  }
	else break;
     }

   /* Out of this loop, argv[i] is the last unprocessed argument */
   return i;
}

/*}}}*/

#ifdef XJED_USE_R6IM
static void move_input_position (void)
{
   XPoint spot;
   /* XVaNestedList list; */

   spot.x = XWin->border + XWin->cursor_col * XWin->font_width;
   spot.y = XWin->cursor_row * XWin->font_height + XWin->border + XWin->font_base;
   
   if (R6IM_Preedit_Attr != NULL)
     XFree (R6IM_Preedit_Attr);
   if (NULL != (R6IM_Preedit_Attr = XVaCreateNestedList (0, XNSpotLocation, &spot, NULL)))
     {
	if (R6IM_Xic != NULL)
	  XSetICValues (R6IM_Xic, XNPreeditAttributes, R6IM_Preedit_Attr, NULL);
     }
}

static void set_geometry (XEvent *report, XIMStyle style, char *attr)
{
   XVaNestedList list;
   static XRectangle ra, *rb;

   if (style == XIMPreeditArea)
     ra.width = report->xconfigure.width - (XWin->font_width * 8);
   else if (style == XIMStatusArea)
     ra.width = XWin->font_width * 8;
   /*   ra.width = report->xconfigure.width; */
   ra.height = XWin->font_height;
   list = XVaCreateNestedList (0, XNAreaNeeded, &ra, NULL);
   XSetICValues (R6IM_Xic, attr, list, NULL);
   XFree (list);

   rb = &ra;
   list = XVaCreateNestedList (0, XNAreaNeeded, rb, NULL);
   XGetICValues (R6IM_Xic, attr, list, NULL);
   XFree (list);

   rb->x = 0;
   if (style == XIMPreeditArea)
     {
	rb->x = XWin->font_width * 8;
	rb->y = report->xconfigure.height - rb->height;
     }
   else if (style == XIMStatusArea)
     {
	rb->x = 0;
	rb->y = report->xconfigure.height - rb->height;

     }

   list = XVaCreateNestedList (0, XNArea, rb, NULL);
   XSetICValues (R6IM_Xic, attr, list, NULL);
   XFree (list);
   /*   XFree (rb); */
}

/*
 * This is more or less stolen startight from XFree86 xterm. This should
 * support all European type languages.
 */
static void i18init (void) /*{{{*/
{
   int i;
   char *p, *s, *ns, *end, tmp[1024], buf[32];
   XIM xim = NULL;
   XIMStyles *xim_styles = NULL;
   int found;

   setlocale(LC_ALL, "");

   if (R6IM_Input_Method != NULL)
     {
	strcpy(tmp, R6IM_Input_Method);
	s=tmp;
	while (*s)
	  {
	     while (*s && (isspace(*s) || (*s == ','))) s++;
	     if (*s == 0) break;
	     end = s;
	     while (*end && (*end != ',')) end++;
	     ns = end;
	     if (*end) ns++;
	     *end-- = 0;
	     while ((end >= s) && isspace(*end)) *end-- = 0;

	     if (*s)
	       {
		  strcpy(buf, "@im=");
		  strcat(buf, s);
		  if (((p = XSetLocaleModifiers(buf)) != NULL)
		      && *p
		      && (NULL != (xim = XOpenIM(This_XDisplay, NULL, NULL, NULL))))
		    break;
	       }
	     s = ns;
	  }
     }

   if ((xim == NULL) && ((p = XSetLocaleModifiers("")) != NULL) && *p)
     xim = XOpenIM(This_XDisplay, NULL, NULL, NULL);

   if ((xim == NULL) && ((p = XSetLocaleModifiers("@im=none")) != NULL) && *p)
     xim = XOpenIM(This_XDisplay, NULL, NULL, NULL);

   if (xim == NULL)
     {
	fprintf(stderr, "Failed to open input method");
	return;
     }

   /* Believe it or not, XGetIMValues return NULL upon success */
   if ((NULL != XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL))
       || (xim_styles == NULL))
     {
        fprintf(stderr, "Input method doesn't support any style\n");
        XCloseIM(xim);
        return;
     }

   found = 0;
   strcpy(tmp, R6IM_Preedit_Type);

   s = tmp;
   while (*s && !found)
     {
	while (*s && (isspace(*s) || (*s == ','))) s++;
	if (*s == 0) break;
	end = s;
	while (*end && (*end != ',')) end++;
	ns = end;
	if (*ns) ns++;
	*end-- = 0;
	while ((end >= s) && isspace(*end)) *end-- = 0;

        if (!strcmp(s, "OverTheSpot"))
	  R6IM_Input_Style = (XIMPreeditPosition | XIMStatusArea);
	else if (!strcmp(s, "OffTheSpot"))
	  R6IM_Input_Style = (XIMPreeditArea | XIMStatusArea);
	else if (!strcmp(s, "Root"))
	  R6IM_Input_Style = (XIMPreeditNothing | XIMStatusNothing);

	/* FIXME!!!  (I think)
	 * Examples on the web show testing of bits via & instead of
	 * the == operator.
	 */
        for (i = 0; (unsigned short)i < xim_styles->count_styles; i++)
	  {
	     if (R6IM_Input_Style == xim_styles->supported_styles[i])
	       {
		  found = 1;
		  break;
	       }
	  }
        s = ns;
     }
   XFree(xim_styles);

   if (found == 0)
     {
        /* fprintf(stderr, "input method doesn't support my preedit type\n"); */
        XCloseIM(xim);
        return;
     }

    /*
     * This program only understands the Root preedit_style yet
     * Then misc.preedit_type should default to:
     *          "OverTheSpot,OffTheSpot,Root"
     *  /MaF
     */
#if 0
   if (R6IM_Input_Style != (XIMPreeditNothing | XIMStatusNothing))
     {
        fprintf(stderr,"This program only supports the 'Root' preedit type\n");
        XCloseIM(xim);
        return;
     }
#else
   if (R6IM_Input_Style == (XIMPreeditNothing | XIMStatusNothing))/* "Root" */
     R6IM_Xic = XCreateIC(xim, XNInputStyle, R6IM_Input_Style,
			  XNClientWindow, This_XWindow,
			  XNFocusWindow, This_XWindow,
			  NULL);
   else if (R6IM_Input_Style == (XIMPreeditPosition | XIMStatusArea))/* "OverTheSpot" */
     {
	XFontSet fs;
	char **miss, *def;
	int n_miss;
	char *fontlist;
	
	R6IM_Spot.x = 0;
	R6IM_Spot.y = 0;
	/*	R6IM_Spot.x = XWin->border + XWin->cursor_col * XWin->font_width;
	 R6IM_Spot.y = XWin->cursor_row * XWin->font_height + XWin->border + XWin->font_base; */
	
	if (NULL != (fontlist = SLmake_string (This_Font_Name)))
	  {
	     fs = XCreateFontSet (This_XDisplay, fontlist, &miss, &n_miss, &def);
	     SLfree(fontlist);
	     
	     R6IM_Preedit_Attr = XVaCreateNestedList (0, XNFontSet, fs,
						      XNSpotLocation, &R6IM_Spot,
						      NULL);
	     R6IM_Xic = XCreateIC(xim, XNInputStyle, R6IM_Input_Style,
				  XNClientWindow, This_XWindow,
				  XNPreeditAttributes, R6IM_Preedit_Attr,
				  XNStatusAttributes, R6IM_Preedit_Attr,
				  NULL);
	  }
     }
   else if (R6IM_Input_Style == (XIMPreeditArea | XIMStatusArea))/* "OffTheSpot" */
     {
	XFontSet fs;
	char **miss, *def;
	int n_miss;
	char *fontlist;

	if (NULL != (fontlist = SLmake_string (This_Font_Name)))
	  {
	     fs = XCreateFontSet (This_XDisplay, fontlist, &miss, &n_miss, &def);
	     SLfree(fontlist);
	     R6IM_Preedit_Attr = XVaCreateNestedList (0, XNFontSet, fs,
						      XNSpotLocation, &R6IM_Spot,
						      NULL);
	     R6IM_Xic = XCreateIC(xim, XNInputStyle, R6IM_Input_Style,
				  XNClientWindow, This_XWindow,
				  XNPreeditAttributes, R6IM_Preedit_Attr,
				  XNStatusAttributes, R6IM_Preedit_Attr,
				  NULL);
	  }
     }
#endif

   if (NULL == R6IM_Xic)
     {
	fprintf(stderr,"Failed to create input context\n");
	XCloseIM(xim);
     }
}

/*}}}*/
#endif

static void set_border_color (char *fgc, char *bgc) /*{{{*/
{
   XColor xfg;
   unsigned int bdr = atoi(bgc);

   if (!Term_Supports_Color)
     return;

   if (alloc_color(fgc, &xfg) < 0)
     return;

   XSetWindowBorder (This_XDisplay, XWin->w, xfg.pixel);
   if (bdr < 1000)
     XSetWindowBorderWidth (This_XDisplay, XWin->w, bdr);
}

/*}}}*/
static JX_SETXXX_RETURN_TYPE JX_set_mono (int obj_unused, char *unused, SLtt_Char_Type c_unused)
{
   (void) obj_unused;
   (void) unused;
   (void) c_unused;
   return JX_SETXXX_RETURN_VAL;
}

static JX_SETXXX_RETURN_TYPE JX_set_color (int i, char *what, char *fg, char *bg)
{
   if (XWin == NULL)
     return JX_SETXXX_RETURN_VAL;

   if (!Term_Supports_Color) 
     return JX_SETXXX_RETURN_VAL;

#if SLANG_VERSION >= 10306
   SLsmg_touch_screen ();
#endif

   if (i == -1)
     {
	if (!strcmp("mouse", what))
	  {
	     set_mouse_color (fg, bg);
	  }
	else if (!strcmp("border", what))
	  {
	     set_border_color (fg, bg);
	  }

	return JX_SETXXX_RETURN_VAL;
     }

   x_set_color_free (i, fg, bg, 1);
   XSetForeground(This_XDisplay, XWin->text_gc[i].gc, XWin->text_gc[i].fg);
   XSetBackground(This_XDisplay, XWin->text_gc[i].gc, XWin->text_gc[i].bg);
   if (i == JNORMAL_COLOR)
     XSetWindowBackground (This_XDisplay, This_XWindow, XWin->text_gc[i].bg);
   return JX_SETXXX_RETURN_VAL;
}

/*}}}*/

static void x_warp_pointer (void) /*{{{*/
{
   X_Warp_Pending = 1;
}

/*}}}*/

static void x_region_2_cutbuffer (void) /*{{{*/
{
   int nbytes;
   char *dat;

   dat = make_buffer_substring(&nbytes);
   if (dat == NULL) return;

   XStoreBytes (This_XDisplay, dat, nbytes);

#if 0
   XChangeProperty (This_XDisplay, DefaultRootWindow (This_XDisplay),
		    XA_CUT_BUFFER0, XA_STRING, 8, PropModeReplace,
		    dat, nbytes);
#endif

   SLfree (dat);
}

/*}}}*/

static int x_insert_cutbuffer (void) /*{{{*/
{
   int nbytes;
   char *dat;

   CHECK_READ_ONLY
     dat = XFetchBytes (This_XDisplay, &nbytes);
   if (nbytes && (dat != NULL)) jed_insert_nbytes ((unsigned char *) dat, nbytes);
   if (dat != NULL) XFree (dat);
   return nbytes;
}

/*}}}*/

static int x_insert_selection (void)
{
#if XJED_USE_COMPOUND_TEXT
   XConvertSelection (This_XDisplay, XA_PRIMARY, Compound_Text_Atom, Xjed_Prop, This_XWindow,
		      Current_Event.xbutton.time);
#else
# if SLANG_VERSION >= 20000
   if (Jed_UTF8_Mode)
     XConvertSelection (This_XDisplay, XA_PRIMARY, UTF8_String_Atom, Xjed_Prop, This_XWindow,
			Current_Event.xbutton.time);
   else
# endif
     XConvertSelection (This_XDisplay, XA_PRIMARY, XA_STRING, Xjed_Prop, This_XWindow,
			Current_Event.xbutton.time);
#endif
   return 0;
}

typedef struct Selection_Data_Type
{
   unsigned int len;
   struct Selection_Data_Type *next;
   unsigned char bytes[1];
}
Selection_Data_Type;

static void free_selection_list (Selection_Data_Type *list)
{
   while (list != NULL)
     {
	Selection_Data_Type *next = list->next;
	SLfree ((char *) list);
	list = next;
     }
}
		
static int append_to_selection_list (unsigned char *buf, unsigned int len,
				 Selection_Data_Type **rootp, Selection_Data_Type **tailp)
{
   Selection_Data_Type *next;

   if (NULL == (next = (Selection_Data_Type *)SLmalloc (sizeof(Selection_Data_Type)+len)))
     return -1;
   memcpy ((char *)next->bytes, buf, len);
   next->len = len;
   next->next = NULL;
   if (*rootp == NULL)
     *rootp = next;
   else
     *tailp = next;
   return 0;
}

static Selection_Data_Type *read_selection (Display *d, Window w, Atom property)
{
   Atom actual_type;
   int actual_format;
   unsigned long nitem, bytes_after;
   unsigned char *data;
   unsigned long total_bytes;
   Selection_Data_Type *list = NULL, *tail = NULL;

   data = NULL;
   /* XGetWindowProperty is a very messed up function.  I do not
    * recall one so poorly designed, including the win32 functions that
    * I have seen.
    */
   if (Success != XGetWindowProperty (d, w, property,
				      0, 0, False, AnyPropertyType,
				      &actual_type, &actual_format,
				      &nitem, &bytes_after, &data))
     return NULL;

   if (data != NULL)
     XFree (data);

   if ((actual_type == None) || (actual_format != 8))
     return NULL;

   total_bytes = 0;
   while (bytes_after != 0)
     {
	unsigned long bytes_read;
	XTextProperty tp;
	char **mb_data;
	int mb_n;
	int status;

	if (Success != XGetWindowProperty (d, w, property, 
					   total_bytes/4, 1 + bytes_after/4,
					   False, AnyPropertyType,
					   &actual_type, &actual_format,
					   &nitem, &bytes_after, &data))
	  break;		       /* What the ?? */
	
	bytes_read = nitem * (actual_format/8);

        tp.value = data;
	tp.encoding = actual_type;
	tp.format = actual_format;
	tp.nitems = nitem;

#if USE_XUTF8_CODE
	if (Jed_UTF8_Mode)
	  status = Xutf8TextPropertyToTextList (This_XDisplay, &tp, &mb_data, &mb_n);
	else
#endif
	status = XmbTextPropertyToTextList(This_XDisplay, &tp, &mb_data, &mb_n);

	if (status == Success)
	  {
	     int i;

	     for (i=0; i < mb_n; i++)
	       {
		  if (-1 == append_to_selection_list ((unsigned char *)mb_data[i], strlen(mb_data[i]), &list, &tail))
		    {
		       free_selection_list (list);
		       XFreeStringList (mb_data);
		       XFree (data);
		       return NULL;
		    }
	       }
	     XFreeStringList (mb_data);
	  }
	else if (-1 == append_to_selection_list (data, bytes_read, &list, &tail))
	  {
	     free_selection_list (list);
	     XFree (data);
	     return NULL;
	  }
	total_bytes += bytes_read;
	XFree (data);
     }

   return list;
}

static int receive_selection (XEvent *ev)
{
   Atom property;
   Selection_Data_Type *list;

   if (None == (property = ev->xselection.property))
     {
	/* Try this */
	(void) x_insert_cutbuffer ();
	return -1;
     }

   list = read_selection (This_XDisplay, This_XWindow, property);
   XDeleteProperty (This_XDisplay, This_XWindow, property);
   if (list == NULL)
     return -1;
   while (list != NULL)
     {
	Selection_Data_Type *next = list->next;
	if (-1 == jed_insert_nbytes (list->bytes, list->len))
	  {
	     free_selection_list (list);
	     return -1;
	  }
	SLfree ((char *) list);
	list = next;
     }
   return 0;
}


static void x_region_2_selection (void)
{
   int nbytes;

   if (Selection_Send_Data != NULL)
     SLfree (Selection_Send_Data);
   Selection_Send_Data = make_buffer_substring (&nbytes);
   if (Selection_Send_Data == NULL) return;
   
   XSetSelectionOwner (This_XDisplay, XA_PRIMARY, This_XWindow, Current_Event.xbutton.time);
   if (This_XWindow != XGetSelectionOwner (This_XDisplay, XA_PRIMARY)) return;
}

static int send_selection (XEvent *ev)
{
   int len;
   XTextProperty tp;
   XSelectionEvent sev;
   XSelectionRequestEvent *xsr;
   int status;
   Atom target;
   int free_tp_value;

   if (NULL == Selection_Send_Data) 
     return 0;

   memset ((char *)&tp, 0, sizeof (tp));

   xsr = &ev->xselectionrequest;
   target = xsr->target;

   if (target == Targets_Atom)
     {
	/* The requester wants to know what targets we support.  How polite. */
#define MAX_SELECTION_TARGETS 5
	Atom target_atoms[MAX_SELECTION_TARGETS];
	unsigned int ntargets = 0;

	target_atoms[ntargets++] = XA_STRING;
	target_atoms[ntargets++] = Text_Atom;
	target_atoms[ntargets++] = Compound_Text_Atom;
#if USE_XUTF8_CODE
	target_atoms[ntargets++] = UTF8_String_Atom;
#endif
	tp.value = (unsigned char *)target_atoms;
	tp.format = 8;
	tp.nitems = sizeof(Atom)*ntargets;
	free_tp_value = 0;
	len = 0;
     }
   else
     {
	int (*text_to_property)(Display *, char **, int, XICCEncodingStyle, XTextProperty *);
	XICCEncodingStyle style;

#if USE_XUTF8_CODE
	if (Jed_UTF8_Mode)
	  text_to_property = Xutf8TextListToTextProperty;
	else
#endif
	  text_to_property = XmbTextListToTextProperty;

	if (target == Compound_Text_Atom)
	  style = XCompoundTextStyle;
	else if (target == UTF8_String_Atom)
	  {
#if USE_XUTF8_CODE
	     style = XUTF8StringStyle;
#else
	     style = XTextStyle;
#endif
	  }
	else if ((target == Text_Atom) || (target == XA_STRING))
	  style = XTextStyle;
	else
	  {
	     char *name = XGetAtomName(This_XDisplay, target);
	     if (name != NULL)
	       {
		  (void) fprintf (stderr, "Unsupported selection target: %s\n", name);
		  XFree (name);
	       }
	     return -1;
	  }   
	status = (*text_to_property) (This_XDisplay, &Selection_Send_Data, 1, style, &tp);

	if ((status != Success) || (tp.value == NULL))
	  return -1;
	
	free_tp_value = 1;
	len = strlen (Selection_Send_Data);
     }

   status = XChangeProperty (This_XDisplay, xsr->requestor,
			     xsr->property, xsr->target, tp.format,
			     PropModeReplace, tp.value, tp.nitems);

   sev.type = SelectionNotify;
   sev.requestor = xsr->requestor;
   sev.selection = xsr->selection;
   sev.target = target;
   sev.time = xsr->time;
   sev.property = xsr->property;
   /* Apparantly XChangeProperty can return BadRequest, even if it succeeds.
    * So ignore its return value except for BadAlloc
    */
   if (status != BadAlloc)
     (void) XSendEvent (This_XDisplay, xsr->requestor, False, (long)NULL, (XEvent*)&sev);

   if (free_tp_value)
     XFree (tp.value);

   return len;
}

static char *x_server_vendor (void)
{
   return The_Xserver_Vendor;
}

#if SLANG_VERSION < 10404
static char *get_termcap_string (char *cap)
{
   return "";
}
#endif


static void x_set_meta_keys (int *maskp)
{
   int mask = *maskp;

   JX_MetaMask = 0;
   if (mask & (1<<0)) JX_MetaMask |= Mod1Mask;
   if (mask & (1<<1)) JX_MetaMask |= Mod2Mask;
   if (mask & (1<<2)) JX_MetaMask |= Mod3Mask;
   if (mask & (1<<3)) JX_MetaMask |= Mod4Mask;
   if (mask & (1<<4)) JX_MetaMask |= Mod5Mask;
}

static SLang_Intrin_Fun_Type sl_x_table[] = /*{{{*/
{
   MAKE_INTRINSIC_S("x_set_window_name", set_window_name, VOID_TYPE),
   MAKE_INTRINSIC_S("x_set_icon_name", set_icon_name, VOID_TYPE),
   MAKE_INTRINSIC("x_warp_pointer", x_warp_pointer, VOID_TYPE, 0),
   MAKE_INTRINSIC("x_insert_cutbuffer", x_insert_cutbuffer, INT_TYPE, 0),
   /* Prototype: Integer x_insert_cutbuffer ();
    * Inserts cutbuffer into the current buffer and returns the number
    * of characters inserted.
    */
   MAKE_INTRINSIC("x_copy_region_to_cutbuffer", x_region_2_cutbuffer, VOID_TYPE, 0),
   /*Prototype: Void x_copy_region_to_cutbuffer();
    */
   MAKE_INTRINSIC("x_insert_selection", x_insert_selection, SLANG_INT_TYPE, 0),
   /* Prototype: Integer x_insert_selection ();
    * This function only requests selection data from the selection owner.
    * If Xjed received EVENT, Xjed inserts selection data into the current buffer.
    * And returns the number of characters inserted.
    */
   MAKE_INTRINSIC("x_copy_region_to_selection", x_region_2_selection, VOID_TYPE, 0),
   /*Prototype: Void x_copy_region_to_selection();
    */
   MAKE_INTRINSIC_IIS("x_set_keysym", x_set_keysym, VOID_TYPE),
  /*Prototype: Void x_set_keysym (Integer keysym, Integer shift, String str);
   *
   * This function may be used to assocate a string 'str' with a key
   * 'keysym' modified by mask @shift@. Pressing the key associated with
   * @keysym@ will then generate the keysequence given by @str@. The
   * function keys are mapped to integers in the range @0xFF00@ to @0xFFFF@.
   * On most systems, the keys that these mappings refer to are located in
   * the file @/usr/include/X11/keysymdef.h@. For example, on my system, the
   * keysyms for the function keys @XK_F1@ to @XK_F35@ fall in the range
   * @0xFFBE@ to @0xFFE0@. So to make the @F1@ key correspond to the string
   * given by the two characters @Ctrl-X@ @Ctrl-C@, simply use:
   * @ x_set_keysym (0xFFBE, 0, "^X^C");
   * The @shift@ argument is an integer with the following meanings:
   * @ 0   : unmodified key
   * @ '$'  : shifted
   * @ '^'  : control
   * Any other value for shift will default to 0 (unshifted).
   */
   MAKE_INTRINSIC("x_server_vendor", x_server_vendor, STRING_TYPE, 0),
   /* Prototype: String x_server_vendor ();
    * This function returns the vendor name of the X server.
    */
   MAKE_INTRINSIC_I("x_set_meta_keys", x_set_meta_keys, SLANG_VOID_TYPE),
#if SLANG_VERSION < 10404
   MAKE_INTRINSIC_S("get_termcap_string", get_termcap_string, STRING_TYPE),
#endif
   MAKE_INTRINSIC_0("x_toggle_visibility", x_toggle_visibility, SLANG_VOID_TYPE),
   MAKE_INTRINSIC(NULL,NULL,0,0)
};

/*}}}*/

static SLang_Intrin_Var_Type X_Variable_Table [] =
{
   MAKE_VARIABLE("ALT_CHAR", &X_Alt_Char, INT_TYPE, 0),
   MAKE_VARIABLE("X_LAST_KEYSYM", &X_Last_Keysym, INT_TYPE, 0),
   MAKE_VARIABLE(NULL,NULL,0,0)
};

static int X_init_slang (void) /*{{{*/
{
   if ((-1 == SLadd_intrin_fun_table (sl_x_table, "XWINDOWS"))
       || (-1 == SLadd_intrin_var_table (X_Variable_Table, NULL)))
     return -1;
   return 0;
}

/*}}}*/

static void X_update_open (void) /*{{{*/
{
   hide_cursor ();
   if (Check_Buffers_Pending)
     {
	check_buffers();
	Check_Buffers_Pending = 0;
     }
   Performing_Update = 1;
}

/*}}}*/

static void X_update_close (void) /*{{{*/
{
   Performing_Update = 0;
   if (XWin->window_mapped == 0) JWindow->trashed = 1;
   if (JWindow->trashed) return;
   show_cursor ();
   if (X_Warp_Pending)
     {
	XWarpPointer (This_XDisplay, None, XWin->w, 0, 0, 0, 0,
		      (XWin->vis_curs_col * XWin->font_width
		       + XWin->border + XWin->font_width / 2),
		      (XWin->vis_curs_row * XWin->font_height
		       + XWin->border + XWin->font_height / 2));
	X_Warp_Pending = 0;
     }

}

/*}}}*/

static void x_define_xkeys (SLKeyMap_List_Type *map) /*{{{*/
{
   SLkm_define_key ("\033[^D", (FVOID_STAR) jed_scroll_right_cmd, map);
   SLkm_define_key ("\033[d", (FVOID_STAR) jed_scroll_right_cmd, map);
   SLkm_define_key ("\033[^C", (FVOID_STAR) jed_scroll_left_cmd, map);
   SLkm_define_key ("\033[c", (FVOID_STAR) jed_scroll_left_cmd, map);
   SLkm_define_key ("\033[a", (FVOID_STAR) bob, map);
   SLkm_define_key ("\033[^A", (FVOID_STAR) bob, map);
   SLkm_define_key ("\033[b", (FVOID_STAR) eob, map);
   SLkm_define_key ("\033[^B", (FVOID_STAR) eob, map);
   SLkm_define_key ("\033[1~", (FVOID_STAR) bol, map);   /* home */
   SLkm_define_key ("\033[4~", (FVOID_STAR) eol, map);   /* end */
}

/*}}}*/

static int JX_reset_video (void) /*{{{*/
{
   JX_reset_scroll_region ();
   JX_goto_rc (0, 0);
   JX_normal_video ();
   /* return vterm_reset_display (); */
   return 0;
}

/*}}}*/

static int JX_init_video (void) /*{{{*/
{
   JX_reset_video ();
   if ((JX_Screen_Rows == 0) || (JX_Screen_Cols == 0))
     {
	JX_Screen_Cols = 80;
	JX_Screen_Rows = 24;
     }

   /* return vterm_init_display (JX_Screen_Rows, JX_Screen_Cols); */
   return 0;
}

/*}}}*/

void flush_output (void) /*{{{*/
{
   if (This_XDisplay == NULL)
     fflush (stdout);
   else
     SLtt_flush_output ();
}

/*}}}*/

/* a hook to parse some command line args. */
int (*X_Argc_Argv_Hook)(int, char **) = X_eval_command_line;

static int JX_flush_output (void)
{
   return 0;
}

static int JX_Zero = 0;

static void get_screen_size (int *r, int *c)
{
   SLtt_get_screen_size ();
   *r = SLtt_Screen_Rows;
   *c = SLtt_Screen_Cols;
}


/* the links to functions and variables here */
void (*tt_beep)(void);
void (*tt_write_string)(char *);

JX_SETXXX_RETURN_TYPE (*tt_set_color)(int, char *, char *, char *);
JX_SETXXX_RETURN_TYPE (*tt_set_color_esc)(int, char *);
JX_SETXXX_RETURN_TYPE (*tt_set_mono) (int, char *, SLtt_Char_Type);

void (*tt_wide_width)(void);
void (*tt_narrow_width)(void);
void (*tt_enable_cursor_keys)(void);
void (*tt_set_term_vtxxx)(int *);
void (*tt_get_screen_size)(int *, int *);

int *tt_Ignore_Beep;
int *tt_Use_Ansi_Colors;
int *tt_Term_Cannot_Scroll;
int *tt_Term_Cannot_Insert;
int *tt_Blink_Mode;

/* int *tt_Baud_Rate; */

static void set_xtt_hooks (void)
{
   tt_beep = JX_beep;
   tt_write_string = JX_write_string;
   tt_get_screen_size = JX_get_display_size;
   tt_set_color = JX_set_color;
   tt_set_mono = JX_set_mono;

   tt_wide_width = JX_wide_width;
   tt_narrow_width = JX_narrow_width;
   tt_enable_cursor_keys = JX_enable_cursor_keys;
   tt_set_term_vtxxx = JX_set_term_vtxxx;

   tt_Ignore_Beep  		= &JX_Ignore_Beep;
   tt_Use_Ansi_Colors  	= &JX_Use_Ansi_Colors;
   tt_Term_Cannot_Scroll  	= &JX_Term_Cannot_Scroll;
   tt_Term_Cannot_Insert  	= &JX_Term_Cannot_Insert;
   tt_Blink_Mode		= &JX_Blink_Mode;
}

static void JX_get_terminfo (void) /*{{{*/
{
   SLsmg_Term_Type tt;

   if ((Batch) || !open_Xdisplay())
    {
       /* This function should match the corresponding function in display.c.
	* I should "include" it to guarantee the correspondence.
	*/
       tt_beep                 = SLtt_beep;
       tt_write_string         = SLtt_write_string;
       tt_get_screen_size      = get_screen_size;
       tt_set_color            = SLtt_set_color;
       tt_set_mono             = SLtt_set_mono;
#if SLANG_VERSION < 20000
       tt_set_color_esc        = SLtt_set_color_esc;
#endif
       tt_wide_width           = SLtt_wide_width;
       tt_narrow_width         = SLtt_narrow_width;
       tt_enable_cursor_keys   = SLtt_enable_cursor_keys;
       tt_set_term_vtxxx       = SLtt_set_term_vtxxx;
       tt_Ignore_Beep          = &SLtt_Ignore_Beep;
       tt_Use_Ansi_Colors      = &SLtt_Use_Ansi_Colors;
       tt_Term_Cannot_Scroll   = &SLtt_Term_Cannot_Scroll;
       tt_Term_Cannot_Insert   = &SLtt_Term_Cannot_Insert;
       tt_Blink_Mode           = &SLtt_Blink_Mode;
       /*     tt_Baud_Rate            = &SLtt_Baud_Rate; */

       if (Batch == 0) SLtt_get_terminfo ();
       return;
    }

   set_xtt_hooks ();

   if (-1 == init_xkeys ())
     {
     }

   JX_Screen_Cols = 80;
   JX_Screen_Rows = 24;
   
   (void) jed_add_init_slang_hook (X_init_slang);
     
   /* init hooks */
   X_Read_Hook = X_read_key;
   X_Input_Pending_Hook = X_input_pending;
   X_Update_Open_Hook = X_update_open;
   X_Update_Close_Hook = X_update_close;
   X_Suspend_Hook = xjed_suspend;
   X_Init_Term_Hook = init_Xdisplay;
   X_Reset_Term_Hook = reset_Xdisplay;
   X_Define_Keys_Hook = x_define_xkeys;
   SLang_Interrupt = xjed_check_kbd;

   /* Set this so that main will not try to read from stdin.  It is quite
    * likely that this is started from a menu or something.
    */
   Stdin_Is_TTY = -1;
   /* We do not need this since we do not have to worry about incoming
    * eight bit escape sequences.
    */
   DEC_8Bit_Hack = 0;

   memset ((char *) &tt, 0, sizeof (SLsmg_Term_Type));

   tt.tt_normal_video = JX_normal_video;
   tt.tt_set_scroll_region = JX_set_scroll_region;
   tt.tt_goto_rc = JX_goto_rc;
   tt.tt_reverse_index = JX_reverse_index;
   tt.tt_reset_scroll_region = JX_reset_scroll_region;
   tt.tt_delete_nlines = JX_delete_nlines;
   tt.tt_cls = JX_cls;
   tt.tt_del_eol = JX_del_eol;
   tt.tt_smart_puts = JX_smart_puts;
   tt.tt_flush_output = JX_flush_output;
   tt.tt_reset_video = JX_reset_video;
   tt.tt_init_video = JX_init_video;

   tt.tt_screen_rows = &JX_Screen_Rows;
   tt.tt_screen_cols = &JX_Screen_Cols;
   tt.tt_term_cannot_scroll = &JX_Term_Cannot_Scroll;
   tt.tt_has_alt_charset = &JX_Zero;

#if SLANG_VERSION >= 20000
   tt.unicode_ok = &Jed_UTF8_Mode;
#endif

   SLsmg_set_terminal_info (&tt);
   Jed_Handle_SIGTSTP = 0;
}

/*}}}*/

void (*tt_get_terminfo)(void) = JX_get_terminfo;

/* Unused but required. */
#ifdef USE_GPM_MOUSE
int (*X_Open_Mouse_Hook)(void);
void (*X_Close_Mouse_Hook)(void);
#endif
