/* Drop-down menus (tty) */
/* Copyright (c) 1999, 2000, 2002-2009 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#if JED_HAS_MENUS

#include <slang.h>

#include "jdmacros.h"

#include <string.h>

#include "buffer.h"
#include "screen.h"
#include "misc.h"
#include "sysdep.h"
#include "cmds.h"
#include "menu.h"
#include "colors.h"

typedef struct _Menu_Node_Type
{
   char *name;
   int type;
#define MENU_NODE_FUN_SLANG	1
#define MENU_NODE_FUN_C		2
#define MENU_NODE_POPUP		3
#define MENU_NODE_SEPARATOR	4
#define MENU_NODE_MENUBAR	5
#define MENU_NODE_KEYSTRING	6

   unsigned int flags;
#define MENU_ACTIVE		1
#define MENU_SELECTION		2
#define MENU_ITEM_UNAVAILABLE	4
#define MENU_POPUP_PREPARED	8
}
Menu_Node_Type;

typedef struct _Menu_Popup_Type
{
   char *name;
   int type;			       /* =MENU_NODE */
   unsigned int flags;

   /* Private data */
   /* These 5 must match Menu_Bar_Type */
   Menu_Node_Type **subnodes;
   unsigned int num_subnodes;
   unsigned int max_subnodes;
   struct _Menu_Popup_Type *parent;
   unsigned int active_node;

   SLang_Name_Type *select_popup_callback;
   SLang_Name_Type *tweak_popup_callback;

   int column;			       /* meaningful when active */
   int row;
   int max_row;
   int max_col;
   unsigned int min_width;
   int visible_node_offset;
}
Menu_Popup_Type;

struct _Menu_Bar_Type
{
   char *name;
   int type;			       /* =MENU_NODE */
   unsigned int flags;
   
   /* Private data */
   /* These 5 must match Menu_Popup_Type */
   Menu_Node_Type **subnodes;
   unsigned int num_subnodes;
   unsigned int max_subnodes;   
   struct _Menu_Popup_Type *parent;   
   unsigned int active_node;

   SLang_Name_Type *init_callback;
   SLang_Name_Type *select_callback;   /* Void select_callback (name) */

   int *item_columns;	       /* locations of items on menu bar */
   unsigned int num_refs;

#define DEFAULT_MENU_PREFIX "F10 key ==> "
   char *prefix_string;
   struct _Menu_Bar_Type *next;
};

typedef struct
{
   char *name;
   int type;
   unsigned int flags;

   SLang_Name_Type *slang_fun;
   SLang_Any_Type *client_data;
}
Menu_SLang_Fun_Type;

typedef struct
{
   char *name;
   int type;
   unsigned int flags;

   int (*c_fun)(void);
}
Menu_C_Fun_Type;

typedef struct
{
   char *name;
   int type;
   unsigned int flags;
   
   char *keystring;
}
Menu_Keystring_Fun_Type;

int Jed_Menus_Active;

static Menu_Bar_Type *Global_Menu_Bar;
static Menu_Bar_Type *Menu_Bar_Root;
static Menu_Bar_Type *Active_Menu_Bar;
static Menu_Popup_Type *Active_Popup;

static void free_menu_node (Menu_Node_Type *);
static int select_menu_cmd (void);

static void free_menu_popup_subnodes (Menu_Popup_Type *m)
{
   Menu_Node_Type **l, **lmax;

   l = m->subnodes;
   lmax = l + m->num_subnodes;

   while (l < lmax)
     {
	free_menu_node (*l);
	l++;
     }
   SLfree ((char *) m->subnodes);

   m->subnodes = NULL;
   m->num_subnodes = 0;
   m->max_subnodes = 0;
}


static void free_menu_popup_private_data (Menu_Popup_Type *m)
{
   free_menu_popup_subnodes (m);
   SLang_free_function (m->select_popup_callback);
   SLang_free_function (m->tweak_popup_callback);
   m->select_popup_callback = NULL;
   m->tweak_popup_callback = NULL;
}

static void free_menu_bar_private_data (Menu_Bar_Type *m)
{
   Menu_Node_Type **l, **lmax;

   if (m == Active_Menu_Bar)
     Active_Menu_Bar = NULL;

   if (m == Global_Menu_Bar)
     Global_Menu_Bar = NULL;

   if (m == Menu_Bar_Root)
     Menu_Bar_Root = m->next;
   else
     {
	Menu_Bar_Type *prev;
	
	prev = Menu_Bar_Root;
	while (prev->next != m)
	  prev = prev->next;
	
	prev->next = m->next;
     }

   l = m->subnodes;
   lmax = l + m->num_subnodes;

   while (l < lmax)
     {
	free_menu_node (*l);
	l++;
     }
   SLfree ((char *) m->subnodes);
   SLfree ((char *) m->item_columns);
   SLang_free_slstring (m->prefix_string);
   SLang_free_function (m->init_callback);
   SLang_free_function (m->select_callback);
}

static void free_keystring_private_data (Menu_Keystring_Fun_Type *m)
{
   SLang_free_slstring (m->keystring);
}

static void free_slangfun_private_data (Menu_SLang_Fun_Type *m)
{
   if (m->client_data != NULL)
     SLang_free_anytype (m->client_data);
   SLang_free_function (m->slang_fun);
}

static void free_menu_node (Menu_Node_Type *m)
{
   Menu_Bar_Type *b;

   if (m == NULL)
     return;

   switch (m->type)
     {
      case MENU_NODE_SEPARATOR:
      case MENU_NODE_FUN_C:
	break;

      case MENU_NODE_FUN_SLANG:
	free_slangfun_private_data ((Menu_SLang_Fun_Type *) m);
	break;

      case MENU_NODE_KEYSTRING:
	free_keystring_private_data ((Menu_Keystring_Fun_Type *) m);
	break;

      case MENU_NODE_MENUBAR:
	b = (Menu_Bar_Type *)m;
	if (b->num_refs > 1)
	  {
	     b->num_refs -= 1;
	     return;
	  }
	free_menu_bar_private_data ((Menu_Bar_Type *) m);
	break;

      case MENU_NODE_POPUP:
	free_menu_popup_private_data ((Menu_Popup_Type *) m);
	break;
     }

   SLang_free_slstring (m->name);
   SLfree ((char *) m);
}

static Menu_Node_Type *create_menu_node (char *name, int node_type,
					 unsigned int sizeof_node)
{
   Menu_Node_Type *m;

   m = (Menu_Node_Type *) jed_malloc0 (sizeof_node);
   if (m == NULL)
     return NULL;

   m->type = node_type;
   
   if (NULL == (m->name = SLang_create_slstring (name)))
     {
	SLfree ((char *)m);
	return NULL;
     }
   
   return m;
}


static Menu_Popup_Type *create_menu_popup (char *name, unsigned int num_items)
{
   Menu_Popup_Type *m;
   
   m = (Menu_Popup_Type *) create_menu_node (name, MENU_NODE_POPUP, sizeof (Menu_Popup_Type));
   if (m == NULL)
     return NULL;

   if (num_items == 0)
     num_items = 5;
   
   if (NULL == (m->subnodes = (Menu_Node_Type **)SLmalloc (num_items * sizeof (Menu_Node_Type *))))
     {
	free_menu_node ((Menu_Node_Type *) m);
	return NULL;
     }
   m->max_subnodes = num_items;
   return m;
}


static Menu_Bar_Type *create_menu_bar (char *name, unsigned int num_items)
{
   Menu_Bar_Type *m;
   char *prefix;

   m = (Menu_Bar_Type *) create_menu_node (name, MENU_NODE_MENUBAR, sizeof (Menu_Bar_Type));
   if (m == NULL)
     return NULL;

   if (num_items == 0)
     num_items = 5;

   if ((NULL == (m->subnodes = (Menu_Node_Type **)SLmalloc (num_items * sizeof (Menu_Node_Type *))))
       || (NULL == (m->item_columns = (int *)jed_malloc0 (num_items * sizeof (int)))))
     {
	free_menu_node ((Menu_Node_Type *) m);
	return NULL;
     }

   m->max_subnodes = num_items;

   if (Menu_Bar_Root != NULL)
     m->next = Menu_Bar_Root;
   Menu_Bar_Root = m;

   m->num_refs = 1;
   
   prefix = DEFAULT_MENU_PREFIX;
   if ((Global_Menu_Bar != NULL)
       && (Global_Menu_Bar->prefix_string != NULL))
     prefix = Global_Menu_Bar->prefix_string;
   
   prefix = SLang_create_slstring (prefix);
   if (prefix == NULL)
     {
	jed_delete_menu_bar (m);
	return NULL;
     }
   m->prefix_string = prefix;

   return m;
}

