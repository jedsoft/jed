/* Copyright (c) 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <stdio.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <string.h>
#include <slang.h>

#include "colors.h"
#include "buffer.h"
#include "screen.h"
#include "display.h"
#include "misc.h"

typedef struct 
{
   char *name;			       /* slstring for new objects */
   int color;
   char *fg;			       /* slstring */
   char *bg;			       /* slstring */
}
Color_Object_Map_Type;

static Color_Object_Map_Type Color_Name_Map [JMAX_COLORS] =
{
   /* %%% COLOR-TABLE-START %%% -- tag used by external script to sync color files */
   {"normal",		JNORMAL_COLOR, NULL, NULL},
   {"cursor",		JCURSOR_COLOR, NULL, NULL},
   {"status",		JSTATUS_COLOR, NULL, NULL},
   {"region",		JREGION_COLOR, NULL, NULL},
   {"menu",		JMENU_COLOR, NULL, NULL},
   {"operator",		JOP_COLOR, NULL, NULL},
   {"number",		JNUM_COLOR, NULL, NULL},
   {"string",		JSTR_COLOR, NULL, NULL},
   {"comment",		JCOM_COLOR, NULL, NULL},
   {"delimiter",	JDELIM_COLOR, NULL, NULL},
   {"preprocess",	JPREPROC_COLOR, NULL, NULL},
   {"message",		JMESSAGE_COLOR, NULL, NULL},
   {"error",		JERROR_COLOR, NULL, NULL},
   {"dollar",		JDOLLAR_COLOR, NULL, NULL},
#if JED_HAS_LINE_ATTRIBUTES
   {"...",		JDOTS_COLOR, NULL, NULL},
#endif   
#if JED_HAS_MENUS
   {"menu_char",	JMENU_CHAR_COLOR, NULL, NULL},
   {"menu_shadow",	JMENU_SHADOW_COLOR, NULL, NULL},
   {"menu_selection",	JMENU_SELECTION_COLOR, NULL, NULL},
   {"menu_popup",	JMENU_POPUP_COLOR, NULL, NULL},
   {"menu_selection_char", JMENU_SELECTED_CHAR_COLOR, NULL, NULL},
#endif
   {"cursorovr",	JCURSOROVR_COLOR, NULL, NULL},
   {"linenum",		JLINENUM_COLOR, NULL, NULL},
   {"trailing_whitespace", JTWS_COLOR, NULL, NULL},
   {"tab",		JTAB_COLOR, NULL, NULL},
   /* These may be useful for some modes */
   {"url",		JURL_COLOR, NULL, NULL},
   {"italic",		JITALIC_COLOR, NULL, NULL},
   {"underline",	JUNDERLINE_COLOR, NULL, NULL},
   {"bold",		JBOLD_COLOR, NULL, NULL},
   {"html",		JHTML_KEY_COLOR, NULL, NULL},
   {"keyword",		JKEY_COLOR, NULL, NULL},
   {"keyword1",		JKEY1_COLOR, NULL, NULL},
   {"keyword2",		JKEY2_COLOR, NULL, NULL},
   {"keyword3",		JKEY3_COLOR, NULL, NULL},
   {"keyword4",		JKEY4_COLOR, NULL, NULL},
   {"keyword5",		JKEY5_COLOR, NULL, NULL},
   {"keyword6",		JKEY6_COLOR, NULL, NULL},
   {"keyword7",		JKEY7_COLOR, NULL, NULL},
   {"keyword8",		JKEY8_COLOR, NULL, NULL},
   {"keyword9",		JKEY9_COLOR, NULL, NULL},
   /* The rest of the colors are user-defined, and the strings are slstrings */
   /* %%% COLOR-TABLE-STOP %%% */
   {NULL, -1, NULL, NULL}
};


int jed_get_color_obj (char *name) /*{{{*/
{
   Color_Object_Map_Type *map, *map_max;
   char ch;

   map = Color_Name_Map;
   map_max = Color_Name_Map + JMAX_COLORS;
   ch = *name;
   while (map < map_max)
     {
	if (map->name == NULL)
	  {
	     if (map < Color_Name_Map + FIRST_USER_COLOR)
	       {
		  map = Color_Name_Map + FIRST_USER_COLOR;
		  continue;
	       }
	     break;
	  }
	if ((ch == map->name[0])
	    && (0 == strcmp (map->name, name)))
	  return map->color;

	map++;
     }

   if (0 == strcmp (name, "keyword0"))
     return JKEY_COLOR;
	
   return -1;
}

/*}}}*/

static int add_color_object (char *name)
{
   int obj;
   Color_Object_Map_Type *map, *map_max;

   obj = jed_get_color_obj (name);
   if (obj != -1)
     return obj;
   
   map = Color_Name_Map + FIRST_USER_COLOR;
   map_max = Color_Name_Map + JMAX_COLORS;
   while (map < map_max)
     {
	if (map->name != NULL)
	  {
	     map++;
	     continue;
	  }
	if (NULL == (name = SLang_create_slstring (name)))
	  return -1;
	
	map->name = name;
	map->color = (int) (map - Color_Name_Map);
	return map->color;
     }
   
   jed_vmessage (0, "*** Warning: no colors available for %s ***", name);
   return -1;
}

