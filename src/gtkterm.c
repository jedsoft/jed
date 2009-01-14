/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#define _BUILD_GTK_JED

#include "config.h"
#include "jed-feat.h"

#define USE_NEW_META_CODE	1
#define HAS_IBM_NUMLOCK_CODE	0
#define SKIP_LOCALE_CODE	0
#ifndef HAVE_SETLOCALE
# define X_LOCALE		1
#endif

#define XJED_USE_COMPOUND_TEXT	0
#ifndef XJED_HAS_XRENDERFONT
# define XJED_HAS_XRENDERFONT	0
#endif

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
#include <sys/time.h>
#include <time.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <slang.h>

#include "jdmacros.h"

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
#include "window.h"
#include "menu.h"

#include "gtkjed.h"

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

typedef struct /*{{{*/
{
   GdkGC     *gc;
   GdkColor  *fg, *bg;
   char      *fgName;
   char      *bgName;
   int        dirty;
}
/*}}}*/
GCGtkInfoType;

typedef struct StructGtkSubWinType
{
   struct StructGtkSubWinType *next;

   Window_Type         *jedWin;
   GtkWidget           *boxWin;
   GtkWidget           *edWin;
   GtkWidget           *stWin;
   GtkWidget           *sbWin;

   gint                edX;
   gint                edY;
   gint                edWidth;
   gint                edHeight;

   GtkAdjustment       *sbAdj;

   int                  numLineSav;
   int                  curLineSav;
   int                  winHeightSav;

   int                  ppY;
   int                  ppX;
   int                  ppHeight;   /* proposed height */
   int                  ppWidth;    /* proposed width  */
}
GtkSubWinType;

typedef struct
{

   Display     *XDisplay;

   /*********************************************************************/
   /****** Stolen from original JXWindow_Type, don't get me wrong, but **/
   /****** this data is still very useful.                ***************/
   /*********************************************************************/

   /* int          height, width; */
   int          border;			        /* inside border */
   int          o_border;		        /* outside border */

   int          current_gc_num;

   int          vis_curs_row, vis_curs_col;     /* position of VISIBLE cursor */

   int          cursor_showing;		        /* true if widow has cursor showing */
   int          focus;			        /* true if window has focus */
   int          window_mapped;		        /* true if window is mapped */

   /* Window tty parameters */

   int insert_mode;		                /* true if inserting */
   int scroll_r1,  scroll_r2;	                /* scrolling region */
   int cursor_row, cursor_col;	                /* row column of cursor (0, 0) origin */

   int visible;			                /* from visibilitynotify */

   /*********************************************************************/

   GdkDisplay      *display;
   GdkScreen       *screen;
   GdkColormap     *colormap;
   int              height;   /* Height of Window in Pixel                     */
   int              width;    /* Width of Window in Pixel                      */

   int              edHeight; /* Total height of all editor windows in lines   */
   int              edWidth;  /* Total width  of all editor windows in rows    */
   int              hCheck;

   int              numWin;   /* Number of windows, not including the Minibuffer  */
   GtkSubWinType   *subWins;
   int              gutSize;  /* size of handle between subWins                */
   GtkSubWinType ***widgetWinXRef;
   GtkSubWinType   *miniWinSW;
   int              inGridHeight;
   int              inGridHeightPixel;
   int              inGridWidthPixel;

   int              maxEdHeight;  /* Max number of Lines of all editor windows in total */
                                  /* Needed only for preallocating memory        */
   int              maxEdWidth;   /* Max rows per line, needed for preallocation */
#if 0
   int             *maxWinLine;   /* maximal window segments for each line     */
   int             *actWinLine;   /* actual number of windows for each line    */
   int            **lineWinXRef;  /* All lines and window segment definition   */
#endif
   /************************************************************************
   |------------------------------------------------------------------------
   |appW: GTK_WINDOW_TOPLEVEL
   | |----------------------------------------------------------------------
   | | appWGrid: vbox
   | | |--------------------------------------------------------------------
   | | |appWMenuBar: menu_bar
   | | | |------------------------------------------------------------------
   | | | |<<< the editor menu >>>
   | | | |------------------------------------------------------------------
   | | |--------------------------------------------------------------------
   | | |appWTBGrid: vbox, toolbar area
   | | | |------------------------------------------------------------------
   | | | | First TB (created dynamically)
   | | | |------------------------------------------------------------------
   | | | | Next TB (created dynamically)
   | | | |------------------------------------------------------------------
   | | | | ...
   | | | |------------------------------------------------------------------
   | | | | N-th TB (created dynamically)
   | | | |------------------------------------------------------------------
   | | |--------------------------------------------------------------------
   | | |appWEdGrid: vbox
   | | | |------------------------------------------------------------------
   | | | | appWASCWin <<< currently not used, will be draw area >>>
   | | | |------------------------------------------------------------------
   | | | | appWTopEdWin: paned Widget or box
   | | | |
   | | | | <<< editor windows, dynamically created >>>
   | | | |
   | | | | ****************************************************
   | | | | * Editor area widgets, implemented as vertical box
   | | | | * containing:
   | | | | * - horizontal box containing
   | | | | *    - draw area
   | | | | *    - scrollbar
   | | | | * - draw area to display status line for buffer
   | | | | * dynamically. All editor area widgets are hierarchically
   | | | | * stacked together with paned widget. If there is only one editor
   | | | | * area widget no paned widget is used/created.
   | | | | * |-----------------------------------------------------
   | | | | * | vertical box
   | | | | * | |---------------------------------------------------
   | | | | * | | horizontal box
   | | | | * | | |------------------------------------------------|
   | | | | * | | | draw area (X: scrollbar)                     | |
   | | | | * | | |                                              | |
   | | | | * | | |                                              |X|
   | | | | * | | |                                              |X|
   | | | | * | | |                                              |X|
   | | | | * | | |                                              | |
   | | | | * | | |                                              | |
   | | | | * | | |------------------------------------------------|
   | | | | * | |---------------------------------------------------
   | | | | * | | draw area to display buffer status line
   | | | | * | |---------------------------------------------------
   | | | | * |-----------------------------------------------------
   | | | | ****************************************************
   | | | |------------------------------------------------------------------
   | | | | appWMiniW (widget for miniWin)
   | | | |------------------------------------------------------------------
   | | |--------------------------------------------------------------------
   | | |appWSB: statusbar
   | | |--------------------------------------------------------------------
   | |----------------------------------------------------------------------
   |------------------------------------------------------------------------
   ************************************************************************/

   /*****************************************************
   * Main container widgets
   *****************************************************/

   GtkWidget       *appW;                 /* GTK_WINDOW_TOPLEVEL  */
   GtkWidget       *appWGrid;             /* vbox                 */

   /*****************************************************
   * Default menu widgets, contain no functionality
   *****************************************************/

   GtkWidget       *appWMenuBar;          /* menu_bar       */
   GtkWidget       *appWMenu;
   GtkWidget       *appWMenuItem;
   GtkWidget       *appWMenuItemOpen;
   GtkWidget       *appWMenuItemSave;
   GtkWidget       *appWMenuItemClose;

   /* GHashTable      *menuArray; */ /* Used to access menu items by path. The path is used as index */
                                /*  into a hash table, that contains pointer to menu items.     */

   /***************************************************
   * Toolbar widget container, toolbars are created
   * dynamically.
   ***************************************************/

   GtkWidget       *appWTbGrid;           /* vbox           */

   /***************************************************
   * Editor area widgets, implemented as vertical box
   * containing:
   * - horizontal box containing
   *    - draw area
   *    - scrollbar
   * - draw area to display status line for buffer
   * dynamically. All editor area widgets are hierarchically
   * stacked together with paned widget. For only one editor
   * area widget no paned widget is used.
   ***************************************************/

   GtkWidget       *appWEdGrid;     /* Contains drawing Area for Menu and appWInGrid                */
   GtkWidget       *appWASCMenu;    /* Draw area containing the ASCII-based menu <<< currently      */
				    /*      not implemented >>>                                     */
   GtkWidget       *appWInGrid;     /* Contains Toplevel paned widget and miniWidget for minibuffer */
                                    /*      following the description above, to be removed in near  */
				    /*      future                                                  */
   GtkWidget       *appWTopEdWin;   /* Toplevel EditorWindow (draw area / paned widget)             */
   GtkWidget       *appWMiniW;      /* miniWidget, Widget to display the minibuffer                 */

   /*****************************************************
   * Status bar widget, currently not used.
   *****************************************************/

   GtkWidget       *appWSB;         /* Statusbar widget                                             */

   /**************************************************
   * Text drawing/rendering objects
   **************************************************/

   PangoContext          *fontContext;
   PangoFontDescription  *fontDescription;
   PangoFontMetrics      *fontMetrics;
   char                  *fontName;
   int                    fontAscent;
   int                    fontDescent;
   int                    fontHeight;
   int                    fontWidth;

   GCGtkInfoType         *textGC;
   GCGtkInfoType         *currentGC;
}
JGtkWinType;

/* extern JGtkWinType  JGtkWinBuf; */
extern JGtkWinType *JGtkWin;

/* #define XJED_MAP 0 */

#define MAX_EDHEIGHT  500; /* default for height of edit windows */
#define MAX_EDWIDTH   500; /* default for width of edit windows  */

void jedGtkUpdateEdSize( int, int );
void updateWidgetWinXRef( GtkSubWinType *, int, int, int, int );

/* static int checkFlushXDisplay(void); */
/* static int checkFlushXDisplayST( int, const char * ); */

static int JX_Screen_Cols;
static int JX_Screen_Rows;
static int JX_Term_Cannot_Scroll =  0;
static int JX_Term_Cannot_Insert =  0;
static int JX_Use_Ansi_Colors =     1;
static int JX_Ignore_Beep =         3;
static int JX_Blink_Mode =          1;
static int JX_MultiClick_Time =     500;   /* milliseconds */

static int JX_MetaMask = Mod1Mask;

#ifdef XJED_USE_R6IM
static char            *R6IM_Input_Method =  NULL;
/* static XIMStyle         R6IM_Input_Style =   0; */
static char            *R6IM_Preedit_Type =  "Root";
/* static XIC              R6IM_Xic; */
/* static void             i18init(void); */
/* static XPoint           R6IM_Spot; */
/* static XVaNestedList    R6IM_Preedit_Attr; */
/* static void             move_input_position (void); */
/* static void             set_geometry (XEvent *, XIMStyle, char *); */
#endif

static int              updateGtkWin = 1;  /* used during splitWin / showAll to prevent drawing!!! */
                               /* see also gtkDraw-function */

static int              doColorSetup = 1;
static int              doCLS = 1;

static gboolean         sbChangeValue( GtkRange *, GtkScrollType,
				       gdouble, gpointer );

void                    printGtkSubWin( GtkSubWinType * );

void                    printSubWins(void);

static void             toggle_cursor( int );

/* static char            *The_Xserver_Vendor; */

/* static int Current_Color; */
static int              X_Alt_Char =     27;
static KeySym           X_Last_Keysym;
static int              X_Warp_Pending = 0;

static gdouble          actSBVal;

#define FC_CMD_KEY_SEQ "\033[FC"

static gpointer         actParaData;
static void          ( *actFunToCall )( gpointer );

static void       modWidgetWinXRef( GtkSubWinType ***, GtkSubWinType *, int, int, int, int );
static void       printWidgetStruct(void);

/******************** Jed internal Event Queue *******************/

typedef enum
{
   JXGtkAnyID            = 1,
   JXGtkGdkEventID       = 2,
   JXKeyFeedID           = 3
}
JXGtkEventID;

typedef struct
{
   JXGtkEventID     type;
}
JXGtkAnyEvent;

typedef struct
{
   JXGtkEventID     type;
   GdkEvent         gdkEvent;
   GtkWidget       *w;
   gpointer        *data;
}
JXGtkGdkEvent;

typedef struct
{
   JXGtkEventID     type;
   char            *keys;
}
JXKeyFeedEvent;

typedef union
{
   JXGtkEventID           type;
   JXGtkAnyEvent          any;
   JXGtkGdkEvent          gdkEvent;
   JXKeyFeedEvent         keyFeed;
}
JXGtkEventType;

/************************************************************************/

static int              Performing_Update;
static int              Check_Buffers_Pending;
static int              No_XEvents;		       /* if true, do nothing */

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

#define XJED_CLASS	           "XTerm"
static char    *This_App_Name =    "xjed";
static char    *This_App_Title =   "XJed";

/* #define Default_Geometry "80x24+0-0" */
#define Default_Geometry           "80x24"
static char    *This_Geometry =    NULL;
static char    *This_Font_Name =   "fixed";

#if XJED_HAS_XRENDERFONT
static char    *This_Face_Size =   "                           ";
#endif

static char    *This_Border_Width_Name =    "0";
static char    *This_Internal_Border_Name = "0";
static char    *This_MFG =                  "green";
static char    *This_MBG =                  "white";
static char    *Iconic =                    NULL;

/* static Atom     Xjed_Prop; */
/* static Atom     Compound_Text_Atom; */
/* static Atom     UTF8_String_Atom; */

/* static XEvent   Current_Event; */
static char    *jgtkSelectionData = NULL;
static char    *jgtkClipboardData = NULL;
/* static int      receive_selection (XEvent *); */
/* static int      send_selection (XEvent *); */

static GCGtkInfoType DefaultGtkGCInfo[JMAX_COLORS] = /*{{{*/
{
/*******************************************************************************
 * GC: X11-gc
 * foreground (unsigned long)
 * background (unsigned long)
 * foreground color name
 * background color name
 * flag if dirty
*******************************************************************************/
   { NULL, NULL, NULL, "black",   "white",   0 },    /* NORMAL */
   { NULL, NULL, NULL, "green",   "red",     0 },    /* CURSOR */
   { NULL, NULL, NULL, "default", "skyblue", 0 },    /* STATUS */
   { NULL, NULL, NULL, "default", "magenta", 0 },    /* REGION */
   { NULL, NULL, NULL, "default", "skyblue", 0 },    /* MENU */
   { NULL, NULL, NULL, "default", "default", 0 },    /* operator */
   { NULL, NULL, NULL, "green",   "default", 0 },    /* numbers */
   { NULL, NULL, NULL, "blue",    "default", 0 },    /* strings */
   { NULL, NULL, NULL, "default", "gray",    0 },    /* comments */
   { NULL, NULL, NULL, "default", "default", 0 },    /* delimeters */
   { NULL, NULL, NULL, "magenta", "default", 0 },    /* preprocess */
   { NULL, NULL, NULL, "blue",    "default", 0 },    /* message */
   { NULL, NULL, NULL, "red",     "default", 0 },    /* error */
   { NULL, NULL, NULL, "magenta", "default", 0 },    /* dollar */
   { NULL, NULL, NULL, "red",     "default", 0 },    /* keyword */
   { NULL, NULL, NULL, "green",   "default", 0 },    /* keyword1 */
   { NULL, NULL, NULL, "red",     "default", 0 },    /* keyword2 */
   { NULL, NULL, NULL, "green",   "magenta", 0 }     /* ... fold mark */
};

/* cheat a little, use VOID_TYPE for boolean arguments */
static XWindow_Arg_Type jGtkArgList[] = /*{{{*/
{
   /* These MUST be in this order!!! */
#define XARG_DISPLAY	0
   {"Display",		"d", 	STRING_TYPE,	NULL,	NULL},
#define JGTK_ARG_NAME	1
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
   {"font",		"fn",	 STRING_TYPE,	NULL,	&This_Font_Name},
   {"fgMouse",	"mfg", 	 STRING_TYPE,	NULL,	&This_MFG},
   {"bgMouse",	"mbg", 	 STRING_TYPE,	NULL,	&This_MBG},
   {"background",	"bg", 	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JNORMAL_COLOR].bgName},
   {"foreground",	"fg",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JNORMAL_COLOR].fgName},
   {"fgStatus",	"sfg", 	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JSTATUS_COLOR].fgName},
   {"bgStatus",	"sbg", 	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JSTATUS_COLOR].bgName},
   {"fgRegion",	"rfg", 	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JREGION_COLOR].fgName},
   {"bgRegion",	"rbg", 	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JREGION_COLOR].bgName},
   {"fgCursor",	"cfg", 	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JCURSOR_COLOR].fgName},
   {"bgCursor",	"cbg", 	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JCURSOR_COLOR].bgName},
   {"fgCursorOvr",	"cofg",  STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JCURSOROVR_COLOR].fgName},
   {"bgCursorOvr",	"cobg",  STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JCURSOROVR_COLOR].bgName},
   {"fgMenu",		"fgm", 	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JMENU_COLOR].fgName},
   {"bgMenu",		"bgm", 	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JMENU_COLOR].bgName},
   {"fgOperator",	"fgop",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JOP_COLOR].fgName},
   {"bgOperator",	"bgop",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JOP_COLOR].bgName},
   {"fgNumber",	"fgnm",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JNUM_COLOR].fgName},
   {"bgNumber",	"bgnm",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JNUM_COLOR].bgName},
   {"fgString",	"fgst",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JSTR_COLOR].fgName},
   {"bgString",	"bgst",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JSTR_COLOR].bgName},
   {"fgComments",	"fgco",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JCOM_COLOR].fgName},
   {"bgComments",	"bgco",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JCOM_COLOR].bgName},
   {"fgKeyword",	"fgkw",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JKEY_COLOR].fgName},
   {"bgKeyword",	"bgkw",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JKEY_COLOR].bgName},
   {"fgKeyword1",	"fgkw1", STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JKEY_COLOR + 1].fgName},
   {"bgKeyword1",	"bgkw1", STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JKEY_COLOR + 1].bgName},
   {"fgKeyword2",	"fgkw2", STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JKEY_COLOR + 2].fgName},
   {"bgKeyword2",	"bgkw2", STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JKEY_COLOR + 2].bgName},
   {"fgDelimiter",	"fgde",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JDELIM_COLOR].fgName},
   {"bgDelimiter",	"bgde",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JDELIM_COLOR].bgName},
   {"fgPreprocess",	"fgpr",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JPREPROC_COLOR].fgName},
   {"bgPreprocess",	"bgpr",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JPREPROC_COLOR].bgName},
   {"bgMessage",	"bgms",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JMESSAGE_COLOR].bgName},
   {"fgMessage",	"fgms",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JMESSAGE_COLOR].fgName},
   {"bgError",	"bger",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JERROR_COLOR].bgName},
   {"fgError",	"fger",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JERROR_COLOR].fgName},
   {"fgDots",		"fgdt",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JDOTS_COLOR].bgName},
   {"bgDots",		"bgdt",	 STRING_TYPE,	NULL,	&DefaultGtkGCInfo[JDOTS_COLOR].bgName},
   {"BorderWidth",	"bw", 	 STRING_TYPE,	NULL,	&This_Border_Width_Name},
   {"title",		NULL,	 STRING_TYPE,	NULL,	&This_App_Title},
   {"BorderColor",	"bd", 	 STRING_TYPE,	NULL,	NULL},
   {"Iconic",		"ic",	 VOID_TYPE,	NULL,	&Iconic},
   {"xrm",		NULL,	 STRING_TYPE,	NULL,	NULL},
   {"internalBorder", "ib",	 STRING_TYPE,	NULL,	&This_Internal_Border_Name},
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

static JGtkWinType    JGtkWinBuf;
JGtkWinType   *JGtkWin = &JGtkWinBuf;

/* static int            gtkButtonEvent = 0; */

static gboolean       eventCallback( GtkWidget *,
	                             GdkEvent  *,
	                             gpointer  * );
static void           jGtkCoverExposedArea( int, int,
					    int, int,
					    int );

static int            gtkInsertSelection( void );
static void           gtkSelectionReceive( GtkWidget *,
				           GtkSelectionData *,
				           gpointer );
static void           jGtkSendSelection( GtkWidget *,
					 GtkSelectionData *,
					 guint,
					 guint,
					 gpointer );

static void           jGtkSetFGBGColor( GtkWidget *, int );

typedef struct
{
   GList         *jXGtkEvList;
   int            jXGtkNumEv;
}
FilterEventData;

static FilterEventData *thisEventBuffer;

typedef struct jGtkScrollInfoStruct
{
   int        set;     /* flagging scrollInfo active (with valid infos) or not */
   int        tl;      /* Target line */
   GtkRange  *rangeW;  /* rangeWidget */
   int        sLSBeg;     /* singleLineSteps (sLS) outstanding */
                /* Scrolling is done by first doing a view single line steps. */
		/* If target is not reached, jump to target (maybe in a view bigger */
                /* steps, not only one big step). */
   int        sLSEnd;  /* singleLineSteps at End of scrolling. */
   int        jF;      /* jumpFlag, if set to one means, no single line steps any more */
                       /* to perform act scroll event, just jump to target (this should be done, */
                       /* if the next scroll */
		       /* event arrives before act scroll event is completed. */
   int        cnt;     /* intermediate solution for slow down scrolling!!! */
   struct timeval   tv;
}
JGtkScrollInfo;

#define JGTK_SCROLL_SLOW_DOWN 20

static JGtkScrollInfo   jGtkSIPrim =
{
   0,
   1,
   NULL,
   0,
   0,
   0,
   JGTK_SCROLL_SLOW_DOWN,
 { 0,
      0
 }
};

           /* Primary scrollInfo (means scrollInfo for act scroll event) */
static JGtkScrollInfo   jGtkSISec  =
{
   0,
   1,
   NULL,
   0,
   0,
   0,
   JGTK_SCROLL_SLOW_DOWN,
 { 0,
      0
 }
};
           /* Secondary scrollInfo is queued if next scroll event arrives before */
	   /* actual scroll event is completed. In further scrollEvent will */
	   /* discard secondary scrollInfo and reset it. */

/************************************/

/************************************
* JXGtkPending
*
* group: event
*
*
* debug print:
*
*
************************************/

static  gboolean
jGtkPending(void)
{
   while ( gtk_events_pending() )
     {
	gtk_main_iteration_do( False );
     }

   return( thisEventBuffer->jXGtkNumEv );
}

/************************************
* JXGtkPeekEvent
*
* group: event
*
*
* debug print:
*
*
************************************/

#if 0
static int
JXGtkPeekEvent( JXGtkEventType *ev )
{
   FilterEventData *cbData = thisEventBuffer;

   while ( !( thisEventBuffer->jXGtkNumEv ) )
     {
	if ( gtk_events_pending() )
	  {
	     gtk_main_iteration();
	  }
     }

   if ( thisEventBuffer->jXGtkNumEv )
     {
	memcpy( ev, ( JXGtkEventType * ) thisEventBuffer->jXGtkEvList->data, sizeof( JXGtkEventType ) );
	return( TRUE );
     }
   else
     {
	return( FALSE );
     }
}
#endif

/*******************************
* JXGtkNextEvent
*
* group: event
*
*
* debug print:
*
********************************/

static int
JXGtkNextEvent( JXGtkEventType *ev )
{
   FilterEventData *cbData = thisEventBuffer;

   while ( !( thisEventBuffer->jXGtkNumEv ) )
     {
	if ( gtk_events_pending() )
	  {
	     gtk_main_iteration();
	  }
     }

   if ( thisEventBuffer->jXGtkNumEv )
     {
	memcpy( ev, ( JXGtkEventType * ) thisEventBuffer->jXGtkEvList->data, sizeof( JXGtkEventType ) );
	SLfree( ( char * ) thisEventBuffer->jXGtkEvList->data );
	thisEventBuffer->jXGtkEvList =
	  g_list_remove( thisEventBuffer->jXGtkEvList,
			 thisEventBuffer->jXGtkEvList->data );
	thisEventBuffer->jXGtkNumEv -= 1;
	if ( thisEventBuffer->jXGtkNumEv < 0 )
	  thisEventBuffer->jXGtkNumEv = 0;
	return( TRUE );
     }
   else
     {
	return( FALSE );
     }
}

/************************************
* JXGtkCheckMaskEvent( Display *display, long mask, JXGtkEventType *ev )
*
* group: event
*
*
* debug print:
*
***********************************/

static Bool
JXGtkCheckMaskEvent( long mask, JXGtkEventType *ev )
{
   FilterEventData *cbData = thisEventBuffer;
   guint  sizeOfBuffer;
   GList *actEle;

   while ( gtk_events_pending() )
     {
	gtk_main_iteration();
     }

   if ( thisEventBuffer->jXGtkNumEv )
     {
	actEle = thisEventBuffer->jXGtkEvList;

	sizeOfBuffer = g_list_length( actEle );

	while ( sizeOfBuffer-- )
	  {
	     if ( mask && ( ( JXGtkEventType * ) actEle->data )->gdkEvent.gdkEvent.type )
	       {
		  memcpy( ev, ( JXGtkEventType * ) actEle->data, sizeof( JXGtkEventType ) );
		  SLfree( ( char * ) actEle->data );
		  thisEventBuffer->jXGtkEvList =
		    g_list_remove( thisEventBuffer->jXGtkEvList,
				   actEle->data );
		  thisEventBuffer->jXGtkNumEv -= 1;
		  if ( thisEventBuffer->jXGtkNumEv < 0 )
		    thisEventBuffer->jXGtkNumEv = 0;
		  return( TRUE );
	       }
	     actEle = actEle->next;
	  }
	return( False );
     }
   else
     {
	return( False );
     }
}

/************************************
* addGdkEvent
*
* group: event
*
*
* debug print:
*
*
************************************/

static void
addGdkEvent( GdkEvent *ev, GtkWidget *w, gpointer *data )
{
   FilterEventData *eB = thisEventBuffer;
   JXGtkEventType *newEv = ( JXGtkEventType * ) SLmalloc( sizeof( JXGtkEventType ) );

   newEv->type = JXGtkGdkEventID;
   newEv->any.type = JXGtkGdkEventID;
   newEv->gdkEvent.type = JXGtkGdkEventID;
   memcpy( &( newEv->gdkEvent.gdkEvent ), ev, sizeof( GdkEvent ) );
   newEv->gdkEvent.w = w;
   newEv->gdkEvent.data = data;
   eB->jXGtkEvList = g_list_append( eB->jXGtkEvList, newEv );
   ( eB->jXGtkNumEv ) += 1;
   /* printf( "File: %s, Line: %d; Num JXGtkEvents: %d\n", __FILE__, __LINE__, eB->jXGtkNumEv ); */
}