void jed_delete_menu_bar (Menu_Bar_Type *m)
{
   if (m == NULL)
     return;

   free_menu_node ((Menu_Node_Type *)m);
}


static int menu_name_eqs (char *a, char *b, char *bmax)
{
   while (*a)
     {
	if ((b == bmax)
	    || (*a != *b))
	  return 0;
	
	a++;
	b++;
     }
   
   return (b == bmax);
}

static Menu_Bar_Type *menu_find_menu_bar (char *name, int do_error)
{
   Menu_Bar_Type *b;
   char *name_max;

   name_max = strchr (name, '.');
   if (name_max == NULL)
     name_max = name + strlen (name);

   b = Menu_Bar_Root;
   while (b != NULL)
     {
	if (menu_name_eqs (b->name, name, name_max))
	  return b;

	b = b->next;
     }

   if (do_error)
     SLang_verror (SL_INTRINSIC_ERROR, 
		   "Unable to a find menu bar called %s", 
		   name);
   return NULL;
}


static Menu_Node_Type *find_subnode (Menu_Popup_Type *m, char *name)
{
   Menu_Node_Type **l, **lmax;

   if (m == NULL)
     return NULL;
   
   l = m->subnodes;
   lmax = l + m->num_subnodes;
   
   while (l < lmax)
     {
	if (0 == strcmp (name, (*l)->name))
	  return *l;
	
	l++;
     }
   return NULL;
}


static int check_subnode_space (Menu_Popup_Type *m, unsigned int dn)
{
   Menu_Node_Type **subnodes;
   unsigned int num;

   if (m->num_subnodes + dn <= m->max_subnodes)
     return 0;

   num = m->max_subnodes + dn;

   subnodes = (Menu_Node_Type **)SLrealloc ((char *)m->subnodes, num * sizeof (Menu_Node_Type *));
   if (subnodes == NULL)
     return -1;
   m->subnodes = subnodes;

   if (m->type == MENU_NODE_MENUBAR)
     {
	Menu_Bar_Type *b = (Menu_Bar_Type *)m;
	int *item_columns;
	unsigned int i;

	item_columns = (int *)SLrealloc ((char *)b->item_columns, num * sizeof(int));
	if (item_columns == NULL)
	  return -1;
	
	for (i = m->max_subnodes; i < num; i++)
	  item_columns [i] = 0;
	b->item_columns = item_columns;
     }

   m->max_subnodes = num;
   return 0;
}

/* This function assumes that there is enough space to insert one item */
static void insert_node_to_popup (Menu_Popup_Type *p, Menu_Node_Type *m, 
				  unsigned int where)
{
   unsigned int len;
   unsigned int n;
   
   n = p->num_subnodes;

   if (where > n)
     where = n;

   while (n > where)
     {
	p->subnodes[n] = p->subnodes[n-1];
	n--;
     }
   p->subnodes[where]  = m;
   p->num_subnodes += 1;

   /* p could be a menu-bar */

   if (p->type != MENU_NODE_POPUP)
     return;

   len = strlen (m->name);
   if (len > p->min_width)
     p->min_width = len;
}

#if 0
static void append_node_to_popup (Menu_Popup_Type *p, Menu_Node_Type *m)
{
   insert_node_to_popup (p, m, p->num_subnodes);
}
#endif
   
static Menu_Popup_Type *insert_popup_menu (Menu_Popup_Type *m, char *name, int where)
{
   Menu_Popup_Type *p;

   if (NULL != (p = (Menu_Popup_Type *)find_subnode (m, name)))
     return p;

   if (-1 == check_subnode_space (m, 1))
     return NULL;

   p = create_menu_popup (name, 5);
   if (p == NULL)
     return NULL;
   
   p->parent = m;
   
   insert_node_to_popup (m, (Menu_Node_Type *)p, where);
   return p;
}

static Menu_Popup_Type *append_popup_menu (Menu_Popup_Type *m, char *name)
{
   return insert_popup_menu (m, name, m->num_subnodes);
}


static Menu_Node_Type *insert_separator (Menu_Popup_Type *m, int where)
{
   Menu_Node_Type *l;

   if (-1 == check_subnode_space (m, 1))
     return NULL;

   l = create_menu_node ("", MENU_NODE_SEPARATOR, sizeof (Menu_Node_Type));
   if (l == NULL)
     return NULL;

   insert_node_to_popup (m, l, where);

   return l;
}

static Menu_Node_Type *append_separator (Menu_Popup_Type *m)
{
   return insert_separator (m, m->num_subnodes);
}


static Menu_SLang_Fun_Type *insert_slang_fun_item (Menu_Popup_Type *m, char *name, 
						   SLang_Name_Type *nt, SLang_Any_Type *cd,
						   int where)
{
   Menu_SLang_Fun_Type *l;

   if (NULL != (l = (Menu_SLang_Fun_Type *)find_subnode (m, name)))
     {
	if (l->type != MENU_NODE_FUN_SLANG)
	  return NULL;

	free_slangfun_private_data (l);
	l->slang_fun = nt;
	l->client_data = cd;
	
	return l;
     }

   if (-1 == check_subnode_space (m, 1))
     return NULL;

   l = (Menu_SLang_Fun_Type *)
     create_menu_node (name, MENU_NODE_FUN_SLANG, sizeof (Menu_SLang_Fun_Type));

   if (l == NULL)
     return NULL;

   l->slang_fun = nt;
   l->client_data = cd;

   insert_node_to_popup (m, (Menu_Node_Type *)l, where);
   return l;
}

static Menu_SLang_Fun_Type *append_slang_fun_item (Menu_Popup_Type *m, char *name,
						   SLang_Name_Type *nt, SLang_Any_Type *cd)
{
   return insert_slang_fun_item (m, name, nt, cd, m->num_subnodes);
}

static Menu_Keystring_Fun_Type *insert_keystring_item (Menu_Popup_Type *m, char *name, 
						       char *k, int where)
{
   Menu_Keystring_Fun_Type *l;

   if (NULL != (l = (Menu_Keystring_Fun_Type *)find_subnode (m, name)))
     {
	if (l->type != MENU_NODE_KEYSTRING)
	  return NULL;
	
	if (NULL == (k = SLang_create_slstring (k)))
	  return NULL;

	free_keystring_private_data (l);
	l->keystring = k;
	return l;
     }


   if (-1 == check_subnode_space (m, 1))
     return NULL;

   l = (Menu_Keystring_Fun_Type *)
     create_menu_node (name, MENU_NODE_KEYSTRING, sizeof (Menu_Keystring_Fun_Type));

   if (l == NULL)
     return NULL;

   if (NULL == (l->keystring = SLang_create_slstring (k)))
     {
	free_menu_node ((Menu_Node_Type *) l);
	return NULL;
     }

   insert_node_to_popup (m, (Menu_Node_Type *)l, where);
   return l;
}

static Menu_Keystring_Fun_Type *append_keystring_item (Menu_Popup_Type *m, char *name, char *k)
{
   return insert_keystring_item (m, name, k, m->num_subnodes);
}

static Menu_Node_Type *get_selected_menu_item (Menu_Popup_Type *p)
{
   Menu_Node_Type *m;

   if (p == NULL)
     return NULL;

   if (p->num_subnodes <= p->active_node)
     {
	p->active_node = 0;
	return NULL;
     }

   m = p->subnodes[p->active_node];
   if (m->flags & MENU_ITEM_UNAVAILABLE)
     {
	p->active_node = 0;
	return NULL;
     }

   /* Make sure this is selected. */
   m->flags |= MENU_SELECTION;
   return m;
}

static int unselect_active_node (Menu_Popup_Type *b)
{
   Menu_Node_Type *m;
   
   if (NULL == (m = get_selected_menu_item (b)))
     return -1;

   m->flags &= ~(MENU_SELECTION|MENU_ACTIVE);
   return 0;
}

static int select_next_active_node (Menu_Popup_Type *b, unsigned int active_node,
				    unsigned int flags)
{
   int wrapped;
   unsigned int num_subnodes;

   if (b == NULL)
     return -1;

   num_subnodes = b->num_subnodes;

   if (num_subnodes == 0)
     {
	b->active_node = 0;
	return -1;
     }

   unselect_active_node (b);

   wrapped = 0;

   while (1)
     {
	Menu_Node_Type *node;

	active_node++;
	if (active_node >= num_subnodes)
	  {
	     active_node = 0;
	     if (wrapped)
	       return -1;

	     wrapped = 1;
	  }

	node = b->subnodes[active_node];
	if ((node->type != MENU_NODE_SEPARATOR)
	    && (0 == (node->flags & MENU_ITEM_UNAVAILABLE)))
	  {
	     node->flags |= MENU_SELECTION|flags;
	     break;
	  }
     }

   b->active_node = active_node;
   return 0;
}