int jed_set_color (int obj, char *fg, char *bg) /*{{{*/
{
   Color_Object_Map_Type *map;

   if ((obj < 0) || (obj >= JMAX_COLORS)) 
     return -1;

   tt_set_color (obj, NULL, fg, bg);
   map = Color_Name_Map + obj;
   if (NULL == (fg = SLang_create_slstring (fg)))
     return -1;
   if (NULL == (bg = SLang_create_slstring (bg)))
     {
	SLang_free_slstring (fg);
	return -1;
     }
   SLang_free_slstring (map->fg); map->fg = fg;
   SLang_free_slstring (map->bg); map->bg = bg;
   
   return 0;
}

/* Intrinsics */

static void set_color (char *obj, char *fg, char *bg) /*{{{*/
{
   int i;

   if (-1 != (i = jed_get_color_obj (obj)))
     (void) jed_set_color (i, fg, bg);
}

/*}}}*/

static void get_color (char *obj)
{
   int i;
   Color_Object_Map_Type *map;

   i = jed_get_color_obj (obj);

   /* object may have not been allocated which in practical terms means that
    * the "normal" color will be used for the object.
    */
   if (i == -1)
     i = JNORMAL_COLOR;		       

   map = Color_Name_Map + i;
   (void) SLang_push_string (map->fg == NULL ? "default" : map->fg);
   (void) SLang_push_string (map->bg == NULL ? "default" : map->bg);
}

static void add_color_object_cmd (char *name)
{
   (void) add_color_object (name);
}

#ifndef IBMPC_SYSTEM
static void set_color_esc (char *obj, char *esc) /*{{{*/
{
   (void) obj; (void) esc;
}

/*}}}*/
#endif

static void set_color_object (int *obj, char *fg, char *bg)
{
   (void) jed_set_color(*obj, fg, bg);
}


#if JED_HAS_COLOR_COLUMNS
static void set_column_colors (int *color, int *c0p, int *c1p) /*{{{*/
{
   unsigned char *p, *pmax, ch;
   int c0, c1;
   unsigned int num;

   if ((*color < 0) || (*color >= JMAX_COLORS)) return;
   ch = (unsigned char) *color;

   c0 = *c0p - 1;
   c1 = *c1p;
   
   if (c0 < 0) c0 = 0;
   if (c1 <= c0) return;
   
   if ((NULL == (p = CBuf->column_colors))
       || (c1 > (int) CBuf->num_column_colors))
     {
	num = c1 + Jed_Num_Screen_Cols;
	p = (unsigned char *) SLrealloc ((char *)p, num);
	if (p == NULL)
	  return;
	CBuf->column_colors = p;
	CBuf->num_column_colors = num;
	SLMEMSET ((char *) p, 0, num);
     }
   
   CBuf->coloring_style = 1;
	
   pmax = p + c1;
   p += c0;
   while (p < pmax) *p++ = ch;
}

/*}}}*/

#endif

static void set_line_color (int *color)
{
   int c = *color;
   if (c < 0)
     return;
   JED_SET_LINE_COLOR (CLine, c);
}

static int get_line_color (void)
{
   return (int)JED_GET_LINE_COLOR(CLine);
}


static SLang_Intrin_Fun_Type Color_Intrinsics [] = /*{{{*/
{
   MAKE_INTRINSIC_I("set_line_color", set_line_color, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("get_line_color", get_line_color, SLANG_INT_TYPE),
   MAKE_INTRINSIC_SSS("_set_color", set_color, VOID_TYPE),
   MAKE_INTRINSIC_S("color_number", jed_get_color_obj, INT_TYPE),
   MAKE_INTRINSIC_S("get_color", get_color, VOID_TYPE),
   MAKE_INTRINSIC_S("add_color_object", add_color_object_cmd, VOID_TYPE),
#ifndef IBMPC_SYSTEM
   MAKE_INTRINSIC_SS("set_color_esc", set_color_esc, VOID_TYPE),
#endif
#if JED_HAS_COLOR_COLUMNS
   MAKE_INTRINSIC_III("set_column_colors", set_column_colors, VOID_TYPE),
   MAKE_INTRINSIC_ISS("set_color_object", set_color_object, VOID_TYPE),
#endif
   SLANG_END_INTRIN_FUN_TABLE
};

int jed_init_color_intrinsics (void)
{
   int i;

#if 1
   /* Sanity check */
   for (i = 0; i < JMAX_COLORS; i++)
     {
	if ((Color_Name_Map[i].name != NULL)
	    && (i != Color_Name_Map[i].color))
	  {
	     jed_verror ("Internal Error: Color Table is corrupt");
	     return -1;
	  }
     }
#endif

   if (-1 == SLadd_intrin_fun_table (Color_Intrinsics, NULL))
     return -1;
   
   return 0;
}