/*************************/

/************************************
* createGtkMiniWin
*
* group: win
*
*
* debug print: "void (%d)\n", 1
*
*
************************************/

static GtkWidget *
createGtkMiniWin(void)
{

   GdkColor white = { 0, 0xffff, 0xffff, 0xffff };
   GtkWidget *w = gtk_drawing_area_new();   /* Edit window */

   gtk_widget_modify_bg(  w, GTK_STATE_NORMAL, &white );

   GTK_WIDGET_SET_FLAGS(  w, GTK_CAN_FOCUS );

   gtk_widget_add_events( w, GDK_KEY_PRESS_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "key_press_event",
		     G_CALLBACK( eventCallback ),
		     NULL );

   gtk_widget_add_events( w, GDK_BUTTON_PRESS_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "button_press_event",
		     G_CALLBACK( eventCallback ),
		     NULL );

   gtk_widget_add_events( w, GDK_BUTTON_RELEASE_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "button_release_event",
		     G_CALLBACK( eventCallback ),
		     NULL );

   g_signal_connect( G_OBJECT( w ),
		     "expose_event",
		     G_CALLBACK( eventCallback ),
		     NULL );

   gtk_widget_add_events( w, GDK_BUTTON_MOTION_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "motion_notify_event",
		     G_CALLBACK( eventCallback ),
		     NULL );

   gtk_widget_add_events( w, GDK_FOCUS_CHANGE_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "focus-out-event",
		     G_CALLBACK( eventCallback ),
		     NULL );
   g_signal_connect( G_OBJECT( w ),
		     "focus-in-event",
		     G_CALLBACK( eventCallback ),
		     NULL );

   g_signal_connect( G_OBJECT( w ),
		     "selection_received",
		     G_CALLBACK( gtkSelectionReceive ),
		     NULL );

   g_signal_connect( G_OBJECT( w ),
		     "selection_get",
		     G_CALLBACK( jGtkSendSelection), NULL );

   gtk_selection_add_target( w,
			     GDK_SELECTION_PRIMARY,
			     GDK_SELECTION_TYPE_STRING,
			     1 );

   return( w );
}

/************************************
* createGtkWinTmpl
*
* group: win
*
*
* debug print: "ce: %d, jw(GtkSubWinType): %x\n", ce, jw
*
*
************************************/

static GtkSubWinType *
createGtkWinTmpl( GtkSubWinType *jw )
{
   JGtkWinType *win = JGtkWin;
   /* EdWinWidgetType edWidgets; */
   GdkColor white = { 0, 0xffff, 0xffff, 0xffff };
   GtkWidget *w = gtk_drawing_area_new();   /* Edit window */
   GtkWidget *ws = gtk_drawing_area_new();  /* Window statusbar */
   GtkWidget *sb = gtk_vscrollbar_new( NULL );
   GtkWidget *eb = gtk_hbox_new( False, 0 );
   GtkWidget *wb = gtk_vbox_new( False, 0 );
   GtkAdjustment *sbAdj = gtk_range_get_adjustment( GTK_RANGE( sb ) );

   gtk_widget_set_size_request( ws, -1, win->fontHeight );

   gtk_widget_modify_bg(  w, GTK_STATE_NORMAL, &white );
   gtk_widget_modify_bg( ws, GTK_STATE_NORMAL, &white );

   GTK_WIDGET_SET_FLAGS(  w, GTK_CAN_FOCUS );
   GTK_WIDGET_SET_FLAGS( ws, GTK_CAN_FOCUS );

   gtk_widget_add_events( w, GDK_KEY_PRESS_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "key_press_event",
		     G_CALLBACK( eventCallback ),
		     jw );

   gtk_widget_add_events( w, GDK_BUTTON_PRESS_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "button_press_event",
		     G_CALLBACK( eventCallback ),
		     w );

   gtk_widget_add_events( ws, GDK_BUTTON_PRESS_MASK );
   g_signal_connect( G_OBJECT( ws ),
		     "button_press_event",
		     G_CALLBACK( eventCallback ),
		     ws );

   gtk_widget_add_events( w, GDK_BUTTON_RELEASE_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "button_release_event",
		     G_CALLBACK( eventCallback ),
		     w );

   gtk_widget_add_events( ws, GDK_BUTTON_RELEASE_MASK );
   g_signal_connect( G_OBJECT( ws ),
		     "button_release_event",
		     G_CALLBACK( eventCallback ),
		     ws );

   gtk_widget_add_events( w, GDK_EXPOSURE_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "expose_event",
		     G_CALLBACK( eventCallback ),
		     jw );

   gtk_widget_add_events( ws, GDK_EXPOSURE_MASK );
   g_signal_connect( G_OBJECT( ws ),
		     "expose_event",
		     G_CALLBACK( eventCallback ),
		     jw );

   gtk_widget_add_events( w, GDK_BUTTON_MOTION_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "motion_notify_event",
		     G_CALLBACK( eventCallback ),
		     jw );

   gtk_widget_add_events( w, GDK_FOCUS_CHANGE_MASK );
   g_signal_connect( G_OBJECT( w ),
		     "focus-out-event",
		     G_CALLBACK( eventCallback ),
		     jw );
   g_signal_connect( G_OBJECT( w ),
		     "focus-in-event",
		     G_CALLBACK( eventCallback ),
		     jw );

   g_signal_connect( G_OBJECT( w ),
		     "configure_event",
		     G_CALLBACK( eventCallback ),
		     jw );

   g_signal_connect( G_OBJECT( w ),
		     "selection_received",
		     G_CALLBACK( gtkSelectionReceive ),
		     jw );

   g_signal_connect( G_OBJECT( w ),
		     "selection_get",
		     G_CALLBACK( jGtkSendSelection), jw );

   gtk_selection_add_target( w,
			     GDK_SELECTION_PRIMARY,
			     GDK_SELECTION_TYPE_STRING,
			     1 );

   g_signal_connect( G_OBJECT( sb ), "change-value",
		     G_CALLBACK( sbChangeValue ),
		     NULL );

   /* Set initial dummy values */
   sbAdj->lower = 0;
   sbAdj->upper = 100;
   sbAdj->value = 30;
   sbAdj->step_increment = 1;
   sbAdj->page_increment = 10;
   sbAdj->page_size = 5;

   g_signal_emit_by_name( G_OBJECT( sbAdj ), "changed" );

   gtk_box_pack_start( GTK_BOX( eb ), w, True, True, 0 );
   gtk_box_pack_start( GTK_BOX( eb ), sb, False, False, 0 );

   gtk_box_pack_start( GTK_BOX( wb ),  eb,  True,  True, 0 );
   gtk_box_pack_start( GTK_BOX( wb ), ws, False, True, 0 );

   jw->edWin = w;
   jw->stWin = ws;
   jw->sbWin = sb;

   return( jw );
}

/************************************
* printWidgetWinXRef
*
* group: win, debug
*
*
* debug print: "(void)", 1
*
*
************************************/

#if 0
static void
printWidgetWinXRef(void)
{
   int i, i1;
   JGtkWinType *win = JGtkWin;

   printf( "Ed Height: %d, Width: %d\n", win->edHeight, win->edWidth );
   printf( " Jed_Num_Screen_Cols: %d, Jed_Num_Screen_Rows: %d, Top_Window_SY: %d\n",
	   Jed_Num_Screen_Cols, Jed_Num_Screen_Rows, Top_Window_SY );

   for ( i = 0; i < win->edHeight; ++i )
     {
	printf( "%3d:", i );
	for ( i1 = 0; i1 < win->edWidth; ++i1 )
	  {
	     if ( win->widgetWinXRef[i][i1] )
	       {
		  if ( i != win->widgetWinXRef[i][i1]->jedWin->sy +
		       win->widgetWinXRef[i][i1]->jedWin->rows )
		    {
	       /* printf( "(%6x-E-%6x)", win->widgetWinXRef[i][i1]->jedWin, win->widgetWinXRef[i][i1]->edWin ); */
		       printf( "E%p-%p ", (void*)win->widgetWinXRef[i][i1]->jedWin, (void*)win->widgetWinXRef[i][i1]->edWin );
		    }
		  else
		    {
	       /* printf( "(%6x-S-%6x)", win->widgetWinXRef[i][i1]->jedWin, win->widgetWinXRef[i][i1]->stWin ); */
		       printf( "S%p-%p ", (void*)win->widgetWinXRef[i][i1]->jedWin, (void*)win->widgetWinXRef[i][i1]->stWin );
		    }
	       }
	     else
	       {
	    /* printf( "(%6x-S-%6x)", NULL, NULL ); */
		  printf( "S%p-%p ", NULL, NULL );
	       }
	  }
	printf( "\n" );
     }
}
#endif

/************************************
* printWidgetWinXRefXY
*
* group: win, debug
*
*
* debug print: "x: %d, y: %d\n", x, y
*
*
************************************/

#if 0
static void
printWidgetWinXRefXY( int x, int y )
{
   int i, i1;
   JGtkWinType *win = JGtkWin;

   printf( "Ed Height: %d, Width: %d\n", win->edHeight, win->edWidth );
   printf( " Jed_Num_Screen_Cols: %d, Jed_Num_Screen_Rows: %d, Top_Window_SY: %d\n",
	   Jed_Num_Screen_Cols, Jed_Num_Screen_Rows, Top_Window_SY );

   for ( i = 0; i <= y; ++i )
     {
	printf( "height[%8d]: ", i );
	for ( i1 = 0; i1 <= x; ++i1 )
	  {
	     if ( win->widgetWinXRef[i][i1] )
	       {
		  if ( i != win->widgetWinXRef[i][i1]->jedWin->sy +
		       win->widgetWinXRef[i][i1]->jedWin->rows )
		    {
		       printf( "(%p-E-%p)", (void*)win->widgetWinXRef[i][i1]->jedWin, (void*)win->widgetWinXRef[i][i1]->edWin );
		    }
		  else
		    {
		       printf( "(%p-S-%p)",(void*) win->widgetWinXRef[i][i1]->jedWin, (void*)win->widgetWinXRef[i][i1]->stWin );
		    }
	       }
	     else
	       {
		  printf( "(%p-S-%p)", NULL, NULL );
	       }
	  }
	printf( "\n" );
     }
}
#endif

static void
printWidgetStruct(void)
{
   JGtkWinType *win = JGtkWin;
   GtkSubWinType *sw = win->subWins;
   GtkWidget *w, *pw;

   do
     {
	printGtkSubWin( sw );
	printf( "Parents: \n" );
	pw = sw->stWin;
	do
	  {
	     pw = gtk_widget_get_parent( pw );
	     printf( " %p(w: %d, h: %d", (void*)pw, pw->allocation.width, pw->allocation.height );
	     if ( pw != gtk_widget_get_parent( sw->stWin ) )
	       printf( ", paned-pos: %d), ", gtk_paned_get_position( GTK_PANED( pw ) ) );
	     else
	       printf( ", box), " );
	  }
	while( pw != win->appWTopEdWin );
	printf( "\n" );
	w = sw->edWin;
	printf( "EdWidget: %p: x: %d, y: %d, width: %d, height: %d\n", (void*)w,
		w->allocation.x, w->allocation.y,
		w->allocation.width, w->allocation.height );
	w = sw->stWin;
	printf( "StWidget: %p: x: %d, y: %d, width: %d, height: %d\n", (void*)w,
		w->allocation.x, w->allocation.y,
		w->allocation.width, w->allocation.height );
	printf( "=========================================\n" );
	sw = sw->next;
     }
   while ( sw != win->subWins );
}

void printGtkSubWin( GtkSubWinType * );

/************************************
* printSubWinSizes
*
* group: win, debug
*
*
* debug print: "(void)"
*
*
************************************/

#if 0
static void
printSubWinSizes(void)
{
   int sumHeight;
   JGtkWinType *win = JGtkWin;
   GtkSubWinType *sw = win->subWins;

   printf( "Font height: %d, width: %d\n", win->fontHeight, win->fontWidth );

   if ( win->appWInGrid )
     {
	printf( "appWInGrid(%p): %5d W, %5d H\n", (void*)win->appWInGrid,
		win->appWInGrid->allocation.width,
		win->appWInGrid->allocation.height );
     }

   /* printf( "Font height: %d, width: %d\n", win->fontHeight, win->fontWidth ); */

   /* sumHeight = win->fontHeight; */
   sumHeight = 0;
   if ( sw )
     do
     {
	/* printGtkSubWinSize( sw ); */
	printf( "SW: %p, edWin(%p): %5d W, %5d H, stWin(%p): %5d W, %5d H\n",
		(void*)sw,
		(void*)sw->edWin, sw->edWin->allocation.width, sw->edWin->allocation.height,
		(void*)sw->stWin, sw->stWin->allocation.width, sw->stWin->allocation.height );
	sumHeight += sw->edWin->allocation.height +
	  sw->stWin->allocation.height;
	sw = sw->next;
     }
   while ( sw != win->subWins );
   printf( "Sum widget height in pixel: %d\n", sumHeight );
}
#endif

/************************************
* printSubWins
*
* group: win, debug
*
*
* debug print: "(void)"
*
*
************************************/

void
printSubWins()
{
   int sumHeight;
   JGtkWinType *win = JGtkWin;
   GtkSubWinType *sw = win->subWins;

   printf( " Jed_Num_Screen_Cols: %d, Jed_Num_Screen_Rows: %d, Top_Window_SY: %d\n",
	   Jed_Num_Screen_Cols, Jed_Num_Screen_Rows, Top_Window_SY );

   if ( win->appWInGrid )
     {
	printf( "Win->appWInGrid: Height: %d, Width: %d\n",
		win->appWInGrid->allocation.height,
		win->appWInGrid->allocation.width );
     }

   printf( "Font height: %d, width: %d\n", win->fontHeight, win->fontWidth );

   sumHeight = win->fontHeight;

   if ( sw )
     do
     {
	printGtkSubWin( sw );
	sumHeight += sw->edWin->allocation.height +
	  sw->stWin->allocation.height;
	printf( "Sum widget height in pixel: %d\n", sumHeight );
	sw = sw->next;
     }
   while ( sw != win->subWins );
}

/************************************
* printJedWin
*
* group: win, debug
*
*
* debug print: "w(Window_Type): %p\n", w
*
*
************************************/

static void
printJedWin( Window_Type *w )
{
   printf( "Jed-Win: %p\n", (void*)w );
   printf( "    next: %p\n", (void*)w->next );
   printf( "    sx: %d, sy: %d\n", w->sx,
	   w->sy );
   printf( "    rows: %d, width: %d\n", w->rows,
	   w->width );
   printf( "    hscroll_column: %d\n", w->hscroll_column );
   printf( "    buffer: %p\n", (void*)w->buffer );
   printf( "    trashed: %d\n", w->trashed );
   printf( "    flags: %d\n", w->flags );
}

/************************************
* printGtkSubWin
*
* group: win, debug
*
*
* debug print: "sw(GtkSubWinType): %p\n", sw
*
*
************************************/

void
printGtkSubWin( GtkSubWinType *sw )
{
   printf( "GtkSubWinType: %p\n", (void*)sw );
   printJedWin( sw->jedWin );
   printf( "  Next: %p\n", (void*)sw->next );
   printf( "  EdWidget: %p\n", (void*)sw->edWin );
   printf( "    Win:    %p\n", (void*)sw->edWin->window );
   printf( "      Height: %d\n", sw->edWin->allocation.height );
   printf( "      Width:  %d\n", sw->edWin->allocation.width );
   printf( "  StWidget: %p\n", (void*)sw->stWin );
   printf( "    Win:    %p\n", (void*)sw->stWin->window );
   printf( "      Height: %d\n", sw->stWin->allocation.height );
   printf( "      Width:  %d\n", sw->stWin->allocation.width );
   printf( "  SbWidget: %p\n", (void*)sw->sbWin );
   printf( "  SbAdj:    %p\n", (void*)sw->sbAdj );
   printf( "  ppY: %d, ppX: %d\n", sw->ppY, sw->ppX );
   printf( "  ppHeight: %d, ppWidth: %d\n", sw->ppHeight, sw->ppWidth );
}

/************************************
* modLineWidgetWinXRef
*
* debug print: "dArr(GtkSubWinType ***): %p, sArr(GtkSubWinType ***): %p, x: %d, y: %d, w: %d, source: %d", dArr, sArr, x, y, w, source
*
************************************/

static void
modLineWidgetWinXRef( GtkSubWinType ***dArr, GtkSubWinType ***sArr, int x, int y, int w, int source )
{
   int i;
   for ( i = 0; i < w; ++i )
     dArr[y][x + i] = sArr[source][x + i];

   /* printWidgetWinXRef(); */
   /* printSubWins(); */
}

/************************************
* updEdWidgetWinXRef
*
* group: win
*
* debug print: "WV: %p, y: %d, x: %d\n", wv, y, x
*
************************************/

void
jgtk_updEdWidgetWinXRef( Window_Type *wv, int y, int h )
{
   int m;
   /* GtkWidget *w = ( GtkWidget * ) wv->window; */
   JGtkWinType *win = JGtkWin;
   GtkSubWinType *sw = win->subWins;

   do
     {
	if ( sw->jedWin == wv )
	  {
	     break;
	  }
	sw = sw->next;
     }
   while ( sw != win->subWins );

   modWidgetWinXRef( win->widgetWinXRef, sw, 0, y, win->edWidth, h );

   /***********
   printWidgetWinXRef();
   printSubWins();
   *************/
}

/************************************
* modWidgetWinXRef
*
* group: debug
*
* debug print: "a(GtkSubWinType ***): %p, val: %p, x: %d, y: %d, w: %d, h: %d\n", a, val, x, y, w, h
*
************************************/

static void
modWidgetWinXRef( GtkSubWinType ***a, GtkSubWinType *val, int x, int y, int w, int h )
{
   int i, i1;

   for ( i = 0; i < h; ++i )
     for ( i1 = 0; i1 < w; ++i1 )
       {
	  a[y + i][x + i1] = val;
       }

   /* printWidgetWinXRef(); */
   /* printSubWins(); */
}

/************************************
* adjustSubWinPanes
*
* debug print:
*
************************************/

static void
adjustSubWinPanes(void)
{
   /* int i = 0; */
   int gutPos, accumHeight;
   JGtkWinType *win = JGtkWin;
   GtkSubWinType *sw = win->subWins, *savSW, *oldSW;
   GtkWidget *parBox, *parPane, *actPane;

   oldSW = sw;
   sw = sw->next;

   if ( gtk_widget_get_parent( sw->stWin ) != win->appWTopEdWin )
     {
	parBox = gtk_widget_get_parent( sw->stWin );

      /* Dbp2( "SW: %p, ParBox: %p\n", sw, parBox ); */

	actPane = gtk_widget_get_parent( parBox );

	gutPos = ( oldSW->jedWin->rows + 1 ) * win->fontHeight;

	accumHeight = gutPos + win->gutSize;

	gtk_paned_set_position( GTK_PANED( gtk_widget_get_parent( parBox ) ), gutPos );

	savSW = sw;

	oldSW = sw;
	sw = sw->next;

	if ( sw != win->subWins )
	  do
	  {
	     parBox = gtk_widget_get_parent( sw->stWin );

	     parPane = gtk_widget_get_parent( parBox );

	     gutPos = accumHeight + ( oldSW->jedWin->rows + 1 ) * win->fontHeight;

	     gtk_paned_set_position( GTK_PANED( parPane ), gutPos );

	     accumHeight = gutPos + win->gutSize;

	     savSW = sw;

	     parBox = gtk_widget_get_parent( sw->next->stWin );
	     oldSW = sw;
	     sw = sw->next;
	  }
	while ( sw != win->subWins );
     }
}

/************************************
* jGtkCheckEdSize
*
*   Resizes widgetWinXRef if necessary to height, width
*
* group: win
*
* debug print: "Height: %d, Width: %d\n", height, width
*
************************************/

void
jGtkCheckEdSize( int height, int width )
{
   int n, m;
   int cpyOld;
   JGtkWinType *win = JGtkWin;
   int newHeight = height > win->maxEdHeight ? height : win->maxEdHeight;
   int newWidth  = width  > win->maxEdWidth  ? width  : win->maxEdWidth;
   GtkSubWinType ***newEdLines;
   GtkSubWinType **tmpLine;

   /* printSubWins(); */

   if ( ! ( height && width && win->edWidth && win->edHeight ) ) return;

   if ( width > win->maxEdWidth )
     {
      /* Dbp1( "Window width (%d) bigger maxEdWidth!!!\n", width ); */
	if ( height > win->maxEdHeight )
	  {
	 /* Dbp1( "Creating new edbuffer\n", 1 ); */
	     newEdLines = ( GtkSubWinType *** ) SLmalloc( sizeof( GtkSubWinType ** ) * width );
	  }
	else
	  {
	     newEdLines = win->widgetWinXRef;
	  }

      /* if height smaller than win->edHeight backup copy lines of last window */
      /* below win->edHeight border. Should the lower border be part of different */
      /* window, function calling this function will do one_window-call!!! */

	if ( height < win->edHeight )
	  {
	     for ( m = 0; m < win->edWidth; ++m )
	       {
		  win->widgetWinXRef[height - 1][m] = win->widgetWinXRef[win->edWidth - 1][m];
	       }

	     win->edHeight = height;
	  }

      /* if height smaller than win->maxEdHeight simply create some new buffer lines */
      /* without copying anything. */

	for ( n = newHeight - 1; n > height; --n )
	  {
	     tmpLine = ( GtkSubWinType ** ) SLmalloc( sizeof( GtkSubWinType * ) * width );
	     SLfree( ( char * ) win->widgetWinXRef[n] );
	     win->widgetWinXRef[n] = tmpLine;
	  }

	for ( n = height - 1; n >= 0; --n )
	  {
	     tmpLine = ( GtkSubWinType ** ) SLmalloc( sizeof( GtkSubWinType * ) * width );

	     cpyOld = n;

	     if ( n == height - 1 ) cpyOld = win->edHeight - 1;
	     else if ( win->edHeight > 1 && win->edHeight - 2 <= n && n <= height - 2 ) cpyOld = win->edHeight - 2;
	     else cpyOld = n;

	     for ( m = 0; m < width; ++m )
	       {
		  tmpLine[m] = m < win->edWidth ? win->widgetWinXRef[cpyOld][m] : win->widgetWinXRef[cpyOld][win->edWidth - 1];
	       }

	     if ( n < win->maxEdHeight ) SLfree( ( char * ) win->widgetWinXRef[n] );
	     newEdLines[n] = tmpLine;
	  }

	SLfree( ( char * ) win->widgetWinXRef );

	win->widgetWinXRef = newEdLines;
     }
   else if ( height > win->maxEdHeight )
     {
      /* Dbp1( "Window height (%d) greater maxEdHeight\n", height ); */
	newEdLines = ( GtkSubWinType *** ) SLmalloc( sizeof( GtkSubWinType ** ) * newHeight );

	for ( n = height - 1; n >= 0; --n )
	  {
	     if ( n >= win->maxEdHeight )
	       tmpLine = ( GtkSubWinType ** ) SLmalloc( sizeof( GtkSubWinType * ) * newWidth );
	     else
	       tmpLine = win->widgetWinXRef[n];

	     if ( n == height - 1 ) cpyOld = win->edHeight - 1;
	     else if ( win->edHeight > 1 && win->edHeight - 2 <= n && n <= height - 2 ) cpyOld = win->edHeight - 2;
	     else cpyOld = n;

	     if ( win->edWidth )
	       for ( m = 0; m < width; ++m )
		 {
		    tmpLine[m] = m < win->edWidth ? win->widgetWinXRef[cpyOld][m] : win->widgetWinXRef[cpyOld][win->edWidth - 1];
		 }

	 /* if ( n < win->maxEdHeight ) SLfree( win->widgetWinXRef[n] ); */
	     newEdLines[n] = tmpLine;
	  }

	SLfree( ( char * ) win->widgetWinXRef );

	win->widgetWinXRef = newEdLines;
     }

   win->maxEdWidth = newWidth;
   win->maxEdHeight = newHeight;

   win->edWidth = width;
   win->edHeight = height;

   /* Dbp( "Win->edWidth: %d\n", win->edWidth ); */
   /* Dbp( "Win->edHeight: %d\n", win->edHeight ); */
   /* printWidgetWinXRef(); */
}

/************************************
* jGtkWidenEd
*
* debug print:
*
************************************/

void
jGtkWidenEd( int height, int newW, int oldW )
{
   int i, i1, n = oldW - 1;
   GtkSubWinType ***img = JGtkWin->widgetWinXRef;

   for ( i = 0; i < height; ++i )
     for ( i1 = oldW; i1 < newW; ++i1 )
       {
	  img[i][i1] = img[i][n];
       }
}

/************************************
* updateWidgetWinXRef
*
*  Overwrites widgetWinXRef with sw for new sy, sx, nHeight, nWidth
*  sets subWinParameter sx, sy, width, height accordingly
*
* group: win
*
* debug print: "sw: %p, sy: %d, sx: %d, nHeight: %d, nWidth: %d\n", sw, sy, sx, nHeight, nWidth
*
************************************/