static int select_prev_active_node (Menu_Popup_Type *b, unsigned int active_node,
				    unsigned int flags)
{
   int wrapped;
   unsigned int num_subnodes;

   if (b == NULL)
     return -1;

   num_subnodes = b->num_subnodes;

   if (num_subnodes == 0)
     {
	b->active_node = 0;
	return -1;
     }

   unselect_active_node (b);

   wrapped = 0;

   while (1)
     {
	Menu_Node_Type *node;

	if (active_node == 0)
	  {
	     active_node = num_subnodes;
	     if (wrapped)
	       return -1;
	     
	     wrapped = 1;
	  }
	active_node--;

	node = b->subnodes[active_node];
	if ((node->type != MENU_NODE_SEPARATOR)
	    && (0 == (node->flags & MENU_ITEM_UNAVAILABLE)))
	  {
	     node->flags |= MENU_SELECTION|flags;
	     break;
	  }
     }

   b->active_node = active_node;
   return 0;
}



static int set_node_selection (Menu_Popup_Type *p, Menu_Node_Type *m)
{  
   unsigned int i, imax;
   Menu_Node_Type **subnodes;

   if ((p == NULL) || (m == NULL)
       || (m->type == MENU_NODE_SEPARATOR)
       || (m->flags & MENU_ITEM_UNAVAILABLE))
     return -1;

   subnodes = p->subnodes;
   imax = p->num_subnodes;
   for (i = 0; i < imax; i++)
     {
	if (subnodes[i] == m)
	  {
	     unselect_active_node (p);
	     p->active_node = i;
	     m->flags |= MENU_SELECTION;
	     return 0;
	  }
     }
   return -1;
}

   
static unsigned int get_active_node_flags (Menu_Popup_Type *p)
{
   Menu_Node_Type *m;
   
   m = get_selected_menu_item (p);
   if (m == NULL)
     return 0;
   
   return m->flags;
}

static int menu_delete_node (Menu_Popup_Type *p, Menu_Node_Type *m)
{
   unsigned int i, imax;

   if ((p == NULL) || (m == NULL))
     return -1;
   
   imax = p->num_subnodes;
   for (i = 0; i < imax; i++)
     {
	if (p->subnodes[i] != m)
	  continue;
	
	if (i == p->active_node)
	  p->active_node = 0;
	
	while (++i < imax)
	  p->subnodes[i-1] = p->subnodes[i];
	
	p->num_subnodes -= 1;
	
	(void) unselect_active_node (p);
	free_menu_node (m);
	return 0;
     }
   return -1;
}

static int menu_delete_nodes (Menu_Popup_Type *p)
{
   unsigned int i, imax;

   if (p == NULL)
     return -1;

   imax = p->num_subnodes;
   for (i = 0; i < imax; i++)
     free_menu_node (p->subnodes[i]);

   p->num_subnodes = 0;
   p->active_node = 0;
   return 0;
}


static int copy_menu (Menu_Popup_Type *dest, Menu_Node_Type *src)
{
   Menu_Node_Type **l, **lmax;
   Menu_Popup_Type *p;
   Menu_Node_Type *m;
   SLang_Any_Type *any;
   Menu_SLang_Fun_Type *sl;

   if (src == (Menu_Node_Type *)dest)
     {
	SLang_verror (SL_INTRINSIC_ERROR,
		      "Unable to copy a menu onto itself");
	return -1;
     }

   switch (src->type)
     {
      case MENU_NODE_POPUP:
	p = (Menu_Popup_Type *) src;
	l = p->subnodes;
	lmax = l + p->num_subnodes;
	
	p = append_popup_menu (dest, src->name);
	if (p == NULL)
	  return -1;
	
	while (l < lmax)
	  {
	     if (-1 == copy_menu (p, *l))
	       {
		  menu_delete_node (dest, (Menu_Node_Type *)p);
		  return -1;
	       }
	     l++;
	  }
	m = (Menu_Node_Type *)p;
	break;
	
      case MENU_NODE_SEPARATOR:
	m = append_separator (dest);
	break;
	
      case MENU_NODE_FUN_SLANG:
	sl = (Menu_SLang_Fun_Type *) src;
	/* Need a s-lang fun for this !!! */
	if ((-1 == SLang_push_anytype (sl->client_data))
	    || (-1 == SLang_pop_anytype (&any)))
	  return -1;

	m = (Menu_Node_Type *) append_slang_fun_item (dest, sl->name,
							   sl->slang_fun, any);
	if (m == NULL)
	  SLang_free_anytype (any);
	break;
	
      case MENU_NODE_KEYSTRING:
	m = (Menu_Node_Type *) append_keystring_item (dest, src->name, ((Menu_Keystring_Fun_Type *)src)->keystring);
	break;
	
      case MENU_NODE_MENUBAR:
	SLang_verror (SL_INTRINSIC_ERROR, "Unable to copy a menu-bar");
	return -1;

      case MENU_NODE_FUN_C:
      default:
	SLang_verror (SL_APPLICATION_ERROR,
		      "Menu Type %d not supported", src->type);
	return -1;
     }
   
   if (m == NULL)
     return -1;
   
   m->flags = src->flags;
   return 0;
}


/*
 *  SLsmg MenuBar interface
 */

static void simulate_hline (unsigned int n)
{
   while (n--)
     SLsmg_write_string ("-");
}

static void simulate_box (int r, int c, unsigned int dr, unsigned int dc)
{
   int rr, rmax;

   if ((dr < 1) || (dc < 2)) return;

   dr--; 
   dc--;
   SLsmg_gotorc (r, c);
   SLsmg_write_string ("+");
   simulate_hline (dc-1);
   SLsmg_write_string ("+");
   SLsmg_gotorc (r + dr, c);
   SLsmg_write_string ("+");
   simulate_hline (dc-1);
   SLsmg_write_string ("+");
   
   rmax = r + dr;
   for (rr = r + 1; rr < rmax; rr++)
     {
	SLsmg_gotorc (rr, c);
	SLsmg_write_string ("|");
	SLsmg_gotorc (rr, c + dc);
	SLsmg_write_string ("|");
     }
}

/* I would prefer to use real up/down arrows but I cannot depend upon their
 * portability.
 */
static void draw_up_arrow (int r, int c)
{
   SLsmg_gotorc (r, c);
   SLsmg_write_string ("(-)");
}

static void draw_down_arrow (int r, int c)
{
   SLsmg_gotorc (r, c);
   SLsmg_write_string ("(+)");
}

static void draw_name (char *name, int color0, int color1, unsigned int field_width)
{
   unsigned char *s, *smax;
   unsigned int name_width;

   s = (unsigned char *) name;
   smax = s + strlen (name);
   name_width = SLsmg_strwidth (s, smax);

   while ((s < smax) && (*s != '&'))
     s++;

   SLsmg_set_color (color0);
   SLsmg_write_chars ((unsigned char *) name, s);

   if (s < smax)
     {
	unsigned char *s1;
	
	name_width--;
	s++;			       /* skip & */
	SLsmg_set_color (color1);
	s1 = jed_multibyte_chars_forward (s, smax, 1, NULL, 1);
	SLsmg_write_chars (s, s1);
	SLsmg_set_color (color0);
	s = s1;

	if (s < smax)
	  SLsmg_write_chars (s, smax);
     }

   if (name_width < field_width)
     SLsmg_write_nstring ("", field_width - name_width);
}

	  
static int draw_menu_bar (Menu_Bar_Type *b)
{
   Menu_Node_Type **m;
   unsigned int i, imax;
   int active;

   SLsmg_gotorc (0, 0);

   SLsmg_set_color (JMENU_COLOR);
   SLsmg_write_string (b->prefix_string);

   m = b->subnodes;
   imax = b->num_subnodes;
   active = -1;

   for (i = 0; i < imax; i++)
     {
	Menu_Popup_Type *p;
	char *name;

	p = (Menu_Popup_Type *) m[i];
	if (p->flags & MENU_ITEM_UNAVAILABLE)
	  continue;

	b->item_columns[i] = SLsmg_get_column ();

	if (p->type == MENU_NODE_SEPARATOR)
	  name = "          ";
	else
	  name = p->name;

	if (p->flags & MENU_SELECTION)
	  {
	     if (p->type == MENU_NODE_POPUP)
	       active = i;

	     draw_name (name, JMENU_SELECTION_COLOR, JMENU_SELECTED_CHAR_COLOR, 0);
	  }
	else
	  draw_name (name, JMENU_COLOR, JMENU_CHAR_COLOR, 0);

	SLsmg_set_color (JMENU_COLOR);
	SLsmg_write_string ("   ");
     }

   SLsmg_set_color (JMENU_COLOR);
   SLsmg_erase_eol ();
   
   return active;
}

static int draw_keystring (Menu_Keystring_Fun_Type *k, int color0, int color1, unsigned int field_width)
{
   int i;
   SLang_Key_Type *key, *key_root;
   FVOID_STAR fp;
   unsigned char type;
   char buf[3];
   unsigned int best_len;
   char *best_keystring;
   SLang_Key_Type *best_key;
   char *name;

   draw_name (k->name, color0, color1, field_width);

   name = k->keystring;
   /* Now try to draw the binding */

   if (NULL == (fp = (FVOID_STAR) SLang_find_key_function(name, CBuf->keymap)))
     type = SLKEY_F_INTERPRET;
   else type = SLKEY_F_INTRINSIC;

   best_keystring = NULL;
   best_len = 0xFFFF;
   best_key = NULL;

   key_root = CBuf->keymap->keymap;

   for (i = 0; i < 256; i++, key_root++)
     {
#ifdef IBMPC_SYSTEM
	if ((i == 0) || (i == 0xE0))
	  continue;
#endif

	key = key_root->next;
	if ((key == NULL) && (type == key_root->type))
	  {
	     if (type == SLKEY_F_INTERPRET)
	       {
		  if (strcmp (name, key_root->f.s))
		    continue;
	       }
	     else if ((type != SLKEY_F_INTRINSIC) || (fp != key_root->f.f))
	       continue;

	     buf[0] = i;
	     buf[1] = 0;
#ifndef IBMPC_SYSTEM
	     if (i == 0)
	       {
		  buf[0] = '^';
		  buf[1] = '@';
		  buf[2] = 0;
	       }
#endif
	     best_keystring = buf;
	     break;
	  }

	while (key != NULL)
	  {
	     char *s;
	     SLang_Key_Type *this_key = key;
	     
	     key = key->next;

	     if (this_key->type != type)
	       continue;

	     if (type == SLKEY_F_INTERPRET)
	       {
		  if (strcmp (name, this_key->f.s))
		    continue;
	       }
	     else if ((type != SLKEY_F_INTRINSIC) || (fp != this_key->f.f))
	       continue;

	     s = SLang_make_keystring (this_key->str);
	     if (s == NULL)
	       continue;

	     if (strlen (s) < best_len)
	       {
		  best_key = this_key;
		  best_len = strlen (s);
	       }
	  }
     }
   
   if (best_keystring == NULL)
     {
	if (best_key == NULL)
	  return 0;
	
	best_keystring = SLang_make_keystring (best_key->str);
	if (best_keystring == NULL)
	  return 0;
     }
   
   best_len = strlen (best_keystring);
   if (best_len > 4)
     return 0;

   SLsmg_forward (-4);
   SLsmg_set_color (color0);
   SLsmg_write_nchars (best_keystring, best_len);
   return 0;
}

static int draw_popup_menu (Menu_Popup_Type *p, int r, int c)
{
   int active_row, active_col;
   unsigned int dr, dc;
   Menu_Node_Type **l, **lmax;
   Menu_Popup_Type *p_active;

   Active_Popup = p;

   if (r == 0)
     r = 1;
   
   dr = p->num_subnodes + 2;
   dc = p->min_width + 5 + 4;	       /* allow room for keystring */
   
   if (c + (int)dc >= Jed_Num_Screen_Cols)
     {
	if ((int)dc > Jed_Num_Screen_Cols)
	  c = 0;
	else
	  c = Jed_Num_Screen_Cols - (int)dc;
     }

   if (r + (int)dr >= Jed_Num_Screen_Rows)
     {
	if ((int)dr >= Jed_Num_Screen_Rows)
	  {
	     r = 1;
	     dr = Jed_Num_Screen_Rows - 1;
	  }
	else
	  r = Jed_Num_Screen_Rows - (int)dr;
     }

   SLsmg_set_color_in_region (JMENU_SHADOW_COLOR, r + 1, c + 1, dr, dc);
   SLsmg_set_color (JMENU_POPUP_COLOR);
   /* SLsmg_fill_region (r, c, dr, dc, ' '); */
   if (Jed_Simulate_Graphic_Chars)
     simulate_box (r, c, dr, dc);
   else
     SLsmg_draw_box (r, c, dr, dc);

   p->row = r;
   p->column = c;
   p->max_row = r + dr;
   p->max_col = c + dc;

   /* Make sure a selection is present */
   (void) get_selected_menu_item (p);

   l = p->subnodes;
   lmax = l + p->num_subnodes;
   
   if (p->num_subnodes + 2 > dr)
     {
	unsigned int page;
	unsigned int nr = dr - 2;
	unsigned int col;
	
	col = c + dc - 4;
	page = p->active_node / nr;
	if (page)
	  {
	     l += page * nr;
	     draw_up_arrow (r, col);
	  }
	if (lmax > l + nr)
	  {
	     lmax = l + nr;
	     draw_down_arrow (r + dr - 1, col);
	  }
	else
	  l = lmax - nr;
     }
   
   p->visible_node_offset = (int) (l - p->subnodes);

   c++;
   r++;
   p_active = NULL;
   active_row = r;
   active_col = c;

   dc -= 3;

   while (l < lmax)
     {
	Menu_Node_Type *m;
	int color0, color1;

	m = *l;

	SLsmg_gotorc (r, c);

	color0 = JMENU_POPUP_COLOR;
	color1 = JMENU_CHAR_COLOR;
	
	if (m->flags & MENU_SELECTION)
	  {
	     active_row = r;
	     active_col = c + dc;
	     color0 = JMENU_SELECTION_COLOR;
	     color1 = JMENU_SELECTED_CHAR_COLOR;
	  }

	if ((m->flags & MENU_ACTIVE)
	    && (m->type == MENU_NODE_POPUP))
	  {
	     p_active = (Menu_Popup_Type *)m;
	     p_active->row = active_row;
	     p_active->column = active_col - dc/2;
	  }

	SLsmg_set_color (color0);
	switch (m->type)
	  {
	   case MENU_NODE_SEPARATOR:
	     SLsmg_write_nchars (" ", 1);
	     if (Jed_Simulate_Graphic_Chars)
	       simulate_hline (dc - 1);
	     else
	       SLsmg_draw_hline (dc-1);
	     SLsmg_write_nchars (" ", 1);
	     break;
	     
	   case MENU_NODE_POPUP:
	     SLsmg_write_nchars (" ", 1);
	     draw_name (m->name, color0, color1, dc);
	     SLsmg_gotorc (r, c + (dc - 2));
	     SLsmg_write_nchars (">>>", 3);
	     break;

	   case MENU_NODE_KEYSTRING:
	     SLsmg_write_nchars (" ", 1);
	     draw_keystring ((Menu_Keystring_Fun_Type *)m, color0, color1, dc);
	     break;

	   default:
	     SLsmg_write_nchars (" ", 1);
	     draw_name (m->name, color0, color1, dc);
	     break;
	  }
	l++;
	r++;
     }
   
   if (p_active != NULL)
     return draw_popup_menu (p_active, p_active->row, p_active->column);
   
   SLsmg_gotorc (active_row, active_col);
   return 0;
}

static Menu_Bar_Type *get_active_menubar (void)
{
   Menu_Bar_Type *b;

   /* Active_Popup = NULL; */
   b = Active_Menu_Bar;

   if (b == NULL)
     {
	Buffer *buf = CBuf;

	if (IN_MINI_WINDOW)
	  {
	     if (NULL == (buf = jed_get_mini_action_buffer ()))
	       return NULL;
	  }

	if (NULL == (b = buf->menubar))
	  b = Global_Menu_Bar;

	if (b == NULL)
	  return NULL;
	
     }
   /* Active_Popup = (Menu_Popup_Type *) b; */
   return b;
}

int jed_redraw_menus (void)
{
   Menu_Popup_Type *p;
   Menu_Bar_Type *b;
   int active;

#if 0
   if (NULL == (b = CBuf->menubar))
     b = Global_Menu_Bar;
#else
   b = get_active_menubar ();
#endif

   Active_Popup = (Menu_Popup_Type *) b;

   if (b == NULL)
     {
	SLsmg_gotorc (0, 0);
	SLsmg_set_color (0);
	SLsmg_erase_eol ();
	return 0;
     }
   
   active = draw_menu_bar (b);
   if (active == -1)
     return 0;

   touch_screen ();

   p = (Menu_Popup_Type *)b->subnodes[active];

   if (0 == (p->flags & MENU_ACTIVE))
     {
	SLsmg_gotorc (0, b->item_columns[active]);
	return 1;
     }

   draw_popup_menu (p, 0, b->item_columns[active]);
   return 1;
}


/* Keyboard Interface */