void
updateWidgetWinXRef( GtkSubWinType *sw, int sy, int sx, int nHeight, int nWidth )
{
   int i, i1;
   int minY, maxY;
   JGtkWinType *win = JGtkWin;

   /*********
   Dbp1( "--%d-------------------------------------------------\n", 1 );
   printWidgetWinXRef();
   printSubWins();
   Dbp1( "--%d-------------------------------------------------\n", 1 );
   *********/

   /* printGtkSubWin( sw ); */

   if ( sy > sw->jedWin->sy + sw->jedWin->rows || sy + nHeight < sw->jedWin->sy ||
	sx > sw->jedWin->sx + sw->jedWin->width || sx + nWidth  < sw->jedWin->sx )
     {
      /* Dbp( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", 1 ); */

	for ( i = sy; i < sy + nHeight; ++i )
	  {
	 /* Dbp( "i: %d\n", i ); */
	     for ( i1 = sx; i1 < sx + nWidth; ++i1 )
	       {
	    /* Dbp( "i1: %d\n", i1 ); */
		  win->widgetWinXRef[i][i1] = sw;
	       }
	  }
     }
   else
     {
	for ( i = sy; i < sw->jedWin->sy; ++i )
	  {
	 /* Dbp( "i: %d\n", i ); */
	     for ( i1 = sx; i1 < sx + nWidth; ++i1 )
	       {
	    /* Dbp( "i1: %d\n", i1 ); */
		  win->widgetWinXRef[i][i1] = sw;
	       }
	  }

	minY = sy < sw->jedWin->sy ? sw->jedWin->sy : sy;
	maxY = sy + nHeight < sw->jedWin->sy + sw->jedWin->rows + 1 ? sy + nHeight : sw->jedWin->sy + sw->jedWin->rows + 1;

      /* Dbp( "Min Y: %d\n", minY ); */
      /* Dbp( "Max Y: %d\n", maxY ); */

	for ( i = minY; i < maxY; ++i )
	  {
	 /* Dbp( "i: %d\n", i ); */
	     for ( i1 = sx; i1 < sw->jedWin->sx; ++i1 )
	       {
	    /* Dbp( "i1: %d\n", i1 ); */
		  win->widgetWinXRef[i][i1] = sw;
	       }

	     for ( i1 = sw->jedWin->sx + sw->jedWin->width; i1 < sx + nWidth; ++i1 )
	       {
	    /* Dbp( "i1: %d\n", i1 ); */
		  win->widgetWinXRef[i][i1] = sw;
	       }
	  }

	for ( i = sw->jedWin->sy + sw->jedWin->rows + 1; i < sy + nHeight; ++i )
	  {
	 /* Dbp( "i: %d\n", i ); */
	     for ( i1 = sx; i1 < sx + nWidth; ++i1 )
	       {
	    /* Dbp( "i1: %d\n", i1 ); */
		  win->widgetWinXRef[i][i1] = sw;
	       }
	  }
     }
   sw->jedWin->sy = sy;
   sw->jedWin->sx = sx;
   sw->jedWin->rows = nHeight - 1;
   sw->jedWin->width = nWidth;

   /**********
   Dbp1( "==%d==========================================\n", 2 );
   printWidgetWinXRef();
   printSubWins();
   Dbp1( "==%d==========================================\n", 2 );
   *******/

   /* printGtkSubWin( sw ); */
}

void
jgtk_updMiniWinWidgetWinXRef( int n )
{
   int i = Jed_Num_Screen_Cols - 1;
   JGtkWinType *win = JGtkWin;

   for ( ; i >= 0; --i )
     {
	win->widgetWinXRef[n][i] = win->miniWinSW;
     }
}

/************************************
* splitEdWin
*
* group: win
*
*
* debug print: "Widget (wv): %p, sy: %d, sx: %d, height: %d, scroll: %d, width: %d\n", wv, sy, sx, height, scroll, width
*
************************************/

void
jgtk_splitEdWin( Window_Type *wv, Window_Type *newJedW, int sy, int sx, int height, int scroll, int width )
{
   int n, m;
   int isFirstPane = 1;
   JGtkWinType *win = JGtkWin;
   GtkWidget *wEd = win->widgetWinXRef[sy][sx]->edWin;
   GtkWidget *wSt = win->widgetWinXRef[sy + height][sx]->stWin;
   GtkWidget *curEdBox = gtk_widget_get_parent( gtk_widget_get_parent( wEd ) );
   GtkWidget *parW = gtk_widget_get_parent( curEdBox );
   GtkWidget *newPaned = gtk_vpaned_new();
   GtkSubWinType *sw, *newSW = createGtkWinTmpl( ( GtkSubWinType * ) SLmalloc( sizeof( GtkSubWinType ) ) );
   /* EdWinWidgetType newW = createGtkWinTmpl( newSW ); */
   GtkWidget *edBox = gtk_widget_get_parent( gtk_widget_get_parent( newSW->edWin ) );

   newSW->edX =
     newSW->edY =
     newSW->edHeight =
     newSW->edWidth = 0;

   /************
   printWidgetWinXRef();
   printSubWins();
   ********/

   g_signal_connect( G_OBJECT( newPaned ),
		     "button_press_event",
		     G_CALLBACK( eventCallback ),
		     NULL );

   g_signal_connect( G_OBJECT( newPaned ),
		     "button_release_event",
		     G_CALLBACK( eventCallback ),
		     NULL );

   /* Dbp2( "win->gutSize: %d, win->numWin: %d\n", win->gutSize, win->numWin ); */
   if ( !win->gutSize && win->subWins != win->subWins->next )
     {
	GtkWidget *topPaned = win->appWTopEdWin;
	win->gutSize =
	  topPaned->allocation.height -
	  gtk_paned_get_child1( GTK_PANED( topPaned ) )->allocation.height -
	  gtk_paned_get_child2( GTK_PANED( topPaned ) )->allocation.height;
      /* Dbp1( "Guttersize: %d\n", win->gutSize ); */
     }

   /* printWidgetStruct(); */

   updateGtkWin = 0;

   /*********************************************
   * Currently, as of gtk+ version 2.12.0 there is
   * no function to return the gutter size. So
   * we have to compute them ourself.
   *********************************************/

   if ( curEdBox != win->appWTopEdWin )
     {
	isFirstPane = ( curEdBox == gtk_paned_get_child1( GTK_PANED( parW ) ) );
     }

   /*********
   gtk_paned_add1( GTK_PANED( newPaned ), curEdBox );

   g_object_unref( curEdBox );
   ***********/

   gtk_paned_add2( GTK_PANED( newPaned ), edBox );

   if ( isFirstPane ) /* also true for curEdBox == win->appWTopEdWin */
     {
	g_object_ref( curEdBox );
	gtk_container_remove( GTK_CONTAINER( parW ), curEdBox );
	gtk_paned_add1( GTK_PANED( newPaned ), curEdBox );
	g_object_unref( curEdBox );
	if ( curEdBox == win->appWTopEdWin )
	  {
	     gtk_box_pack_start( GTK_BOX( win->appWInGrid ), newPaned, True, True, 0 );
	     win->appWTopEdWin = newPaned;
	 /* Dbp4( "Splitting top window: old: %p, newBox: %p, new.ed: %p, new.status: %p\n", curEdBox, edBox, newW.ed, newW.st ); */
	  }
	else
	  {
	     gtk_paned_add1( GTK_PANED( parW ), newPaned );
         /* Dbp2( "Splitting first pane: old: %p, new: %p\n", curEdBox, edBox ); */
	  }
     }
   else
     {
	GtkWidget *gParW = gtk_widget_get_parent( parW );

      /* Db; */

	if ( parW != win->appWTopEdWin )
	  {
	     isFirstPane = ( parW == gtk_paned_get_child1( GTK_PANED( gParW ) ) );
	  }

      /* Db; */

	g_object_ref( parW );
	gtk_container_remove( GTK_CONTAINER( gParW ), parW );
	gtk_paned_add1( GTK_PANED( newPaned ), parW );
	g_object_unref( parW );

      /* Db; */

	if ( parW == win->appWTopEdWin )
	  {
	     gtk_box_pack_start( GTK_BOX( win->appWInGrid ), newPaned, True, True, 0 );
	     win->appWTopEdWin = newPaned;
	  }
	else
	  {
	     gtk_paned_add1( GTK_PANED( gParW ), newPaned );
	  }
     }

   /*******
   Db;

   printWidgetStruct();
   **********/

   sw = win->subWins;

   do
     {
	if ( sw->jedWin == wv )
	  {
	     break;
	  }
	sw = sw->next;
     }
   while ( sw != win->subWins );

   newSW->jedWin = newJedW;
   /* newSW->edWin = newW.ed; */
   /* newSW->stWin = newW.st; */
   /* newSW->sbWin = newW.sb; */

   newSW->sbAdj = gtk_range_get_adjustment( GTK_RANGE( newSW->sbWin ) );
   newSW->numLineSav =
     newSW->curLineSav =
     newSW->winHeightSav = 0;

   newSW->ppHeight = 0;
   newSW->next = sw->next;
   sw->next = newSW;

   win->numWin++;

   /************
   gtk_widget_show_all( win->appWTopEdWin );

   if ( gtk_events_pending() )
     {
      gtk_main_iteration();
      }
   *****************/

   /********/
   /* Db; */

   /* printWidgetStruct(); */
   /**************/

   modWidgetWinXRef( win->widgetWinXRef, newSW, sx, sy, width, height + 1 );

   /*****
   printf( "vvvvvv111111111111111111111111111111111111111111111111111111vvvvvv\n" );
   printWidgetWinXRef();
   printSubWins();
   printf( "^^^^^^111111111111111111111111111111111111111111111111111111^^^^^^\n" );
   ******/
   /* Db; */

   adjustSubWinPanes();

   /* gtk_widget_show_all( parW ); */

   /* gtk_widget_show_all( newPaned ); */

   gtk_widget_grab_focus( newSW->edWin );

   /*********
   Db;

   printWidgetStruct();
   ************/
   /*******/
   /* printf( "vvvvvv2222222222222222222222222222222222222222222222222222222vvvvvv\n" ); */
   /* updateInGridHeight(); */
   /* printf( "^^^^^^2222222222222222222222222222222222222222222222222222222^^^^^^\n" ); */
   /*****/

   /***********
   Db;
   printWidgetStruct();
   **********/

   /* adjustSubWinPanes(); */
   updateGtkWin = 1;
   gtk_widget_show_all( win->appWTopEdWin );

   jGtkSetFGBGColor( newSW->edWin, JNORMAL_COLOR );
   jGtkSetFGBGColor( newSW->stWin, JNORMAL_COLOR );
}

/************************************
* updOWEdWin
*
* group: win
*
* debug print: "WV: %p, nsy: %d, nsx: %d, nheight: %d, nwidth: %d\n", wv, nsy, nsx, nheight, nwidth
*
************************************/

void
jgtk_updOWEdWin( Window_Type *wv, int nsy, int nsx, int nheight, int nwidth )
{
   int n, m;
   JGtkWinType *win = JGtkWin;
   GtkSubWinType *winList = win->subWins;
   GtkWidget *actEdWin = win->widgetWinXRef[wv->sy][wv->sx]->edWin;
   GtkWidget *ow = gtk_widget_get_parent( gtk_widget_get_parent( actEdWin ) );
   GtkWidget *parOW = gtk_widget_get_parent( ow );
   /* GtkWidget *childOW = gtk_paned_get_child2( GTK_PANED( ow ) ); */
   GtkSubWinType *sw, *owSW, *delSW;

   /* printf( "updOWEdWin: ow: %p\n", ow ); */

   /* if ( childOW ) gtk_widget_destroy( childOW ); */

   /* Updating list of subWins ************/

   sw = win->subWins;

   do
     {
	if ( sw->jedWin == wv )
	  {
	     owSW = sw;
	     sw = sw->next;
	  }
	else
	  {
	     delSW = sw;
	     sw = sw->next;
	     SLfree( ( char * ) delSW );
	  }
     }
   while ( sw != win->subWins );

   owSW->ppHeight = 0;
   owSW->next = owSW;
   win->subWins = owSW;
   win->numWin = 1;

   /* Updating widgetWinXRef **************/

   modWidgetWinXRef( win->widgetWinXRef, owSW, nsx, nsy, nwidth, nheight );

   /**************
   for ( m = 0; m <= nheight; ++m )
     for ( n = 0; n < nwidth; ++n )
       win->widgetWinXRef[nsy + m][nsx + n] = owSW;
   ***************/

   if ( ow != win->appWTopEdWin )
     {
      /* printf( "Removing one window from pane: %p\n", parOW ); */
	updateGtkWin = 0;
	g_object_ref( ow );
	gtk_container_remove( GTK_CONTAINER( parOW ), ow );
	gtk_widget_destroy( win->appWTopEdWin );
	win->appWTopEdWin = ow;
	gtk_box_pack_start( GTK_BOX( win->appWInGrid ), ow, True, True, 0 );
	g_object_unref( ow );
	gtk_widget_show_all( win->appWTopEdWin );
	gtk_widget_grab_focus( actEdWin );
	updateGtkWin = 1;
     }

   /**********
   printf( "(updOWEdWin): -----vvv---------------\n" );
   printWidgetWinXRef();
   printf( "(updOWEdWin): -----^^^---------------\n" );
   ********/

}

/************************************
* createTopEdWin
*
* group: win
*
*
* debug print: "w: %p, sy: %d, sx: %d, height: %d, scroll: %d, width: %d\n", w, sy, sx, height, scroll, width
*
************************************/

void
jgtk_createTopEdWin( Window_Type *w, int sy, int sx, int height, int scroll, int width )
{
   int n, m;
   JGtkWinType *win = JGtkWin;
   GtkSubWinType *newSW = createGtkWinTmpl( ( GtkSubWinType * ) SLmalloc( sizeof( GtkSubWinType ) ) );

   newSW->next = newSW;
   newSW->jedWin = w;

   newSW->sbAdj = gtk_range_get_adjustment( GTK_RANGE( newSW->sbWin ) );
   newSW->numLineSav =
     newSW->curLineSav =
     newSW->winHeightSav = 0;

   newSW->ppHeight = 0;
   win->subWins = newSW;

   jGtkCheckEdSize( win->edHeight + sy + height + 1, sx + width );

   if ( win->appWMiniW )
     {
	modLineWidgetWinXRef( win->widgetWinXRef, win->widgetWinXRef, 0, win->edHeight + sy + height, sx + width, 0 );
     }

   if ( win->appWTopEdWin )
     {
	gtk_widget_destroy( win->appWTopEdWin );
     }

   win->appWTopEdWin = gtk_widget_get_parent( gtk_widget_get_parent( newSW->edWin ) );

   gtk_box_pack_start( GTK_BOX( win->appWInGrid ), win->appWTopEdWin, True, True, 0 );

   /* printf( "createTopEdWin: newDrawArea: %p\n", win->appWTopEdWin ); */

   modWidgetWinXRef( win->widgetWinXRef, newSW, sx, sy, width, height + 1 );
#if 0
   for ( m = 0; m <= height; ++m )
     for ( n = 0; n < width; ++n )
       {
	/* printf( "Height: %d, width: %d, n: %d, sy: %d, sx: %d\n", height, width, n, sy, sx ); */
	/* printf( "win->widgetWinXRef: %p\n", win->widgetWinXRef ); */
	  win->widgetWinXRef[sy + m][sx + n] = newSW;
       }
#endif
   /* Db; */
   /* printWidgetWinXRef(); */

   gtk_widget_show_all( win->appWTopEdWin );

   gtk_widget_grab_focus( newSW->edWin );

   jGtkSetFGBGColor( newSW->edWin, JNORMAL_COLOR );
   jGtkSetFGBGColor( newSW->stWin, JNORMAL_COLOR );

   /* Db; */
   /* printWidgetWinXRef(); */
}

/************************************
* delEdWin
*
* group: win
*
*
* debug print: "WV: %p, SubW: %p, sy: %d, sx: %d, height: %d, width: %d, lastLine: %d\n", wv, subW, sy, sx, height, width, lastLine
*
************************************/

void
jgtk_delEdWin( Window_Type *wv, Window_Type *subW, int sy, int sx, int height, int width, int lastLine )
{
   int m, n;
   int isFirstPane;
   JGtkWinType *win = JGtkWin;
   /* GtkWidget *w = ( GtkWidget * ) wv->window; */
   GtkWidget *wBox = gtk_widget_get_parent( gtk_widget_get_parent( win->widgetWinXRef[wv->sy][wv->sx]->edWin ) );
   GtkWidget *parW = gtk_widget_get_parent( wBox );
   GtkWidget *gParW; /* Grandparent */
   GtkWidget *remW;
   GtkSubWinType *subWSW = win->widgetWinXRef[subW->sy][subW->sx];
   GtkSubWinType *wvSW = win->widgetWinXRef[wv->sy][wv->sx];
   GtkSubWinType *sw;

   if ( wBox == win->appWTopEdWin )
     {
	return;
     }

   if ( subW->sy == wv->sy )
     {
	subWSW = win->widgetWinXRef[subW->sy + subW->rows - 1][subW->sx];
     }

   if ( subWSW->next == wvSW )
     {
	subWSW->next = wvSW->next;
     }
   else
     {
	sw = win->subWins;

	do
	  {
	     if ( sw->next == wvSW ) break;
	     sw = sw->next;
	  }
	while ( sw != win->subWins );

	sw->next = subWSW;
     }

   SLfree( ( char * ) wvSW );

   if ( wvSW == win->subWins )
     {
	win->subWins = subWSW;
     }

   --win->numWin;

   modWidgetWinXRef( win->widgetWinXRef, subWSW, sx, sy, width, height );

   /***************
   for ( n = 0; n < height; ++n )
     for ( m = 0; m < width; ++m )
       {
        win->widgetWinXRef[sy + n][sx + m] = subWSW;
	}
   ****************/

   if ( wv->sy == subW->sy )
     {
	modWidgetWinXRef( win->widgetWinXRef, subWSW, wv->sx, wv->sy + wv->rows, width, 1 );
      /****************
      for ( m = 0; m < width; ++m )
	{
	 win->widgetWinXRef[wv->sy + wv->rows][wv->sx + m] = subWSW;
	 }
      ******************/
     }
   else
     {
	modWidgetWinXRef( win->widgetWinXRef, subWSW, wv->sx, subW->sy + subW->rows, width, 1 );
#if 0
	for ( m = 0; m < width; ++m )
	  {
	 /* win->widgetWinXRef[wv->sy - 1][wv->sx + m].jedWin = subW; */
	     win->widgetWinXRef[subW->sy + subW->rows][wv->sx + m] = subWSW;
	  }
#endif
     }

   updateGtkWin = 0;

   if ( wBox == gtk_paned_get_child1( GTK_PANED( parW ) ) )
     {
	remW = gtk_paned_get_child2( GTK_PANED( parW ) );
     }
   else
     {
	remW = gtk_paned_get_child1( GTK_PANED( parW ) );
     }

   g_object_ref( remW );
   gtk_container_remove( GTK_CONTAINER( parW ), remW );

   if ( parW == win->appWTopEdWin )
     {
	gtk_widget_destroy( parW );
	win->appWTopEdWin = remW;
	gtk_box_pack_start( GTK_BOX( win->appWInGrid ), remW, True, True, 0 );
	g_object_unref( remW );
     }
   else
     {
	gParW = gtk_widget_get_parent( parW );
	isFirstPane = ( parW == gtk_paned_get_child1( GTK_PANED( gParW ) ) );
	gtk_widget_destroy( parW );
	if ( isFirstPane )
	  {
	     gtk_paned_add1( GTK_PANED( gParW ), remW );
	  }
	else
	  {
	     gtk_paned_add2( GTK_PANED( gParW ), remW );
	  }
     }

   gtk_widget_show_all( remW );

   gtk_widget_grab_focus( subWSW->edWin );

   updateGtkWin = 1;

   /******************
   printWidgetWinXRef();
   ****************/

}

/************************************
* createEditorMiniWin
*
* group: win
*
*
* debug print: "mw(Window_Type): %p\n", mw
*
*
************************************/

int
jgtk_createEditorMiniWin( Window_Type *mw )
{
   int i;
   JGtkWinType *win = JGtkWin;
   GtkWidget *newMiniWin = createGtkMiniWin();

   jGtkCheckEdSize( 1, mw->width );

   if ( win->appWMiniW )
     {
	gtk_widget_destroy( win->appWMiniW );
     }

   win->appWMiniW = newMiniWin;

   gtk_widget_set_size_request( win->appWMiniW, -1, win->fontHeight );

   gtk_box_pack_start( GTK_BOX( win->appWEdGrid ), newMiniWin, False, False, 0 );

   win->edWidth = mw->width;
   win->edHeight = 1;

   gtk_widget_show_all( newMiniWin );

   gtk_widget_grab_focus( newMiniWin );

   win->miniWinSW = ( GtkSubWinType * ) SLmalloc( sizeof( GtkSubWinType ) );

   win->miniWinSW->jedWin = mw;
   win->miniWinSW->next = NULL;
   win->miniWinSW->edWin = newMiniWin;
   win->miniWinSW->stWin = newMiniWin;
   win->miniWinSW->ppHeight = 0;

   modWidgetWinXRef( win->widgetWinXRef, win->miniWinSW, 0, win->edHeight - 1, win->edWidth, 1 );

   for ( i = 0; i < JMAX_COLORS; ++i )
     {
	DefaultGtkGCInfo[i].gc = gdk_gc_new( win->appWMiniW->window );
     }

   /* Db; */
   /* printf( "################################################################\n" ); */
   /*printWidgetWinXRef(); */
   /* printGtkSubWin( sw ); */
   /* printf( "################################################################\n" ); */

   /* Return( newMiniWin ); */
   
   return 0;
}

/************************************
* jGtkWinDestroy
*
* group: app
*
*
* debug print:
*
*
************************************/

static void
jGtkWinDestroy( GtkWidget *widget,
		gpointer data )
{
   exit( 0 );
}

/************************************
* jGtkSetWinSizes
*
* debug print:
*
************************************/

void jGtkSetWinSizes(void)
{
   JGtkWinType   *win = JGtkWin;
   GtkSubWinType *sw = win->subWins;

   adjustSubWinPanes();

   return;

   do
     {
      /* Dbp1( "Height: %d\n", ( sw->jedWin->rows + 1 ) * win->fontHeight); */
	gtk_widget_set_size_request( sw->edWin, -1, sw->jedWin->rows * win->fontHeight );
	sw = sw->next;
     }
   while ( sw != win->subWins );

}

/************************************
* updateHeightOfSubWinStatusWins
*
* debug print:
*
************************************/

static void
updateHeightOfSubWinStatusWins(void)
{

   JGtkWinType *win = JGtkWin;
   GtkSubWinType *sw = win->subWins;

   if ( sw )
     do
     {
	gtk_widget_set_size_request( sw->stWin, -1, win->fontHeight );
	sw = sw->next;
     }
   while ( sw != win->subWins );

   if ( win->miniWinSW )
     {
	gtk_widget_set_size_request( win->miniWinSW->edWin, -1, win->fontHeight );
     }
}

/************************************
* loadFont
*
* debug print: "FontName: |%s|\n", fontName
*
************************************/

static void
loadFont( char *fontName )
{
   JGtkWinType *win = JGtkWin;

   SLfree( win->fontName );
   win->fontName = SLmalloc( strlen( fontName ) + 1 );
   strcpy( win->fontName, fontName );

   win->fontDescription = pango_font_description_from_string( win->fontName );
   win->fontMetrics = pango_context_get_metrics( win->fontContext,
						 win->fontDescription,
						 NULL );
   win->fontAscent = pango_font_metrics_get_ascent( win->fontMetrics ) / PANGO_SCALE;
   win->fontDescent = pango_font_metrics_get_descent( win->fontMetrics ) / PANGO_SCALE;

   /* printf( "Scale: %d; Ascent: %d, Descent: %d\n", PANGO_SCALE, win->fontAscent, win->fontDescent ); */

   win->fontHeight = win->fontAscent + win->fontDescent;

   win->fontWidth = pango_font_metrics_get_approximate_char_width( win->fontMetrics ) / PANGO_SCALE;
   /* printf( "Scale: %d; charWidth: %d\n", PANGO_SCALE, win->fontWidth ); */

   updateHeightOfSubWinStatusWins();

   jed_init_display();
   jed_redraw_screen( 1 );
}

static void
jGtkMenuCreateDef( JGtkWinType *win )
{

   jgtk_initToolbarArray();

   jgtk_initMenubarStruct();

   if ( !win->appWMenuBar )
     {
      /* Dbp1( "win->appWMenuBar: %p\n", win->appWMenuBar ); */
	win->appWMenuBar = gtk_menu_bar_new();

      /* Db; */

	win->appWMenu = gtk_menu_new();

	win->appWMenuItemOpen = gtk_menu_item_new_with_label( "Open" );
	win->appWMenuItemSave = gtk_menu_item_new_with_label( "Save" );
	win->appWMenuItemClose = gtk_menu_item_new_with_label( "Close" );

	gtk_widget_show( win->appWMenuItemOpen );
	gtk_widget_show( win->appWMenuItemSave );
	gtk_widget_show( win->appWMenuItemClose );

	gtk_menu_shell_append( GTK_MENU_SHELL( win->appWMenu ), win->appWMenuItemOpen );
	gtk_menu_shell_append( GTK_MENU_SHELL( win->appWMenu ), win->appWMenuItemSave );
	gtk_menu_shell_append( GTK_MENU_SHELL( win->appWMenu ), win->appWMenuItemClose );

	win->appWMenuItem = gtk_menu_item_new_with_label( "File" );

	gtk_menu_item_set_submenu( GTK_MENU_ITEM( win->appWMenuItem ), win->appWMenu );

	gtk_menu_bar_append( GTK_MENU_BAR( win->appWMenuBar ), win->appWMenuItem );
     }

   gtk_widget_show_all( win->appWMenuBar );
}

/************************************
* createGtkMainWindow
*
* group: win
*
*
* debug print: "(void)"
*
*
************************************/