static int find_active_popup (void)
{
   Menu_Popup_Type *p;

   if (Active_Menu_Bar == NULL)
     return -1;
   
   p = (Menu_Popup_Type *) Active_Menu_Bar;
   
   while (1)
     {
	Active_Popup = p;
	p = (Menu_Popup_Type *) get_selected_menu_item (p);

	if ((p == NULL) || (p->type != MENU_NODE_POPUP))
	  return 0;

	if (0 == (p->flags & MENU_ACTIVE))
	  return 0;

	if (0 == (p->flags & MENU_POPUP_PREPARED))
	  return select_menu_cmd ();
     }
}
	
	
static int next_menubar_cmd (void)
{
   unsigned int flags;
   Menu_Popup_Type *p;
   
   if (NULL == (p = (Menu_Popup_Type *) Active_Menu_Bar))
     return -1;

   flags = get_active_node_flags (p) & MENU_ACTIVE;
   
   /* (void) unselect_active_node ((Menu_Popup_Type *)Active_Menu_Bar); */
   (void) select_next_active_node (p, p->active_node, flags);
   (void) find_active_popup ();
   return 1;
}

static int prev_menubar_cmd (void)
{
   unsigned int flags;
   Menu_Popup_Type *p;
   
   if (NULL == (p = (Menu_Popup_Type *) Active_Menu_Bar))
     return -1;

   flags = get_active_node_flags (p) & MENU_ACTIVE;
   /* (void) unselect_active_node ((Menu_Popup_Type *)Active_Menu_Bar); */
   (void) select_prev_active_node (p, p->active_node, flags);
   (void) find_active_popup ();
   return 1;
}

int jed_exit_menu_bar (void)
{
   while (Active_Popup != NULL)
     {
	unselect_active_node (Active_Popup);
	Active_Popup->active_node = 0;
	Active_Popup->flags &= ~MENU_POPUP_PREPARED;
	Active_Popup = Active_Popup->parent;
     }

   Active_Menu_Bar = NULL;
   Jed_Menus_Active = 0;
   return 1;
}

static int down_menu_cmd (void)
{
   if (Active_Popup == NULL)
     return -1;
   
   /* unselect_active_node (Active_Popup); */
   select_next_active_node (Active_Popup, Active_Popup->active_node, 0);
   return 1;
}

static int up_menu_cmd (void)
{
   if (Active_Popup == NULL)
     return -1;
   
   /* unselect_active_node (Active_Popup); */
   select_prev_active_node (Active_Popup, Active_Popup->active_node, 0);
   return 1;
}

static int pgup_menu_cmd (void)
{
   if (Active_Popup == NULL)
     return -1;

   select_next_active_node (Active_Popup, Active_Popup->num_subnodes, 0);
   return 1;
}

static int pgdn_menu_cmd (void)
{
   if (Active_Popup == NULL)
     return -1;

   select_prev_active_node (Active_Popup, 0, 0);
   return 1;
}

static int execute_keystring (char *s)
{
   int (*f)(void);

   if (NULL != (f = (int (*)(void)) (SLang_find_key_function(s, CBuf->keymap))))
     return (*f) ();
   
   if ((*s == '.')
       || !SLang_execute_function(s))
     SLang_load_string(s);

   return 1;
}

static char *get_full_popup_name (Menu_Popup_Type *p)
{
   Menu_Popup_Type *parent;
   unsigned int len;
   char *name;
   char *s;

   len = strlen (p->name);
   parent = p->parent;
   while (parent != NULL)
     {
	len += 1 + strlen (parent->name);
	parent = parent->parent;
     }
   
   name = SLmalloc (len + 1);
   if (name == NULL)
     return NULL;
   
   s = (name + len) - strlen (p->name);
   strcpy (s, p->name);
   parent = p->parent;
   while (parent != NULL)
     {
	s--;
	*s = '.';
	len = strlen (parent->name);
	s -= len;
	strncpy (s, parent->name, len);
	parent = parent->parent;
     }
   
   return name;
}

static int run_popup_callback (Menu_Popup_Type *p, SLang_Name_Type *cb)
{
   char *name;

   if (NULL == (name = get_full_popup_name (p)))
     return -1;

   if (-1 == SLang_push_string (name))
     {
	SLfree (name);
	return -1;
     }
   SLfree (name);

   if (0 != SLexecute_function (cb))
     return -1;

   return 0;
}

   
static int prepare_popup (Menu_Popup_Type *p)
{
   SLang_Name_Type *cb;

   if (NULL != (cb = p->select_popup_callback))
     {
	free_menu_popup_subnodes (p);
	if (-1 == run_popup_callback (p, cb))
	  return -1;
     }
   
   if (NULL != (cb = p->tweak_popup_callback))
     {
	if (-1 == run_popup_callback (p, cb))
	  return -1;
     }
   
   p->flags |= MENU_POPUP_PREPARED;
   return 0;
}


static int select_menu_cmd (void)
{
   Menu_Node_Type *m;
   Menu_Popup_Type *p;
   Menu_C_Fun_Type *c;
   SLang_Name_Type *nt;
   char *str;

   m = get_selected_menu_item (Active_Popup);
   if (m == NULL)
     return -1;

   switch (m->type)
     {
      case MENU_NODE_POPUP:
#if 0
	m->flags |= MENU_ACTIVE;
	p = (Menu_Popup_Type *) m;
#else
	/* Do it this way to avoid DEC C ansi-aliasing warning. */
	p = (Menu_Popup_Type *) m;
	p->flags |= MENU_ACTIVE;
#endif
	if (-1 == prepare_popup (p))
	  return -1;
	Active_Popup = p;
	select_next_active_node (p, p->num_subnodes, 0);
	return 1;

      case MENU_NODE_FUN_SLANG:
	nt = ((Menu_SLang_Fun_Type *) m)->slang_fun;
	jed_exit_menu_bar ();
	if (nt == NULL)
	  return -1;
	if ((-1 == SLang_start_arg_list ())
	    || (-1 == SLang_push_anytype (((Menu_SLang_Fun_Type*)m)->client_data))
	    || (-1 == SLang_end_arg_list ()))
	  return -1;

	return SLexecute_function (nt);

      case MENU_NODE_KEYSTRING:
	str = ((Menu_Keystring_Fun_Type *) m)->keystring;
	/* Grab a copy while it is still available */
	if (NULL == SLang_create_slstring (str))
	  return -1;
	jed_exit_menu_bar ();
	(void) execute_keystring (str);
	SLang_free_slstring (str);
	break;

      case MENU_NODE_FUN_C:
	c = (Menu_C_Fun_Type *) m;
	jed_exit_menu_bar ();
	if (c->c_fun != NULL)
	  return -1;
	return c->c_fun ();
	
     }
   return 0;
}

static int back_menu_cmd (void)
{
   if ((Active_Popup == NULL)
       || (Active_Popup == (Menu_Popup_Type *)Active_Menu_Bar)
       || (Active_Popup->parent == (Menu_Popup_Type *) Active_Menu_Bar))
     return jed_exit_menu_bar ();

   Active_Popup->flags &= ~(MENU_ACTIVE|MENU_POPUP_PREPARED);
   Active_Popup = Active_Popup->parent;
   return 1;
}

static Menu_Node_Type *find_subnode_via_char (Menu_Popup_Type *p, char ch)
{
   Menu_Node_Type **l, **lmax;
   unsigned int two;

   if (p == NULL)
     return NULL;
   
   two = 2;

   while (two)
     {
	l = p->subnodes;
	lmax = l + p->num_subnodes;
   
	while (l < lmax)
	  {
	     Menu_Node_Type *m;
	     char *s;
	     
	     m = *l++;
	     
	     if (m->type == MENU_NODE_SEPARATOR)
	       continue;
	     
	     if (m->flags & MENU_ITEM_UNAVAILABLE)
	       continue;

	     s = strchr (m->name, '&');
	     if (s == NULL)
	       continue;
	
	     if (s[1] == ch)
	       return m;
	  }
	
	ch = (char) CHANGE_CASE(ch);
	two--;
     }
   
   return NULL;
}

static int select_via_char_cmd (void)
{
   Menu_Node_Type *m;
   
   m = find_subnode_via_char (Active_Popup, (char) SLang_Last_Key_Char);
   if (m == NULL)
     return -1;

   set_node_selection (Active_Popup, m);
   return select_menu_cmd ();
}

   
static SLKeyMap_List_Type *Menu_Keymap;

static SLKeymap_Function_Type Menu_Keymap_Functions [] =
{
     {"next_menubar_menu", next_menubar_cmd},
     {"prev_menubar_menu", prev_menubar_cmd},
     {"exit_menubar",	jed_exit_menu_bar},
     {"next_menu_item",	down_menu_cmd},
     {"prev_menu_item",	up_menu_cmd},
     {"top_menu_item",	pgup_menu_cmd},
     {"bot_menu_item",	pgdn_menu_cmd},
     {"select_menu_item", select_menu_cmd},
     {"back_menu",	back_menu_cmd},

     {NULL, NULL}
};

#ifndef IBMPC_SYSTEM
# ifdef HAS_MOUSE
static int xterm_mouse_cmd (void);
# endif
#endif

static int init_menu_keymap (void)
{
   int ch;
   char simple[3];

   if (NULL == (Menu_Keymap = SLang_create_keymap ("menu", NULL)))
     return -1;

   Menu_Keymap->functions = Menu_Keymap_Functions;
   
   simple[1] = 0;
   for (ch = 0; ch < 256; ch++)
     {
	simple[0] = (char) ch;
	SLkm_define_key (simple, (FVOID_STAR) select_via_char_cmd, Menu_Keymap);
     }
   simple [0] = 27;
   simple [2] = 0;
   for (ch = 'a'; ch <= 'z'; ch++)
     {
	simple [1] = (char) ch;
	SLkm_define_key (simple, (FVOID_STAR) select_via_char_cmd, Menu_Keymap);
     }

   
   SLkm_define_key (" ", (FVOID_STAR) back_menu_cmd, Menu_Keymap);
   SLkm_define_key ("^?", (FVOID_STAR) back_menu_cmd, Menu_Keymap);
   SLkm_define_key ("^H", (FVOID_STAR) back_menu_cmd, Menu_Keymap);
   SLkm_define_key ("^G", (FVOID_STAR) jed_exit_menu_bar, Menu_Keymap);
   SLkm_define_key ("^Z", (FVOID_STAR) sys_spawn_cmd, Menu_Keymap);
   SLkm_define_key ("^M", (FVOID_STAR) select_menu_cmd, Menu_Keymap);

#ifndef IBMPC_SYSTEM
   SLkm_define_key ("\033[A", (FVOID_STAR) up_menu_cmd, Menu_Keymap);
   SLkm_define_key ("\033OA", (FVOID_STAR) up_menu_cmd, Menu_Keymap);
   SLkm_define_key ("\033[B", (FVOID_STAR) down_menu_cmd, Menu_Keymap);
   SLkm_define_key ("\033OB", (FVOID_STAR) down_menu_cmd, Menu_Keymap);
   SLkm_define_key ("\033[C", (FVOID_STAR) next_menubar_cmd, Menu_Keymap);
   SLkm_define_key ("\033OC", (FVOID_STAR) next_menubar_cmd, Menu_Keymap);
   SLkm_define_key ("\033[D", (FVOID_STAR) prev_menubar_cmd, Menu_Keymap);
   SLkm_define_key ("\033OD", (FVOID_STAR) prev_menubar_cmd, Menu_Keymap);
   SLkm_define_key ("\033[5~", (FVOID_STAR) pgup_menu_cmd, Menu_Keymap);
   SLkm_define_key ("\033[6~", (FVOID_STAR) pgdn_menu_cmd, Menu_Keymap);
# ifdef __unix__
   SLkm_define_key ("^(ku)", (FVOID_STAR) up_menu_cmd, Menu_Keymap);
   SLkm_define_key ("^(kd)", (FVOID_STAR) down_menu_cmd, Menu_Keymap);
   SLkm_define_key ("^(kr)", (FVOID_STAR) next_menubar_cmd, Menu_Keymap);
   SLkm_define_key ("^(kl)", (FVOID_STAR) prev_menubar_cmd, Menu_Keymap);
   SLkm_define_key ("^(kP)", (FVOID_STAR) pgup_menu_cmd, Menu_Keymap);
   SLkm_define_key ("^(kN)", (FVOID_STAR) pgdn_menu_cmd, Menu_Keymap);
# endif
# ifdef HAS_MOUSE
   SLkm_define_key ("\033[M", (FVOID_STAR) xterm_mouse_cmd, Menu_Keymap);
# endif
#else				       /* IBMPC_SYSTEM */
#include "doskeys.h"
   
   SLkm_define_key (PC_UP, (FVOID_STAR) up_menu_cmd, Menu_Keymap);
   SLkm_define_key (PC_UP1, (FVOID_STAR) up_menu_cmd, Menu_Keymap);
   SLkm_define_key (PC_DN, (FVOID_STAR) down_menu_cmd, Menu_Keymap);
   SLkm_define_key (PC_DN1, (FVOID_STAR) down_menu_cmd, Menu_Keymap);
   SLkm_define_key (PC_RT, (FVOID_STAR) next_menubar_cmd, Menu_Keymap);
   SLkm_define_key (PC_RT1, (FVOID_STAR) next_menubar_cmd, Menu_Keymap);
   SLkm_define_key (PC_LT, (FVOID_STAR) prev_menubar_cmd, Menu_Keymap);
   SLkm_define_key (PC_LT1, (FVOID_STAR) prev_menubar_cmd, Menu_Keymap);
   SLkm_define_key (PC_PGUP, (FVOID_STAR) pgup_menu_cmd, Menu_Keymap);
   SLkm_define_key (PC_PGUP1, (FVOID_STAR) pgup_menu_cmd, Menu_Keymap);
   SLkm_define_key (PC_PGDN, (FVOID_STAR) pgdn_menu_cmd, Menu_Keymap);
   SLkm_define_key (PC_PGDN1, (FVOID_STAR) pgdn_menu_cmd, Menu_Keymap);
#endif

#ifdef HAS_MOUSE
   SLkm_define_key ("\033^@", (FVOID_STAR) jed_mouse_cmd, Menu_Keymap);
#endif
   return 0;
}


SLang_Key_Type *jed_menu_do_key (void)
{
   int ch;

   if (Executing_Keyboard_Macro
       || (Read_This_Character != NULL))
     return SLang_do_key (Menu_Keymap, jed_getkey);

   ch = my_getkey ();

   if (SLKeyBoard_Quit || SLang_get_error ())
     {
	jed_exit_menu_bar ();
	return NULL;
     }

   if (ch == 27)		       /* ESC */
     {
	int tsecs = 2;
	if (0 == input_pending (&tsecs))
	  ch = 127;
     }
   ungetkey (&ch);
   
   return SLang_do_key (Menu_Keymap, jed_getkey);
}

static int exec_menubar_callback (Menu_Bar_Type *b, SLang_Name_Type **ntp)
{
   SLang_Name_Type *nt;
   
   nt = *ntp;
   if (nt == NULL)
     return 0;

   if (SLang_get_error ())
     return -1;

   if (-1 == SLang_push_string (b->name))
     return -1;
   
   if (-1 == SLexecute_function (nt))
     {
	if (Active_Menu_Bar == b)      /* disable callback */
	  {
	     SLang_free_function (nt);
	     *ntp = NULL;
	  }
	return -1;
     }

   return 0;
}


void jed_notify_menu_buffer_changed (void)
{
   Menu_Bar_Type *b;
   int err;

   if (NULL == (b = get_active_menubar ()))
     return;

   err = SLang_get_error ();
   SLang_set_error (0);
   (void) exec_menubar_callback (b, &b->init_callback);
   if (SLang_get_error () == 0)
     SLang_set_error (err);
}


int jed_select_menu_bar (void)
{
   Menu_Popup_Type *p;
   Menu_Node_Type **l, **lmax;

   if (IN_MINI_WINDOW)
     {
	/* For now do not allow access to menus from the minibuffer
	 * since the minibuffer is not recursive
	 */
	return -1;
     }
   
   if (NULL == (Active_Menu_Bar = get_active_menubar ()))
     return -1;
   
   p = (Menu_Popup_Type *) Active_Menu_Bar;
   Active_Popup = p;

   l = p->subnodes;
   lmax = l + p->num_subnodes;

   /* Reset each sub-node to its default value.  This is necessary 
    * to ensure that keyboard macros will work when one switches from
    * one node to another via next/prev_menubar_cmd functions.
    */
   while (l < lmax)
     {
	Menu_Popup_Type *pp = (Menu_Popup_Type *) *l++;

	if (pp->type == MENU_NODE_POPUP)
	  {
	     (void) unselect_active_node (pp);
	     pp->active_node = 0;
	  }
     }

   p->active_node = 0;
   
   if (-1 == exec_menubar_callback (Active_Menu_Bar, &Active_Menu_Bar->select_callback))
     {
	jed_exit_menu_bar ();
	return -1;
     }
   
   (void) get_selected_menu_item (p);
   touch_screen ();
   Jed_Menus_Active = 1;
   return 0;
}

/* Interpreter interface */