static void
createGtkMainWindow(void)
{
   GtkWidget *tmpFrame;
   JGtkWinType  *win = JGtkWin;
   GdkColor white = { 0, 0xffff, 0xffff, 0xffff };
   /********
   GParamSpec *param = g_param_spec_int( "testval",
					 "testing",
					 "test test test",
					 -10, 1000, 0,
					 G_PARAM_READABLE );
   GValue val;
   ******/

   win->appW = gtk_window_new( GTK_WINDOW_TOPLEVEL );

   /*********
   gtk_widget_class_install_style_property( GTK_WIDGET_CLASS( G_OBJECT_GET_CLASS( win->appW ) ),
					    param );

   gtk_rc_parse( "/root/.gtkjed" );

   g_value_init( &val, G_PARAM_SPEC_VALUE_TYPE( param ) );

   gtk_widget_style_get_property( win->appW, "testval", &val );

   Dbp1( "Value: %d\n", g_value_get_int( &val ) );
   ***********/

   gtk_window_set_default_size( GTK_WINDOW( win->appW ), 100, 300 );

   g_signal_connect( G_OBJECT( win->appW ),
		     "destroy",
		     G_CALLBACK( jGtkWinDestroy ),
		     NULL );

   win->appWGrid = gtk_vbox_new( False, 0 );

   win->appWMenuBar = NULL;

   jGtkMenuCreateDef( win );

   gtk_box_pack_start( GTK_BOX( win->appWGrid ), win->appWMenuBar, False, False, 0 );

   win->appWTbGrid = gtk_vbox_new( False, 0 );
   gtk_box_pack_start( GTK_BOX( win->appWGrid ), win->appWTbGrid, False, False, 0 );

   win->appWEdGrid = gtk_vbox_new( False, 0 );
   gtk_box_pack_start( GTK_BOX( win->appWGrid ), win->appWEdGrid, True, True, 0 );

   g_signal_connect( G_OBJECT( win->appWEdGrid ),
		     "configure_event",
		     G_CALLBACK( eventCallback ), NULL );

   win->widgetWinXRef = NULL;

   win->appWInGrid = gtk_vbox_new( False, 0 );
   gtk_box_pack_start( GTK_BOX( win->appWEdGrid ), win->appWInGrid, True, True, 0 );

   win->appWASCMenu =
     win->appWTopEdWin =
     win->appWMiniW = NULL;

   win->appWSB = gtk_statusbar_new();
   gtk_widget_add_events( win->appWSB, GDK_KEY_PRESS_MASK );
   gtk_box_pack_start( GTK_BOX( win->appWGrid ), win->appWSB, False, True, 0 );

   gtk_container_add( GTK_CONTAINER( win->appW ), win->appWGrid );

   win->fontContext = gtk_widget_create_pango_context( win->appWSB );
   win->fontDescription = pango_font_description_from_string( win->fontName );
   win->fontMetrics = pango_context_get_metrics( win->fontContext,
						 win->fontDescription,
						 NULL );
   win->fontAscent = pango_font_metrics_get_ascent( win->fontMetrics ) / PANGO_SCALE;
   win->fontDescent = pango_font_metrics_get_descent( win->fontMetrics ) / PANGO_SCALE;

   win->fontHeight = win->fontAscent + win->fontDescent;

   win->fontWidth = pango_font_metrics_get_approximate_char_width( win->fontMetrics ) / PANGO_SCALE;

   gtk_widget_show_all( win->appW );
}

/************************************
* getEdWinFromJedWin
*
* group: win
*
*
* debug print: "Window_Type: %p\n", jWin
*
*
************************************/

static GtkWidget *
getEdWinFromJedWin( Window_Type *jWin )
{
   GtkSubWinType *edWin = JGtkWin->subWins;

   do
     {
	if ( edWin->jedWin == jWin ) return( edWin->edWin );
	edWin = edWin->next;
     }
   while ( edWin != JGtkWin->subWins );

   return( NULL );
}

/************************************
* getGtkSubWinFromRange
*
* group: scroll
*
*
* debug print: "Range: %p\n", r
*
************************************/

static GtkSubWinType *
getGtkSubWinFromRange( GtkRange *r )
{
   GtkSubWinType *sb = JGtkWin->subWins;

   do
     {
	if ( sb->sbWin == ( GtkWidget * ) r ) return( sb );
	sb = sb->next;
     }
   while ( sb != JGtkWin->subWins );

   return( JGtkWin->subWins );
}

/* Shamelessly stolen from emacsmsc.sl */

/************************************
* jGtkScrollUpN
*
* group: scroll
*
*
* debug print:
*
*
************************************/

static void
jGtkScrollUpN( int n )
{
   int i = window_line();
   if ( jed_up( n ) ) recenter( &i ) ;
   bol();
   /* Return( 1 ); */
}

/************************************
* jGtkScrollDownN
*
* group: scroll
*
*
* debug print:
*
*
************************************/

static int
jGtkScrollDownN( int n )
{
   int i = window_line();
   if ( jed_down( n ) ) recenter( &i );
   bol();
   return 0;
}

/************************************
* jGtkScrollUp
*
* group: scroll
*
*
* debug print:
*
*
************************************/

static void
jGtkScrollUp( void )
{
   int i = window_line();
   if ( jed_up( 1 ) ) recenter( &i );
   bol();
   /* Return( 1 ); */
}

/************************************
* jGtkScrollDown
*
* group: scroll
*
*
* debug print:
*
*
************************************/

static int
jGtkScrollDown( void )
{
   int i = window_line();
   if ( jed_down( 1 ) ) recenter( &i );
   bol();
   return 0;
}

/************************************
* jGtkDiffTime
*
* group: scroll
*
*
* debug print:
*
*
************************************/

static long
jGtkDiffTime( struct timeval *st )
{
   struct timeval ct;
   long diff;

   gettimeofday( &ct, NULL );

   /*******
   printf( "ct.tv_sec: %ld, ct.tv_usec: %ld, st->tv_sec: %ld, st->tv_usec: %ld\n",
	      ct.tv_sec, ct.tv_usec,
	      st->tv_sec, st->tv_usec );
   ***************/

   diff = ( 1000000 * ( ct.tv_sec - st->tv_sec ) + ( long ) ct.tv_usec - ( long ) st->tv_usec ) / 1000;
   /* Dbp1( "Diff: %d\n", diff ); */
   return( diff );
}

static void fcSBGotoLine( gpointer );

/************************************
* jGtkGotoLine
*
* group: scroll
*
*
* debug print:
*
*
************************************/

static void
jGtkGotoLine(void)
{
   if ( jGtkSIPrim.set  )
     { /* The delay ( curr: 30 ) should sometime be configurable from Jed/slang */

	if ( jGtkDiffTime( &jGtkSIPrim.tv ) > 30 )
	  {
	     gettimeofday( &jGtkSIPrim.tv, NULL );

	 /* Dbp1( "jGtkSIPrim.tl: %d\n", jGtkSIPrim.tl ); */

	     if ( LineNum < (unsigned int) jGtkSIPrim.tl )
	       {
		  if ( jGtkSIPrim.sLSBeg > 0 )
		    {
		       jGtkScrollDown();
		       jGtkSIPrim.sLSBeg--;
	       /* Dbp( "jGtkSIPrim.sLS: %d\n", jGtkSIPrim.sLS ); */
		    }
		  else
		    {
		       int jumpLines = jGtkSIPrim.tl - LineNum - jGtkSIPrim.sLSEnd;
		       if ( jumpLines > 0 )
			 {
			    jGtkScrollDownN( jumpLines );
			    jGtkSIPrim.sLSBeg = jGtkSIPrim.sLSEnd;
			 }
		       else
			 {
			    jGtkSIPrim.sLSBeg = jGtkSIPrim.tl - LineNum;
			 }
		    }
	       }
	     else
	       {
		  if ( LineNum > (unsigned int) jGtkSIPrim.tl )
		    {
		       if ( jGtkSIPrim.sLSBeg > 0 )
			 {
			    jGtkScrollUp();
			    jGtkSIPrim.sLSBeg--;
		  /* Dbp( "jGtkSIPrim.sLS: %d\n", jGtkSIPrim.sLS ); */
			 }
		       else
			 {
			    int jumpLines = LineNum - jGtkSIPrim.tl - jGtkSIPrim.sLSEnd;

			    if ( jumpLines > 0 )
			      {
				 jGtkScrollUpN( jumpLines );
				 jGtkSIPrim.sLSBeg = jGtkSIPrim.sLSEnd;
			      }
			    else
			      {
				 jGtkSIPrim.sLSBeg = LineNum - jGtkSIPrim.tl;
			      }
			 }
		    }
	       }
	  }

	if ( LineNum == (unsigned int) jGtkSIPrim.tl )
	  {
	     if ( jGtkSISec.set )
	       {
		  jGtkSIPrim = jGtkSISec;
		  gettimeofday( &jGtkSIPrim.tv, NULL );
		  jGtkSISec.set = 0;
	       }
	     else
	       {
		  jGtkSIPrim.set = 0;
	       }
	  }

	if ( (unsigned int) jGtkSIPrim.tl != LineNum && jGtkSIPrim.set )
	  {
	     jgtk_createKeyEvents( SLang_process_keystring( FC_CMD_KEY_SEQ ) );
	     actParaData = ( gpointer ) NULL;
	     actFunToCall = fcSBGotoLine;
	  }

     }
}

/************************************
* fcSBGotoLine
*
* group: scroll
*
*
* debug print: "data: %p\n", data
*
*
************************************/

static void
fcSBGotoLine( gpointer data )
{
   /* GtkRange *sbRange = ( GtkRange * ) data; */
   GtkRange *sbRange = ( GtkRange * ) jGtkSIPrim.rangeW;
   int actLine = ( int ) actSBVal;
   GtkSubWinType *subWin = getGtkSubWinFromRange( sbRange );
   Window_Type *actWin = JWindow;

   if ( subWin->jedWin != JWindow )
     {
	do
	  {
	     other_window();
	  }
	while ( JWindow != subWin->jedWin && JWindow != actWin );
     }

   /* goto_line( &actLine ); */
   jGtkGotoLine();

   if ( JWindow != actWin )
     {
	do
	  {
	     other_window();
	  }
	while ( JWindow != actWin );
     }

   update( NULL, 1, 0, 0 );
}

/************************************
* jGtkFillScrollInfoStruct
*
* group: scroll
*
*
* debug print:
*
*
************************************/

static void
jGtkFillScrollInfoStruct( JGtkScrollInfo *si, GtkRange *w, gdouble val )
{
   /* struct timezone tz; */
   GtkSubWinType *sw = getGtkSubWinFromRange( w );

   si->set = 1;
   si->tl = val; /* - sw->jedWin->rows * val / Max_LineNum; */
   si->tl = si->tl < 1 ? 1 : ( si->tl > (int)Max_LineNum ? (int)Max_LineNum : si->tl );
   /* Dbp1( "si->tl: %d\n", si->tl ); */
   si->rangeW = w;
   /* The value of 10 should sometime be configurable form Jed/slang */
   si->sLSBeg =
     si->sLSEnd = sw->jedWin->rows > 10 ? 10 : sw->jedWin->rows;
   si->jF = 0;
   si->cnt = JGTK_SCROLL_SLOW_DOWN;
   gettimeofday( &si->tv, NULL );
}

/************************************
* jGtkFillScrollInfo
*
* group: scroll
*
*
* debug print:
*
*
************************************/

static void
jGtkFillScrollInfo( GtkRange *w, gdouble val )
{
   if ( jGtkSIPrim.set )
     {
	jGtkSIPrim.jF = 1;
	jGtkFillScrollInfoStruct( &jGtkSISec, w, val );
     }
   else
     {
	jGtkFillScrollInfoStruct( &jGtkSIPrim, w, val );
     }
}

/************************************
* sbChangeValue
*
* group: scroll
*
*
* debug print: "GtkRange: %p, GtkStrollType: %d, val: %f, ud(gpointer): %p\n", w, st, val, ud
*
*
************************************/

static gboolean
sbChangeValue( GtkRange *w, GtkScrollType st, gdouble val, gpointer ud )
{
   /* Dbp1( "Value: %f\n", ( float ) val ); */

   actSBVal = val;

   val = val < 1 ? 1 : ( val > gtk_range_get_adjustment( w )->upper ? gtk_range_get_adjustment( w )->upper + 1 : val + 1 );

   jgtk_createKeyEvents( SLang_process_keystring( FC_CMD_KEY_SEQ ) );
   /* Dbp1( "val: %f\n", val ); */
   jGtkFillScrollInfo( w, val );
   actParaData = ( gpointer ) w;
   actFunToCall = fcSBGotoLine;
   return( True );
}

#if 0
static int
checkIfSizeChanged( GdkEventConfigure *ev )
{
   JGtkWinType *win = JGtkWin;
   GtkSubWinType *sw = win->subWins;

   do
     {
	if ( sw->edWin->window == ev->window )
	  {
	 /********
	 if ( ev->x      == sw->edX &&
	      ev->y      == sw->edY &&
	      ev->width  == sw->edWidth &&
	      ev->height == sw->edHeight )
	 *******/
	 /*******/
	     if ( sw->edWin->allocation.x      == sw->edX &&
		  sw->edWin->allocation.y      == sw->edY &&
		  sw->edWin->allocation.width  == sw->edWidth &&
		  sw->edWin->allocation.height == sw->edHeight )
	 /*********/
	       {
		  return( False );
	       }
	     else
	       {
	    /*******/
		  sw->edX    = sw->edWin->allocation.x;
		  sw->edY    = sw->edWin->allocation.y;
		  sw->edWidth  = sw->edWin->allocation.width;
		  sw->edHeight = sw->edWin->allocation.height;
	    /************/

	    /********
	    sw->edX    = ev->x;
	    sw->edY    = ev->y;
	    sw->edWidth  = ev->width;
	    sw->edHeight = ev->height;
            ********/

		  return( True );
	       }
	  }
	sw = sw->next;
     }
   while ( sw != win->subWins );

   return( True );
}
#endif

/************************************
* eventCallback
*
* group: event
*
*
* debug print:
*
*
************************************/

static gboolean
eventCallback( GtkWidget *w,
	       GdkEvent  *ev,
	       gpointer  *data )
{
   static int dragPaned;
   static GtkSubWinType *paneWin1, *paneWin2;
   Window_Type *jWin;
   int tmpInt;
   GtkSubWinType *actWin = ( GtkSubWinType * ) data;
   JGtkWinType *win = JGtkWin;
   int block_expose = 0;
   static int doNoEvents;
   /* printf( "EventCallback: %s, %d\n", __FILE__, __LINE__ ); */
   /* printf( "  Length of queue: %d\n", thisEventBuffer->numGtkEv ); */

   /* printf( "Window: %p\n", win ); */
   /* printf( "Windows: %p\n", win->subWins ); */

   if ( doNoEvents ) return( False );

   switch ( ev->type )
     {
      case GDK_EXPOSE:
	/* printf( "Gdk Expose EventCallback: %s, %d\n", __FILE__, __LINE__ ); */
	/* printf( "    Expose event\n" ); */
	if ( actWin )
	  {
	     if ( block_expose == 0 )
	       {
		  if ( w == actWin->edWin )
		    {
		       jGtkCoverExposedArea( ev->expose.area.x + actWin->jedWin->sx * win->fontWidth,
					     ev->expose.area.y + actWin->jedWin->sy * win->fontHeight,
					     ev->expose.area.width,
					     ev->expose.area.height,
					     ev->expose.count );
		    }
		  else
		    {
		       jGtkCoverExposedArea( ev->expose.area.x + actWin->jedWin->sx * win->fontWidth,
					     ev->expose.area.y + ( actWin->jedWin->sy +
								   actWin->jedWin->rows ) * win->fontHeight,
					     ev->expose.area.width,
					     ev->expose.area.height,
					     ev->expose.count );
		    }
	       }
	     else
	       {
		  if ( ev->expose.count == 0 )
		    {
		       jed_redraw_screen( 1 );
		       block_expose = 0;
		    }
	       }
	  }
	break;
      case GDK_FOCUS_CHANGE:
	/* printf( "Focus change: %s, %d\n", __FILE__, __LINE__ ); */
	toggle_cursor( ev->focus_change.in );
	break;
      case GDK_CONFIGURE:
        /* if ( !updateGtkWin ) Return; */
	/*****************/
        /* printf( "Gdk Configure: %s, %d: data: %p, widget: %p\n", __FILE__, __LINE__, data, w ); */
        /* printf( "File: %s, Line: %d: Width: %d, Height: %d\n", __FILE__, __LINE__, JGtkWin->width, JGtkWin->height ); */

        /* printSubWins(); */
	/*************
	printf( "vvvvvv~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~vvvvvv\n" );
	printSubWinSizes();
	printf( "^^^^^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^\n" );
        ************/
        /* printf( "File: %s, Line: %d: Width: %d, Height: %d\n", __FILE__, __LINE__, JGtkWin->width, JGtkWin->height ); */

        /* Dbp3( "Configure-Event: Widget: %p, Event: %p, data: %p\n", w, ev, data ); */

        if (
	    /**********
	    ( win->appWInGrid->allocation.height != win->inGridHeightPixel ||
	       win->appWInGrid->allocation.width  != win->inGridWidthPixel ) &&
	     *********/
	    win->subWins && actWin )
	  {
	     int sy, sx, i;
	     int hCheck;
	     int newWidth, newHeight;
	     GtkSubWinType *sw;

	   /* if ( !checkIfSizeChanged( ( GdkEventConfigure * ) ev ) ) break; */

	   /*******/
	   /* Dbp2( "w: %p, actWin: %p\n", w, actWin ); */

	   /******
	   printf( "1: vvvvvv---------------------------------------vvvvvv\n" );
	   printWidgetWinXRef();
	   printSubWins();
	   printf( "1: ^^^^^^---------------------------------------^^^^^^\n" );
	   **********/
	   /**********/

	     if ( !dragPaned )
	       {
		  sw = win->subWins;

		  hCheck =
		    sy = win->subWins->jedWin->sy;
		  sx = 0;

	      /* printSubWins(); */

		  do
		    {
		       newHeight = sw->edWin->allocation.height / win->fontHeight + 1;
		       hCheck += newHeight;
		       if ( newHeight < 3 ) newHeight = 3;
		 /* Dbp4( "sy: %d, newHeight: %d, sw->edWin->allocation.height: %d, win->fontHeight: %d\n", sy, newHeight, sw->edWin->allocation.height, win->fontHeight ); */
		 /* newWidth = sw->edWin->allocation.width / win->fontWidth + 1; */
		       newWidth = win->appWInGrid->allocation.width / win->fontWidth;
		       if ( !newWidth ) newWidth = 1;
		 /* Db; */ updateWidgetWinXRef( sw, sy, sx, newHeight, newWidth );
		 /* touch_window( sw->jedWin ); */
		       sy = sy + sw->jedWin->rows + 1;
		 /* printf( "SW: %p, height: %d, sum height: %d, hCheck: %d\n", sw, newHeight, sy, hCheck ); */
		       sw = sw->next;
		 /* Dbp( "  --> sy: %d\n", sy ); */
		    }
		  while ( sw != win->subWins );

	      /*************************
	       * Update Minibuffer!!!
	       *************************/

	      /* newWidth = win->miniWinSW->edWin->allocation.width / win->fontWidth + 1; */

	      /********/
	      /* Dbp( "Miniwin: sy: %d\n", sy ); */
	      /* Dbp( "MiniWin: win->miniWinSW: %p\n", win->miniWinSW ); */
	      /* printGtkSubWin( win->miniWinSW ); */
	      /************/
		  if ( win->miniWinSW )
		    {
		       modWidgetWinXRef( win->widgetWinXRef, win->miniWinSW, 0, sy, newWidth, 1 );
		 /***************
		  for ( i = 0; i < newWidth; ++i )
		    {
		     win->widgetWinXRef[sy][win->miniWinSW->jedWin->sx + i] = win->miniWinSW;
		     }
		  **************/
		       sy++;
		       hCheck++;
		    }

	   /**********
	   printf( "1.5: ---------------------------------------\n" );
	   printWidgetWinXRefXY( win->miniWinSW->jedWin->sx + newWidth, sy );
	   printSubWins();
	   printf( "1.5: ---------------------------------------\n" );
	   ***************/

		  win->hCheck = hCheck;
		  win->inGridHeight = sy;
	   /* Dbp( "WinGridHeight: %d\n", win->inGridHeight ); */

	   /* Db; */
	   /* printf( "win->hCheck: %d, win->inGridHeight: %d\n", win->hCheck, win->inGridHeight ); */

	   /* Dbp2( "win->edWidth: %d, win->edHeight: %d\n", win->edWidth, win->edHeight ); */
	   /* Dbp2( "newWidth:     %d, sy:            %d\n", newWidth, sy ); */

		  win->edWidth = newWidth;
		  win->edHeight = sy;

	      /* Dbp4( "win->hCheck: %d, win->inGridHeight: %d, win->edWidth: %d, win->edHeight: %d\n", win->hCheck, win->inGridHeight, win->edWidth, win->edHeight ); */

	   /***********/
		  JGtkWin->width = win->appWInGrid->allocation.width;
		  JGtkWin->height = sy * win->fontHeight;
           /************/

		  win->edWidth = newWidth;
		  win->edHeight = sy;

		  win->inGridHeightPixel = win->appWInGrid->allocation.height;
		  win->inGridWidthPixel = win->appWInGrid->allocation.width;

#if 0
		  printf( "1.5: ---------------------------------------\n" );
	   /* printWidgetWinXRefXY( win->miniWinSW->jedWin->sx + newWidth, sy - 1 ); */
		  printWidgetWinXRef();
		  printSubWins();
		  printf( "1.5: ---------------------------------------\n" );
#endif

	   /**************/
		  doColorSetup = 0;
		  doCLS = 0;
		  jed_init_display();
		  jed_redraw_screen( 1 );
		  doCLS = 1;
		  doColorSetup = 1;
	   /************/
           /*********
	   printf( "2: vvvvvv---------------------------------------vvvvvv\n" );
	   printWidgetWinXRef();
	   printSubWins();
	   printf( "2: ^^^^^^---------------------------------------^^^^^^\n" );
	   ********/
	       }
	     else
	       {
		  if ( w == paneWin1->edWin ||
		       w == paneWin2->edWin )
		    {
		       GtkSubWinType *subW;
		       int resizeEdFlag = 0;
		       int _sy, _newHeight, newHeightP1, newHeightP2, _newWidth;
		       int sumHeight = paneWin1->jedWin->rows + 1 +
			 paneWin2->jedWin->rows + 1;

		       _newHeight = paneWin1->edWin->allocation.height / win->fontHeight + 1;
		       if ( _newHeight < 3 || paneWin1->jedWin->rows <= 3 )
			 {
			    if ( _newHeight < 3 ) _newHeight = 3;
			    resizeEdFlag = 1;
			 }

		       _newWidth = paneWin1->edWin->allocation.width / win->fontWidth;
		       updateWidgetWinXRef( paneWin1, paneWin1->jedWin->sy, 0, _newHeight, _newWidth );

		       newHeightP1 = _newHeight;

		       _sy = paneWin1->jedWin->sy + paneWin1->jedWin->rows + 1;

		 /***************
		 if ( resizeEdFlag )
		   {
		    _newHeight = paneWin2->edWin->allocation.height / win->fontHeight + 1;
		    }
		 else
		   _newHeight = sumHeight - _newHeight;
                 ***************/

		       newHeightP2 =  paneWin2->edWin->allocation.height / win->fontHeight + 1;

		 /* Dbp2( "Allocation.height: %d, newHeightP2: %d\n", paneWin2->edWin->allocation.height, newHeightP2 ); */

		       if ( newHeightP2 != sumHeight - _newHeight )
			 {
			    resizeEdFlag = 1;
			 }

		       _newHeight = newHeightP2;

		 /*********
                 if ( newHeightP2 > sumHeight - _newHeight )
		   {
		    _newHeight = newHeightP2;
		    resizeEdFlag = 1;
		    }
		 else
		   {
		    _newHeight = sumHeight - _newHeight;
		    }
		 **********/

		    /****************
		    if ( newHeightP2 < 3 )
		      {
		       _newHeight = 3;
		       resizeEdFlag = 1;
		       }
		    else
		      {
		       _newHeight = newHeightP2;
		       }
		    ****************/

		       if ( _newHeight < 3 || paneWin2->jedWin->rows <= 3 )
			 {
			    if ( _newHeight < 3 ) _newHeight = 3;
			    resizeEdFlag = 1;
			 }

		       updateWidgetWinXRef( paneWin2, _sy, 0, _newHeight, _newWidth );

		       newHeightP2 = _newHeight;

		 /* Dbp5( "ResizeEdFlag: %d, SumHeight old: %d, SumHeight new: %d, newHeightP1: %d, newHeightP2: %d\n", resizeEdFlag, sumHeight, newHeightP1 + newHeightP2, newHeightP1, newHeightP2 ); */

		       if ( newHeightP1 + newHeightP2 == sumHeight )
			 resizeEdFlag = 0;

		       if ( resizeEdFlag )
			 {
		    /*******
		    subW = JGtkWin->subWins;
		    _sy = subW->jedWin->sy;
		    **********/

			    subW = paneWin2->next;
			    _sy = paneWin2->jedWin->sy + paneWin2->jedWin->rows + 1;

			    if ( subW != JGtkWin->subWins )
			      do
			      {
				 _newHeight = subW->edWin->allocation.height / win->fontHeight + 1;
				 if ( _newHeight < 3 ) _newHeight = 3;
				 _newWidth = win->appWInGrid->allocation.width / win->fontWidth;
				 if ( !_newWidth ) _newWidth = 1;
				 updateWidgetWinXRef( subW, _sy, 0, _newHeight, _newWidth );
				 _sy = _sy + _newHeight;
				 subW = subW->next;
			      }
			    while ( subW != JGtkWin->subWins );

			    if ( win->miniWinSW )
			      {
				 modWidgetWinXRef( win->widgetWinXRef, win->miniWinSW, 0, _sy, _newWidth, 1 );
				 _sy++;
			      }

			    win->inGridHeight = _sy;
			    win->edHeight = _sy;

			    JGtkWin->height = _sy * win->fontHeight;

			    win->edHeight = _sy;

			    win->inGridHeightPixel = win->appWInGrid->allocation.height;

 		    /* Dbp1( "===========================================================================================%d\n", 1 ); */
 		    /* Dbp1( "===========================================================================================%d\n", 1 ); */
 		    /* Dbp1( "===========================================================================================%d\n", 1 ); */

			    doNoEvents = 1;
			    doColorSetup = 0;
			    doCLS = 0;
			    jed_init_display();
			    update( NULL, 1, 0, 0 );
		    /* jed_redraw_screen( 1 ); */
			    doCLS = 1;
			    doColorSetup = 1;
			    doNoEvents = 0;
 		    /* Dbp1( "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^%d\n", 1 ); */
 		    /* Dbp1( "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^%d\n", 1 ); */
 		    /* Dbp1( "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^%d\n", 1 ); */
			 }
		       else
			 {
		    /**********
		    jWin = JWindow;
		    do
		      {
		       touch_window_hard( JWindow, 0 );
		       JWindow = JWindow->next;
		       }
		    while ( jWin != JWindow );
		    **************/
			    touch_window_hard( paneWin1->jedWin, 1 );
			    touch_window_hard( paneWin2->jedWin, 1 );
		    /* touch_screen(); */
		    /* jed_redraw_screen( 1 ); */
			    update( NULL, 1, 0, 0 );
			 }
		    }
	       }
	  }

      /***********
      printf( "99: vvvvvv~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~vvvvvv\n" );
      printWidgetWinXRef();
      printSubWins();
      printf( "99: ^^^^^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^\n" );
      ********/
	if ( JGtkWin->appWMiniW )
	  gtk_widget_grab_focus( JGtkWin->appWMiniW );
	break;
      case GDK_KEY_PRESS:
        /* printf( "File: %s Line: %d GDK_KEY_PRESS event!!!\n", __FILE__, __LINE__ ); */
	/* printf( "KeyCode: \n" ); */
        addGdkEvent( ev, w, data );
	break;
      case GDK_BUTTON_PRESS:
	/* printf( "File: %s Line: %d Button press\n", __FILE__, __LINE__ ); */
	if ( GTK_IS_PANED( w ) )
	  {
	     GtkWidget *ch1 = gtk_paned_get_child1( GTK_PANED( w ) );
	     GList *childList;
	     GtkWidget *stWidget;
	     GtkSubWinType *sw = JGtkWin->subWins;
	   /* GtkWidget *ch2; */

	   /* Dbp1( "Paned: %p Button press!!\n", w ); */

	     if ( GTK_IS_PANED( ch1 ) )
	       {
		  ch1 = gtk_paned_get_child2( GTK_PANED( ch1 ) );
	       }

	     if ( GTK_IS_BOX( ch1 ) )
	       {
		  childList = gtk_container_get_children( GTK_CONTAINER( ch1 ) );
		  stWidget = ( GtkWidget * ) childList->next->data;
		  g_list_free( childList );

		  while ( sw->stWin != stWidget &&
			  sw->next != JGtkWin->subWins )
		    {
		       sw = sw->next;
		    }

		  if ( sw->stWin == stWidget )
		    {
		       paneWin1 = sw;
		       paneWin2 = paneWin1->next;
		       dragPaned = 1;
		 /* Dbp2( "Found paneWin1: %p, paneWin2: %p\n", paneWin1, paneWin2 ); */
		       return( False );
		    }
	       }

	   /* Dbp1( "Widget structure does not match: %p\n", ch1 ); */

	     paneWin1 = paneWin2 = NULL;
	     dragPaned = 0;
	     return( False );
	  }
        else
	  addGdkEvent( ev, w, data );
	break;
      case GDK_BUTTON_RELEASE:
	/* printf( "File: %s Line: %d Button release\n", __FILE__, __LINE__ ); */
	if ( GTK_IS_PANED( w ) )
	  {
	   /* Dbp1( "Paned: %p Button release!!\n", w ); */
	     dragPaned = 0;
	     paneWin1 = paneWin2 = NULL;
	     return( False );
	  }
        else
          addGdkEvent( ev, w, data );
	break;
      case GDK_MOTION_NOTIFY:
        /* printf( "File: %s Line: %d Motion event\n", __FILE__, __LINE__ ); */
        /* printf( "File: %s Line: %d x: %f, y: %f\n", __FILE__, __LINE__, ev->motion.x, ev->motion.y ); */
        addGdkEvent( ev, w, data );
	break;
      default:
        /* thisEventBuffer->gtkEvList = g_list_append( thisEventBuffer->gtkEvList, gdk_event_copy( ev ) ); */
        /* ( thisEventBuffer->numGtkEv )++; */
        /* printf( "  Length of queue: %d\n", thisEventBuffer->numGtkEv ); */
	break;
     }

   return( True );
}

#if SLANG_VERSION >= 20000

/************************************
* bytes_to_wchars
*
* group:
*
*
* debug print:
*
*
************************************/

static SLwchar_Type *
bytes_to_wchars (unsigned char *s, unsigned int nchars )
{
   unsigned int i;

   SLwchar_Type *w = (SLwchar_Type *)SLmalloc(nchars*sizeof(SLwchar_Type));
   if (w == NULL)
     return NULL;

   for (i = 0; i < nchars; i++)
     w[i] = s[i];

   return w;
}

/************************************
* utf8nt_to_wchars
*
* group:
*
*
* debug print:
*
*
************************************/

static SLwchar_Type *
utf8nt_to_wchars(unsigned char *s, unsigned int len, unsigned int *ncharsp)
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
	if ( SLutf8_decode(s, smax, &w[i], &n ) == NULL )
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

/************************************
* gtkXDrawString
*
* group:
*
*
* debug print: "drawW: %p, gcNum: %d, text: %p, x: %d, y: %d\n", drawW, gcNum, text, x, y
*
************************************/

static void
jGtkDrawString( GtkWidget *drawW, int gcNum, char *text, int x, int y )
{
   JGtkWinType *win = JGtkWin;
   GdkColor *fg = DefaultGtkGCInfo[gcNum].fg;
   GdkColor *bg = DefaultGtkGCInfo[gcNum].bg;
   PangoRenderer *renderer;
   PangoLayout   *layout;

   if ( !drawW->window )
     {
      /* Dbp1( "Widget (%p) Window: NULL, should not occur!!!\n", drawW ); */
	return;
     }

   renderer = gdk_pango_renderer_get_default( win->screen );
   gdk_pango_renderer_set_drawable( GDK_PANGO_RENDERER( renderer ), drawW->window );
   gdk_pango_renderer_set_gc( GDK_PANGO_RENDERER( renderer ), drawW->style->black_gc );
   layout = pango_layout_new( win->fontContext );

   pango_layout_set_text( layout, text, -1 );
   pango_layout_set_font_description( layout, win->fontDescription );
   gdk_pango_renderer_set_override_color( GDK_PANGO_RENDERER( renderer ),
					  PANGO_RENDER_PART_FOREGROUND, fg );
   gdk_pango_renderer_set_override_color( GDK_PANGO_RENDERER( renderer ),
					  PANGO_RENDER_PART_BACKGROUND, bg );
   pango_layout_context_changed( layout );
   pango_renderer_draw_layout( renderer, layout, x * PANGO_SCALE, y * PANGO_SCALE );

   /* free the objects we created */
   g_object_unref ( layout );
}

/* This function does the low-level drawing.
 * It can draw the new text, but setting 'overimpose' to 1 it draws the string
 * over the existing text (used for unicode combining characters).
 * It can use Xft (if configured), or plain X functions.
 */

/************************************
* gtkXDraw
*
* group:
*
*
* debug print: "gcNum: %d, row: %d, col: %d, w: %p, nChars: %d, overimpose: %d\n", gcNum, row, col, w, nchars, overimpose
*
************************************/

static int
jGtkDraw( int gcNum, int row, int col, SLwchar_Type *w, int nchars, int overimpose )
{
   char *tmpText = "A";
   char *text, *buf, *sav;
   int bufLen = 8*nchars + 1;
   int i;
   JGtkWinType *win = JGtkWin;
   int x = col * win->fontWidth;
   int y = row * win->fontHeight;

   if ( row == 0 ) return( 1 );

   if ( !updateGtkWin ) return( 1 );

   buf =
     sav =
     text = SLmalloc( bufLen + 1 );

   for ( i = 0; i < nchars; ++i )
     {
	if ( sav && bufLen > 0 )
	  {
	     buf = sav;
	     sav = ( char * ) SLutf8_encode( w[i], ( unsigned char * ) buf, bufLen );
	     bufLen -= sav - buf;
	  }
     }

   if ( !sav ) return( 0 );

   *sav = '\0';

   if ( row > 0 && col >= 0 && row < Jed_Num_Screen_Rows && col < Jed_Num_Screen_Cols )
     {  /* row > 0 needed because routines for ascii-menu not implemented!!!! */

      /* printf( "win->widgetWinXRef[%d][%d]\n", row, col ); */
      /* printf( "win->widgetWinXRef[%d][%d]: %p\n", row, col, win->widgetWinXRef[row][col] ); */

	if ( win->widgetWinXRef[row][col] )
	  {
	     Window_Type *jedWin = win->widgetWinXRef[row][col]->jedWin;

	     if ( row == jedWin->sy + jedWin->rows )
	       {
            /* printf( "win->widgetWinXRef[%d][%d]: StWin: %p\n", row, col, win->widgetWinXRef[row][col]->stWin ); */
		  jGtkDrawString( win->widgetWinXRef[row][col]->stWin, gcNum, text, x, 0 );
	       }
	     else
	       {
            /* printf( "win->widgetWinXRef[%d][%d]: EdWin: %p\n", row, col, win->widgetWinXRef[row][col]->edWin ); */
		  jGtkDrawString( win->widgetWinXRef[row][col]->edWin, gcNum, text, x, y - jedWin->sy*win->fontHeight );
	       }
	  }
     }
   SLfree( text );
   return 1;
   /* printf( "End drawing\n" ); */
}

/* Get 'n' characters from SLSMG screen, at position ('row', 'col'). */
/************************************
* smg_read_at
*
* group:
*
*
* debug print:
*
*
************************************/

static unsigned int
smg_read_at( int row, int col, SLsmg_Char_Type *s, unsigned int n)
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
/************************************
* JX_write_smgchar
*
* group:
*
*
* debug print:
*
*
************************************/

static void
JX_write_smgchar( int row, int col, SLsmg_Char_Type *s )
{
   int color = SLSMG_EXTRACT_COLOR(*s) & SLSMG_COLOR_MASK;

   if ((color >= JMAX_COLORS) || (color < 0))
     color = 0;

#if SLANG_VERSION >= 20000
   if (s->nchars > 0)
     {
	(void) jGtkDraw( color, row, col, s->wchars, 1, 0 );
     }

   if (Jed_UTF8_Mode)
     {
	unsigned int i;

	for (i = 1; i < s->nchars; i++)
	  (void) jGtkDraw( color, row, col, &s->wchars[i], 1, 1 );
     }
#else
     {
	SLwchar_Type ch = SLSMG_EXTRACT_CHAR(*s);
	(void) jGtkDraw( color, row, col, &ch, 1, 0 );
     }
#endif

}

/* Write to screen a row of SLsmg_Chars, handling combining characters
 * (X doesn't seem to handle these...) to position (row, col).
 * This function doesn't touch the cursor position.
 */

/************************************
* JX_write_smgchars
*
* group:
*
*
* debug print:
*
*
************************************/

static void
JX_write_smgchars( int row, int col, SLsmg_Char_Type *s, SLsmg_Char_Type *smax )
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

   is_dual_font = 0;

   s0 = s;
   while (s < smax)
     {
	color = (SLSMG_EXTRACT_COLOR(*s) & SLSMG_COLOR_MASK);
	if ((color < 0) || (color >= JMAX_COLORS))
	  color = 0;

	if ( oldcolor != color		   /* Color changed. */
	     || (b >= bend)		   /* Space finished */
	     || SLSMG_COUNT_CHARS(*s) > 1)  /* a combining character */
	  {
	     (void) jGtkDraw( oldcolor, row, col, buf, b-buf, 0 );
	     col += (int)(s-s0);
	     s0 = s;
	     b = buf;
	     oldcolor = color;
	  }
#if SLANG_VERSION >= 20000
	if ( s->nchars > 1 )
	  {
	 /* this cell has combining characters */
	     JX_write_smgchar(row, col, s);
	     col++;
	     s0 = s + 1;
	  }
	else if ( s->nchars == 0 )
	  {
	 /* SLsmg thinks this is a double width character, but the font has no such characters */
	     if ( is_dual_font == 0 )
	       *b++ = ' ';
	  }
	else
#endif
	  *b++ = SLSMG_EXTRACT_CHAR(*s);
	s++;
     }
   if ( b != buf )
     (void) jGtkDraw( color, row, col, buf, b-buf, 0 );
}

/************************************
* hide_cursor
*
* group:
*
*
* debug print:
*
*
************************************/

static void
hide_cursor( void )
{
   SLsmg_Char_Type sc;

   if ( No_XEvents ||
        ( JGtkWin->cursor_showing == 0 ) )
     return;

   JGtkWin->cursor_showing = 0;
   if ( 0 == smg_read_at( JGtkWin->vis_curs_row,
			  JGtkWin->vis_curs_col, &sc, 1 ) )
     {
	SLSMGCHAR_SET_CHAR(sc, ' ');
	SLSMGCHAR_SET_COLOR(sc, JNORMAL_COLOR);
	sc.nchars = 1;
     }

   JX_write_smgchar( JGtkWin->vis_curs_row, JGtkWin->vis_curs_col, &sc);
}
/*}}}*/

/************************************
* updateScrollbar
*
* group:
*
*
* debug print:
*
*
************************************/

void
updateScrollbar( Window_Type *jw )
{
   int subWinNotFound = 1;
   GtkSubWinType *subWin = JGtkWin->subWins;

   /* printSubWins(); */

   do
     {
	if ( subWin->jedWin == jw )
	  {
	     subWinNotFound = 0;
	  }
	else
	  {
	     subWin = subWin->next;
	  }
     }
   while ( subWinNotFound && subWin != JGtkWin->subWins );

   if ( subWinNotFound ) return;

   if ( subWin->numLineSav != subWin->jedWin->buffer->max_linenum  ||
	subWin->curLineSav != subWin->jedWin->buffer->linenum  ||
	subWin->winHeightSav != subWin->jedWin->rows )
     {
	subWin->sbAdj->upper = subWin->jedWin->buffer->max_linenum - 1 > 0 ? subWin->jedWin->buffer->max_linenum - 1 : 0;
	subWin->sbAdj->value = subWin->jedWin->buffer->linenum - 1 > 0 ? subWin->jedWin->buffer->linenum - 1 : 0;
	subWin->sbAdj->page_size = subWin->jedWin->rows;

	subWin->numLineSav = subWin->jedWin->buffer->max_linenum;
	subWin->curLineSav = subWin->jedWin->buffer->linenum;
	subWin->winHeightSav = subWin->jedWin->rows;

      /* Dbp1( "Adjustment:   -->  %p\n", subWin->sbAdj ); */
	g_signal_emit_by_name( G_OBJECT( subWin->sbAdj ), "changed" );
     }
}

/************************************
* gtkCopyRect
*
* group:
*
*
* debug print: "x1: %d, y1: %d, x2: %d, y2: %d, x3: %d, y3: %d\n", x1, y1, x2, y2, x3, y3
*
************************************/

static void
gtkCopyRect( int x1, int y1, int x2, int y2, int x3, int y3 )
{
   int w, h;
   Window_Type *jedWin1 = JGtkWin->widgetWinXRef[y1][x1]->jedWin;
   Window_Type *jedWin3 = JGtkWin->widgetWinXRef[y3][x3]->jedWin;
   GtkWidget *win1 = JGtkWin->widgetWinXRef[y1][x1]->edWin;
   GtkWidget *win3 = JGtkWin->widgetWinXRef[y3][x3]->edWin;

   if ( win1 != win3 ) return;

   w = (x2 - x1) * JGtkWin->fontWidth;
   h = (y2 - y1) * JGtkWin->fontHeight;

   if ((w <= 0) || (h <= 0)) return;

   x3 = x3 * JGtkWin->fontWidth;
   x1 = x1 * JGtkWin->fontWidth;
   y3 = y3 * JGtkWin->fontHeight;
   y1 = y1 * JGtkWin->fontHeight;
   hide_cursor ();

   if ( win1 == win3 )
     {
	gdk_gc_set_exposures( win1->style->white_gc, 1 );
	gdk_draw_drawable( win1->window, win1->style->white_gc, win3->window,
			   x1, y1 - jedWin1->sy * JGtkWin->fontHeight,
			   x3, y3 - jedWin3->sy * JGtkWin->fontHeight, w, h );
     }
   else
     {
	printf( "file: %s, Line: %d: GtkCopyRect: widgets differ!!!\n", __FILE__, __LINE__ );
     }
}

/*}}}*/

/************************************
* gtkBlankRect
*
* group:
*
*
* debug print: "x1: %d, y1: %d, x2: %d, y2: %d \n", x1, y1, x2, y2
*
************************************/

static void
gtkBlankRect (int x1,  int y1, int x2, int y2 ) /*{{{*/
{
   int w, h;
   Window_Type *jedWin1 = NULL;
   Window_Type *jedWin2 = NULL;
   GtkWidget *win1 = NULL;
   GtkWidget *win2 = NULL;

   /* Dbp4( "JedWin1: %p, JedWin2: %p, Win1: %p, Win2: %p\n", jedWin1, jedWin2, win1, win2 ); */

   jedWin1 = JGtkWin->widgetWinXRef[y1][x1]->jedWin;

   /* Dbp4( "JedWin1: %p, JedWin2: %p, Win1: %p, Win2: %p\n", jedWin1, jedWin2, win1, win2 ); */

   win1 = JGtkWin->widgetWinXRef[y1][x1]->edWin;

   /* Dbp4( "JedWin1: %p, JedWin2: %p, Win1: %p, Win2: %p\n", jedWin1, jedWin2, win1, win2 ); */

   if (No_XEvents || (JGtkWin->window_mapped == 0)) return;

   w = (x2 - x1) * JGtkWin->fontWidth;
   h = (y2 - y1) * JGtkWin->fontHeight;

   if ((w <= 0) || (h <= 0)) return;

   x1 = x1 * JGtkWin->fontWidth;
   y1 = y1 * JGtkWin->fontHeight;
   hide_cursor ();

   gdk_gc_set_foreground( DefaultGtkGCInfo[JNORMAL_COLOR].gc, DefaultGtkGCInfo[JNORMAL_COLOR].bg );

   /*  gdk_draw_rectangle( win1->window, win1->style->white_gc, 1, */
   gdk_draw_rectangle( win1->window, DefaultGtkGCInfo[JNORMAL_COLOR].gc, 1,
		       x1, y1 - jedWin1->sy * JGtkWin->fontHeight,
		       w, h );

   gdk_gc_set_foreground( DefaultGtkGCInfo[JNORMAL_COLOR].gc, DefaultGtkGCInfo[JNORMAL_COLOR].fg );
}

/*}}}*/

/************************************
* copy_rect
*
* group:
*
*
* debug print:
*
*
************************************/

static void
copy_rect(int x1, int y1, int x2, int y2, int x3, int y3)  /*{{{*/
{
   int w, h;

   gtkCopyRect( x1, y1, x2, y2, x3, y3 );
}

/*}}}*/

/************************************
* blank_rect
*
* group:
*
*
* debug print:
*
*
************************************/

static void
blank_rect (int x1,  int y1, int x2, int y2 ) /*{{{*/
{
   int w, h;

   gtkBlankRect( x1, y1, x2, y2 );
}

/*}}}*/

/************************************
* JX_set_scroll_region
*
* group: interface
*
*
* debug print: "r1: %d, r2: %d\n", r1, r2
*
*
************************************/

static void
JX_set_scroll_region(int r1, int r2)
{
   JGtkWin->scroll_r1 = r1;
   JGtkWin->scroll_r2 = r2;
   /* vterm_set_scroll_region (r1, r2); */
}

/************************************
* jGtkSetFocus
*
* debug print: "(void) dummy: %d\n", 1
*
************************************/

void jGtkSetFocus(void)
{
   JGtkWin->focus = 1;
}

/************************************
* JX_reset_scroll_region
*
* group:  interface
*
*
* debug print:
*
*
************************************/

static void
JX_reset_scroll_region (void ) /*{{{*/
{
   JX_set_scroll_region (0, JX_Screen_Rows - 1);
}

/*}}}*/

/************************************
* show_cursor
*
* group:
*
*
* debug print:
*
*
************************************/

static void
show_cursor( void ) /*{{{*/
{
   SLsmg_Char_Type sc;
   int row, col, b;
   int color;
   GC gc;
   XGCValues gcv;

   /* Db; */

   if ( No_XEvents )
     return;

   /* Db; */

   if ( JGtkWin->cursor_showing ) hide_cursor ();

   /* Db; */

   JGtkWin->cursor_showing = 1;
   row = JGtkWin->vis_curs_row = JGtkWin->cursor_row;
   col = JGtkWin->vis_curs_col = JGtkWin->cursor_col;
   b = JGtkWin->border;

   /* Dbp2( ">>>>>>>>>> row: %d, col: %d\n", row, col ); */
   /* Db; */

   if ( ( CBuf != NULL ) && ( CBuf->flags & OVERWRITE_MODE ) )
     color = JCURSOROVR_COLOR;
   else
     color = JCURSOR_COLOR;

   /* Db; */

   if ( JGtkWin->focus )
     {
      /* Db; */
	if ( smg_read_at(row, col, &sc, 1) == 0 )
	  SLSMGCHAR_SET_CHAR( sc, ' ' );
	SLSMGCHAR_SET_COLOR( sc, color );
	JX_write_smgchar( row, col, &sc );
     }
   else
     {
      /* Draw rectangle !!!!!!!! */
	GtkWidget *w;
	Window_Type *jedWin;

      /* Dbp3( "WidgetWinXRef: %p, Row: %d, Col: %d\n", JGtkWin->widgetWinXRef, row, col ); */

	if ( row > 0 && col >= 0 )
	  {
	 /* Dbp3( "WidgetWinXRef: %p, Row: %d, Col: %d\n", JGtkWin->widgetWinXRef, row, col ); */

	     w = JGtkWin->widgetWinXRef[row][col]->edWin;
	     jedWin = JGtkWin->widgetWinXRef[row][col]->jedWin;

	 /* Dbp4( "Color: %d Default gc: %p, Default fg col: %p Default bg col: %p\n", */
	 /*       color, DefaultGtkGCInfo[color].gc, DefaultGtkGCInfo[color].fg, DefaultGtkGCInfo[color].bg ); */

	     gdk_gc_set_foreground( DefaultGtkGCInfo[color].gc, DefaultGtkGCInfo[color].bg );
	     gdk_gc_set_background( DefaultGtkGCInfo[color].gc, DefaultGtkGCInfo[color].fg );

	     if ( w && w->window )
	       gdk_draw_rectangle( w->window,
				   DefaultGtkGCInfo[color].gc,
				   0,
				   (col - jedWin->sx)*JGtkWin->fontWidth,
				   (row - jedWin->sy)*JGtkWin->fontHeight,
				   JGtkWin->fontWidth - 1,
				   JGtkWin->fontHeight - 1 );

	     gdk_gc_set_foreground( DefaultGtkGCInfo[color].gc, DefaultGtkGCInfo[color].fg );
	     gdk_gc_set_background( DefaultGtkGCInfo[color].gc, DefaultGtkGCInfo[color].bg );
	  }
     }

}

/*}}}*/

/************************************
* toggle_cursor
*
* group:
*
*
* debug print:
*
*
************************************/