static Menu_Node_Type *find_menu_node1 (Menu_Popup_Type *m, 
					char *name,
					int stop_at_last_popup)
{
   char *name_end;
   Menu_Node_Type **l, **lmax;

   name_end = strchr (name, '.');
   if (name_end == NULL)
     name_end = name + strlen (name);

   l = m->subnodes;
   lmax = l + m->num_subnodes;
   
   while (l < lmax)
     {
	Menu_Popup_Type *p;
	
	p = (Menu_Popup_Type *) *l++;

	if (0 == menu_name_eqs (p->name, name, name_end))
	  continue;

	if (*name_end == 0)
	  {
	     if (stop_at_last_popup
		 && ((p->type != MENU_NODE_POPUP) 
		     || (p->type != MENU_NODE_MENUBAR)))
	       return (Menu_Node_Type *)m;

	     return (Menu_Node_Type *)p;
	  }
	     
	if (p->type == MENU_NODE_POPUP)
	  return find_menu_node1 (p, name_end + 1, stop_at_last_popup);

	return NULL;
     }

   return NULL;
}

   

static Menu_Node_Type *find_menu_node (char *name, int stop_at_last_popup)
{
   char *s;
   Menu_Node_Type *m;

   m = (Menu_Node_Type *) menu_find_menu_bar (name, 1);
   if (m == NULL)
     return m;

   s = strchr (name, '.');
   if (s == NULL)
     return m;

   m = find_menu_node1 ((Menu_Popup_Type *)m, s + 1, stop_at_last_popup);

   if (m == NULL)
     SLang_verror (SL_INTRINSIC_ERROR,
		   "Unable to find menu node %s",
		   name);
   return m;
}

static Menu_Popup_Type *find_menu_popup (char *name)
{
   Menu_Popup_Type *m;
   
   m = (Menu_Popup_Type *) find_menu_node (name, 0);
   if (m == NULL)
     return m;

   if ((m->type != MENU_NODE_POPUP)
       && (m->type != MENU_NODE_MENUBAR))
     {
	SLang_verror (SL_INVALID_PARM, 
		      "%s is not a menu node", name);
	return NULL;
     }

   return m;
}

/* If name is NULL or non-existent, then insert at end */
static unsigned int find_where_to_insert (Menu_Popup_Type *p, char *name)
{
   unsigned int i, num;

   num = p->num_subnodes;

   if (name != NULL) for (i = 0; i < num; i++)
     {
	if (0 == strcmp (name, p->subnodes[i]->name))
	  return i;
     }
   return num;
}

static void create_menu_bar_cmd (char *name)
{
   if (0 == strcmp ("Global", name))
     {
	Menu_Bar_Type *g = create_menu_bar (name, 5);
	if (g != NULL)
	  jed_delete_menu_bar (Global_Menu_Bar);
	Global_Menu_Bar = g;
	return;
     }

   if (NULL != menu_find_menu_bar (name, 0))
     return;

   (void) create_menu_bar (name, 5);
}

static void set_buffer_menu_bar_cmd (char *name)
{
   Menu_Bar_Type *b;

   b = menu_find_menu_bar (name, 1);
   if (b == NULL)
     return;
   
   jed_delete_menu_bar (CBuf->menubar);
   
   CBuf->menubar = b;
   b->num_refs += 1;
}

static int pop_where_to_insert (Menu_Popup_Type *p, unsigned int *where)
{
   if (SLang_peek_at_stack () == SLANG_STRING_TYPE)
     {
	char *s;
	
	if (-1 == SLang_pop_slstring (&s))
	  return -1;
	*where = find_where_to_insert (p, s);
	SLang_free_slstring (s);
	return 0;
     }
   
   if (-1 == SLang_pop_uinteger (where))
     return -1;
   
   return 0;
}

static void insert_popup_menu_cmd (char *dest, char *menu)
{
   Menu_Popup_Type *m;
   unsigned int where;

   m = find_menu_popup (dest);

   if (m == NULL)
     return;

   if (-1 == pop_where_to_insert (m, &where))
     return;

   insert_popup_menu (m, menu, where);
}

   
static void append_popup_menu_cmd (char *dest, char *menu)
{
   Menu_Popup_Type *m;

   m = find_menu_popup (dest);

   if (m == NULL)
     return;

   insert_popup_menu (m, menu, m->num_subnodes);
}

static void insert_separator_cmd (char *name)
{
   Menu_Popup_Type *m;
   unsigned int where;

   m = find_menu_popup (name);

   if (m == NULL)
     return;

   if (-1 == pop_where_to_insert (m, &where))
     return;

   insert_separator (m, where);
}

static void append_separator_cmd (char *name)
{
   Menu_Popup_Type *m;

   m = find_menu_popup (name);

   if (m != NULL)
     insert_separator (m, m->num_subnodes);
}

static void insert_menu_item_cmd_internal (int is_fun, int do_append)
{
   Menu_Popup_Type *m;
   SLang_Name_Type *nt = NULL;
   SLang_Any_Type *client_data = NULL;
   char *str = NULL;
   char *menu = NULL;
   char *name = NULL;
   unsigned int where;

   if (is_fun)
     {
	if (-1 == SLang_pop_anytype (&client_data))
	  return;

	if (NULL == (nt = SLang_pop_function ()))
	  goto free_and_return;
     }
   else if (-1 == SLang_pop_slstring (&str))
     return;

   if (-1 == SLang_pop_slstring (&name))
     goto free_and_return;

   if (-1 == SLang_pop_slstring (&menu))
     goto free_and_return;
   
   if (NULL == (m = find_menu_popup (menu)))
     goto free_and_return;
   
   if (do_append)
     where = m->num_subnodes;
   else if (-1 == pop_where_to_insert (m, &where))
     goto free_and_return;

   if (nt != NULL)
     {
	if (NULL != insert_slang_fun_item (m, name, nt, client_data, where))
	  {
	     client_data = NULL;
	     nt = NULL;
	  }
     }
   else
     (void) insert_keystring_item (m, name, str, where);
   /* drop */

   free_and_return:

   if (client_data != NULL) 
     SLang_free_anytype (client_data);
   SLang_free_slstring (str);
   SLang_free_slstring (menu);
   SLang_free_slstring (name);
   SLang_free_function (nt);
}

static void insert_menu_item_cmd (void)
{
   insert_menu_item_cmd_internal ((SLang_Num_Function_Args == 5), 0);
}

static void append_menu_item_cmd (void)
{
   insert_menu_item_cmd_internal ((SLang_Num_Function_Args == 4), 1);
}

static void menu_delete_item_cmd (char *menu)
{
   Menu_Popup_Type *parent;
   Menu_Node_Type *child;

   parent = (Menu_Popup_Type *)find_menu_node (menu, 1);
   child = find_menu_node (menu, 0);
   if ((parent == NULL) || (child == NULL))
     {
	SLang_verror (SL_INVALID_PARM, 
		      "Menu %s does not exist", menu);
	return;
     }
   menu_delete_node (parent, child);
}

static void menu_delete_items_cmd (char *menu)
{
   Menu_Popup_Type *p;

   if (NULL == (p = find_menu_popup (menu)))
     return;

   menu_delete_nodes (p);
}

static void set_object_available_cmd (char *name, int *flag)
{
   Menu_Node_Type *m;
   
   if (NULL == (m = find_menu_node (name, 0)))
     return;
   
   if (*flag) m->flags &= ~MENU_ITEM_UNAVAILABLE;
   else
     m->flags |= MENU_ITEM_UNAVAILABLE;
}

static int pop_popup_callback (Menu_Popup_Type **pp, int type,
			       SLang_Name_Type **ntp)
{
   SLang_Name_Type *nt;
   char *popup;
   Menu_Popup_Type *p;

   if (SLang_peek_at_stack () == SLANG_NULL_TYPE)
     {
	(void) SLang_pop_null ();
	nt = NULL;
     }
   else if (NULL == (nt = SLang_pop_function ()))
     return -1;

   if (-1 == SLang_pop_slstring (&popup))
     {
	SLang_free_function (nt);
	return -1;
     }

   p = find_menu_popup (popup);
   if (p == NULL)
     {
	SLang_free_function (nt);
	SLang_free_slstring (popup);
	return -1;
     }
   
   if (type && (p->type != type))
     {
	SLang_verror (SL_INVALID_PARM, "%s does not specify the proper menu type", popup);
	SLang_free_function (nt);
	SLang_free_slstring (popup);
	return -1;
     }
   
   SLang_free_slstring (popup);

   *ntp = nt;
   *pp = p;
   return 0;
}

static int pop_menubar_callback (Menu_Bar_Type **bp, SLang_Name_Type **nt)
{
   return pop_popup_callback ((Menu_Popup_Type **)bp, MENU_NODE_MENUBAR, nt);
}