static void
toggle_cursor (int on ) /*{{{*/
{
   /* Dbp1( ">>> %d <<<#####################################################################\n", on ); */
   if (on)
     {
	if ( JGtkWin->focus) return;
	JGtkWin->focus = 1;
     }
   else
     {
	if ( JGtkWin->focus == 0) return;
	JGtkWin->focus = 0;
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

/************************************
* JX_write_string
*
* group:
*
*
* debug print:
*
*
************************************/

static void
JX_write_string (char *s ) /*{{{*/
{
   unsigned int nchars;
   SLwchar_Type *w;
   unsigned int nbytes = strlen(s);

   /* printf( "File: %s, Line: %d: JX_write_string\n", __FILE__, __LINE__ ); */

#if SLANG_VERSION >= 20000

   if (Jed_UTF8_Mode)
     w = utf8nt_to_wchars((unsigned char *)s, nbytes, &nchars);
   else
     {
	w = bytes_to_wchars((unsigned char *)s, nbytes);
	nchars = nbytes;
     }
   if ( w == NULL )
     goto write_done;

#else
   nchars = nbytes;
   w = s;
#endif

   if ((No_XEvents == 0) && JGtkWin->window_mapped)
     {
	hide_cursor ();
	(void) jGtkDraw( JGtkWin->current_gc_num, JGtkWin->cursor_row, JGtkWin->cursor_col, w, nchars, 0);
     }
#if SLANG_VERSION >= 20000
   SLfree((char *)w);
write_done:
#endif

   JGtkWin->cursor_col += nchars;
   if ( JGtkWin->cursor_col >= JX_Screen_Cols)
     JGtkWin->cursor_col = JX_Screen_Cols - 1;
   if (!Performing_Update)
     show_cursor();
}

/************************************
* JX_goto_rc
*
* group: interface
*
*
* debug print: "r: %d, c: %d\n", r, c
*
*
************************************/

static void
JX_goto_rc(int r, int c)  /*{{{*/
{
   /* Dbp2( ">>>>>>>>>>>>>>>> r: %d, c: %d\n", r, c ); */

   if ( JGtkWin == NULL) return;
   if ( JGtkWin->cursor_showing) hide_cursor ();
   if (r >= JX_Screen_Rows) r = JX_Screen_Rows - 1;
   if (c >= JX_Screen_Cols) c = JX_Screen_Cols - 1;

   JGtkWin->cursor_row = r + JGtkWin->scroll_r1;
   JGtkWin->cursor_col = c;

   /* Dbp1( "JGtkWin->scroll_r1: %d\n", JGtkWin->scroll_r1 ); */
   /* Dbp2( "JGtkWin->cursor_row: %d, JGtkWin->cursor_col: %d\n", JGtkWin->cursor_row, JGtkWin->cursor_col ); */

   if (Performing_Update) return;
   show_cursor ();
}

/*}}}*/

/* Must respect scrolling region */
/************************************
* JX_delete_nlines
*
* group: interface
*
*
* debug print: "n: %d\n", n
*
*
************************************/

static void
JX_delete_nlines(int n)  /*{{{*/
{
   int r1, r2;
   JGtkWinType *win = JGtkWin;

   r1 = JGtkWin->cursor_row;
   r2 = JGtkWin->scroll_r2;

   /* Dbp2( "JGtkWin->cursor_row: %d, JGtkWin->scroll_r2: %d\n", JGtkWin->cursor_row, JGtkWin->scroll_r2 ); */

   if (r1 <= r2 - n) copy_rect(0, r1 + n, JX_Screen_Cols, r2 + 1,
			       0, r1);

   jGtkCoverExposedArea( 0, (r2 - n)*win->fontHeight,
			 JX_Screen_Cols*win->fontWidth, win->fontHeight,
			 1 );

   r2++;
   blank_rect(0, r2 - n, JX_Screen_Cols, r2);
}

/*}}}*/

/************************************
* JX_reverse_index
*
* group: interface
*
*
* debug print: "n: %d\n", n
*
*
************************************/

static void
JX_reverse_index( int n )  /*{{{*/
{
   int r1, r2;

   /* vterm_reverse_index (n); */

   if (No_XEvents || ( JGtkWin->window_mapped == 0))
     return;

   r1 = JGtkWin->scroll_r1;
   r2 = JGtkWin->scroll_r2;

   if (r2 >= r1 + n) copy_rect(0, r1, JX_Screen_Cols, r2 - n + 1,
			       0, r1 + n);

   blank_rect(0, r1, JX_Screen_Cols, r1 + n);
}

/*}}}*/

/************************************
* JX_beep
*
* group:
*
*
* debug print:
*
*
************************************/

static void
JX_beep(void)  /*{{{*/
{

   if ( No_XEvents ) return;

   flush_input();

   if ( JGtkWin->appWMiniW &&
	JGtkWin->appWMiniW->window )
     {
	if ( JX_Ignore_Beep & 0x1 )
	  {
#if 0				       /* not defined in my version of gtk */
	     gdk_window_beep( JGtkWin->appWMiniW->window );
#endif
	  }

	if ( JX_Ignore_Beep & 0x2 )
	  {
	     struct timeval bgTime;
	     GdkColor colorWhite = { 0, 0xffff, 0xffff, 0xffff },
	     colorBlack = { 0, 0x0, 0x0, 0x0 };
	     GdkGC *gc = gdk_gc_new( JGtkWin->appWMiniW->window );
	     GtkSubWinType *sw = JGtkWin->subWins;

	     gdk_gc_set_rgb_fg_color( gc, &colorWhite );
	     gdk_gc_set_rgb_bg_color( gc, &colorBlack );

	     gdk_gc_set_function( gc, GDK_INVERT );

	     do
	       {
		  if ( sw->edWin && sw->edWin->window )
		    {
		       gdk_draw_rectangle( sw->edWin->window,
					   gc, TRUE,
					   0, 0,
					   sw->edWin->allocation.width,
					   sw->edWin->allocation.height );
		    }
		  if ( sw->stWin && sw->stWin->window )
		    {
		       gdk_draw_rectangle( sw->stWin->window,
					   gc, TRUE,
					   0, 0,
					   sw->stWin->allocation.width,
					   sw->stWin->allocation.height );
		    }
		  sw = sw->next;
	       }
	     while ( sw != JGtkWin->subWins );

	     gdk_display_flush( JGtkWin->display );

	     gettimeofday( &bgTime, NULL );

	 /* Wait 100 ms */
	     while ( jGtkDiffTime( &bgTime ) < 100 )
	       {
		  ;
	       }

	     do
	       {
		  if ( sw->edWin && sw->edWin->window )
		    {
		       gdk_draw_rectangle( sw->edWin->window,
					   gc, TRUE,
					   0, 0,
					   sw->edWin->allocation.width,
					   sw->edWin->allocation.height );
		    }
		  if ( sw->stWin && sw->stWin->window )
		    {
		       gdk_draw_rectangle( sw->stWin->window,
					   gc, TRUE,
					   0, 0,
					   sw->stWin->allocation.width,
					   sw->stWin->allocation.height );
		    }
		  sw = sw->next;
	       }
	     while ( sw != JGtkWin->subWins );

	     gdk_display_flush( JGtkWin->display );

	     gdk_gc_unref( gc );
	  }
     }

}

/*}}}*/

/************************************
* JX_del_eol
*
* group: interface
*
*
* debug print: "(void %d)", 0
*
*
************************************/

static void
JX_del_eol(void)  /*{{{*/
{
   blank_rect( JGtkWin->cursor_col, JGtkWin->cursor_row, JX_Screen_Cols, JGtkWin->cursor_row + 1);
}

/*}}}*/

/************************************
* JX_reverse_video
*
* group:
*
*
* debug print:
*
*
************************************/

static void
JX_reverse_video(int color)  /*{{{*/
{
   if ((color < 0) || (color >= JMAX_COLORS))
     return;
}

/*}}}*/

/************************************
* JX_normal_video
*
* group: interface
*
*
* debug print:
*
*
************************************/

static void
JX_normal_video(void)  /*{{{*/
{
   JX_reverse_video (JNORMAL_COLOR);
}

/*}}}*/

/************************************
* JX_smart_puts
*
* group: interface
*
*
* debug print:
*
*
************************************/

static void
JX_smart_puts(SLsmg_Char_Type *neww, SLsmg_Char_Type *oldd, int len, int row)
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

/************************************
* jGtkCoverExposedArea
*
* group:
*
*
* debug print:
*
*
************************************/

static void
jGtkCoverExposedArea( int x, int y, int width, int height, int count)   /*{{{*/
{
   JGtkWinType *win = JGtkWin;
   SLsmg_Char_Type *s;
   int row, max_col, max_row, col;
   int width_chars, len;

   Performing_Update++;
   /* VTerm_Suspend_Update++; */
   hide_cursor ();
   col = x / win->fontWidth;
   row = y / win->fontHeight;

   width_chars = 2 + width / win->fontWidth;
   max_col = col + width_chars;
   max_row = 2 + row + height / win->fontHeight;
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

#include "xkeys.c"

/* Return 1 if event is listed in the switch or zero otherwise.  The switch
 * events are considered harmless--- that is, processing them does not really
 * interfere with internal JED state (redisplay, etc...).  More bluntly,
 * harmless means that the events can be processesed while checking for
 * pending input.
 */
static int Debug_gtkjed = 0;

/*}}}*/

/************************************
* jGtkFillJMouse
*
* group:
*
*
* debug print:
*
*
************************************/

static void
jGtkFillJMouse( JMouse_Type *jmouse, /*{{{*/
		unsigned char type, int x, int y, unsigned long t,
		unsigned int button, unsigned int state )
{
   unsigned char s;
#if JED_HAS_MULTICLICK
   static unsigned long last_press_time;
   static unsigned int clicks;
   static unsigned int last_button;

   /* printf( "In fill_jmouse\n" ); */

   if (type == JMOUSE_DOWN)
     {
	/* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
	if ((last_button == button)
	    && (last_press_time + JX_MultiClick_Time > t))
	  {
	   /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
	     clicks++;
	     if (clicks == 2)
	       type = JMOUSE_DOUBLE_CLICK;
	     else
	       type = JMOUSE_TRIPLE_CLICK;
	  }
	else
	  {
	   /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
	     clicks = 1;
	     last_button = button;
	  }
	last_press_time = t;
     }
   else if ((clicks > 1) && (last_button == button))
     {
	/* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
	/* Last was a multi-click.  Ignore this event. */
	type = JMOUSE_IGNORE_EVENT;
     }
#endif
   jmouse->type = type;
   jmouse->x = 1 + x / JGtkWin->fontWidth;

   /* if ( y < XWin->border ) jmouse->y = 0; */
   if ( y < JGtkWin->border ) jmouse->y = 0;
   else jmouse->y = 1 + y / JGtkWin->fontHeight;

   /* Dbp2( "Mouse y: %d jmouse->y: %d\n", y, jmouse->y ); */

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
   /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */

}

/*}}}*/

/************************************
* getJGtkSubWin
*
* group:
*
*
* debug print:
*
*
************************************/

static GtkSubWinType *
getJGtkSubWin( GtkWidget *w )
{
   GtkSubWinType *tmpW = JGtkWin->subWins;

   do
     {
	if ( tmpW->edWin == w ||
	     tmpW->stWin == w )
	  {
	     return( tmpW );
	  }
	tmpW = tmpW->next;
     }
   while ( tmpW != JGtkWin->subWins );

   return( NULL );
}

/************************************
* translateCoord
*
* group:
*
*
* debug print: "FontHeight: %d, Widget: %p, x: %d, y: %d\n", JGtkWin->fontHeight, w, x, y
*
************************************/

static void
translateCoord( GtkWidget *w, int x, int y, int *newX, int *newY )
{
   /* GtkSubWinType *tmpWin = JGtkWin->subWins; */
   GtkSubWinType *actSubWin = getJGtkSubWin( w );
   int actSubWinSy;

   /* Dbp1( "ActSubWin: %p\n", actSubWin ); */

   /* printSubWins(); */

   *newX = x;

   if ( actSubWin )
     {
	if ( actSubWin->edWin == w )
	  {  /* edit windows */
	     *newY = actSubWin->jedWin->sy  * JGtkWin->fontHeight + y;
	  }
	else
	  {  /* status windows */
	     *newY = ( actSubWin->jedWin->sy  + actSubWin->jedWin->rows ) *
	       JGtkWin->fontHeight + y;
	  }
     }
   else
     { /* minibuffer window */
	*newY = ( Jed_Num_Screen_Rows - 1 ) * JGtkWin->fontHeight + y;
     }
}

/*************************/

#define MOUSE_DRAG_THRESHOLD 3

/* if force is true, wait for an event.  If force is false, only
 *  process events that exist.  This will Return either when there
 *  are no more events or a key/mouse event is processed Returning
 *  1 in the process */
/************************************
* X_process_events
*
* group:
*
*
* debug print:
*
*
************************************/

static int
X_process_events( int force, char *buf,
		  unsigned int buflen,
		  int *n_chars )  /*{{{*/
{
   char *savBuf;
   int xGtkPend, jXGtkPend;
   XEvent report;
   JXGtkEventType jXEv;
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

   /* printf( "File: %s Line: %d X_process_events\n", __FILE__, __LINE__ ); */

   /* while ( force || XGtkPending( This_XDisplay ) || jGtkPending( This_XDisplay ) ) */
   /* while ( force || jGtkPending( This_XDisplay ) ) */
   while ( force || jGtkPending() )
     {
	/* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */

     /* if ( jGtkPending( This_XDisplay ) ) */
	if ( jGtkPending() )
	  {
        /* JXGtkNextEvent( This_XDisplay, &jXEv ); */
	     JXGtkNextEvent( &jXEv );

	     switch ( jXEv.type )
	       {
		case JXGtkGdkEventID:
		  switch ( jXEv.gdkEvent.gdkEvent.type )
		    {
		     case GDK_KEY_PRESS:
	          /* printf( "File: %s, Line: %d: JXEvent, KeyPress\n", __FILE__, __LINE__ ); */
		       bufp = buf;

		       *n_chars = 0;

		       if ( jXEv.gdkEvent.gdkEvent.key.keyval <= 0xFF )
			 {
	             /* printf( "File: %s, Line: %d: buf: %p, bufp %p\n", __FILE__, __LINE__, buf, bufp ); */

			    bufp = SLutf8_encode( gdk_keyval_to_unicode( jXEv.gdkEvent.gdkEvent.key.keyval ),
						  buf,
						  buflen );
	             /* printf( "File: %s, Line: %d: buf: %p, bufp %p\n", __FILE__, __LINE__, buf, bufp ); */

			    if ( bufp )
			      {
	                /* printf( "File: %s, Line: %d: buf: %p, bufp %p\n", __FILE__, __LINE__, buf, bufp ); */
				 *n_chars = bufp - buf;
				 if ( *n_chars == 1 && ( jXEv.gdkEvent.gdkEvent.key.state & GDK_CONTROL_MASK ) )
				   {
	                   /* printf( "File: %s, Line: %d: buf: %p, bufp %p\n", __FILE__, __LINE__, buf, bufp ); */
				      if ( *buf >= 'a' && *buf <= 'z' ) *buf = *buf - 0x60;
				      if ( *buf >= 'A' && *buf <= 'Z' ) *buf = *buf - 0x40;
				   }
			      }
			    else *n_chars = 0;
			 }
		       else
			 {
		     /*   I do not know why!!! */
			    *buf = 1;
			 }

		       bufp = ( char * ) map_keysym_to_keyseq( jXEv.gdkEvent.gdkEvent.key.keyval, jXEv.gdkEvent.gdkEvent.key.state &
							       ( ShiftMask | ControlMask ) );
	          /* printf( "File: %s, Line: %d: bufp: %p\n", __FILE__, __LINE__, bufp ); */

		       if (bufp != NULL)
			 {
			    *n_chars = (unsigned char) *bufp++;
#if USE_NEW_META_CODE
			    if ( jXEv.gdkEvent.gdkEvent.key.state & JX_MetaMask)
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
			    if ( bufp == NULL )
			      bufp = buf;

			    if ( jXEv.gdkEvent.gdkEvent.key.state & JX_MetaMask )
			      {
				 ch1 = *bufp;
				 if ( X_Alt_Char <= 0 ) *buf |= 0x80;
				 else
				   {
				      *bufp++ = (unsigned char) X_Alt_Char;
				      *bufp = (unsigned char) ch1;
				      *n_chars = 2;
				   }
			      }
			    else if ( jXEv.gdkEvent.gdkEvent.key.state & ControlMask )
			      {
				 if (*buf == ' ') *buf = 0;
				 else if (*buf == '-') *buf = 0x1F;
			      }
			 }

		       if ( *n_chars == 0 ) break;
		       return 1;
		       break;
		     case GDK_MOTION_NOTIFY:
	          /* printf( "File: %s Line: %d GDK_MOTION_NOTIFY\n", __FILE__, __LINE__ ); */

	          /* Make sure we get the last event of this type */
		       while ( JXGtkCheckMaskEvent( GDK_MOTION_NOTIFY, &jXEv ) )
			 ;
	          /* printf( "File: %s Line: %d GDK_MOTION_NOTIFY\n", __FILE__, __LINE__ ); */

		       if ( jXEv.gdkEvent.w )
			 {
			    translateCoord( jXEv.gdkEvent.w,
					    jXEv.gdkEvent.gdkEvent.button.x,
					    jXEv.gdkEvent.gdkEvent.button.y,
					    &posx, &posy );
			 }

		       keys_buttons = jXEv.gdkEvent.gdkEvent.motion.state;

	          /* This will ensure that modifier keys are not pressed while
	          we are in motion. */
	          /* printf( "File: %s Line: %d GDK_MOTION_NOTIFY\n", __FILE__, __LINE__ ); */

		       if ( ( last_event == GDK_MOTION_NOTIFY )
			    && ( motion_state != keys_buttons ) )
			 break;
	          /* printf( "File: %s Line: %d GDK_MOTION_NOTIFY\n", __FILE__, __LINE__ ); */

		       motion_state = keys_buttons;

		       memset( ( char * ) &jmouse, 0, sizeof( jmouse ) );
		       last_x = jmouse.x;
		       last_y = jmouse.y;
	          /* printf( "File: %s Line: %d GDK_MOTION_NOTIFY\n", __FILE__, __LINE__ ); */

		       jGtkFillJMouse( &jmouse, JMOUSE_DRAG,
				       posx, posy, jXEv.gdkEvent.gdkEvent.motion.time,
				       last_motion_down_button, keys_buttons );
	          /* printf( "File: %s Line: %d GDK_MOTION_NOTIFY\n", __FILE__, __LINE__ ); */

 		  /* Dbp3( " Button_press_x: %d, posx: %d: Diff abs: %d\n", button_press_x, posx, abs( button_press_x - posx ) ); */
 		  /* Dbp3( " Button_press_y: %d, posy: %d: Diff abs: %d\n", button_press_y, posy, abs( button_press_y - posy ) ); */

#if MOUSE_DRAG_THRESHOLD
		       if ( ( abs(button_press_x - posx ) <= MOUSE_DRAG_THRESHOLD )
			    && ( abs( button_press_y - posy ) <= MOUSE_DRAG_THRESHOLD ) )
			 {
		     /* Db; */
			    break;
			 }
#endif
	          /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
		       if ((last_x == jmouse.x) && (last_y == jmouse.y)) break;
	          /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
		       if (-1 == (ch1 = jed_mouse_add_event (&jmouse)))
			 break;		       /* queue full */

	           /* return ESC ^@ */
	          /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
		       *buf++ = esc; *buf++ = 0; *buf++ = ch1;
	          /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
		       *n_chars = 3;

	          /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
		       last_event = MotionNotify;
	          /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
		       return 1;

		     case GDK_BUTTON_PRESS:
		  /* gtkButtonEvent = 1; */
	          /* Prohibit dragging more than one button at a time. */
	          /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
		       if ( last_event == GDK_MOTION_NOTIFY ) break;
	            /* drop */

		     case GDK_BUTTON_RELEASE:
		  /* gtkButtonEvent = 1; */
	          /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
		       if ( ( last_event == GDK_MOTION_NOTIFY ) &&
			    ( jXEv.gdkEvent.gdkEvent.button.button != (unsigned int) last_motion_down_button))
			 break;

	     	  /* printf( "File: %s Line: %d Button press\n", __FILE__, __LINE__ ); */
		       last_event = 0;
	          /* printf( "File: %s Line: %d Button press\n", __FILE__, __LINE__ ); */

		       if ( jXEv.gdkEvent.w )
			 {
			    translateCoord( jXEv.gdkEvent.w,
					    jXEv.gdkEvent.gdkEvent.button.x,
					    jXEv.gdkEvent.gdkEvent.button.y,
					    &rootx, &rooty );
		     /* Dbp2( "Posx: %d, Posy: %d\n", rootx, rooty ); */

			    jGtkFillJMouse (&jmouse,
					    ((jXEv.gdkEvent.gdkEvent.type == GDK_BUTTON_RELEASE ) ? JMOUSE_UP : JMOUSE_DOWN ),
					    rootx, rooty, jXEv.gdkEvent.gdkEvent.button.time,
					    jXEv.gdkEvent.gdkEvent.button.button, jXEv.gdkEvent.gdkEvent.button.state);
	     	  /* printf( "File: %s Line: %d\n", __FILE__, __LINE__ ); */
			 }
		       else
			 {
			    jGtkFillJMouse (&jmouse,
					    ((jXEv.gdkEvent.gdkEvent.type == GDK_BUTTON_RELEASE ) ? JMOUSE_UP : JMOUSE_DOWN ),
					    jXEv.gdkEvent.gdkEvent.button.x, jXEv.gdkEvent.gdkEvent.button.y, jXEv.gdkEvent.gdkEvent.button.time,
					    jXEv.gdkEvent.gdkEvent.button.button, jXEv.gdkEvent.gdkEvent.button.state);

			 }

#if MOUSE_DRAG_THRESHOLD
		       if ( jXEv.gdkEvent.gdkEvent.type == GDK_BUTTON_PRESS )
			 {
			    button_press_x = rootx;
			    button_press_y = rooty;
			 }
		       else
			 {
			    button_press_x = 0;
			    button_press_y = 0;
			 }
	          /* printf( "File: %s Line: %d Button press\n", __FILE__, __LINE__ ); */

#endif

		       if ( -1 == (ch1 = jed_mouse_add_event (&jmouse)))
			 break;		       /* queue full */

		       if ( ( jXEv.gdkEvent.gdkEvent.type == GDK_BUTTON_PRESS ) &&
			    ( 0 == ( jXEv.gdkEvent.gdkEvent.button.state &
				     ( GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK |
				       GDK_BUTTON4_MASK | GDK_BUTTON5_MASK ) ) ) )
			 last_motion_down_button = jXEv.gdkEvent.gdkEvent.button.button;

		       savBuf = buf;

	          /* ESC ^@ is a  mouse prefix */
		       *buf++ = esc; *buf++ = 0; *buf++ = ch1;
		       *n_chars = 3;
	          /* printf( "File: %s Line: %d Button press (%2x|%2x|%2x)\n", __FILE__, __LINE__, savBuf[0], savBuf[1], savBuf[2] ); */

		       return 1;
		     default:
		       printf( "Unknown GdkEvent: %s, %d.\n", __FILE__, __LINE__ );
		       break;
		    }
		  break;
		case JXKeyFeedID:
	     /* Dbp3( "Key Feed Event: keys: |%p|: %d, %p\n", jXEv.keyFeed.keys, *( jXEv.keyFeed.keys ), *( jXEv.keyFeed.keys + 1 ) ); */
		  strncpy( buf, ( jXEv.keyFeed.keys ) + 1, ( *( jXEv.keyFeed.keys ) ) - 1 );
		  *n_chars = ( *( jXEv.keyFeed.keys ) ) - 1;
		  SLfree( jXEv.keyFeed.keys );
	     /* Dbp1( "n_chars: %d\n", *n_chars ); */
		  return 1;
		  break;
		default:
		  printf( "Unknown JXGtkEvent: %s, %d!!!\n", __FILE__, __LINE__ );
		  break;
	       }
	  }

     }

   return 0;
}

/*}}}*/

/************************************
* X_read_key
*
* group: interface
*
*
* debug print:
*
*
************************************/

static int
X_read_key( void )  /*{{{*/
{
   int nread;
   char buf[64];

   /* printf( "File: %s Line: %d X_read_key before\n", __FILE__, __LINE__ ); */
   (void) X_process_events (1, buf, sizeof (buf), &nread);
   /* printf( "File: %s Line: %d X_read_key after, nread: %d\n", __FILE__, __LINE__, nread ); */
   if (nread > 1) ungetkey_string(buf + 1, nread - 1);
   /* Dbp2( "Nread: %d, *buf: %d\n", nread, *buf ); */
   if ( *buf == (char) Jed_Abort_Char )
     {
	if ( Ignore_User_Abort == 0 )
	  SLang_set_error (USER_BREAK);
	SLKeyBoard_Quit = 1;
      /* Dbp2( "SLang_get_error: %d, USER_BREAK: %d\n", SLang_get_error(), USER_BREAK ); */
     }
   return (int) *buf;
}

/*}}}*/

/************************************
* X_input_pending
*
* called by sys_input_pending
*
* group: interface
*
*
* debug print:
*
*
************************************/

static int
X_input_pending( void )  /*{{{*/
{
   XEvent ev;
   int n;

   while ( gtk_events_pending() )
     {
	gtk_main_iteration_do( False );
     }

   if ( jGtkPending() )
     {
	return( 1 );
     }

   return 0;
}

/*}}}*/

/************************************
* createKeyEvents
*
* group:
*
*
* debug print:
*
*
************************************/

int jgtk_createKeyEvents( char *keyStr )
{
   FilterEventData *eB = thisEventBuffer;
   JXGtkEventType *newEv = ( JXGtkEventType * ) SLmalloc( sizeof( JXGtkEventType ) );

   /* Dbp3( "String: |%p|-|%p|-|%p|\n", keyStr[0], keyStr[1], keyStr[2] ); */

   newEv->type = JXKeyFeedID;
   newEv->any.type = JXKeyFeedID;
   newEv->keyFeed.type = JXKeyFeedID;
   /* newEv->keyFeed.keys = keyStr; */
   newEv->keyFeed.keys = SLmalloc( sizeof( ( *keyStr ) + 1 ) );
   strncpy( newEv->keyFeed.keys, keyStr, *keyStr );
   newEv->keyFeed.keys[(unsigned char) *keyStr] = '\0';
   eB->jXGtkEvList = g_list_append( eB->jXGtkEvList, newEv );
   ( eB->jXGtkNumEv ) += 1;

   return 0;
}

/************************************
* JX_get_display_size
*
* group:
*
*
* debug print:
*
*
************************************/

static void
JX_get_display_size( int *rows, int *cols )  /*{{{*/
{
   int sy, sx, i;
   int newHeight, newWidth;
   int hCheck;
   GtkSubWinType *sw;
   JGtkWinType *win = JGtkWin;

   if ( win->subWins )
     {
	sw = win->subWins;

	hCheck =
	  sy = win->subWins->jedWin->sy;
	sx = 0;

      /********
      printWidgetWinXRef();
      printSubWins();
      *******/

	do
	  {
	     newHeight = sw->edWin->allocation.height / win->fontHeight + 1;
	     hCheck += newHeight;
	     if ( newHeight < 3 ) newHeight = 3;
	     newWidth = win->appWInGrid->allocation.width / win->fontWidth;
	     sy = sy + sw->jedWin->rows + 1;
	     sw = sw->next;
	  }
	while ( sw != win->subWins );

	if ( win->miniWinSW )
	  {
	     sy++;
	     hCheck++;
	  }

	win->hCheck = hCheck;
	win->inGridHeight = sy;

      /* printf( "win->hCheck: %d, win->inGridHeight: %d\n", hCheck, win->inGridHeight ); */

	win->edWidth = newWidth;
	win->edHeight = sy;

	JX_Screen_Cols = win->edWidth;
	JX_Screen_Rows = win->edHeight;
     }
   else
     {
	JX_Screen_Cols = JGtkWin->width / JGtkWin->fontWidth;
	JX_Screen_Rows = JGtkWin->height / JGtkWin->fontHeight;
     }

   Jed_Num_Screen_Cols = JX_Screen_Cols;
   Jed_Num_Screen_Rows = JX_Screen_Rows;

   *cols = JX_Screen_Cols;
   *rows = JX_Screen_Rows;

   /* Dbp2( "Jed_Num_Screen_Cols: %d, Jed_Num_Screen_Rows: %d\n", Jed_Num_Screen_Cols, Jed_Num_Screen_Rows ); */
}
/*}}}*/

/************************************
* JX_set_term_vtxxx
*
* group:
*
*
* debug print:
*
*
************************************/

static void  JX_set_term_vtxxx (int *n ) /*{{{*/
{
   (void) n;
}

/*}}}*/

/************************************
* JX_narrow_width
*
* group:
*
*
* debug print:
*
*
************************************/

static void  JX_narrow_width (void ) /*{{{*/
{
}

/*}}}*/

/************************************
* JX_wide_width
*
* group:
*
*
* debug print:
*
*
************************************/

static void   JX_wide_width (void ) /*{{{*/
{
}

/*}}}*/

/************************************
* JX_enable_cursor_keys
*
* group:
*
*
* debug print:
*
*
************************************/

/* ???????????????????? */

static void  JX_enable_cursor_keys(void)  /*{{{*/
{
}

/*}}}*/

static void
jGtkSetFGBGColor( GtkWidget *w, int c )
{
   /* colorChangeFlag = 1; */
   gtk_widget_modify_fg( w, GTK_STATE_NORMAL,      DefaultGtkGCInfo[c].fg );
   gtk_widget_modify_fg( w, GTK_STATE_ACTIVE,      DefaultGtkGCInfo[c].fg );
   gtk_widget_modify_fg( w, GTK_STATE_PRELIGHT,    DefaultGtkGCInfo[c].fg );
   gtk_widget_modify_fg( w, GTK_STATE_SELECTED,    DefaultGtkGCInfo[c].fg );
   gtk_widget_modify_fg( w, GTK_STATE_INSENSITIVE, DefaultGtkGCInfo[c].fg );

   gtk_widget_modify_bg( w, GTK_STATE_NORMAL,      DefaultGtkGCInfo[c].bg );
   gtk_widget_modify_bg( w, GTK_STATE_ACTIVE,      DefaultGtkGCInfo[c].bg );
   gtk_widget_modify_bg( w, GTK_STATE_PRELIGHT,    DefaultGtkGCInfo[c].bg );
   gtk_widget_modify_bg( w, GTK_STATE_SELECTED,    DefaultGtkGCInfo[c].bg );
   gtk_widget_modify_bg( w, GTK_STATE_INSENSITIVE, DefaultGtkGCInfo[c].bg );
   /* colorChangeFlag = 0; */
}

/************************************
* JX_cls
*
* group: interface
*
*
* debug print: "\n"
*
************************************/

static void
JX_cls( void )  /*{{{*/
{
   GtkSubWinType *sw = JGtkWin->subWins;
   GtkSubWinType *swBeg = sw;

   if ( !sw ) return;

   if (No_XEvents) return;

   if ( !doCLS ) return;

   if ( JGtkWin->window_mapped == 0) return;

   do
     {
      /* gdk_draw_rectangle( sw->edWin, sw->edWin->style->white_gc, 1, 0, 0, xxx, yyy ); */
      /* gdk_draw_rectangle( sw->stWin, sw->stWin->style->white_gc, 1, 0, 0, xxx, yyy ); */
	if ( sw->edWin->window )
	  {
	     if ( doColorSetup )
	       jGtkSetFGBGColor( sw->edWin, JNORMAL_COLOR );
	     gdk_window_clear( sw->edWin->window );
	  }
	else
	  {
	 /* Dbp1( "EdWin NULL should not occur!!!\n", 1 ); */
	 /********
	 printWidgetWinXRef();
	 printSubWins();
	 ******/
	  }

	if ( sw->stWin->window )
	  {
	     if ( doColorSetup )
	       jGtkSetFGBGColor( sw->stWin, JNORMAL_COLOR );
	     gdk_window_clear( sw->stWin->window );
	  }
	else
	  {
	 /* Dbp1( "StWin NULL should not occur!!!\n", 1 ); */
	 /*******
	 printWidgetWinXRef();
	 printSubWins();
	 ******/
	  }
	sw = sw->next;
     }
   while ( sw != swBeg );

   /********
   gdk_draw_rectangle( JGtkWin->appWDrawArea->window, JGtkWin->appWDrawArea->style->white_gc, 1,
    		       0, 0, JGtkWin->width, JGtkWin->height );
   *******/
}

/*}}}*/

/* This routine is called from S-Lang inner interpreter.  It serves
   as a poor mans version of an interrupt 9 handler */
/************************************
* xjed_check_kbd
*
* group: interface
*
*
* debug print:
*
*
************************************/

static void
xjed_check_kbd( void )  /*{{{*/
{
   char buf[64];
   int n;
   register char *b, *bmax;

   if (Batch || No_XEvents) return;

   while ( jGtkPending() )
     {
      /* printf( "File: %s Line: %d xjed_check_kbd before\n", __FILE__, __LINE__ ); */

	if (X_process_events (0, buf, sizeof (buf), &n) == 0) continue;

      /* printf( "File: %s Line: %d xjed_check_kbd after\n", __FILE__, __LINE__ ); */

	b = buf; bmax = b + n;
	while (b < bmax)
	  {
	     if ( *b == (char) Jed_Abort_Char )
	       {
		  if ( Ignore_User_Abort == 0 )
		    SLang_set_error (USER_BREAK);
		  if ( b != buf ) buffer_keystring (buf, (int) (b - buf));
		  SLKeyBoard_Quit = 1;
	    /* Dbp2( "SLang_get_error: %d, USER_BREAK: %d\n", SLang_get_error(), USER_BREAK ); */
		  break;
	       }
	     b++;
	  }
	if (!SLKeyBoard_Quit) buffer_keystring (buf, n);
     }
}

/*}}}*/

/************************************
* xjed_suspend
*
* group: interface
*
*
* debug print:
*
*
************************************/

static void
xjed_suspend( void )  /*{{{*/
{
   if ( No_XEvents ) return;
   /* if ( XWin->focus ) */
   if ( JGtkWin->focus )
     {
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

/************************************
* set_window_name
*
* debug print: "Name: |%s|\n", s
*
************************************/

static void
set_window_name( char *s ) /*{{{*/
{
   if ( Batch ) return;
   if ( JGtkWin->appW )
     gtk_window_set_title( GTK_WINDOW( JGtkWin->appW ), s );
   /* XStoreName (This_XDisplay, XWin->w, s); */
}

/************************************
* set_icon_name
*
* debug print: "Icon file name: |%s|\n", s
*
************************************/

static void
set_icon_name( char *s ) /*{{{*/
{
   if ( Batch ) return;
   if ( JGtkWin->appW )
     gtk_window_set_default_icon_from_file( s, NULL );

   /* XSetIconName(This_XDisplay, XWin->w, s); */
}

/************************************
* setupGtkColor
*
* group:
*
*
* debug print:
*
*
************************************/

static char *
setupGtkColor( char *colName, GdkColor **oldColP, char *defColName, GdkColor *defCol )
{
   char *newName;

   if ( *oldColP )
     {
	SLfree( ( char * ) *oldColP );
     }
   *oldColP = ( GdkColor * ) SLmalloc( sizeof( GdkColor ) );

   if ( colName )
     {
	if ( strcmp( colName, "default" ) )
	  {
	     if ( !strncmp( colName, "bright", 6 ) )
	       {
		  colName += 6;
	       }

	     if ( gdk_color_parse( colName, *oldColP ) )
	       {
		  newName = SLmalloc( strlen( colName ) + 1 );
		  strcpy( newName, colName );
		  gdk_colormap_alloc_color( JGtkWin->colormap,
					    *oldColP,
					    1, 1 );
	       }
	     else
	       {
		  newName = SLmalloc( strlen( defColName ) + 1 );
		  strcpy( newName, defColName );
		  memcpy( *oldColP, defCol, sizeof( GdkColor ) );
	       }
	  }
	else
	  {
	     newName = SLmalloc( strlen( defColName ) + 1 );
	     strcpy( newName, defColName );
	     memcpy( *oldColP, defCol, sizeof( GdkColor ) );
	  }
     }
   else
     {
	newName = SLmalloc( strlen( defColName ) + 1 );
	strcpy( newName, defColName );
	memcpy( *oldColP, defCol, sizeof( GdkColor ) );
     }

   return( newName );
}

/************************************
* setupAndParseGtkColors
*
* group:
*
*
* debug print:
*
*
************************************/

static int
setupAndParseGtkColors(void)
{
   int i;
   char *savName;
   GdkColor *fgCol, *bgCol;
   GCGtkInfoType   *savGtkGCInfo;

   savGtkGCInfo = DefaultGtkGCInfo;

   JGtkWin->textGC = DefaultGtkGCInfo;

   fgCol = ( GdkColor * ) SLmalloc( sizeof( GdkColor ) );
   bgCol = ( GdkColor * ) SLmalloc( sizeof( GdkColor ) );

   fgCol->pixel = 0; fgCol->red = 0x0;    fgCol->green = 0x0;    fgCol->blue = 0x0;
   bgCol->pixel = 0; bgCol->red = 0xffff; bgCol->green = 0xffff; bgCol->blue = 0xffff;

   JGtkWin->textGC->fg = fgCol;
   JGtkWin->textGC->bg = bgCol;

   savName = JGtkWin->textGC->fgName;
   JGtkWin->textGC->fgName = SLmalloc( strlen( savName ) + 1 );
   strcpy( JGtkWin->textGC->fgName, savName );

   savName = JGtkWin->textGC->bgName;
   JGtkWin->textGC->bgName = SLmalloc( strlen( savName ) + 1 );
   strcpy( JGtkWin->textGC->bgName, savName );

   for ( i = 1; i < JMAX_COLORS; ++i )
     {
      /* printf( "file: %s line: %d: i: %d\n", __FILE__, __LINE__, i ); */
      /* printf( "FG: %s, BG: %s\n", DefaultGtkGCInfo[i].fgName, DefaultGtkGCInfo[i].bgName ); */

	DefaultGtkGCInfo[i].fgName = setupGtkColor( DefaultGtkGCInfo[i].fgName,
						    &DefaultGtkGCInfo[i].fg,
						    JGtkWin->textGC->fgName,
						    JGtkWin->textGC->fg );

	DefaultGtkGCInfo[i].bgName = setupGtkColor( DefaultGtkGCInfo[i].bgName,
						    &DefaultGtkGCInfo[i].bg,
						    JGtkWin->textGC->bgName,
						    JGtkWin->textGC->bg );
     }
   return( 0 );
}

/* #if 0 */

/************************************
* setup_and_parse_colors
*
* group:
*
*
* debug print:
*
*
************************************/
#if 0
static int
setup_and_parse_colors (void ) /*{{{*/
{
   unsigned long fg, bg, tmp;
   char *fg_name, *bg_name;
   int i;

   setupAndParseGtkColors();

   return 0;
}

/*}}}*/
#endif

/* #endif */

/* ??????????????? */

/************************************
* set_mouse_color
*
* group:
*
*
* debug print:
*
*
************************************/

static void
set_mouse_color( char *fgc, char *bgc ) /*{{{*/
{
   XColor xfg, xbg;

}

/************************************
* create_XWindow
*
* group:
*
*
* debug print:
*
*
************************************/

static void
create_XWindow(void)  /*{{{*/
{
   int i;
   int bdr, x, y, flags;
   unsigned int width, height;
   XSizeHints sizehint;
   XClassHint xcls;
   XWMHints wmhint;
   long key_event_type;
   FilterEventData *tmpData;
   GdkDisplay *tmpDisp;

   thisEventBuffer =
     tmpData = ( FilterEventData * ) SLmalloc( sizeof( FilterEventData ) );

   /* tmpData->win =          win; */
   /* tmpData->evList =       NULL; */
   /* tmpData->numEv =        0; */
   tmpData->jXGtkEvList =  NULL;
   tmpData->jXGtkNumEv  =  0;

   tmpDisp = gdk_display_get_default();

   JGtkWin->height = /* win->height = */ 200;
   JGtkWin->width  = /* win->width = */  200;

   createGtkMainWindow();

   JGtkWin->numWin = 0;
   JGtkWin->gutSize = 0;
   JGtkWin->hCheck = 0;
   JGtkWin->inGridHeight = 0;
   JGtkWin->inGridHeightPixel = 0;
   JGtkWin->inGridWidthPixel = 0;
   JGtkWin->subWins = NULL;
   JGtkWin->miniWinSW = NULL;

   JGtkWin->maxEdHeight = MAX_EDHEIGHT;
   JGtkWin->maxEdWidth  = MAX_EDWIDTH;

   JGtkWin->widgetWinXRef = ( GtkSubWinType *** ) SLmalloc( sizeof( GtkSubWinType ** ) * JGtkWin->maxEdHeight );

   /* printf( "JGtkWin->maxEdHeight: %d\n", JGtkWin->maxEdHeight ); */

   for ( ; JGtkWin->maxEdHeight; --JGtkWin->maxEdHeight )
     {
	JGtkWin->widgetWinXRef[JGtkWin->maxEdHeight - 1] = ( GtkSubWinType ** )
	  SLmalloc( sizeof( GtkSubWinType * ) * JGtkWin->maxEdWidth );
#if 0
	for ( i = 0; i < JGtkWin->maxEdWidth; ++i )
	  {
	 /* printf( "JGtkWin->maxEdHeigth: %d, i: %d\n", JGtkWin->maxEdHeight, i ); */
	 /* printf( "JGtkWin->widgetWinXRef: %p\n", JGtkWin->widgetWinXRef ); */
	     JGtkWin->widgetWinXRef[JGtkWin->maxEdHeight - 1][i] = NULL;
	  }
#endif
     }

   modWidgetWinXRef( JGtkWin->widgetWinXRef, NULL, 0, 0, JGtkWin->maxEdWidth, JGtkWin->maxEdHeight );

   JGtkWin->maxEdHeight = MAX_EDHEIGHT;
#if 0
   Return( ( Window ) 5 /* DUMMY */ );
#endif
}

/*}}}*/

void jGtkAttachMenubar( GtkWidget *mb )
{
   if ( mb == JGtkWin->appWMenuBar ) return;

   gtk_box_pack_start( GTK_BOX( JGtkWin->appWGrid ), mb, False, False, 0 );
   gtk_box_reorder_child( GTK_BOX( JGtkWin->appWGrid ), mb, 1 );
   if ( JGtkWin->appWMenuBar )
     gtk_container_remove( GTK_CONTAINER( JGtkWin->appWGrid ), JGtkWin->appWMenuBar );
   JGtkWin->appWMenuBar = mb;
   gtk_widget_show_all( mb );
}

/************************************
* jGtkAddToolbar
*
* debug print: "tb: %p, where: %d\n", tb, where
*
************************************/

void jGtkAddToolbar( GtkWidget *tb, int where )
{
   int n;

   if ( !GTK_IS_CONTAINER( JGtkWin->appWTbGrid ) )
     {
      /* Dbp1( "Internal error: appWTbGrid: %p not a container!!\n", JGtkWin->appWTbGrid ); */
     }

   if ( where != -1 )
     {
	n = g_list_length( gtk_container_get_children( GTK_CONTAINER( JGtkWin->appWTbGrid ) ) );

	if ( where > n ) where = n;
     }

   /* Dbp1( "Where: %d\n", where ); */

   gtk_box_pack_start( GTK_BOX( JGtkWin->appWTbGrid ), tb, False, False, 0 );

   gtk_box_reorder_child( GTK_BOX( JGtkWin->appWTbGrid ), tb, where );
   gtk_widget_show_all( tb );
}

/************************************
* x_err_handler
*
* group:
*
*
* debug print:
*
*
************************************/
#if 0
static int
x_err_handler (Display *d, XErrorEvent *ev ) /*{{{*/
{
   char errmsg[256];

   return 1;
}

/*}}}*/
#endif
/************************************
* x_ioerr_handler
*
* group:
*
*
* debug print:
*
*
************************************/
#if 0
static int
x_ioerr_handler (Display *d ) /*{{{*/
{
   No_XEvents = 1;
   exit_error("XWindows IO error", 0);
   return d == NULL;  /* just use d to avoid a warning */
}

/*}}}*/
#endif

/************************************
* open_Xdisplay
*
* group:
*
*
* debug print:
*
*
************************************/

static int
open_Xdisplay (void ) /*{{{*/
{
   char dname [256];
   char *n;
   int numArg = 0;

   memset( (char *) JGtkWin, 0, sizeof( JGtkWinType ));

   /* gtk_init( &numArg, NULL ); */

   JGtkWin->display = gdk_display_get_default();

   JGtkWin->screen = gdk_display_get_default_screen( JGtkWin->display );
   JGtkWin->colormap = gdk_screen_get_default_colormap( JGtkWin->screen );

#define FONT "monospace 6"

   JGtkWin->fontName = SLmalloc( strlen( FONT ) + 1 );
   strcpy( JGtkWin->fontName, FONT );

   JGtkWin->XDisplay = gdk_x11_display_get_xdisplay( JGtkWin->display );

   return 1;
}

#ifdef SIXTEEN_BIT_SYSTEM
# define VFILE_BUF_SIZE 1024
#else
# define VFILE_BUF_SIZE 4096
#endif

static char *
getLibFile( char *file ) /*{{{*/
{
   static char retFile[JED_MAX_PATH_LEN];
   char libfsl[JED_MAX_PATH_LEN], libfslc[JED_MAX_PATH_LEN];
   char *lib, *tmpLib, *type, *libf;
   unsigned int n;

   /* Dbp1( "file: |%s|\n", file ); */

   libf = file;
   /* lib = Jed_Library; */
   lib = NULL;

   if ( ( 0 == strncmp( file, "./", 2 ) ) ||
#ifdef IBMPC_SYSTEM
	( 0 == strncmp( file, ".\\", 2 ) ) ||
	( 0 == strncmp( file, "..\\", 3 ) ) ||
#endif
        ( 0 == strncmp( file, "../", 3 ) ) )
     {
	if ( NULL == ( lib = jed_get_cwd() ) )
	  lib = "";
     }
   else if ( SLpath_is_absolute_path( file ) )
     lib = "";
   /**************
   else if ( ( lib == NULL ) || ( *lib == 0 ) )
     exit_error( "JED_ROOT environment variable needs set.", 0 );
   ***************/
   else
     {
	tmpLib = SLpath_get_load_path ();      /* SLstring */
	if ((tmpLib == NULL) || (*tmpLib == 0))
	  exit_error("The JED_ROOT environment variable needs set.", 0);
	else
	  {
	     lib = ( char * ) SLmalloc( strlen( tmpLib ) + 1 );
	     strcpy( lib, tmpLib );
	  }
     }

   /* Dbp1( "Lib: |%s|\n", lib ); */

   n = 0;
   while ( 0 == SLextract_list_element (lib, n, ',', libfsl, sizeof( libfsl ) ) )
     {
	n++;

	fixup_dir(libfsl);
	safe_strcat (libfsl, file, sizeof (libfsl));
	safe_strcpy (libfsl, jed_standardize_filename_static(libfsl), sizeof (libfsl));

	libf = libfsl;

	if ( ( 1 == file_status( libf ) ) )
	  break;

	libf = NULL;
     }

   if ( libf )
     {
	safe_strcpy( retFile, libf, sizeof( retFile ) );

	return( retFile );
     }
   else
     {
	return( NULL );
     }
}

/*}}}*/

#define JGTK_NUM_KEYFILES 2

#define JGTK_KEYFILE_BASE_NAME   "gtkjed.ini"
#define JGTK_KEYFILE_HOME_NAME   "~/." JGTK_KEYFILE_BASE_NAME

static GKeyFile **
getKeyFiles(void)
{
   int i = 0;
   char *fn;
   char *expFn;
   GKeyFile **kf = ( GKeyFile ** ) SLmalloc( sizeof( GKeyFile * ) * ( JGTK_NUM_KEYFILES + 1 ) );

   kf[i] = g_key_file_new();

   fn = getLibFile( JGTK_KEYFILE_BASE_NAME );

   /* Dbp1( "NewTest: |%s|\n", fn ? fn : "(NULL)" ); */

   if ( fn )
     {
	if ( g_key_file_load_from_file( kf[i], fn, G_KEY_FILE_KEEP_TRANSLATIONS |
					G_KEY_FILE_KEEP_COMMENTS,
					NULL ) )
	  {
	     i++;
	     kf[i] = g_key_file_new();
	  }
     }

   fn = jed_expand_filename( SLmake_string( JGTK_KEYFILE_HOME_NAME ) );

   /* Dbp1( "Begin: %s!!\n", fn ); */

   if ( g_key_file_load_from_file( kf[i], fn,
				   G_KEY_FILE_KEEP_TRANSLATIONS |
				   G_KEY_FILE_KEEP_COMMENTS, NULL ) )
     {
	i++;
     }
   else
     {
	g_key_file_free( kf[i] );
     }

   /* Dbp1( "NewTest01: %d\n", i ); */

   kf[i] = NULL;
   return( kf );
}

static void
getGtkDefaults(void)
{
   XWindow_Arg_Type *jGtkArgs = jGtkArgList + XARG_START;
   GKeyFile **kfs = getKeyFiles();
   GKeyFile **tmpKfs;

   if ( *kfs )
     {
	while ( jGtkArgs->name != NULL )
	  {
	     if ( jGtkArgs->dflt == NULL )
	       {
		  jGtkArgs++;
		  continue;
	       }

	     if ( jGtkArgs->type != VOID_TYPE  &&
		  jGtkArgs->value == NULL )
	       {
		  static char *classNames[] =
		    {
		       "UGtkjed", "UGtkJed", "ugtkjed", NULL
		    };
		  char *p, *cn;
		  char **cnp;

		  p = NULL;
		  if ( Jed_UTF8_Mode )
		    {
		       cnp = classNames;
		       while( NULL != ( cn = *cnp++ ) )
			 {
			    tmpKfs = kfs;
			    while ( *tmpKfs )
			      {
				 p = g_key_file_get_string( *tmpKfs, cn, jGtkArgs->name, NULL );
				 if ( p ) break;
				 tmpKfs++;
			      }
			    if ( p != NULL )
			      break;
			 }
		    }

		  if ( p == NULL )
		    {
		       cnp = classNames;
		       while ( NULL != ( cn = *cnp++ ) )
			 {
			    cn++; /* Skip leading U */
			    tmpKfs = kfs;
			    while ( *tmpKfs )
			      {
				 p = g_key_file_get_string( *tmpKfs, cn, jGtkArgs->name, NULL );
				 if ( p ) break;
				 tmpKfs++;
			      }
			    if ( p != NULL )
			      break;
			 }
		    }

		  if ( p == NULL )
		    {
		       tmpKfs = kfs;
		       while ( *tmpKfs )
			 {
			    p = g_key_file_get_string( *tmpKfs, "UXTerm", jGtkArgs->name, NULL );
			    if ( p ) break;
			    tmpKfs++;
			 }
		       if ( p == NULL )
			 {
			    tmpKfs = kfs;
			    while ( *tmpKfs )
			      {
				 p = g_key_file_get_string( *tmpKfs, "XTerm", jGtkArgs->name, NULL );
				 if ( p ) break;
				 tmpKfs++;
			      }
			 }
		    }

		  if ( p != NULL )
		    jGtkArgs->value = p;
	       }

	     if ( jGtkArgs->value != NULL )
	       *jGtkArgs->dflt = jGtkArgs->value;

	 /* Dbp2( "Arg: |%s|: Value: |%s|\n", jGtkArgs->name, jGtkArgs->value ? jGtkArgs->value : "(NULL)" ); */

	     jGtkArgs++;
	  }
     }
}

/* returns socket descriptor */
/************************************
* init_Xdisplay
*
* group: interface
*
*
* debug print:
*
*
************************************/

static int
init_Xdisplay (void ) /*{{{*/
{
#ifdef XJED_USE_R6IM
   setlocale(LC_ALL, "");
#endif

   if (jGtkArgList[JGTK_ARG_NAME].value != NULL)
     {
	This_App_Name = jGtkArgList[JGTK_ARG_NAME].value;
     }

   /* Db; */

   getGtkDefaults();

   JGtkWin->border = atoi(This_Internal_Border_Name);
   if ( JGtkWin->border < 0)
     JGtkWin->border = 0;

#if XJED_HAS_XRENDERFONT
# if 0
   /* if a parameter to -fs was supplied, we assume the user wants XFT */
   if (strlen(This_Face_Size))
     {
	if ( ( JGtkWin->face_size = atof( This_Face_Size ) ) <= 0 )
	  JGtkWin->face_size = 0;
     }
   else
     /* there was no -fs, so we don't do anything */
     JGtkWin->face_size = 0;

    /* make sure that XWin->xftdraw is null in any case */
   JGtkWin->xftdraw = NULL;
# endif
#endif

   if ( -1 == setupAndParseGtkColors() )		       /* This allocs and parses colors */
     exit (1);

   create_XWindow();

   JGtkWin->window_mapped = 1;

   return ConnectionNumber( JGtkWin->XDisplay );
}

/*}}}*/

/************************************
* reset_Xdisplay
*
* group: interface
*
*
* debug print:
*
*
************************************/

static void
reset_Xdisplay (void ) /*{{{*/
{
}

/*}}}*/

#define UPCSE(x)  (((x) <= 'z') && ((x) >= 'a') ? (x) - 32 : (x))
/************************************
* myXstrcmp
*
* group:
*
*
* debug print:
*
*
************************************/

static int
myXstrcmp(char *a, char *b)  /*{{{*/
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
/************************************
* X_eval_command_line
*
* group:
*
*
* debug print:
*
*
************************************/

static int
X_eval_command_line (int argc, char **argv ) /*{{{*/
{
   char *arg;
   int i;
   int cpyArgc = argc;
   char **cpyArgv = ( char ** ) SLmalloc ( (1+argc) * sizeof( char * ) );
   int n;
   XWindow_Arg_Type *opt;

   if (cpyArgv == NULL)
     return -1;

   /******************************
   * Create a copy of argv because gtk_init might modify if
   ******************************/

   for ( i = 0; i < argc; ++i )
     {
	cpyArgv[i] = SLmalloc( 1+strlen( argv[i] ) );
	if (cpyArgv[i] == NULL)
	  return -1;

	strcpy( cpyArgv[i], argv[i] );
     }
   cpyArgv[argc] = NULL;

   gtk_init( &cpyArgc, &cpyArgv );

   /****************************
   * Overide original argv, because I don't know
   * what gtk did to the original cpyArgv/argv
   * array (might it be freed?).
   ****************************/

   n = argc - cpyArgc;

   argv[0] = cpyArgv[0]; /* program name */

   for ( i = 1; i < cpyArgc; ++i )
     { /*****************************
	* Recreate the format jed likes to process
	* argc and argv (this means unprocessed arguments
	* are left at the end of argv).
	******************************/
	argv[i + n] = cpyArgv[i];
     }

   for ( i = n + 1; i < argc; ++i )
     {
	arg = argv[i];
	if (*arg != '-') break;
	if ( 0 == strcmp( "--debug-xjed", arg ) ||
	     0 == strcmp( "--debug-gtkjed", arg ) )
	  {
	     Debug_gtkjed = 1;
	     continue;
	  }

	arg++;
	opt = jGtkArgList;
	while ( opt->name != NULL )
	  {
	     if ( STREQS(opt->name, arg )
		  || (( opt->name1 != NULL) && STREQS(opt->name1, arg ))) break;
	     opt++;
	  }

	if ( opt->name == NULL ) break;

	if ( opt->type == VOID_TYPE ) opt->value = "on";
	else if ( i + 1 < argc )
	  {
	     i++;
	     opt->value = argv[i];
	  }
	else break;
     }

   /*******************
   if ( argc > 1 )
     {
      Return( argc - 1 );
      }
   else
     {
      Return( argc );
      }
   ******************/
   return( i );
}

/*}}}*/

/************************************
* set_border_color
*
* group:
*
*
* debug print:
*
*
************************************/

static void
set_border_color (char *fgc, char *bgc ) /*{{{*/
{
   XColor xfg;
   unsigned int bdr = atoi(bgc);

}

/*}}}*/

/************************************
* JX_set_mono
*
* group:
*
*
* debug print:
*
*
************************************/

static JX_SETXXX_RETURN_TYPE
JX_set_mono (int obj_unused, char *unused, SLtt_Char_Type c_unused )
{
   (void) obj_unused;
   (void) unused;
   (void) c_unused;
   return JX_SETXXX_RETURN_VAL;
}

/************************************
* jGtkSetColor
*
* group:
*
*
* debug print:
*
*
************************************/

static JX_SETXXX_RETURN_TYPE
jGtkSetColor (int i, char *what, char *fg, char *bg )
{
   char *savName;
   GdkColor *fgCol, *bgCol;
   GdkColor *savCol;

   /* printf( "JGtk_set_color: File: %s line: %d: i: %d FG: %s, BG: %s\n", __FILE__, __LINE__, i, fg, bg ); */

   if ( JGtkWin->appW == NULL)
     return JX_SETXXX_RETURN_VAL;

   if (!Term_Supports_Color)
     return JX_SETXXX_RETURN_VAL;

#if SLANG_VERSION >= 10306
   SLsmg_touch_screen ();
#endif

   if (i == -1)
     {
	if ( !strcmp("mouse", what) )
	  {
	     set_mouse_color (fg, bg);
	  }
	else if (!strcmp("border", what))
	  {
	     set_border_color (fg, bg);
	  }
	return JX_SETXXX_RETURN_VAL;
     }

   savName = DefaultGtkGCInfo[i].fgName;
   DefaultGtkGCInfo[i].fgName = setupGtkColor( fg,
					       &( DefaultGtkGCInfo[i].fg ),
					       JGtkWin->textGC->fgName,
					       JGtkWin->textGC->fg );
   SLfree( savName );

   savName = DefaultGtkGCInfo[i].bgName;
   DefaultGtkGCInfo[i].bgName = setupGtkColor( bg,
					       &( DefaultGtkGCInfo[i].bg ),
					       JGtkWin->textGC->bgName,
					       JGtkWin->textGC->bg );
   SLfree( savName );

   return JX_SETXXX_RETURN_VAL;
}

/*}}}*/

/************************************
* JX_set_color
*
* group:
*
*
* debug print:
*
*
************************************/

static JX_SETXXX_RETURN_TYPE
JX_set_color (int i, char *what, char *fg, char *bg )
{

   jGtkSetColor( i, what, fg, bg );

   return JX_SETXXX_RETURN_VAL;
}

/*}}}*/

/************************************
* x_warp_pointer
*
* group:
*
*
* debug print:
*
*
************************************/

static void
x_warp_pointer (void ) /*{{{*/
{
   /* int x, y; */
   /* GtkWidget *w = getEdWinFromJedWin( JWindow ); */
   X_Warp_Pending = 1;

#if 0
   if ( w )
     {
	X_Warp_Pending = 1;

	gdk_window_get_origin( w->window, &x, &y );

	Dbp3( "Windows: %p, x: %d, y: %d\n", w->window, x, y );

	x = JGtkWin->fontWidth*( JGtkWin->vis_curs_col + 0.5 ) + x;
	y = JGtkWin->fontHeight*( JGtkWin->vis_curs_row - 0.5 ) + y;

	Dbp3( "Windows: %p, x: %d, y: %d\n", w->window, x, y );

	gdk_display_warp_pointer( gdk_drawable_get_display( ( GdkDrawable * ) w->window ),
				  gdk_drawable_get_screen( ( GdkDrawable * ) w->window ),
				  x, y );
     }
#endif

}

/*}}}*/

static void
jgtk_clear_clipboard( GtkClipboard *clb,
		      gpointer data )
{
}

static void
jgtk_provide_clipboard( GtkClipboard *clb,
			GtkSelectionData *data,
			guint info,
			gpointer userData )
{
   /* gtk_clipboard_set_text( clb, jgtkClipboardData, strlen( jgtkClipboardData ) ); */
   /* Dbp3( "ClipboardData: %p: len: %d, data: |%s|\n", jgtkClipboardData, strlen( jgtkClipboardData ), jgtkClipboardData ); */
   gtk_selection_data_set( data, gdk_atom_intern( "STRING", FALSE ), 8, jgtkClipboardData, strlen( jgtkClipboardData ) );
}

static void
jgtk_receive_clipboard_targets( GtkClipboard *clb,
				GdkAtom *atoms,
				gint n,
				gpointer data )
{
   int i;

   /* Dbp2( "Clipboard can be provided with %d target types by atoms: %p\n", n, atoms ); */

   for ( i = 0; i < n; ++i )
     {
      /* Dbp2( "Target(%3d): |%s|\n", atoms + i, gdk_atom_name( atoms[i] ) ); */
     }

}

static void
jgtk_receive_clipboard( GtkClipboard *clb,
			const gchar *text,
			gpointer data )
{
   /* Dbp1( "Text: |%s|\n", text ); */
   jed_insert_nbytes( text, strlen( text ) );
   update( NULL, 1, 0, 0 );
}

static int
jgtk_insert_clipboard(void)
{

   gtk_clipboard_request_text( gtk_clipboard_get( GDK_SELECTION_CLIPBOARD ),
			       jgtk_receive_clipboard,
			       NULL );

#if 0
   gtk_clipboard_request_targets( gtk_clipboard_get( GDK_SELECTION_CLIPBOARD ),
				  jgtk_receive_clipboard_targets,
				  NULL );
#endif
   return( 0 );
}

static void
jgtk_region_2_clipboard(void)
{
   int nbytes;
   GdkAtom textAtom = gdk_atom_intern( "STRING", FALSE );
   static GtkTargetEntry tt
     =
     {
	"STRING",
	GTK_TARGET_SAME_WIDGET
#if 0
	| GTK_TARGET_OTHER_APP | GTK_TARGET_OTHER_WIDGET
#endif
	| GTK_TARGET_SAME_APP,
	8
     };

   /*************
   gtk_clipboard_set_with_data( GtkClipboard *clb,
				const GtkTargetEntry *targets,
				guint n_targets,
				GtkClipboardGetFunc gf: jgtk_provide_clipboard,
				GtkClipboardClearFunc cf,
				gpointer userData );
   **************/
   if ( jgtkClipboardData )
     {
	SLfree( jgtkClipboardData );
     }

   jgtkClipboardData = make_buffer_substring( &nbytes );

   gtk_clipboard_set_with_data( gtk_clipboard_get( GDK_SELECTION_CLIPBOARD ),
				&tt,
				1,
				jgtk_provide_clipboard,
				jgtk_clear_clipboard,
				NULL );
}

/************************************
* x_insert_selection
*
* group:
*
*
* debug print:
*
*
************************************/

static int
x_insert_selection( void )
{
   /* if ( gtkButtonEvent ) */
   /*  { */
   return( gtkInsertSelection() );
   /*    } */
   /* Return 0; */
}

/************************************
* gtkInsertSelection
*
* group:
*
*
* debug print:
*
*
************************************/

static int
gtkInsertSelection( void )
{
   GdkAtom textAtom = gdk_atom_intern( "STRING", FALSE );

   /* printf( "File %s, Line %d: Insert gtk Selection!!!\n", __FILE__, __LINE__ ); */

   /***********
   gtk_selection_convert( JGtkWin->appWDrawArea, GDK_SELECTION_PRIMARY, textAtom,
			  GDK_CURRENT_TIME );
   ********************/

   if ( JGtkWin->appWMiniW )
     {
	gtk_selection_convert( JGtkWin->appWMiniW, GDK_SELECTION_PRIMARY, textAtom,
			       GDK_CURRENT_TIME );
     }

   return( 0 );
}

/************************************
* gtkSelectionReceive
*
* group:
*
*
* debug print:
*
*
************************************/

static void
gtkSelectionReceive( GtkWidget *w,
		     GtkSelectionData *selData,
		     gpointer usrData )
{
   GdkAtom textAtom = gdk_atom_intern( "STRING", FALSE );
   char *txt;

   if ( selData->length < 0 )
     {
      /* printf( "File %s, Line %d: Selection retrieval failed!!!\n", __FILE__, __LINE__ ); */
	return;
     }

   if ( selData->type != textAtom )
     {
      /* printf( "File %s, Line %d: Selection was not return as STRING!!!\n", __FILE__, __LINE__ ); */
	return;
     }

   txt = ( char * ) selData->data;

   jed_insert_nbytes( txt, selData->length );

   /* printf( "File %s, Line %d: Selectiontext returned:\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n", __FILE__, __LINE__, txt ); */
}

/************************************
* gtkRegionToSelection
*
* group:
*
*
* debug print:
*
*
************************************/

static void
gtkRegionToSelection( void )
{
   int nbytes;

   /* printf( "File %s, Line %d: gtkRegionToSelection!!!\n", __FILE__, __LINE__ ); */

   if ( jgtkSelectionData != NULL )
     {
	SLfree( jgtkSelectionData );
     }
   jgtkSelectionData = make_buffer_substring( &nbytes );

   if ( jgtkSelectionData == NULL ) return;

   if ( JGtkWin->appWMiniW )
     {
	gtk_selection_owner_set( JGtkWin->appWMiniW,
				 GDK_SELECTION_PRIMARY,
				 GDK_CURRENT_TIME );
     }

   /*************
   gtk_selection_owner_set( JGtkWin->appWDrawArea,
			    GDK_SELECTION_PRIMARY,
			    GDK_CURRENT_TIME );
   *******/
}

/************************************
* jGtkSendSelection
*
* group:
*
*
* debug print:
*
*
************************************/

static void
jGtkSendSelection( GtkWidget *w,
		   GtkSelectionData *selData,
		   guint             info,
		   guint             time_stamp,
		   gpointer          usrData )
{
   /* printf( "File %s, Line %d: jGtkSendSelection!!!\n", __FILE__, __LINE__ ); */

   gtk_selection_data_set( selData, GDK_SELECTION_TYPE_STRING,
			   8, jgtkSelectionData, strlen( jgtkSelectionData ) );
}

#if 0
typedef struct
Selection_Data_Type
{
   unsigned int len;
   struct Selection_Data_Type *next;
   unsigned char bytes[1];
}
Selection_Data_Type;
#endif

/************************************
* x_region_2_selection
*
* group:
*
*
* debug print:
*
*
************************************/

static void
x_region_2_selection (void )
{
   int nbytes;

   /* if ( gtkButtonEvent ) */
   /*  { */
   gtkRegionToSelection();
   /*   } */
}

/************************************
* x_server_vendor
*
* group:
*
*
* debug print:
*
*
************************************/

static char *
x_server_vendor (void )
{
   return( "" );
}

/************************************
* jgtk_server_vendor
*
* group:
*
*
* debug print:
*
*
************************************/

static void
jgtk_server_vendor (void )
{
}

#if SLANG_VERSION < 10404
/************************************
* get_termcap_string
*
* group:
*
*
* debug print:
*
*
************************************/

static char *
get_termcap_string  (char *cap)
{
   return "";
}
#endif

/************************************
* x_set_meta_keys
*
* group:
*
*
* debug print:
*
*
************************************/

static void x_set_meta_keys(int *maskp )
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

   MAKE_INTRINSIC_S(    "x_set_window_name", set_window_name, VOID_TYPE),
   MAKE_INTRINSIC_S( "jgtk_set_window_name", set_window_name, VOID_TYPE),
   MAKE_INTRINSIC_S(    "x_set_icon_name", set_icon_name, VOID_TYPE),
   MAKE_INTRINSIC_S( "jgtk_set_icon_name", set_icon_name, VOID_TYPE),

   MAKE_INTRINSIC(   "x_warp_pointer", x_warp_pointer, VOID_TYPE, 0),
   MAKE_INTRINSIC("jgtk_warp_pointer", x_warp_pointer, VOID_TYPE, 0),

#if 0
   /* Maybe some day */
   MAKE_INTRINSIC_S(  "jgtk_insert_clipboard_by_name",         jgtk_insert_clipboard_by_name, INT_TYPE, 0 ),
   MAKE_INTRINSIC_S(  "jgtk_get_clipboard_by_name",            jgtk_get_clipboard_by_name, STRING_TYPE ),
   MAKE_INTRINSIC_SS( "jgtk_set_clipboard_by_name",            jgtk_set_clipboard_by_name, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "jgtk_copy_region_to_clipboard_by_name", jgtk_region_2_clipboard_by_name, VOID_TYPE, 0 ),
#endif

   /* Cutbuffer no longer supported. */
   /* (see also: http://www.freedesktop.org/Standards/clipboards-spec) */
   /* Instead of cutbuffer gtk clipboard is used. */
   MAKE_INTRINSIC(   "x_insert_cutbuffer", jgtk_insert_clipboard, INT_TYPE, 0),
   MAKE_INTRINSIC("jgtk_insert_clipboard", jgtk_insert_clipboard, INT_TYPE, 0),
   /* Prototype: Integer x_insert_cutbuffer ();
    * Inserts cutbuffer into the current buffer and returns the number
    * of characters inserted.
    */
   MAKE_INTRINSIC("x_copy_region_to_cutbuffer", jgtk_region_2_clipboard, VOID_TYPE, 0),
   MAKE_INTRINSIC("jgtk_copy_region_to_clipboard", jgtk_region_2_clipboard, VOID_TYPE, 0),
   /*Prototype: Void x_copy_region_to_cutbuffer();
    */

   MAKE_INTRINSIC(    "x_insert_selection", x_insert_selection, SLANG_INT_TYPE, 0),
   MAKE_INTRINSIC( "jgtk_insert_selection", x_insert_selection, SLANG_INT_TYPE, 0),
   /* Prototype: Integer x_insert_selection ();
    * This function only requests selection data from the selection owner.
    * If Xjed received EVENT, Xjed inserts selection data into the current buffer.
    * And returns the number of characters inserted.
    */
   MAKE_INTRINSIC(    "x_copy_region_to_selection", x_region_2_selection, VOID_TYPE, 0),
   MAKE_INTRINSIC( "jgtk_copy_region_to_selection", x_region_2_selection, VOID_TYPE, 0),
   /*Prototype: Void x_copy_region_to_selection();
    */
   MAKE_INTRINSIC_IIS(    "x_set_keysym", x_set_keysym, VOID_TYPE),
   MAKE_INTRINSIC_IIS( "jgtk_set_keysym", x_set_keysym, VOID_TYPE),
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
   MAKE_INTRINSIC_I(    "x_set_meta_keys", x_set_meta_keys, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_I( "jgtk_set_meta_keys", x_set_meta_keys, SLANG_VOID_TYPE),

   MAKE_INTRINSIC( "x_server_vendor", x_server_vendor, STRING_TYPE, 0),
   /* Prototype: String x_server_vendor ();
    * This function returns the empty string for the X server.
    * Only provided for compatibily reasons.
    */
   MAKE_INTRINSIC( "jgtk_server_vendor", jgtk_server_vendor, VOID_TYPE, 0),
   /* Prototype: String x_server_vendor ();
    * Do nothing. Provided to identify gtkjed.
    */

   MAKE_INTRINSIC( "jgtk_print_widget_struct", printWidgetStruct, VOID_TYPE, 0 ),

#if 0
   MAKE_INTRINSIC( "scrollDown", jGtkScrollDown, SLANG_VOID_TYPE, 0 ),
   /* Prototype: int jGtkScrollDown( void );
    * Scrolls down one line. Cursor remains in the same line relative to
    * the window.
    */

   MAKE_INTRINSIC( "scrollUp", jGtkScrollUp, SLANG_INT_TYPE, 0 ),
   /* Prototype: int jGtkScrollUp( void );
    * Scrolls up one line. Cursor remains in the same line relative to
    * the window.
    */
#endif

   MAKE_INTRINSIC_S("jgtk_load_font", loadFont, VOID_TYPE ),

#if SLANG_VERSION < 10404
   MAKE_INTRINSIC_S("get_termcap_string", get_termcap_string, STRING_TYPE),
#endif
   MAKE_INTRINSIC(NULL,NULL,0,0)
};

/*}}}*/

static SLang_Intrin_Var_Type X_Variable_Table [] =
{
   MAKE_VARIABLE("ALT_CHAR", &X_Alt_Char, INT_TYPE, 0),
   MAKE_VARIABLE("X_LAST_KEYSYM", &X_Last_Keysym, INT_TYPE, 0),
   MAKE_VARIABLE(NULL,NULL,0,0)
};

/************************************
* X_init_slang
*
* group:
*
*
* debug print:
*
*
************************************/

static int  X_init_slang (void ) /*{{{*/
{
   /* cfXDVarAsk = 1; */
   if ((-1 == SLadd_intrin_fun_table (sl_x_table, "XWINDOWS"))
       || (-1 == SLadd_intrin_var_table (X_Variable_Table, NULL)))
     return -1;
   return 0;
}

/*}}}*/

/************************************
* X_update_open
*
* group: interface
*
*
* debug print:
*
*
************************************/

static void
X_update_open (void ) /*{{{*/
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

/************************************
* X_update_close
*
* group: interface
*
*
* debug print:
*
*
************************************/

static void
X_update_close (void ) /*{{{*/
{

   Performing_Update = 0;
   show_cursor ();

   if ( X_Warp_Pending )
     {
	int x, y;
	GtkWidget *w = getEdWinFromJedWin( JWindow );

	if ( w )
	  {
	     gdk_window_get_origin( w->window, &x, &y );

	 /* Dbp3( "Windows: %p, x: %d, y: %d\n", w->window, x, y ); */

	     x = JGtkWin->fontWidth*( JGtkWin->vis_curs_col + 0.5 ) + x;
	     y = JGtkWin->fontHeight*( JGtkWin->vis_curs_row - 0.5 ) + y;

	 /* Dbp3( "Windows: %p, x: %d, y: %d\n", w->window, x, y ); */

	     gdk_display_warp_pointer( gdk_drawable_get_display( ( GdkDrawable * ) w->window ),
				       gdk_drawable_get_screen( ( GdkDrawable * ) w->window ),
				       x, y );
	  }

	X_Warp_Pending = 0;
     }
}

/*}}}*/

/************************************
* funCallCB
*
* group:
*
*
* debug print:
*
*
************************************/

static int
funCallCB(void)
{
   ( *actFunToCall )( actParaData );
   return 0;
}

/************************************
* x_define_xkeys
*
* group: interface
*
*
* debug print:
*
*
************************************/

static void
x_define_xkeys (SLKeyMap_List_Type *map ) /*{{{*/
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

   SLkm_define_key( FC_CMD_KEY_SEQ, (FVOID_STAR) funCallCB, Global_Map );
}

/*}}}*/

/************************************
* JX_reset_video
*
* group: interface
*
*
* debug print:
*
*
************************************/

static int  JX_reset_video (void ) /*{{{*/
{
   JX_reset_scroll_region ();
   JX_goto_rc (0, 0);
   JX_normal_video ();
   /* Return vterm_reset_display (); */
   return 0;
}

/*}}}*/

/************************************
* JX_init_video
*
* group: interface
*
*
* debug print:
*
*
************************************/

static int  JX_init_video (void ) /*{{{*/
{
   JX_reset_video ();
   if ((JX_Screen_Rows == 0) || (JX_Screen_Cols == 0))
     {
	JX_Screen_Cols = 80;
	JX_Screen_Rows = 24;
     }

   /* Return vterm_init_display (JX_Screen_Rows, JX_Screen_Cols); */
   return 0;
}

/*}}}*/

/************************************
* flush_output
*
* group:
*
*
* debug print:
*
*
************************************/

void  flush_output (void ) /*{{{*/
{
   if ( JGtkWin->XDisplay == NULL)
     fflush (stdout);
   else
     SLtt_flush_output ();
}

/*}}}*/

/* a hook to parse some command line args. */
int (*X_Argc_Argv_Hook)(int, char **) = X_eval_command_line;

/************************************
* JX_flush_output
*
* group: interface
*
*
* debug print:
*
*
************************************/

static int
JX_flush_output (void )
{
   return 0;
}

static int JX_Zero = 0;

/************************************
* get_screen_size
*
* group:
*
*
* debug print:
*
*
************************************/

static void
get_screen_size (int *r, int *c )
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

/************************************
* set_xtt_hooks
*
* group:
*
*
* debug print:
*
*
************************************/

static void
set_xtt_hooks( void )
{
   tt_beep =             JX_beep;
   tt_write_string =     JX_write_string;
   tt_get_screen_size =  JX_get_display_size;
   tt_set_color =        JX_set_color;
   tt_set_mono =         JX_set_mono;

   tt_wide_width =          JX_wide_width;
   tt_narrow_width =        JX_narrow_width;
   tt_enable_cursor_keys =  JX_enable_cursor_keys;
   tt_set_term_vtxxx =      JX_set_term_vtxxx;

   tt_Ignore_Beep =         &JX_Ignore_Beep;
   tt_Use_Ansi_Colors =     &JX_Use_Ansi_Colors;
   tt_Term_Cannot_Scroll =  &JX_Term_Cannot_Scroll;
   tt_Term_Cannot_Insert =  &JX_Term_Cannot_Insert;
   tt_Blink_Mode =          &JX_Blink_Mode;
}

/************************************
* JX_get_terminfo
*
* group:
*
*
* debug print:
*
*
************************************/

static void
JX_get_terminfo( void ) /*{{{*/
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
   X_Read_Hook =           X_read_key;
   X_Input_Pending_Hook =  X_input_pending;
   X_Update_Open_Hook =    X_update_open;
   X_Update_Close_Hook =   X_update_close;
   X_Suspend_Hook =        xjed_suspend;
   X_Init_Term_Hook =      init_Xdisplay;
   X_Reset_Term_Hook =     reset_Xdisplay;
   X_Define_Keys_Hook =    x_define_xkeys;
   SLang_Interrupt =       xjed_check_kbd;

   /* Set this so that main will not try to read from stdin.  It is quite
    * likely that this is started from a menu or something.
    */
   Stdin_Is_TTY = -1;
   /* We do not need this since we do not have to worry about incoming
    * eight bit escape sequences. tt_goto_rc
    */
   DEC_8Bit_Hack = 0;

   memset ((char *) &tt, 0, sizeof (SLsmg_Term_Type));

   tt.tt_normal_video =         JX_normal_video;
   tt.tt_set_scroll_region =    JX_set_scroll_region;
   tt.tt_goto_rc =              JX_goto_rc;
   tt.tt_reverse_index =        JX_reverse_index;
   tt.tt_reset_scroll_region =  JX_reset_scroll_region;
   tt.tt_delete_nlines =        JX_delete_nlines;
   tt.tt_cls =           JX_cls;
   tt.tt_del_eol =       JX_del_eol;
   tt.tt_smart_puts =    JX_smart_puts;
   tt.tt_flush_output =  JX_flush_output;
   tt.tt_reset_video =   JX_reset_video;
   tt.tt_init_video =    JX_init_video;

   tt.tt_screen_rows =   &JX_Screen_Rows;
   tt.tt_screen_cols =   &JX_Screen_Cols;
   tt.tt_term_cannot_scroll = &JX_Term_Cannot_Scroll;
   tt.tt_has_alt_charset =    &JX_Zero;

#if SLANG_VERSION >= 20000
   tt.unicode_ok = &Jed_UTF8_Mode;
#endif

   SLsmg_set_terminal_info (&tt);
   Jed_Handle_SIGTSTP = 0;
   if ( 1 )  return;
}

/*}}}*/

void (*tt_get_terminfo)(void) = JX_get_terminfo;

/* Unused but required. */
#ifdef USE_GPM_MOUSE
int (*X_Open_Mouse_Hook)(void);
void (*X_Close_Mouse_Hook)(void);
#endif