static void set_select_menubar_callback (void)
{
   Menu_Bar_Type *b;
   SLang_Name_Type *nt;

   if (-1 == pop_menubar_callback (&b, &nt))
     return;

   b->select_callback = nt;
}

static void set_init_menubar_callback (void)
{
   Menu_Bar_Type *b;
   SLang_Name_Type *nt;
   
   if (-1 == pop_menubar_callback (&b, &nt))
     return;

   b->init_callback = nt;
}


static void set_select_popup_callback (void)
{
   Menu_Popup_Type *p;
   SLang_Name_Type *nt;

   if (-1 == pop_popup_callback (&p, MENU_NODE_POPUP, &nt))
     return;

   p->select_popup_callback = nt;
}

static void set_tweak_popup_callback (void)
{
   Menu_Popup_Type *p;
   SLang_Name_Type *nt;

   if (-1 == pop_popup_callback (&p, MENU_NODE_POPUP, &nt))
     return;

   p->tweak_popup_callback = nt;
}

static void copy_menu_cmd (char *destname, char *srcname)
{
   Menu_Popup_Type *dest;
   Menu_Node_Type *src;

   dest = find_menu_popup (destname);
   if (dest == NULL)
     return;
   
   src = find_menu_node (srcname, 0);
   if (src == NULL)
     return;
   
   (void) copy_menu (dest, src);
}

static void set_menu_bar_prefix_string (char *menubar, char *s)
{
   Menu_Bar_Type *b;
   
   if (NULL == (b = menu_find_menu_bar (menubar, 1)))
     return;

   s = SLang_create_slstring (s);
   if (s == NULL)
     return;
   
   SLang_free_slstring (b->prefix_string);
   b->prefix_string = s;
}

   
static void popup_menu_cmd (char *menu)
{
   char *popup;

   (void) jed_exit_menu_bar ();
   if (-1 == jed_select_menu_bar ())
     return;

   if (NULL == (menu = SLmake_string (menu)))
     return;

   /* for now ignore the menubar name and use default */
   popup = strchr (menu, '.');
   if (popup != NULL)
     popup++;

   while ((popup != NULL) && (Active_Popup != NULL))
     {
	Menu_Node_Type *m;
	char *next_popup;
	
	next_popup = strchr (popup, '.');
	if (next_popup != NULL)
	  *next_popup++ = 0;

	if (NULL == (m = find_subnode (Active_Popup, popup)))
	  {
	     SLang_verror (SL_INVALID_PARM, 
			   "Unable to find a popup menu called %s", menu);
	     break;
	  }
	set_node_selection (Active_Popup, m);
	if (-1 == select_menu_cmd ())
	  break;
	
	popup = next_popup;
     }
   
   SLfree (menu);
}


static SLang_Intrin_Fun_Type Menu_Table[] =
{
   MAKE_INTRINSIC_S("menu_create_menu_bar", create_menu_bar_cmd, VOID_TYPE),
   MAKE_INTRINSIC_SS("menu_append_popup", append_popup_menu_cmd, VOID_TYPE),
   MAKE_INTRINSIC_SS("menu_insert_popup", insert_popup_menu_cmd, VOID_TYPE),
   MAKE_INTRINSIC_S("menu_use_menu_bar", set_buffer_menu_bar_cmd, VOID_TYPE),
   MAKE_INTRINSIC_S("menu_append_separator", append_separator_cmd, VOID_TYPE),
   MAKE_INTRINSIC_S("menu_insert_separator", insert_separator_cmd, VOID_TYPE),
   MAKE_INTRINSIC_0("menu_append_item", append_menu_item_cmd, VOID_TYPE),
   MAKE_INTRINSIC_0("menu_insert_item", insert_menu_item_cmd, VOID_TYPE),
   MAKE_INTRINSIC_S("menu_delete_item", menu_delete_item_cmd, VOID_TYPE),
   MAKE_INTRINSIC_S("menu_delete_items", menu_delete_items_cmd, VOID_TYPE),
   MAKE_INTRINSIC_SI("menu_set_object_available", set_object_available_cmd, VOID_TYPE),
   MAKE_INTRINSIC_0("menu_set_select_menubar_callback", set_select_menubar_callback, VOID_TYPE),
   MAKE_INTRINSIC_0("menu_set_init_menubar_callback", set_init_menubar_callback, VOID_TYPE),
   MAKE_INTRINSIC_0("menu_set_select_popup_callback", set_select_popup_callback, VOID_TYPE),
   MAKE_INTRINSIC_0("menu_set_tweak_popup_callback", set_tweak_popup_callback, VOID_TYPE),
   MAKE_INTRINSIC_SS("menu_copy_menu", copy_menu_cmd, VOID_TYPE),
   MAKE_INTRINSIC_SS("menu_set_menu_bar_prefix", set_menu_bar_prefix_string, VOID_TYPE),
   MAKE_INTRINSIC_S("menu_select_menu", popup_menu_cmd, VOID_TYPE),
   MAKE_INTRINSIC(NULL,NULL,0,0)
};

int jed_init_menus (void)
{
   if (-1 == init_menu_keymap ())
     return -1;

   if (-1 == SLadd_intrin_fun_table (Menu_Table, NULL))
     return -1;
#if 0
   if (-1 == make_global_menubar ())
     return -1;
#endif
   return 0;
}

#ifdef HAS_MOUSE
static int select_menu_via_rc (int type, int r, int c)
{
   Menu_Popup_Type *p;
   
   p = Active_Popup;
   while (p != (Menu_Popup_Type *)Active_Menu_Bar)
     {
	if ((r >= p->row) 
	    && (r < p->max_row)
	    && (c >= p->column)
	    && (c < p->max_col))
	  break;
	
	p = p->parent;
     }
   
   if ((p == NULL) || (p->type == MENU_NODE_MENUBAR))
     {
	unsigned int i;
	int *item_columns;

	if (Active_Popup == NULL)
	  return -1;
	
	if (r != 0)
	  {
	     if (type != JMOUSE_DRAG)
	       return -1;

	     if (Active_Popup->type != MENU_NODE_MENUBAR)
	       return 0;
	  }
	
	unselect_active_node ((Menu_Popup_Type *) Active_Menu_Bar);

	i = Active_Menu_Bar->num_subnodes;
	item_columns = Active_Menu_Bar->item_columns;
	while (i > 0)
	  {
	     i--;

	     if ((i != 0) && (item_columns[i] > c))
	       continue;

	     p = (Menu_Popup_Type *)Active_Menu_Bar->subnodes[i];
	     if (p->type == MENU_NODE_SEPARATOR)
	       continue;
		  
	     if (p->flags & MENU_ITEM_UNAVAILABLE)
	       continue;

	     if (-1 == set_node_selection ((Menu_Popup_Type *) Active_Menu_Bar,
					   (Menu_Node_Type *) p))
	       return -1;
	     
	     if (-1 == find_active_popup ())
	       return -1;

	     if (p->type == MENU_NODE_POPUP)
	       return select_menu_cmd ();
	  }
	return -1;
     }
 
   if (p == Active_Popup)
     {
	r -= (p->row + 1);
	r += p->visible_node_offset;
	if ((r >= (int)p->num_subnodes) || (r < 0))
	  return 0;
	
	if (-1 == set_node_selection (p, p->subnodes[r]))
	  return 0;
	
	if ((type != JMOUSE_DRAG)
	    || ((p->subnodes[r]->type == MENU_NODE_POPUP)
		&& (c + 1 >= p->max_col)))
	  select_menu_cmd ();

	return 0;
     }
   
   while (Active_Popup != p)
     back_menu_cmd ();

   return 0;
}

int jed_menu_handle_mouse (unsigned int type, 
			   int x, int y, int button, int shift)
{
   (void) shift; (void) button;

   if ((type != JMOUSE_UP) && (type != JMOUSE_DRAG))
     return -1;

   if ((Jed_Menus_Active == 0)
       && (-1 == jed_select_menu_bar ()))
     return -1;

   if (-1 == select_menu_via_rc (type, y-1, x-1))
     {
	if (type != JMOUSE_DRAG)
	  jed_exit_menu_bar ();
     }

   return 0;
}

#ifndef IBMPC_SYSTEM
static int xterm_mouse_cmd (void)
{
   int x, y, b;
   
   b = my_getkey ();
   x = (unsigned char) my_getkey () - 32;
   y = (unsigned char) my_getkey () - 32;
   
   /* We need to trigger on the button release event */
   
   b -= 32;
   if ((b & 3) != 3)
     return -1;
   
   return jed_menu_handle_mouse (JMOUSE_UP, x, y, 0, 0);
}


#endif 				       /* !IBMPC_SYSTEM */
#endif				       /* HAS_MOUSE */
#endif				       /* JED_HAS_MENUS */
