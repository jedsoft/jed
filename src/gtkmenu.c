/* Drop-down menus (GTK) */
/*
 * This file is part of JED editor library source.
 *
 * You may always distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#define _BUILD_GTK_JED

#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#if JED_HAS_MENUS

# include <gtk/gtk.h>
# include <gdk/gdkx.h>
# include <gdk/gdkkeysyms.h>

# include <slang.h>

# include "jdmacros.h"

# include <string.h>

# include "buffer.h"
# include "screen.h"
# include "misc.h"
# include "sysdep.h"
# include "cmds.h"
# include "menu.h"
# include "colors.h"

#include "gtkjed.h"

# define GTK_HAS_TOOLTIPS 0

# if 0
static GHashTable    *menuArray;

/*************************************************************************
 *
 * Datastructure to manage GtkMenus and Toolbars
 *
 * For Menus:
 *
 *   Settings for the Menubar:
 *      parent     = NULL
 *      menuItem   = menubar
 *      subMenu    = NULL
 *      path       = menu path
 *
 *   Settings for the submenu:
 *      parent     = parent of type GtkJedMenuType *
 *      menuItem   = current menu item (item in current menu)
 *      subMenu    = the newly created submenu
 *      path       = menu path
 *
 *   Settings for the menu item:
 *      parent     = parent of type GtkJedMenuType *
 *      menuItem   = current menu item (item in current menu)
 *      subMenu    = NULL
 *      path       = menu path
 *
 ************************************************************************/

typedef struct GtkJedMenuTypeStruct
{
   struct GtkJedMenuTypeStruct *parent;
   GtkWidget                   *menuItem;
   GtkWidget                   *subMenu;
   char                        *path;
}
GtkJedMenuType;

# endif

 /* static int         toolbarInputSelected = 0; */
 /* static int         tbActive = 0; */

extern void       gtkCreateTBCmd( char * );
extern void       gtkSetBufferMenuBarCmd( char * );
extern void       gtkInsertPopupMenuCmd( char *, char * );
extern void       gtkAppendTBCmd( char *, char * );
extern void       gtkInsertSeparatorCmd( char * );
extern void       gtkAppendSeparatorCmd( char * );
extern void       gtkMenuItemNew( char *, char *, int );
extern void       gtkMenuDeleteItemCmd( char * );
extern void       gtkMenuDeleteItemsCmd( char * );
extern void       gtkSetObjectAvailableCmd( char * );

typedef struct _Menu_Node_Type
{
   char *name;
   int type;
# define MENU_NODE_FUN_SLANG	1
      /* typedef struct {} Menu_SLang_Fun_Type; */
# define MENU_NODE_FUN_C		2
      /* typedef struct {} Menu_C_Fun_Type; */
# define MENU_NODE_POPUP		3
      /* typedef struct _Menu_Popup_Type {} Menu_Popup_Type; */
# define MENU_NODE_SEPARATOR	4
      /* typedef struct _Menu_Node_Type {} Menu_Node_Type; */
# define MENU_NODE_MENUBAR	5
      /* struct _Menu_Bar_Type {}; */
# define MENU_NODE_KEYSTRING	6
      /* typedef struct {} Menu_Keystring_Fun_Type; */
# define MENU_NODE_TYPEDKEYS     7
      /* typedef struct {} Menu_Keystring_Fun_Type; */

   unsigned int flags;
# define MENU_ACTIVE		     1
# define MENU_SELECTION		     2
# define MENU_ITEM_UNAVAILABLE	     4
# define MENU_POPUP_PREPARED	     8
# define JGTK_MENU_POPUP_PREPARED    16

   GtkWidget               *menuItem;
   GtkWidget               *subMenu;
}
Menu_Node_Type;

typedef struct _Menu_Popup_Type
{
   char *name;
   int type;			       /* =MENU_NODE_POPUP 3 */
   unsigned int flags;

   GtkWidget               *menuItem;
   GtkWidget               *subMenu;

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
   int type;			       /* =MENU_NODE_MENUBAR 5 */
   unsigned int flags;

   GtkWidget               *menuItem;  /* gtk_menu_bar */
   GtkWidget               *subMenu;   /* NULL */

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

# define DEFAULT_MENU_PREFIX "F10 key ==> "
   char *prefix_string;
   struct _Menu_Bar_Type *next;
};

typedef struct
{
   char *name;
   int type;              /* =MENU_NODE_FUN_SLANG 1 */
   unsigned int flags;

   GtkWidget               *menuItem;
   GtkWidget               *subMenu;

   SLang_Name_Type *slang_fun;
   SLang_Any_Type *client_data;
}
Menu_SLang_Fun_Type;

typedef struct
{
   char *name;
   int type;              /* =MENU_NODE_FUN_C 2 */
   unsigned int flags;

   GtkWidget               *menuItem;
   GtkWidget               *subMenu;

   int (*c_fun)(void);
}
Menu_C_Fun_Type;

typedef struct
{
   char *name;
   int type;              /* =MENU_NODE_KEYSTRING 6, =Menu_NODE_TYPEDKEYS 7 */
   unsigned int flags;

   GtkWidget               *menuItem;
   GtkWidget               *subMenu;

   char *keystring;
}
Menu_Keystring_Fun_Type;

/*******************************************************************************/
/***** creating Toolbars *******************************************************/
/*******************************************************************************/

extern void       gtkCreateToolbarCmd( Menu_Bar_Type * );
extern void       gtkAddBufferToolbarCmd( char * );
static void       gtkToolbarStockItemNew( Menu_Popup_Type *, Menu_Node_Type *, char *, int );
static void       gtkToolbarItemNew( Menu_Popup_Type *, Menu_Node_Type *, char *, int );
static void       gtkToolbarAppendSeparatorCmd( Menu_Popup_Type *, Menu_Node_Type * );
static void       gtkToolbarInsertSeparatorCmd( Menu_Popup_Type *, Menu_Node_Type *, int );

extern int        createKeyEvents( char * );

static void       jGtkInsertBufferToolbarCmd( char *, int * );

static int        jGtkSelectTB(void);
static int        jGtkExecTB(void);

static gboolean   menuBarActivateCurrentCB( GtkWidget *,
					    GdkEvent  *,
					    gpointer  * );

int        Jed_Menus_Active;

static Menu_Bar_Type     *Global_Menu_Bar;
static Menu_Bar_Type     *Menu_Bar_Root;
static Menu_Bar_Type     *Active_Menu_Bar;
static Menu_Popup_Type   *Active_Popup;

static void free_menu_node (Menu_Node_Type *);
static int select_menu_cmd (void);

extern void              execTBCmd( char * );
static int               tbExecSlangFlag = 0;
static Menu_Node_Type   *actTBCmdNode;
static int               tbInputSelected = 0;

static Menu_Popup_Type  *activeTopSubmenu = NULL;

static void       jGtkCreateMenuBarCmd( Menu_Bar_Type * );
static void       jGtkInsertPopupMenuCmd( Menu_Popup_Type *, int );
static void       jGtkSimpleInsertPopupMenuCmd( Menu_Popup_Type *, int );
static void       jGtkMenuItemNew( Menu_Popup_Type *, Menu_Node_Type *, int );
static void       jGtkMenuKeystringItemNew( Menu_Popup_Type *, Menu_Node_Type *, int );

static void       jGtkInsertSeparatorCmd( Menu_Popup_Type *, Menu_Node_Type *, int );
/* static void       jGtkAppendSeparatorCmd( Menu_Node_Type * ); */

void       toolbarCallback( GtkWidget *, gpointer );

# define TB_SEL_KEY_SEQ         "\033[TB"
# define TB_EXE_KEY_SEQ         "\033[TX"

static char   tbExeKeySeq[] =  "\x1B[TX";  /* = TB_EXE_KEY_SEQ */
static int    tbExeKeyInx   =  0;

/************************************
* free_menu_popup_subnodes
*
* debug print: "Menu: %x\n", m
*
************************************/

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

/************************************
* free_menu_popup_private_data
*
* debug print: "Menu: %x\n", m
*
************************************/

static void free_menu_popup_private_data (Menu_Popup_Type *m)
{
   free_menu_popup_subnodes (m);
   SLang_free_function (m->select_popup_callback);
   SLang_free_function (m->tweak_popup_callback);
   m->select_popup_callback = NULL;
   m->tweak_popup_callback = NULL;
}

/************************************
* free_menu_bar_private_data
*
* debug print: "Menu: %x\n", m
*
************************************/

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

/************************************
* free_keystring_private_data
*
* debug print: "Menu Key String Function: %x\n", m
*
************************************/

static void free_keystring_private_data (Menu_Keystring_Fun_Type *m)
{
   SLang_free_slstring (m->keystring);
}

/************************************
* free_slangfun_private_data
*
* debug print: "Menu: %x\n", m
*
************************************/

static void free_slangfun_private_data (Menu_SLang_Fun_Type *m)
{
   if (m->client_data != NULL)
     SLang_free_anytype (m->client_data);
   SLang_free_function (m->slang_fun);
}

/************************************
* free_menu_node
*
* debug print: "Menu: %x, Name: |%s|\n", m, m->name
*
************************************/

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

   /* Dbp1( "m->menuItem: %x\n", m->menuItem ); */

   if ( m->menuItem ) gtk_widget_destroy( m->menuItem );

   /* Dbp1( "m->subMenu: %x\n", m->subMenu ); */

   if ( m->subMenu ) gtk_widget_destroy( m->subMenu );

   SLang_free_slstring (m->name);
   SLfree ((char *) m);
}

/************************************
* create_menu_node
*
* debug print: "Name: %s, Node type: %d, Size of node: %d\n", name, node_type, sizeof_node
*
************************************/

static Menu_Node_Type *create_menu_node (char *name, int node_type,
					 unsigned int sizeof_node)
{
   Menu_Node_Type *m;

   m = (Menu_Node_Type *) jed_malloc0 (sizeof_node);
   if (m == NULL)
     return NULL;

   m->type = node_type;
   m->menuItem = NULL;
   m->subMenu = NULL;
   /* m->flags = 0; */

   if (NULL == (m->name = SLang_create_slstring (name)))
     {
	SLfree ((char *)m);
	return NULL;
     }

   return m;
}

/************************************
* create_menu_popup
*
* debug print: "Name: %s, Num items: %d\n", name, num_items
*
************************************/

static Menu_Popup_Type *
create_menu_popup (char *name, unsigned int num_items)
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

/************************************
* create_menu_bar
*
* debug print: "Name: %s, Num items: %d\n", name, num_items
*
************************************/

static Menu_Bar_Type *
create_menu_bar( char *name,
		 unsigned int num_items )
{
   Menu_Bar_Type *m;
   char *prefix;

   m = ( Menu_Bar_Type * ) create_menu_node( name, MENU_NODE_MENUBAR, sizeof( Menu_Bar_Type ) );
   if ( m == NULL )
     return NULL;

   if ( num_items == 0 )
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

/************************************
* jed_delete_menu_bar
*
* debug print: "Menu Bar Type: %x\n", m
*
************************************/

void
jed_delete_menu_bar (Menu_Bar_Type *m)
{
   if ( m == NULL )
     return;

   free_menu_node ((Menu_Node_Type *)m);
}

/************************************
* menu_name_eqs
*
* debug print: "A: |%s|, B: |%s|, bMax: |%s|\n", a, b, bmax
*
************************************/

static int
menu_name_eqs( char *a, char *b, char *bmax )
{
   while (*a)
     {
	if ( ( b == bmax )
	     || ( *a != *b ) )
	  return 0;
	a++;
	b++;
     }

   return( b == bmax );
}

/************************************
* menu_find_menu_bar
*
* debug print: "Name: |%s|, do error: %d\n", name, do_error
*
************************************/

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

/************************************
* find_subnode
*
* debug print: "Menu popup type: %x, Name: |%s|\n", m, name
*
************************************/

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

/************************************
* check_subnode_space
*
* debug print: "Menu: %x, Dn: %d\n", m, dn
*
************************************/

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

/************************************
* insert_node_to_popup
* This function assumes that there is enough space to insert one item *
*
* debug print: "Menu popup type: %x, menu node type: %x, where: %d\n", p, m, where
*
************************************/

static void
insert_node_to_popup( Menu_Popup_Type *p,
		      Menu_Node_Type *m,
		      unsigned int where )
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

   len = strlen( m->name );
   if ( len > p->min_width )
     p->min_width = len;
}

/************************************
* insert_popup_menu
*
* debug print: "Menu popup type: %x, Name: |%s|, where: %d\n", m, name, where
*
************************************/

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

/************************************
* append_popup_menu
*
* debug print: "Menu: %x, Name: |%s|\n", m, name
*
************************************/

static Menu_Popup_Type *append_popup_menu (Menu_Popup_Type *m, char *name)
{
   return insert_popup_menu (m, name, m->num_subnodes);
}

/************************************
* insert_separator
*
* debug print: "Menu: %x, where: %d\n", m, where
*
************************************/

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

/************************************
* append_separator
*
* debug print: "Menu: %x\n", m
*
************************************/

static Menu_Node_Type *append_separator (Menu_Popup_Type *m)
{
   return insert_separator (m, m->num_subnodes);
}

/************************************
* insert_slang_fun_item
*
* debug print: "Menu: %x, Name: |%s|, Nt: %x, Cd: %x, Where: %d\n", m, name, nt, cd, where
*
************************************/

static Menu_SLang_Fun_Type *
insert_slang_fun_item( Menu_Popup_Type *m, char *name,
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

/************************************
* append_slang_fun_item
*
* debug print: "Menu: %x, Name: |%s|, Nt: %x, Cd: %x\n", m, name, nt, cd
*
************************************/

static Menu_SLang_Fun_Type *append_slang_fun_item (Menu_Popup_Type *m, char *name,
						   SLang_Name_Type *nt, SLang_Any_Type *cd)
{
   return insert_slang_fun_item (m, name, nt, cd, m->num_subnodes);
}

/************************************
* insert_keystring_item
*
* debug print: "Menu: %x, Name: |%s|, k: |%s|, Where: %d\n", m, name, k, where
*
************************************/

static Menu_Keystring_Fun_Type *
insert_keystring_item( Menu_Popup_Type *m, char *name,
		       char *k, int where )
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

   /* Dbp( "l->keystring: |%s|\n", l->keystring ); */

   insert_node_to_popup (m, (Menu_Node_Type *)l, where);
   return l;
}

/************************************
* append_keystring_item
*
* debug print: "Menu popup type: %x, Name: |%s|, k: |%s|\n", m, name, k
*
************************************/

static Menu_Keystring_Fun_Type *
append_keystring_item (Menu_Popup_Type *m, char *name, char *k)
{
   return insert_keystring_item (m, name, k, m->num_subnodes);
}

/************************************
* insertSpecCmdItem
*********************
* Currently no use of specCmd. Maybe for future use
*
* debug print: "Menu: %x, Name: %s, SpecCmd: %s, KeySeq: %s, Where: %d", m, name, specCmd, keySeq, where
*
************************************/

static Menu_Keystring_Fun_Type *
insertSpecCmdItem( Menu_Popup_Type *m, char *name,
		   char *specCmd, char *keySeq, int where )
{
   Menu_Keystring_Fun_Type *l;
   char *buf;
   char *k;

   /* printf( "File: %s, Line: %d: insertSpecCmdItem\n", __FILE__, __LINE__ ); */

   if (NULL != (l = (Menu_Keystring_Fun_Type *)find_subnode (m, name)))
     {
      /* printf( "File: %s, Line: %d: insertSpecCmdItem\n", __FILE__, __LINE__ ); */
	if (l->type != MENU_NODE_TYPEDKEYS)
	  return NULL;
      /* printf( "File: %s, Line: %d: insertSpecCmdItem\n", __FILE__, __LINE__ ); */

	l->keystring = SLang_create_slstring( keySeq );

# if 0
	Dbp( "buf: |%s|\n", buf );

      /* printf( "File: %s, Line: %d: insertSpecCmdItem: Length: %d\n", __FILE__, __LINE__, *buf ); */

	if ( NULL == ( k = SLmalloc( sizeof( ( *buf ) + 1 ) ) ) )
	  return NULL;

	strncpy( k, buf, *buf );
	k[*buf] = '\0';

      /* if (NULL == (k = SLang_create_slstring (k))) */
      /*   Return NULL; */
      /* free_keystring_private_data (l); */
      /* */
# endif

	l->keystring = k;
	return l;
     }

   /* printf( "File: %s, Line: %d: insertSpecCmdItem\n", __FILE__, __LINE__ ); */

   if (-1 == check_subnode_space (m, 1))
     return NULL;
   /* printf( "File: %s, Line: %d: insertSpecCmdItem\n", __FILE__, __LINE__ ); */

   l = (Menu_Keystring_Fun_Type *)
     create_menu_node (name, MENU_NODE_TYPEDKEYS, sizeof (Menu_Keystring_Fun_Type));

   /* printf( "File: %s, Line: %d: insertSpecCmdItem\n", __FILE__, __LINE__ ); */

   if (l == NULL)
     return NULL;

   /* printf( "File: %s, Line: %d: insertSpecCmdItem\n", __FILE__, __LINE__ ); */

   buf = SLang_process_keystring( keySeq );
   /* printf( "File: %s, Line: %d: insertSpecCmdItem: Length: %d 1: |%x| 2: |%c|\n", __FILE__, __LINE__, *buf, *buf, buf[1] ); */

   /* if (NULL == (l->keystring = SLang_create_slstring (k))) */
   if ( NULL == ( k = SLmalloc( sizeof( ( *buf ) + 1 ) ) ) )
     {
	free_menu_node ((Menu_Node_Type *) l);
	return NULL;
     }

   strncpy( k, buf, *buf );
   k[(unsigned char)*buf] = '\0';

   /* printf( "File: %s, Line: %d: insertSpecCmdItem: Node: %x, String: %x, 1: |%x| 2: |%x| 3: |%x|\n", */
   /*	   __FILE__, __LINE__, l, k, *k, k[1], k[2] ); */

   l->keystring = k;

   insert_node_to_popup (m, (Menu_Node_Type *)l, where);
   return l;

   /* Return( NULL ); */
}

/************************************
* get_selected_menu_item
*
* debug print: "Menu: %x\n", p
*
************************************/

static Menu_Node_Type *get_selected_menu_item (Menu_Popup_Type *p)
{
   Menu_Node_Type *m;

   if (p == NULL)
     return NULL;

   /* Db; */

   if (p->num_subnodes <= p->active_node)
     {
        /* Db; */
	p->active_node = 0;
	return NULL;
     }

   /* Db; */
   m = p->subnodes[p->active_node];

   if (m->flags & MENU_ITEM_UNAVAILABLE)
     {
      /* Db; */
	p->active_node = 0;
	return NULL;
     }

   /* Make sure this is selected. */
   m->flags |= MENU_SELECTION;
   /* Db; */
   return m;
}

/************************************
* unselect_active_node
*
* debug print: "Menu: %x\n", b
*
************************************/

static int unselect_active_node (Menu_Popup_Type *b)
{
   Menu_Node_Type *m;

   if (NULL == (m = get_selected_menu_item (b)))
     return -1;

   m->flags &= ~(MENU_SELECTION|MENU_ACTIVE);
   return 0;
}

/************************************
* select_next_active_node
*
* debug print: "Menu: %x, active: %d, flags: %d\n", b, active_node, flags
*
************************************/

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

/************************************
* select_prev_active_node
*
* debug print: "Menu %x, Active Node: %d, flags %d\n", b, active_node, flags
*
************************************/

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

/************************************
* set_node_selection
*
* debug print: "Menu %x, Menu-node: %x\n", p, m
*
************************************/

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

/************************************
* get_active_node_flags
*
* debug print: "Menu: %x\n", p
*
************************************/

static unsigned int get_active_node_flags (Menu_Popup_Type *p)
{
   Menu_Node_Type *m;

   m = get_selected_menu_item (p);
   if (m == NULL)
     return 0;

   return m->flags;
}

/************************************
* menu_delete_node
*
* debug print: "Menu: %x, Menu node: %x\n", p, m
*
************************************/

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

/************************************
* menu_delete_nodes
*
* debug print: "Menu: %x\n", p
*
************************************/

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

/************************************
* copy_menu
*
* debug print: "Menu dest: %x, Menu node src: %x\n", dest, src
*
************************************/

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

/************************************
* gtkCopyMenu
*
* debug print: "Menu dest: %x: |%s|, Menu node src: %x: |%s|\n", dest, dest->name, src, src->name
*
************************************/

static int
gtkCopyMenu( Menu_Popup_Type *dest, Menu_Node_Type *src )
{
   int numItems;
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
        /* Db; */
	p = (Menu_Popup_Type *) src;
	l = p->subnodes;
	lmax = l + p->num_subnodes;

        numItems = dest->num_subnodes;

	p = append_popup_menu (dest, src->name);

	if (p == NULL)
	  return -1;

        jGtkInsertPopupMenuCmd( p, numItems );

	while (l < lmax)
	  {
	     /* if (-1 == copy_menu (p, *l)) */
	     if (-1 == gtkCopyMenu (p, *l))
	       {
		  menu_delete_node (dest, (Menu_Node_Type *)p);
		  return -1;
	       }
	     l++;
	  }
	m = (Menu_Node_Type *)p;
	break;

      case MENU_NODE_SEPARATOR:
        /* Db; */
	m = append_separator (dest);
        if ( m )
	  jGtkInsertSeparatorCmd( dest, m, dest->num_subnodes - 1 );
	break;

      case MENU_NODE_FUN_SLANG:
        /* Db; */
	sl = (Menu_SLang_Fun_Type *) src;
	/* Need a s-lang fun for this !!! */
	if ((-1 == SLang_push_anytype (sl->client_data))
	    || (-1 == SLang_pop_anytype (&any)))
	  return -1;

	m = (Menu_Node_Type *) append_slang_fun_item (dest, sl->name,
						      sl->slang_fun, any);
        /* jGtkMenuItemNew( dest, m, dest->num_subnodes - 1 ); */
	if (m == NULL)
	  SLang_free_anytype (any);
        else
          jGtkMenuItemNew( dest, m, dest->num_subnodes - 1 );
	break;

      case MENU_NODE_KEYSTRING:
        /* Db; */
	m = (Menu_Node_Type *) append_keystring_item (dest, src->name, ((Menu_Keystring_Fun_Type *)src)->keystring);
        if ( m )
	  jGtkMenuKeystringItemNew( dest, m, dest->num_subnodes - 1 );
	break;

      case MENU_NODE_TYPEDKEYS:
        /* Db; */
	SLang_verror (SL_INTRINSIC_ERROR, "Copy of Typed keys should not occur! Internal error.");
        return -1;

      case MENU_NODE_MENUBAR:
        /* Db; */
	SLang_verror (SL_INTRINSIC_ERROR, "Unable to copy a menu-bar");
	return -1;

      case MENU_NODE_FUN_C:
      default:
        /* Db; */
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

/************************************
* simulate_hline
*
* debug print: "Line: %d\n", n
*
************************************/

static void simulate_hline (unsigned int n)
{
   while (n--)
     SLsmg_write_string ("-");
}

/************************************
* simulate_box
*
* debug print: "r: %d, c: %d, dr: %d, dc: %d\n", r, c, dr, dc
*
************************************/

static void simulate_box (int r, int c, unsigned int dr, unsigned int dc)
{
   int rr, rmax;

   if ((dr < 1) || (dc < 2)) return;

   dr--;
   dc--;
   /* Dbp2( "--- r: %d c: %d ---------------------------\n", r, c ); */
   SLsmg_gotorc (r, c);
   SLsmg_write_string ("+");
   simulate_hline (dc-1);
   SLsmg_write_string ("+");
   /* Dbp2( "--- r: %d c: %d ---------------------------\n", r + dr, c ); */
   SLsmg_gotorc (r + dr, c);
   SLsmg_write_string ("+");
   simulate_hline (dc-1);
   SLsmg_write_string ("+");

   rmax = r + dr;
   for (rr = r + 1; rr < rmax; rr++)
     {
      /* Dbp2( "--- r: %d c: %d ---------------------------\n", rr, c ); */
	SLsmg_gotorc (rr, c);
	SLsmg_write_string ("|");
      /* Dbp2( "--- r: %d c: %d ---------------------------\n", rr, c + dc ); */
	SLsmg_gotorc (rr, c + dc);
	SLsmg_write_string ("|");
     }
}

/* I would prefer to use real up/down arrows but I cannot depend upon their
 * portability.
 */
/************************************
* draw_up_arrow
*
* debug print: "r: %d, c: %d\n", r, c
*
************************************/

static void draw_up_arrow (int r, int c)
{
   /* Dbp2( "--- r: %d c: %d ---------------------------\n", r, c ); */
   SLsmg_gotorc (r, c);
   SLsmg_write_string ("(-)");
}

/************************************
* draw_down_arrow
*
* debug print: "r: %d, c: %d\n", r, c
*
************************************/

static void draw_down_arrow (int r, int c)
{
   /* Dbp2( "--- r: %d c: %d ---------------------------\n", r, c ); */
   SLsmg_gotorc (r, c);
   SLsmg_write_string ("(+)");
}

/************************************
* draw_name
*
* debug print: "Name %s, color0: %d, color1: %d, field_width: %d\n", name, color0, color1, field_width
*
************************************/

static void draw_name (char *name, int color0, int color1, unsigned int field_width)
{
   char *s;
   unsigned int n;

   s = name;

   while (*s && (*s != '&'))
     s++;

   n = (unsigned int) (s - name);
   if (n)
     {
	SLsmg_set_color (color0);
	SLsmg_write_nchars (name, n);
     }

   if (*s != 0)
     {
	unsigned int dn;

	s++;
	SLsmg_set_color (color1);
	SLsmg_write_nchars (s, 1);
	n++;
	SLsmg_set_color (color0);
	if (*s != 0)
	  {
	     s++;
	     dn = strlen (s);
	     SLsmg_write_nchars (s, dn);
	     n += dn;
	  }
     }

   if (n < field_width)
     SLsmg_write_nstring ("", field_width - n);
}

/************************************
* draw_menu_bar
*
* debug print: "Menu bar: %x\n", b
*
************************************/

static int draw_menu_bar (Menu_Bar_Type *b)
{
   Menu_Node_Type **m;
   unsigned int i, imax;
   int active;

   /* Dbp2( "--- r: %d c: %d ---------------------------\n", 0, 0 ); */

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

/************************************
* draw_keystring
*
* debug print: "Menu keystring fun type: %x, color0: %d, color1: %d, field_widht: %d\n", k, color0, color1, field_width
*
************************************/

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
# ifdef IBMPC_SYSTEM
	if ((i == 0) || (i == 0xE0))
	  continue;
# endif

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
# ifndef IBMPC_SYSTEM
	     if (i == 0)
	       {
		  buf[0] = '^';
		  buf[1] = '@';
		  buf[2] = 0;
	       }
# endif
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

/************************************
* draw_popup_menu
*
* debug print: "Menu: %x, r: %d, c: %d\n", p, r, c
*
************************************/

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

        /* Dbp2( "--- r: %d c: %d ---------------------------\n", r, c ); */

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
	     /* Dbp2( "--- r: %d c: %d ---------------------------\n", r, c + ( dc - 2 ) ); */
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

   /* Dbp2( "--- r: %d c: %d ---------------------------\n", active_row, active_col ); */

   SLsmg_gotorc (active_row, active_col);
   return 0;
}

/************************************
* get_active_menubar
*
* debug print: "(void)\n"
*
************************************/

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

/************************************
* jed_redraw_menus
*
* debug print: "(void)\n"
*
************************************/

int jed_redraw_menus (void)
{
   Menu_Popup_Type *p;
   Menu_Bar_Type *b;
   int active;

   return 0;

# if 0
   if (NULL == (b = CBuf->menubar))
     b = Global_Menu_Bar;
# else
   b = get_active_menubar ();
# endif

   Active_Popup = (Menu_Popup_Type *) b;

   if (b == NULL)
     {
	/* Dbp2( "--- r: %d c: %d ---------------------------\n", 0, 0 ); */
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
        /* Dbp2( "--- r: %d c: %d ---------------------------\n", 0, b->item_columns[active] ); */
	SLsmg_gotorc (0, b->item_columns[active]);
	return 1;
     }

   draw_popup_menu (p, 0, b->item_columns[active]);
   return 1;
}

/* Keyboard Interface */

/************************************
* find_active_popup
*
* debug print: "(void)\n"
*
************************************/

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

/************************************
* next_menubar_cmd
*
* debug print: "(void)\n"
*
************************************/

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

/************************************
* prev_menubar_cmd
*
* debug print: "(void)\n"
*
************************************/

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

/************************************
* jed_exit_menu_bar
*
* debug print: "(void)\n"
*
************************************/

int
jed_exit_menu_bar (void)
{
   while (Active_Popup != NULL)
     {
	unselect_active_node (Active_Popup);
	Active_Popup->active_node = 0;
	Active_Popup->flags &= ~MENU_POPUP_PREPARED;
	Active_Popup = Active_Popup->parent;
     }

   Active_Menu_Bar = NULL;
   /* Db; */
   /* Dbp1( "################################################################\n", 1 ); */
   Jed_Menus_Active = 0;
   return 1;
}

/************************************
* down_menu_cmd
*
* debug print: "(void)\n"
*
************************************/

static int down_menu_cmd (void)
{
   if (Active_Popup == NULL)
     return -1;

   /* unselect_active_node (Active_Popup); */
   select_next_active_node (Active_Popup, Active_Popup->active_node, 0);
   return 1;
}

/************************************
* up_menu_cmd
*
* debug print: "(void)\n"
*
************************************/

static int up_menu_cmd (void)
{
   if (Active_Popup == NULL)
     return -1;

   /* unselect_active_node (Active_Popup); */
   select_prev_active_node (Active_Popup, Active_Popup->active_node, 0);
   return 1;
}

/************************************
* pgup_menu_cmd
*
* debug print: "(void)\n"
*
************************************/

static int pgup_menu_cmd (void)
{
   if (Active_Popup == NULL)
     return -1;

   select_next_active_node (Active_Popup, Active_Popup->num_subnodes, 0);
   return 1;
}

/************************************
* pgdn_menu_cmd
*
* debug print: "(void)\n"
*
************************************/

static int pgdn_menu_cmd (void)
{
   if (Active_Popup == NULL)
     return -1;

   select_prev_active_node (Active_Popup, 0, 0);
   return 1;
}

/************************************
* execute_keystring
*
* debug print: "Key string: %s\n", s
*
************************************/

static int execute_keystring (char *s)
{
   int (*f)(void);

   /* printf( "File: %s, Line: %d: exec keystring keymap: %x\n", __FILE__, __LINE__, CBuf->keymap ); */

   if (NULL != (f = (int (*)(void)) (SLang_find_key_function(s, CBuf->keymap))))
     {
      /* printf( "File: %s, Line: %d: exec keystring function: %x\n", __FILE__, __LINE__, f ); */
	return (*f) ();
     }

   /* Dbp( "Function: |%s|\n", s ); */

   if ( ( *s == '.' ) ||
        !SLang_execute_function( s ) )
     SLang_load_string(s);

   /* printf( "File: %s, Line: %d exec keystring load string\n", __FILE__, __LINE__ ); */

   return 1;
}

/************************************
* get_full_popup_name
*
* debug print: "Menu: %x\n", p
*
************************************/

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

/************************************
* run_popup_callback
*
* debug print: "Menu: %x, Name: %x\n", p, cb
*
************************************/

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

/************************************
* prepare_popup
*
* debug print: "Menu: %x\n", p
*
************************************/

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

/************************************
* jGtkPreparePopup
*
* debug print: "p: %x, cb: %x\n", p, cb
*
************************************/

static int
jGtkPreparePopup( Menu_Popup_Type *p, SLang_Name_Type *cb )
{
   /* SLang_Name_Type *cb; */

   if ( cb )
     {
	free_menu_popup_subnodes( p );
	if ( -1 == run_popup_callback( p, cb ) )
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

/************************************
* execMenuTbCmd
*
* debug print: "Menu: %x\n", m
*
************************************/

static int
execMenuTbCmd( Menu_Node_Type *m )
{
   Menu_Popup_Type *p;
   Menu_Popup_Type *mp = ( Menu_Popup_Type * ) m;
   Menu_C_Fun_Type *c;
   SLang_Name_Type *nt;
   char *str;

   if ( activeTopSubmenu )
     {
	if ( m->type != MENU_NODE_POPUP )
	  {
	 /* Dbp1( "Deselecting: %x\n", activeTopSubmenu->parent->subMenu ); */
	     gtk_menu_shell_deselect( ( GtkMenuShell * ) activeTopSubmenu->parent->subMenu );
	     activeTopSubmenu = NULL;
	  }
     }

   /* printf( "File: %s, Line: %d\n", __FILE__, __LINE__ ); */

   if (m == NULL)
     return -1;

   switch (m->type)
     {
      case MENU_NODE_POPUP:
# if 0
	m->flags |= MENU_ACTIVE;
	p = (Menu_Popup_Type *) m;
# else
	/* Do it this way to avoid DEC C ansi-aliasing warning. */
	p = (Menu_Popup_Type *) m;
	/* p->flags |= MENU_ACTIVE; */
# endif
	if (-1 == prepare_popup (p))
	  return -1;
	Active_Popup = p;
	/* select_next_active_node (p, p->num_subnodes, 0); */

        /* Dbp1( "Activate: %x\n", m ); */

        /****/
        gtk_menu_shell_select_item( ( GtkMenuShell * ) mp->parent->subMenu,
				    p->menuItem );

        if ( !activeTopSubmenu ) activeTopSubmenu = mp;
        /*****/

        /*******
        gtk_menu_shell_select_first( ( GtkMenuShell * ) mp->parent->subMenu,
				    1 );
        *******/
# if 0
        gtk_menu_popup( ( GtkMenu * ) mp->subMenu, NULL, NULL,
			/* mp->parent->subMenu, */
			/* mp->parent->menuItem, */
			NULL, NULL, 0,
			gtk_get_current_event_time() );
# endif
        /*************
        gtk_menu_shell_activate_item( ( GtkMenuShell * ) mp->parent->subMenu,
				      p->menuItem,
				      1 );
        *********/

	return 1;

      case MENU_NODE_FUN_SLANG:
        /* Dbp( "MENU_NODE_FUN_SLANG: %d\n", 1 ); */
	nt = ((Menu_SLang_Fun_Type *) m)->slang_fun;
	/* jed_exit_menu_bar (); */
	if (nt == NULL)
	  return -1;
	if ((-1 == SLang_start_arg_list ())
	    || (-1 == SLang_push_anytype (((Menu_SLang_Fun_Type*)m)->client_data))
	    || (-1 == SLang_end_arg_list ()))
	  return -1;

        /* Dbp( "SLexecute_functions: |%x|\n", nt ); */

	return SLexecute_function (nt);

      case MENU_NODE_KEYSTRING:
        /* printf( "File: %s, Line: %d\n", __FILE__, __LINE__ ); */
	str = ((Menu_Keystring_Fun_Type *) m)->keystring;
	/* printf( "File: %s, Line: %d: String to execute: %s\n", __FILE__, __LINE__, str ); */
	/* Grab a copy while it is still available */
	if (NULL == SLang_create_slstring (str))
	  return -1;
	/* jed_exit_menu_bar (); */

        /* Dbp( "execute_keystring: |%s|\n", str ); */
	(void) execute_keystring (str);
	SLang_free_slstring (str);
	break;

      case MENU_NODE_FUN_C:
        /* Dbp( "MENU_NODE_FUN_C: %d\n", 1 ); */
	c = (Menu_C_Fun_Type *) m;
	/* jed_exit_menu_bar (); */
	if (c->c_fun != NULL)
	  return -1;
	return c->c_fun ();
      case MENU_NODE_TYPEDKEYS:
        /* Dbp2( "m: %x keystring: %x\n", m, (( Menu_Keystring_Fun_Type * ) m)->keystring ); */
	str = (( Menu_Keystring_Fun_Type * ) m)->keystring;
	return( jgtk_createKeyEvents( str ) );
     }
   return 0;
}

static int
execMenuTbCmdNode( Menu_Node_Type *m )
{
   /* Menu_Popup_Type *p; */
   Menu_C_Fun_Type *c;
   SLang_Name_Type *nt;
   char *str;

   /* Dbp1( "m->name: |%s|\n", m->name ); */

   /* printf( "File: %s, Line: %d\n", __FILE__, __LINE__ ); */

   if (m == NULL)
     return -1;

   switch (m->type)
     {
      case MENU_NODE_POPUP:
        /* Dbp1( "Popup node type should not occur here: %x\n", m ); */
        return( -1 );
# if 0
#  if 0
	m->flags |= MENU_ACTIVE;
	p = (Menu_Popup_Type *) m;
#  else
	/* Do it this way to avoid DEC C ansi-aliasing warning. */
	p = (Menu_Popup_Type *) m;
	p->flags |= MENU_ACTIVE;
#  endif
	if (-1 == prepare_popup (p))
	  return -1;
	Active_Popup = p;
	select_next_active_node (p, p->num_subnodes, 0);
	return 1;
# endif

      case MENU_NODE_FUN_SLANG:
        /* Dbp( "MENU_NODE_FUN_SLANG: %d\n", 1 ); */
	nt = ((Menu_SLang_Fun_Type *) m)->slang_fun;
	/* jed_exit_menu_bar (); */
	if (nt == NULL)
	  return -1;
	if ((-1 == SLang_start_arg_list ())
	    || (-1 == SLang_push_anytype (((Menu_SLang_Fun_Type*)m)->client_data))
	    || (-1 == SLang_end_arg_list ()))
	  return -1;

        /* Dbp( "SLexecute_functions: |%x|\n", nt ); */

	return SLexecute_function (nt);

      case MENU_NODE_KEYSTRING:
        /* printf( "File: %s, Line: %d\n", __FILE__, __LINE__ ); */
	str = ((Menu_Keystring_Fun_Type *) m)->keystring;
	/* printf( "File: %s, Line: %d: String to execute: %s\n", __FILE__, __LINE__, str ); */
	/* Grab a copy while it is still available */
	if (NULL == SLang_create_slstring (str))
	  return -1;
	/* jed_exit_menu_bar (); */

        /* Dbp( "execute_keystring: |%s|\n", str ); */
	(void) execute_keystring (str);
	SLang_free_slstring (str);
	break;

      case MENU_NODE_FUN_C:
        /* Dbp( "MENU_NODE_FUN_C: %d\n", 1 ); */
	c = (Menu_C_Fun_Type *) m;
	/* jed_exit_menu_bar (); */
	if (c->c_fun != NULL)
	  return -1;
	return c->c_fun ();
      case MENU_NODE_TYPEDKEYS:
        /* Dbp2( "m: %x keystring: %x\n", m, (( Menu_Keystring_Fun_Type * ) m)->keystring ); */
	str = (( Menu_Keystring_Fun_Type * ) m)->keystring;
	return( jgtk_createKeyEvents( str ) );
     }
   return 0;
}

/************************************
* select_menu_cmd
*
* debug print: "(void)\n"
*
************************************/

static int
select_menu_cmd (void)
{
   return( execMenuTbCmd( get_selected_menu_item( Active_Popup ) ) );
}

/************************************
* back_menu_cmd
*
* debug print: "(void)\n"
*
************************************/

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

/************************************
* find_subnode_via_char
*
* debug print: "Menu: %x, ch: %c\n", p, ch
*
************************************/

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

/************************************
* select_via_char_cmd
*
* debug print: "(void)\n"
*
************************************/

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

# ifndef IBMPC_SYSTEM
#  ifdef HAS_MOUSE
static int xterm_mouse_cmd (void);
#  endif
# endif

/************************************
* init_menu_keymap
*
* debug print: "(void)\n"
*
************************************/

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

# ifndef IBMPC_SYSTEM
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
#  ifdef __unix__
   SLkm_define_key ("^(ku)", (FVOID_STAR) up_menu_cmd, Menu_Keymap);
   SLkm_define_key ("^(kd)", (FVOID_STAR) down_menu_cmd, Menu_Keymap);
   SLkm_define_key ("^(kr)", (FVOID_STAR) next_menubar_cmd, Menu_Keymap);
   SLkm_define_key ("^(kl)", (FVOID_STAR) prev_menubar_cmd, Menu_Keymap);
   SLkm_define_key ("^(kP)", (FVOID_STAR) pgup_menu_cmd, Menu_Keymap);
   SLkm_define_key ("^(kN)", (FVOID_STAR) pgdn_menu_cmd, Menu_Keymap);
#  endif
#  ifdef HAS_MOUSE
   SLkm_define_key ("\033[M", (FVOID_STAR) xterm_mouse_cmd, Menu_Keymap);
#  endif
# else				       /* IBMPC_SYSTEM */
#  include "doskeys.h"

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
# endif

# ifdef HAS_MOUSE
   SLkm_define_key ("\033^@", (FVOID_STAR) jed_mouse_cmd, Menu_Keymap);
# endif

   /* Toolbar callback key */

/*   SLkm_define_key( TB_CMD_KEY_SEQ, (FVOID_STAR) jed_select_menu_bar, Global_Map ); */
   SLkm_define_key( TB_SEL_KEY_SEQ, (FVOID_STAR) jGtkExecTB, Global_Map );
   /* SLkm_define_key( TB_EXE_KEY_SEQ, (FVOID_STAR) jGtkExecTB, Menu_Keymap ); */

   return 0;
}

/* key_interpret */
static int
jGtkExecTB()
{

   activeTopSubmenu = NULL;

   if ( actTBCmdNode )
     {
     /* execTBCmd( actTBCmd->path ); */
	execMenuTbCmdNode( actTBCmdNode );
	actTBCmdNode = NULL;
      /* jed_redraw_screen( 1 ); */
      /* update((Line *) NULL, 1, 0, 1); // File: screen.c */
	SLang_restart( 1 );
	touch_screen();
     }

   /* Dbp1( "actTBCmdNode: %x\n", actTBCmdNode ); */

   return( 0 );
}

static int
jGtkFeedTBKeySeq(void)
{

   /* Dbp2( "Key: %d: |%c|\n", tbExeKeySeq[tbExeKeyInx], tbExeKeySeq[tbExeKeyInx] ); */

   return( tbExeKeySeq[tbExeKeyInx++] );
}

/************************************
* jed_menu_do_key
*
* debug print: "(void)\n"
*
************************************/

SLang_Key_Type *
jed_menu_do_key (void)
{
   int ch;

   if ( tbInputSelected )
     {
      /* Dbp1( "ActTBCmdNode: |%x|\n", actTBCmdNode ); */
      /* tbExecSlangFlag = 1; */
	tbInputSelected = 0;
      /* tbActive = 0; */
      /* Db; */
      /* Dbp1( "################################################################\n", 1 ); */
	Jed_Menus_Active = 0;

	tbExeKeyInx = 0;

      /* Return SLang_do_key( Menu_Keymap, jed_getkey ); */
	return SLang_do_key( Menu_Keymap, jGtkFeedTBKeySeq );
     }
   else
     {
	if ( Executing_Keyboard_Macro ||
	     ( Read_This_Character != NULL ) )
	  return SLang_do_key( Menu_Keymap, jed_getkey );

	ch = my_getkey();

	if ( SLKeyBoard_Quit || SLang_get_error () )
	  {
	     jed_exit_menu_bar();
	     return NULL;
	  }

	if ( ch == 27 )		       /* ESC */
	  {
	     int tsecs = 2;
	     if ( 0 == input_pending( &tsecs ) )
	       ch = 127;
	  }
	ungetkey (&ch);

	return SLang_do_key( Menu_Keymap, jed_getkey );
     }
}

/************************************
* exec_menubar_callback
*
* debug print: "Menu bar: %x, Name types: %x\n", b, ntp
*
************************************/

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

/************************************
* jed_notify_menu_buffer_changed
*
* debug print: "(void)\n"
*
************************************/

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

static int
jGtkSelectTB(void)
{
   /* if ( IN_MINI_WINDOW ) Return( -1 ); */
   tbInputSelected = 1;
   Jed_Menus_Active = 1;
   return 0;
}

/************************************
* jed_select_menu_bar
*
* debug print: "(void)\n"
*
************************************/

int
jed_select_menu_bar (void)
{
   Menu_Popup_Type *p;
   Menu_Node_Type **l, **lmax;

   /* Db; */

   if (IN_MINI_WINDOW)
     {
	/* For now do not allow access to menus from the minibuffer
	 * since the minibuffer is not recursive
	 */
	return -1;
     }

# if 0
   if ( tbActive )
     {
	Dbp( "Toolbar activated %d\n", 1 );
	tbInputSelected = 1;
     }
   else
# endif
     {
      /* Dbp( "Menus activate %d\n", 1 ); */
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

	if ( -1 == exec_menubar_callback( Active_Menu_Bar, &Active_Menu_Bar->select_callback ) )
	  {
	     jed_exit_menu_bar ();
	     return -1;
	  }

      /* Db; */
      /* Dbp1( "######################################################\n", 1 ); */
      /* ( void ) get_selected_menu_item (p); */
      /* Dbp1( "######################################################\n", 1 ); */
     }

   touch_screen ();
   /* Db; */
   /* Dbp1( "################################################################\n", 1 ); */
   /* Jed_Menus_Active = 1; */
   return 0;
}

/* Interpreter interface */

/************************************
* find_menu_node1
*
* debug print: "Menu: %x, Name: %s, stop at last popup: %d\n", m, name, stop_at_last_popup
*
************************************/

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

/************************************
* find_menu_node
*
* debug print: "Name: %s, stop at last popup: %d\n", name, stop_at_last_popup
*
************************************/

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

/************************************
* find_menu_popup
*
* debug print: "Name: %s\n", name
*
************************************/

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
/************************************
* find_where_to_insert
*
* debug print: "Menu: %x, Name: %s\n", p, name
*
************************************/

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

/************************************
* create_menu_bar_cmd
*
* debug print: "Name: %s\n", name
*
************************************/

static void
create_menu_bar_cmd( char *name )
{
   /* gtkCreateMenuBarCmd( name ); */

   if ( 0 == strcmp( "Global", name ) )
     {
	Menu_Bar_Type *g = create_menu_bar( name, 5 );
	if ( g != NULL )
	  {
	     jed_delete_menu_bar( Global_Menu_Bar );
	     jGtkCreateMenuBarCmd( g );
      /* Global_Menu_Bar = g; */
	     if ( !Active_Menu_Bar )
	       {
		  gtkSetBufferMenuBarCmd( name );
	       }
	  }
	Global_Menu_Bar = g;
	return;
     }

   if ( NULL != menu_find_menu_bar( name, 0 ) )
     return;

   jGtkCreateMenuBarCmd( create_menu_bar( name, 5 ) );
}

/************************************
* set_buffer_menu_bar_cmd
*
* debug print: "Name: %s\n", name
*
************************************/

static void
set_buffer_menu_bar_cmd (char *name)
{
   Menu_Bar_Type *b;

   gtkSetBufferMenuBarCmd( name );
   b = menu_find_menu_bar (name, 1);
   if (b == NULL)
     return;

   if ( b != CBuf->menubar )
     {
	jed_delete_menu_bar (CBuf->menubar);
	CBuf->menubar = b;
	b->num_refs += 1;
     }
}

/************************************
* pop_where_to_insert
*
* debug print: "Menu: %x, where: %d\n", p, where
*
************************************/

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

/************************************
* insert_popup_menu_cmd
*
* debug print: "Dest: |%s|, Menu: |%s|\n", dest, menu
*
************************************/

static void
insert_popup_menu_cmd (char *dest, char *menu)
{
   Menu_Popup_Type *m;
   unsigned int where;

   m = find_menu_popup (dest);

   if (m == NULL)
     return;

   if (-1 == pop_where_to_insert (m, &where))
     return;

   jGtkInsertPopupMenuCmd( insert_popup_menu( m, menu, where ), where );
}

/************************************
* append_popup_menu_cmd
*
* debug print: "Dest %s, Menu: %s\n", dest, menu
*
************************************/

static void
append_popup_menu_cmd (char *dest, char *menu)
{
   Menu_Popup_Type *m;

   /* gtkAppendPopupMenuCmd( dest, menu ); */
   m = find_menu_popup (dest);

   if (m == NULL)
     return;

   jGtkInsertPopupMenuCmd( insert_popup_menu (m, menu, m->num_subnodes), m->num_subnodes );
}

/************************************
* insert_separator_cmd
*
* debug print: "Name: %s\n", name
*
************************************/

static void
insert_separator_cmd (char *name)
{
   Menu_Popup_Type *m;
   unsigned int where;

   /* gtkInsertSeparatorCmd( name ); */
   m = find_menu_popup (name);

   if (m == NULL)
     return;

   if (-1 == pop_where_to_insert (m, &where))
     return;

   jGtkInsertSeparatorCmd( m, insert_separator( m, where ), where );
}

/************************************
* append_separator_cmd
*
* debug print: "Name: %s\n", name
*
************************************/

static void
append_separator_cmd (char *name)
{
   Menu_Popup_Type *m;

   /* gtkAppendSeparatorCmd( name ); */
   m = find_menu_popup (name);

   if (m != NULL)
     jGtkInsertSeparatorCmd( m, insert_separator( m, m->num_subnodes ), m->num_subnodes - 1 );
}

/************************************
* insert_menu_item_cmd_internal
*
* debug print: "Is fun: %d, do append: %d\n", is_fun, do_append
*
************************************/

static void
insert_menu_item_cmd_internal( int is_fun, int do_append )
{
   Menu_Popup_Type *m;
   Menu_Node_Type *newItem;
   SLang_Name_Type *nt = NULL;
   SLang_Any_Type *client_data = NULL;
   char *str = NULL;
   char *menu = NULL;
   char *name = NULL;
   unsigned int where;

   /* printf( "File: %s, Line: %d\n", __FILE__, __LINE__ ); */

   if ( is_fun )
     {
	if ( -1 == SLang_pop_anytype( &client_data ) )
	  return;

	if ( NULL == ( nt = SLang_pop_function() ) )
	  goto free_and_return;
     }
   else if ( -1 == SLang_pop_slstring( &str ) )
     return;

   if ( -1 == SLang_pop_slstring( &name ) )
     goto free_and_return;

   if ( -1 == SLang_pop_slstring( &menu ) )
     goto free_and_return;

   if ( NULL == ( m = find_menu_popup( menu ) ) )
     goto free_and_return;

   if ( do_append )
     where = m->num_subnodes;
   else if ( -1 == pop_where_to_insert( m, &where ) )
     goto free_and_return;

   /* Creates crash because second argument sometimes NULL!!! */
   /* Dbp( "Menu: |%s|, Name: |%s|\n", ( menu, name ) ); */

   /* gtkMenuItemNew( menu, name, do_append ); */

   if ( nt != NULL )
     {
	newItem = ( Menu_Node_Type * ) insert_slang_fun_item( m, name, nt, client_data, where );
	if ( NULL != newItem )
	  {
	     jGtkMenuItemNew( m, newItem, where );
	     client_data = NULL;
	     nt = NULL;
	  }
     }
   else
     jGtkMenuKeystringItemNew( m, ( Menu_Node_Type * ) insert_keystring_item( m, name, str, where ), where );
   /* drop */

free_and_return:

   if ( client_data != NULL )
     SLang_free_anytype( client_data );
   SLang_free_slstring( str );
   SLang_free_slstring( menu );
   SLang_free_slstring( name );
   SLang_free_function( nt );
}

/************************************
* insert_menu_item_cmd
*
* debug print: "(void)\n"
*
************************************/

static void
insert_menu_item_cmd (void)
{
   /* printf( "File: %s, Line: %d\n", __FILE__, __LINE__ ); */
   insert_menu_item_cmd_internal( ( SLang_Num_Function_Args == 5 ), 0 );
}

/************************************
* append_menu_item_cmd
*
* debug print: "(void)\n"
*
************************************/

static void
append_menu_item_cmd (void)
{
   /* printf( "File: %s, Line: %d\n", __FILE__, __LINE__ ); */
   insert_menu_item_cmd_internal( ( SLang_Num_Function_Args == 4 ), 1 );
}

/************************************
* menu_delete_item_cmd
*
* debug print: "Menu: %s\n", menu
*
************************************/

static void
menu_delete_item_cmd( char *menu )
{
   Menu_Popup_Type *parent;
   Menu_Node_Type *child;

   /* gtkMenuDeleteItemCmd( menu ); */
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

/************************************
* menu_delete_items_cmd
*
* debug print: "Menu: %s\n", menu
*
************************************/

static void
menu_delete_items_cmd (char *menu)
{
   Menu_Popup_Type *p;

   /* gtkMenuDeleteItemsCmd( menu ); */

   if (NULL == (p = find_menu_popup (menu)))
     return;

   menu_delete_nodes (p);
}

static int
getChildPos( Menu_Popup_Type *parent, Menu_Node_Type *child )
{
   unsigned int i;

   for ( i = 0; i < parent->num_subnodes; ++i )
     {
	if ( parent->subnodes[i] == child ) return( (int) i );
     }

   return( -1 );
}

static void
setObjectAvailable( Menu_Node_Type *parent, Menu_Node_Type *item )
{
   int n;

   if ( parent == item ) return;
         /* someone trying to make the menubar unavailable */

   if ( !item->menuItem ) return;

   /* if ( !parent->subMenu ) Return; */

   if ( !gtk_widget_get_parent( item->menuItem ) )
     {
	n = getChildPos( ( Menu_Popup_Type * ) parent, item );
	if ( n >= 0 )
	  {
	     gtk_menu_shell_insert( GTK_MENU_SHELL( parent->subMenu ), item->menuItem, n );
	     g_object_unref( item->menuItem );
	  }
     }
}

static void
setObjectUnavailable( Menu_Node_Type *parent, Menu_Node_Type *item )
{
   GtkWidget *pW;

   if ( parent == item ) return;

   if ( !item->menuItem ) return;

   pW = gtk_widget_get_parent( item->menuItem );
   if ( !pW ) return;

   g_object_ref( item->menuItem );

   gtk_container_remove( GTK_CONTAINER( pW ), item->menuItem );
}

/************************************
* set_object_available_cmd
*
* debug print: "Name: %s, Flag: %d\n", name, flag
*
************************************/

static void set_object_available_cmd (char *name, int *flag)
{
   Menu_Node_Type *m;

   gtkSetObjectAvailableCmd( name );

   if (NULL == (m = find_menu_node (name, 0)))
     return;

   if ( *flag )
     {
	m->flags &= ~MENU_ITEM_UNAVAILABLE;
	setObjectAvailable( find_menu_node( name, 1 ), m );
     }
   else
     {
	m->flags |= MENU_ITEM_UNAVAILABLE;
	setObjectUnavailable( find_menu_node( name, 1 ), m );
     }

}

/************************************
* pop_popup_callback
*
* debug print: "Menus: %x, type: %d, Names: %x\n", pp, type, ntp
*
************************************/

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

/************************************
* pop_menubar_callback
*
* debug print: "Menu bar: %x, Name type: %x\n", bp, nt
*
************************************/

static int pop_menubar_callback (Menu_Bar_Type **bp, SLang_Name_Type **nt)
{
   return pop_popup_callback ((Menu_Popup_Type **)bp, MENU_NODE_MENUBAR, nt);
}

/************************************
* set_select_menubar_callback
*
* debug print: "(void)\n"
*
************************************/

static void
set_select_menubar_callback (void)
{
   Menu_Bar_Type *b;
   SLang_Name_Type *nt;

   if (-1 == pop_menubar_callback (&b, &nt))
     return;

   b->select_callback = nt;
}

/************************************
* set_init_menubar_callback
*
* debug print: "(void)\n"
*
************************************/

static void set_init_menubar_callback (void)
{
   Menu_Bar_Type *b;
   SLang_Name_Type *nt;

   if (-1 == pop_menubar_callback (&b, &nt))
     return;

   b->init_callback = nt;
}

/************************************
* set_select_popup_callback
*
* debug print: "(void)\n"
*
************************************/

static void set_select_popup_callback (void)
{
   Menu_Popup_Type *p;
   SLang_Name_Type *nt;

   if (-1 == pop_popup_callback (&p, MENU_NODE_POPUP, &nt))
     return;

   p->select_popup_callback = nt;
}

/************************************
* set_tweak_popup_callback
*
* debug print: "(void)\n"
*
************************************/

static void set_tweak_popup_callback (void)
{
   Menu_Popup_Type *p;
   SLang_Name_Type *nt;

   if (-1 == pop_popup_callback (&p, MENU_NODE_POPUP, &nt))
     return;

   p->tweak_popup_callback = nt;
}

/************************************
* copy_menu_cmd
*
* debug print: "Dest: %s, Src: %s\n", destname, srcname
*
************************************/

static void
copy_menu_cmd (char *destname, char *srcname)
{
   Menu_Popup_Type *dest;
   Menu_Node_Type *src;

   dest = find_menu_popup (destname);
   if (dest == NULL)
     return;

   src = find_menu_node (srcname, 0);
   if (src == NULL)
     return;

   /* (void) copy_menu (dest, src); */
   gtkCopyMenu( dest, src );
}

/************************************
* set_menu_bar_prefix_string
*
* debug print: "Menubar: |%s|, s: |%s|\n", menubar, s
*
************************************/

static void
set_menu_bar_prefix_string (char *menubar, char *s)
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

/************************************
* execTBCmd
*
* debug print: "Menu: %s\n", menu
*
************************************/

void
execTBCmd( char *menu )
{
   char *menuMax;
   char *popup;
   Menu_Popup_Type *activePopup;
   char *savMenu = SLmalloc( strlen( menu ) + 1 );
   Menu_Bar_Type *savBar = NULL;
   Menu_Bar_Type *b = Menu_Bar_Root;

   if (savMenu == NULL)
     return;

   strcpy( savMenu, menu );

   /* Dbp1( "menu: %s\n", menu ); */

   menuMax = strchr( menu, '.' );
   if ( menuMax == NULL )
     menuMax = menu + strlen( menu );

   while ( b != NULL )
     {
      /* Dbp1( "menuBarName: %s\n", b->name ); */
	if ( menu_name_eqs( b->name, menu, menuMax ) )
	  {
	     savBar = b;
	     b = NULL;
	  }
	else
	  b = b->next;
     }

   /* (void) jed_exit_menu_bar (); */
   /* if (-1 == jed_select_menu_bar ()) */
   /*   Return; */

   if ( !savBar )
     {
      /* printf( "Cannot find menuBar: %s\n", menu ); */
	return;
     }

   activePopup = ( Menu_Popup_Type * ) savBar;

   if (NULL == (menu = SLmake_string (menu)))
     return;

   /* for now ignore the menubar name and use default */
   popup = strchr (menu, '.');
   if (popup != NULL)
     popup++;

   while ((popup != NULL) && (activePopup != NULL))
     {
	Menu_Node_Type *m;
	char *next_popup;

	/* Dbp1( "next_popup: %s\n", next_popup ); */

	next_popup = strchr (popup, '.');

	/* Dbp1( "next_popup: %s\n", next_popup ); */

	if (next_popup != NULL)
	  *next_popup++ = 0;

	/* Dbp1( "next_popup: %s\n", next_popup ); */
	/* Dbp1( "popup:      %s\n", popup ); */
        /* Dbp1( "Active_Popup-name: %s\n", activePopup->name ); */

	if ( NULL == ( m = find_subnode( activePopup, popup ) ) )
	  {
	     SLang_verror( SL_INVALID_PARM,
			   "Unable to find a popup menu called %s", menu );
	     break;
	  }
	/* set_node_selection( activePopup, m ); */
	if ( -1 == execMenuTbCmd( m ) )
	  break;

	popup = next_popup;
     }

   SLfree( menu );
}

static Menu_Node_Type *
getMenuNodeByPath( char *menu )
{
   char *menuMax;
   char *popup;
   Menu_Popup_Type *activePopup;
   Menu_Popup_Type *p;
   /* char *savMenu = SLmalloc( strlen( menu ) + 1 ); */
   Menu_Bar_Type *savBar = NULL;
   Menu_Bar_Type *b = Menu_Bar_Root;

   /* strcpy( savMenu, menu ); */

   /* Dbp1( "menu: %s\n", menu ); */

   menuMax = strchr( menu, '.' );
   if ( menuMax == NULL )
     menuMax = menu + strlen( menu );

   while ( b != NULL )
     {
      /* Dbp1( "menuBarName: %s\n", b->name ); */
	if ( menu_name_eqs( b->name, menu, menuMax ) )
	  {
	     savBar = b;
	     b = NULL;
	  }
	else
	  b = b->next;
     }

   /* (void) jed_exit_menu_bar (); */
   /* if (-1 == jed_select_menu_bar ()) */
   /*   Return; */

   if ( !savBar )
     {
      /* printf( "Cannot find menuBar: %s\n", menu ); */
      /* SLfree( savMenu ); */
	return( NULL );
     }

   activePopup = ( Menu_Popup_Type * ) savBar;

   if (NULL == (menu = SLmake_string (menu)))
     return( NULL );

   /* for now ignore the menubar name and use default */
   popup = strchr (menu, '.');
   if (popup != NULL)
     popup++;

   while ((popup != NULL) && (activePopup != NULL))
     {
	Menu_Node_Type *m;
	char *next_popup;

	/* Dbp1( "next_popup: %s\n", next_popup ); */

	next_popup = strchr (popup, '.');

	/* Dbp1( "next_popup: %s\n", next_popup ); */

	if (next_popup != NULL)
	  *next_popup++ = 0;

	/* Dbp1( "next_popup: %s\n", next_popup ); */
	/* Dbp1( "popup:      %s\n", popup ); */
        /* Dbp1( "Active_Popup-name: %s\n", activePopup->name ); */

	if ( NULL == ( m = find_subnode( activePopup, popup ) ) )
	  {
	     SLang_verror( SL_INVALID_PARM,
			   "Unable to find a popup menu called %s", menu );
	     break;
	  }
	/* set_node_selection( activePopup, m ); */
	/* if ( -1 == execMenuTbCmd( m ) ) */
	/*   break; */

        if ( m->type == MENU_NODE_POPUP )
	  {
	     p = ( Menu_Popup_Type * ) m;
	     if ( -1 == prepare_popup( p ) )
	       {
		  break;
	       }
	     Active_Popup = p;
	     select_next_active_node( p, p->num_subnodes, 0 );
	  }
        else
	  {
	     return( m );
	  }

	popup = next_popup;
     }

   SLfree( menu );
   /* SLfree( savMenu ); */
   return( NULL );
}

/************************************
* popup_menu_cmd
*
* debug print: "Menu: |%s|\n", menu
*
************************************/

static void popup_menu_cmd (char *menu)
{
   char *popup;

   /* printf( "File: %s, Line: %d: menu: %s\n", __FILE__, __LINE__, menu ); */

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

      /* printf( "File: %s, Line: %d: next_popup: %s\n", __FILE__, __LINE__, next_popup ); */

	next_popup = strchr (popup, '.');

      /* printf( "File: %s, Line: %d: next_popup: %s\n", __FILE__, __LINE__, next_popup ); */

	if (next_popup != NULL)
	  *next_popup++ = 0;

      /* printf( "File: %s, Line: %d: next_popup: %s\n", __FILE__, __LINE__, next_popup ); */
      /* printf( "File: %s, Line: %d: popup:      %s\n", __FILE__, __LINE__, popup ); */
      /* printf( "File: %s, Line: %d: Active_Popup-name: %s\n", __FILE__, __LINE__, Active_Popup->name ); */

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

/************************************
* insertToolbarItemCmdInternal
*
* debug print: "Is fun, %d, do append: %d, isStopck: %d\n", is_fun, do_append, isStock
*
************************************/

static void
insertToolbarItemCmdInternal( int is_fun, int do_append, int isStock )
{
   Menu_Popup_Type *m;
   Menu_Node_Type *nn; /* new node */
   SLang_Name_Type *nt = NULL;
   SLang_Any_Type *client_data = NULL;
   char *keySeq = NULL;
   char *specCmd = NULL;
   char *str = NULL;
   char *menu = NULL;
   char *name = NULL;
   char *iconName = NULL;
   unsigned int where;

   /* printf( "File: %s, Line: %d\n", __FILE__, __LINE__ ); */

   if ( is_fun == 2 )
     {
	if ( SLang_Num_Function_Args == 5 )
	  {
	     if ( -1 == SLang_pop_slstring( &keySeq ) )
	       {
		  return;
	       }
	     if ( -1 == SLang_pop_slstring( &specCmd ) )
	       goto free_and_return;
	 /* printf( "FILE: %s, Line: %d: keySeq: %s, specCmd: %s\n", __FILE__, __LINE__, keySeq, specCmd ); */
	  }
	else if ( -1 == SLang_pop_slstring( &specCmd ) )
	  {
	 /* printf( "FILE: %s, Line: %d: specCmd: %s\n", __FILE__, __LINE__, specCmd ); */
	     return;
	  }
     }
   else
     {
	if ( is_fun )
	  {
	     if ( -1 == SLang_pop_anytype( &client_data ) )
	       return;

	     if ( NULL == ( nt = SLang_pop_function() ) )
	       goto free_and_return;
	  }
	else if ( -1 == SLang_pop_slstring( &str ) )
	  return;
     }

   if ( -1 == SLang_pop_slstring( &iconName ) )
     goto free_and_return;

   if ( -1 == SLang_pop_slstring( &name ) )
     goto free_and_return;

   if ( -1 == SLang_pop_slstring( &menu ) )
     goto free_and_return;

   /* printf( "FILE: %s, Line: %d: menu: %s\n", __FILE__, __LINE__, menu ); */
   /* printf( "FILE: %s, Line: %d: name: %s\n", __FILE__, __LINE__, name ); */
   /* printf( "FILE: %s, Line: %d: iconName: %s\n", __FILE__, __LINE__, iconName ); */

   if ( NULL == ( m = find_menu_popup( menu ) ) )
     goto free_and_return;

   /* Dbp1( "m: %x\n", m ); */

   if ( do_append )
     where = m->num_subnodes;
   else if ( -1 == pop_where_to_insert( m, &where ) )
     goto free_and_return;

   if ( where > m->num_subnodes || do_append )
     {
	where = -1;
     }

   if ( is_fun == 2 )
     {
	nn = ( Menu_Node_Type * ) insertSpecCmdItem( m, name, specCmd, keySeq, where );
     }
   else
     {
	if ( nt != NULL )
	  {
	     if ( NULL != ( nn = ( Menu_Node_Type * ) insert_slang_fun_item( m, name, nt, client_data, where ) ) )
	       {
		  client_data = NULL;
		  nt = NULL;
	       }
	  }
	else
	  nn = ( Menu_Node_Type * ) insert_keystring_item( m, name, str, where );
   /* drop */
     }

   if ( isStock )
     {
	gtkToolbarStockItemNew( m, nn, iconName, where );
     }
   else
     {
	gtkToolbarItemNew( m, nn, iconName, where );
     }

free_and_return:

   if ( client_data != NULL )
     SLang_free_anytype( client_data );
   SLang_free_slstring( keySeq );
   SLang_free_slstring( specCmd );
   SLang_free_slstring( str );
   SLang_free_slstring( menu );
   SLang_free_slstring( name );
   SLang_free_slstring( iconName );
   SLang_free_function( nt );
}

static void
toolbarExtension(void)
{
   /* Do nothing. Only to allow to check, whether toolbars can be used!!! */
}

/************************************
* createToolbarCmd
*
* debug print: "Name: %s\n", name
*
************************************/

static void
createToolbarCmd( char *name )
{
   /* printf( "File: %s, Line: %d: Function(%s): create_tool_bar_cmd\n", __FILE__, __LINE__, name ); */
   /* gtkCreateToolbarCmd( name ); */
   gtkCreateToolbarCmd( create_menu_bar( name, 10 ) );
   /* ( void ) create_menu_bar( name, 10 ); */
}

/************************************
* addBufferToolbarCmd
*
* debug print: "Name: %s\n", name
*
************************************/

static void
addBufferToolbarCmd( char *name )
{
   /* printf( "File: %s, Line: %d: Funktion(%s): add_buffer_tool_bar_cmd\n", __FILE__, __LINE__, name ); */
   gtkAddBufferToolbarCmd( name );
}

/************************************
* insertToolbarItemCmd
*
* debug print: "(void)\n"
*
************************************/

static void
insertToolbarItemCmd( void )
{
   /* printf( "File: %s, Line: %d: append_tool_bar_item_cmd\n", __FILE__, __LINE__ ); */
   insertToolbarItemCmdInternal( ( SLang_Num_Function_Args == 6 ), 0, 0 );
}

/************************************
* insertToolbarStockItemCmd
*
* debug print: "(void)\n"
*
************************************/

static void
insertToolbarStockItemCmd( void )
{
   /* printf( "File: %s, Line: %d: append_tool_bar_item_cmd\n", __FILE__, __LINE__ ); */
   insertToolbarItemCmdInternal( ( SLang_Num_Function_Args == 6 ), 0, 1 );
}

/************************************
* insertToolbarSpecialCmd
*
* debug print: "(void)\n"
*
************************************/

static void
insertToolbarSpecialCmd( void )
{
   /* printf( "File: %s, Line: %d: Funktion: append_tool_bar_special_cmd\n", __FILE__, __LINE__ ); */
   insertToolbarItemCmdInternal( 2, 0, 0 );
}

/************************************
* insertToolbarStockSpecialCmd
*
* debug print: "(void)\n"
*
************************************/

static void
insertToolbarStockSpecialCmd( void )
{
   /* printf( "File: %s, Line: %d: Funktion: append_tool_bar_stock_special_cmd\n", __FILE__, __LINE__ ); */
   insertToolbarItemCmdInternal( 2, 0, 1 );
}

/************************************
* appendToolbarStockItemCmd
*
* debug print: "(void)\n"
*
************************************/

static void
appendToolbarItemCmd( void )
{
   /* printf( "File: %s, Line: %d: append_tool_bar_item_cmd\n", __FILE__, __LINE__ ); */
   insertToolbarItemCmdInternal( ( SLang_Num_Function_Args == 5 ), 1, 0 );
}

/************************************
* appendToolbarStockItemCmd
*
* debug print: "(void)\n"
*
************************************/

static void
appendToolbarStockItemCmd( void )
{
   /* printf( "File: %s, Line: %d: Funktion(%s): append_tool_bar_stock_item_cmd\n", __FILE__, __LINE__ ); */
   insertToolbarItemCmdInternal( ( SLang_Num_Function_Args == 5 ), 1, 1 );
}

/************************************
* appendToolbarSpecialCmd
*
* debug print: "(void)\n"
*
************************************/

static void
appendToolbarSpecialCmd( void )
{
   /* printf( "File: %s, Line: %d: Funktion: append_tool_bar_special_cmd\n", __FILE__, __LINE__ ); */
   insertToolbarItemCmdInternal( 2, 1, 0 );
}

/************************************
* appendToolbarStockSpecialCmd
*
* debug print: "(void)\n"
*
************************************/

static void
appendToolbarStockSpecialCmd( void )
{
   /* printf( "File: %s, Line: %d: Funktion: append_tool_bar_stock_special_cmd\n", __FILE__, __LINE__ ); */
   insertToolbarItemCmdInternal( 2, 1, 1 );
}

/************************************
* appendToolbarSeparatorItemCmd
*
* debug print: "Name: %s\n", name
*
************************************/

static void
appendToolbarSeparatorItemCmd( char *name )
{
   Menu_Popup_Type *m;

   /* gtkToolbarAppendSeparatorCmd( name ); */
   m = find_menu_popup (name);

   if (m != NULL)
     gtkToolbarAppendSeparatorCmd( m, insert_separator( m, m->num_subnodes ) );
}

/************************************
* insertToolbarSeparatorItemCmd
*
* debug print: "Name: %s\n", name
*
************************************/

static void
insertToolbarSeparatorItemCmd( char *name )
{
   unsigned int where;
   Menu_Popup_Type *m;

   /* gtkToolbarAppendSeparatorCmd( name ); */
   m = find_menu_popup (name);

   if ( m == NULL )
     return;

   if (-1 == pop_where_to_insert( m, &where ))
     return;

   if ( where > m->num_subnodes ) where = m->num_subnodes;

   gtkToolbarInsertSeparatorCmd( m, insert_separator( m, where ), where );
}

/************************************
* toolbarGetNumItemsCmd
*
* debug print: "Menu: %s\n", menu
*
************************************/

static int
toolbarGetNumItemsCmd( char *menu )
{
   Menu_Popup_Type *p;

   /* gtkMenuDeleteItemsCmd( menu ); */

   if (NULL == (p = find_menu_popup (menu)))
     return( - 1 );

   if ( p->type != MENU_NODE_POPUP &&
	p->type != MENU_NODE_MENUBAR )
     {
	return( -1 );
     }

   return( p->num_subnodes );
}

static SLang_Intrin_Fun_Type Menu_Table[] =
{

   /****************************************************************************************************/
   /***** Menu functions       *************************************************************************/
   /****************************************************************************************************/
   MAKE_INTRINSIC_S(  "menu_create_menu_bar", create_menu_bar_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_SS( "menu_append_popup", append_popup_menu_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_SS( "menu_insert_popup", insert_popup_menu_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "menu_use_menu_bar", set_buffer_menu_bar_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "menu_append_separator", append_separator_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "menu_insert_separator", insert_separator_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "menu_append_item", append_menu_item_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "menu_insert_item", insert_menu_item_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "menu_delete_item", menu_delete_item_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "menu_delete_items", menu_delete_items_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_SI( "menu_set_object_available", set_object_available_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "menu_set_select_menubar_callback", set_select_menubar_callback, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "menu_set_init_menubar_callback", set_init_menubar_callback, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "menu_set_select_popup_callback", set_select_popup_callback, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "menu_set_tweak_popup_callback", set_tweak_popup_callback, VOID_TYPE ),
   MAKE_INTRINSIC_SS( "menu_copy_menu", copy_menu_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_SS( "menu_set_menu_bar_prefix", set_menu_bar_prefix_string, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "menu_select_menu", popup_menu_cmd, VOID_TYPE ),

   /****************************************************************************************************/
   /***** Toolbar functions    *************************************************************************/
   /****************************************************************************************************/

   MAKE_INTRINSIC_0(  "toolbar_extension", toolbarExtension, VOID_TYPE ),

   /* MAKE_INTRINSIC_S(  "toolbar_create_toolbar", createToolbarCmd, VOID_TYPE ), */
   MAKE_INTRINSIC_S(  "toolbar_create", createToolbarCmd, VOID_TYPE ),
   /* MAKE_INTRINSIC_S(  "toolbar_add_toolbar", addBufferToolbarCmd, VOID_TYPE ), */
   /* MAKE_INTRINSIC_S(  "toolbar_append_toolbar", addBufferToolbarCmd, VOID_TYPE ), */
   MAKE_INTRINSIC_S(  "toolbar_append", addBufferToolbarCmd, VOID_TYPE ),
   /* MAKE_INTRINSIC_SI( "toolbar_insert_toolbar", jGtkInsertBufferToolbarCmd, VOID_TYPE ), */
   MAKE_INTRINSIC_SI( "toolbar_insert", jGtkInsertBufferToolbarCmd, VOID_TYPE ),
   /* MAKE_INTRINSIC_SI( "toolbar_detach", ???, VOID_TYPE ), */

   MAKE_INTRINSIC_0(  "toolbar_append_item", appendToolbarItemCmd, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "toolbar_append_stock_item", appendToolbarStockItemCmd, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "toolbar_append_separator", appendToolbarSeparatorItemCmd, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "toolbar_append_stock_spec", appendToolbarStockSpecialCmd, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "toolbar_append_spec", appendToolbarSpecialCmd, VOID_TYPE ),

   MAKE_INTRINSIC_0(  "toolbar_insert_item", insertToolbarItemCmd, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "toolbar_insert_stock_item", insertToolbarStockItemCmd, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "toolbar_insert_separator", insertToolbarSeparatorItemCmd, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "toolbar_insert_stock_spec", insertToolbarStockSpecialCmd, VOID_TYPE ),
   MAKE_INTRINSIC_0(  "toolbar_insert_spec", insertToolbarSpecialCmd, VOID_TYPE ),

   MAKE_INTRINSIC_S(  "toolbar_delete_items", menu_delete_items_cmd, VOID_TYPE ),
   MAKE_INTRINSIC_S(  "toolbar_delete_item", menu_delete_item_cmd, VOID_TYPE ),

   MAKE_INTRINSIC_S(  "toolbar_get_num_items", toolbarGetNumItemsCmd, INT_TYPE ),

   MAKE_INTRINSIC( NULL, NULL, 0, 0 )
};

/************************************
* jGtkDestroyWidget
*
* debug print: "w: %x, data: %x, *data: %x\n", w, data, *data
*
************************************/

static void
jGtkDestroyWidget( GtkWidget *w, gpointer data )
{
   GtkWidget **wp = ( GtkWidget ** ) data;

   *wp = NULL;
}

/************************************
* jed_init_menus
*
* debug print: "(void)\n"
*
************************************/

int jed_init_menus (void)
{
   if (-1 == init_menu_keymap ())
     return -1;

   if (-1 == SLadd_intrin_fun_table (Menu_Table, NULL))
     return -1;
# if 0
   if (-1 == make_global_menubar ())
     return -1;
# endif
   return 0;
}

/********************************************************************
 ********************************************************************
 *****     Menu functions
 *******************************************************************
 *******************************************************************/

/* extern void        popup_menu_cmd( char * ); */

void
jgtk_initMenubarStruct(void)
{
   if ( Global_Menu_Bar )
     {
	set_buffer_menu_bar_cmd( "Global" );
     }
}

# if 0

static int
checkMenuPathExists( char *dest, char *name )
{
   char *newPath = SLmalloc( strlen( dest ) + 1 + strlen( name ) + 1 );
   char *savPath = newPath;

   if (newPath == NULL)
     return -1;

   strcpy( newPath, dest );
   newPath += strlen( dest );

   *newPath = '.';
   newPath += 1;

   strcpy( newPath, name );

   if ( g_hash_table_lookup( menuArray, savPath ) )
     {
	SLfree( savPath );
	return( 1 );
     }
   else
     {
	SLfree( savPath );
	return( 0 );
     }
}

char *
addNewMenuPath( char *dest, char *name, GtkJedMenuType *menu )
{
   char *newPath = SLmalloc( strlen( dest ) + 1 + strlen( name ) + 1 );
   char *savPath = newPath;

   if (newPath == NULL)
     return NULL;

   strcpy( newPath, dest );
   newPath += strlen( dest );

   *newPath = '.';
   newPath += 1;

   strcpy( newPath, name );

   g_hash_table_insert( menuArray, savPath, menu );

   return( savPath );
}

# endif

static int
isMnemonic( char *name )
{
   while ( *name )
     {
	if ( *name == '&' )
	  {
	     return( True );
	  }
	++name;
     }
   return( False );
}

static char *
createMenuNameMnemonic( char *name )
{
   char *savName;
   char *gtkName = SLmalloc( strlen( name ) + 1 );
   int notEnd;

   if (gtkName == NULL)
     return NULL;

   strcpy( gtkName, name );

   savName = gtkName;

   notEnd = 1;

   while( *savName && notEnd )
     {
	if ( *savName == '&' )
	  {
	     *savName = '_';
	     notEnd = 0;
	  }
	++savName;
     }

   return( gtkName );
}

gboolean
menuBarActivateCurrentCB( GtkWidget *w,
			  GdkEvent  *ev,
			  gpointer  *data )
{
/*    printf( "Current Menubar activated!!!!!!!!!!!!!\n" ); */
   /* Db; */
   /* Return( FALSE ); */
   jed_select_menu_bar();
   Jed_Menus_Active = 0;

   return( FALSE );
}

/************************************
* jGtkGetKeystring
*
* debug print: "Menu keystring fun type: %x, color0: %d, color1: %d, field_widht: %d\n", k, color0, color1, field_width
*
************************************/

static int
jGtkGetKeystring( Menu_Keystring_Fun_Type *k, char *retBuf )
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

   /* draw_name (k->name, color0, color1, field_width); */

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
# ifdef IBMPC_SYSTEM
	if ((i == 0) || (i == 0xE0))
	  continue;
# endif

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
# ifndef IBMPC_SYSTEM
	     if (i == 0)
	       {
		  buf[0] = '^';
		  buf[1] = '@';
		  buf[2] = 0;
	       }
# endif
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

   /* Dbp2( "Best keystring: %x, Best key: %x\n", best_keystring, best_key ); */

   if ( best_keystring && *best_keystring > 0 && *best_keystring < 32 )
     {
	best_keystring[2] = '\0';
	best_keystring[1] = *best_keystring + 64;
	best_keystring[0] = '^';
     }

   if ((best_keystring != NULL)
       && ((*best_keystring & 0x80) || ((unsigned char)*best_keystring < 32)))
     best_keystring = NULL;

   if (best_keystring == NULL)
     {
      /* Db; */
	if (best_key == NULL)
	  return 0;

        /* Db; */
	best_keystring = SLang_make_keystring (best_key->str);
	if (best_keystring == NULL)
	  return 0;
     }

   best_len = strlen (best_keystring);
   if (best_len > 4)
     return 0;

   /* Db; */
   strcpy( retBuf, best_keystring );

   return( best_len );

   /**********
   SLsmg_forward (-4);
   SLsmg_set_color (color0);
   SLsmg_write_nchars (best_keystring, best_len);
   Return 0;
   *****/
}

static void
jGktDrawKeyStringAccelKey( Menu_Keystring_Fun_Type *km )
{
   char keyBuf[5];
   char keyBufExt[50] = "                                                  ";
   GList *cl;

   /* Dbp2( "Drawing: %x, |%s|\n", *n, ( *n )->name ); */
   /* km = ( Menu_Keystring_Fun_Type * ) *n; */
   if ( jGtkGetKeystring( km, keyBuf ) )
     {
	*keyBufExt = ' ';
      /* Dbp1( "AccelKey:    |%s|\n", keyBuf ); */
	if ( !strncmp( keyBuf, "^[", 2 ) )
	  {
	     strcpy( keyBufExt + 5, "ESC " );
	 /* Dbp1( "AccelKeyExt: |%s|\n", keyBufExt ); */
	     strcpy( keyBufExt + 9, keyBuf + 2 );
	 /* Dbp1( "AccelKeyExt: |%s|\n", keyBufExt ); */
	  }
	else
	  {
	     strcpy( keyBufExt + 5, keyBuf );
	  }

     }
   else
     {
	*keyBuf = '\0';
	*keyBufExt = '\0';
     }

   /* Dbp1( "AccelKey:    |%s|\n", keyBuf ); */
   /* Dbp1( "AccelKeyExt: |%s|\n", keyBufExt ); */
   /* */
   cl = gtk_container_get_children(
				   ( GtkContainer * )
				   gtk_container_get_children( ( GtkContainer * ) km->menuItem )->data );
   if ( cl && cl->next && GTK_IS_LABEL( ( GtkWidget * ) cl->next->data ) )
     {
	gtk_label_set_text( ( GtkLabel * ) cl->next->data, keyBufExt );
     }

}

static void
jGtkDrawAccelKey( Menu_Popup_Type *p )
{
   unsigned int i = p->num_subnodes;
   Menu_Node_Type **n = p->subnodes;

   for ( i = 0; i < p->num_subnodes; ++i )
     {
	if ( ( *n )->type == MENU_NODE_KEYSTRING )
	  {
	     jGktDrawKeyStringAccelKey( ( Menu_Keystring_Fun_Type * ) *n );
	  }
	++n;
     }
}

static gboolean
jGtkPopupMenuActivateCB( GtkMenuItem *w,
			 gpointer     data )
{
   Menu_Popup_Type *m = ( Menu_Popup_Type * ) data;
   /* Dbp2( "Popup %x: |%s| activated\n", m, m->name ); */
   if ( ! ( m->flags & JGTK_MENU_POPUP_PREPARED ) )
     {
	jGtkDrawAccelKey( m );
      /* Dbp2( "Prepare popup %x: |%s|\n", m, m->name ); */
      /* m->flags |= JGTK_MENU_POPUP_PREPARED; */
	jGtkPreparePopup( m, m->select_popup_callback );
     }
   return( FALSE );
}

static gboolean
jGtkPrepareCB( GtkMenuItem *w, gpointer data )
{
   Menu_Popup_Type *m = ( Menu_Popup_Type * ) data;
   /* Dbp2( "Popup %x: |%s| prepared\n", m, m->name ); */
   /* jGtkPreparePopup( m, m->select_popup_callback ); */
   return( FALSE );
}

static void
jGtkResetPopupGtkPrepared( Menu_Popup_Type *m )
{
   Menu_Node_Type **l, **lmax;

   m->flags &= ~JGTK_MENU_POPUP_PREPARED;

   l = m->subnodes;
   lmax = l + m->num_subnodes;

   while ( l < lmax )
     {
	if ( ( *l )->type == 3 ) /* *l->type == 5 should not occur */
	  jGtkResetPopupGtkPrepared( ( Menu_Popup_Type * ) *l );

	++l;
     }
}

static gboolean
jGtkDeactivateCB( GtkMenuItem *w, gpointer data )
{
   Menu_Popup_Type *m = ( Menu_Popup_Type * ) data;
   /* Dbp2( "Menu shell %x: |%s| deactivated\n", m, m->name ); */
   jGtkResetPopupGtkPrepared( m );
   /* jGtkPreparePopup( m, m->select_popup_callback ); */
   return( FALSE );
}

static gboolean
jGtkActivateCurrentCB( GtkMenuItem *w, gboolean force, gpointer data )
{
   Menu_Popup_Type *m = ( Menu_Popup_Type * ) data;
   /* Dbp2( "Menu shell %x: |%s| activate-current\n", m, m->name ); */
   /* jGtkPreparePopup( m, m->select_popup_callback ); */
   return( FALSE );
}

static void
menuCallback( Menu_Node_Type *menu )
{

   execMenuTbCmd( menu );

   jGtkSetFocus();
   Jed_Menus_Active = 0;

   update((Line *) NULL, 0, 0, 1); /* File: screen.c */

   jGtkSetFocus();
}

# if 0

int 
gtkCreateTBCmd( char *name )
{
   char *newKey = SLmalloc( strlen( name ) + 1 );

   if (newKey == NULL)
     return -1;

   /* GtkJedMenuType *newMenu = ( GtkJedMenuType * ) SLmalloc( sizeof( GtkJedMenuType ) ); */

   /* printf( "File: %s, Line: %d, gtkCreateMenuBarCmd: |%s|\n", __FILE__, __LINE__, name ); */

   strcpy( newKey, name );
   /* printf( "File: %s, Line: %d, gtkCreateMenuBarCmd: |%s|\n", __FILE__, __LINE__, newKey ); */
   newMenu->parent = NULL;
   newMenu->subMenu = NULL;
   newMenu->menuItem = gtk_menu_bar_new();

   g_signal_connect( G_OBJECT( newMenu->menuItem ),
		     "button-release-event",
		     G_CALLBACK( menuBarActivateCurrentCB ),
		     newMenu->menuItem );
/*******
   g_signal_connect( G_OBJECT( newMenu->menuItem ),
		     "button-press-event",
		     G_CALLBACK( menuBarActivateCurrentCB ),
		     newMenu->menuItem );
***********/

   /* printf( "File: %s, Line: %d, Name: %s, menu: %x\n", __FILE__, __LINE__, newKey, newMenu ); */
   g_hash_table_insert( menuArray, newKey, newMenu );
   return 0;
}

# endif

static void
jGtkCreateMenuBarCmd( Menu_Bar_Type *m )
{
   /* char *newKey = SLmalloc( strlen( name ) + 1 ); */
   /* GtkJedMenuType *newMenu = ( GtkJedMenuType * ) SLmalloc( sizeof( GtkJedMenuType ) ); */

   /* printf( "File: %s, Line: %d, gtkCreateMenuBarCmd: |%s|\n", __FILE__, __LINE__, name ); */

   /* strcpy( newKey, name ); */
   /* printf( "File: %s, Line: %d, gtkCreateMenuBarCmd: |%s|\n", __FILE__, __LINE__, newKey ); */
   /* newMenu->parent = NULL; */
   /* newMenu->subMenu = NULL; */
   /* newMenu->menuItem = gtk_menu_bar_new(); */

   m->subMenu = gtk_menu_bar_new();

   g_signal_connect( G_OBJECT( m->subMenu ),
		     "button-release-event",
		     G_CALLBACK( menuBarActivateCurrentCB ),
		     NULL );

   g_signal_connect( G_OBJECT( m->subMenu ),
		     "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     &( m->subMenu ) );

   g_signal_connect( G_OBJECT( m->subMenu ),
		     "deactivate",
		     G_CALLBACK( jGtkDeactivateCB ),
		     m );

   g_signal_connect( G_OBJECT( m->subMenu ),
		     "activate-current",
		     G_CALLBACK( jGtkActivateCurrentCB ),
		     m );
}

void
gtkSetBufferMenuBarCmd( char *name )
{
   Menu_Bar_Type *menu = menu_find_menu_bar( name, True );

   /* Db; */
   /* Dbp2( ">>>: ----------->Menu: |%s|%x|<<<\n", name, menu ); */

   if ( menu )
     jGtkAttachMenubar( menu->subMenu );

# if 0
   if ( menu && menu->menuItem != JGtkWin->appWMenuBar )
     {
      /* Dbp1( "Menu: %x\n", menu ); */
	gtk_box_pack_start( GTK_BOX( JGtkWin->appWGrid ), menu->menuItem, False, False, 0 );
	gtk_box_reorder_child( GTK_BOX( JGtkWin->appWGrid ), menu->menuItem, 1 );
	if ( JGtkWin->appWMenuBar )
	  gtk_container_remove( GTK_CONTAINER( JGtkWin->appWGrid ), JGtkWin->appWMenuBar );
	JGtkWin->appWMenuBar = menu->menuItem;
	gtk_widget_show_all( menu->menuItem );
     }
# endif

}

# if 0

static void
jGtkSimpleAppendTBCmd( char *dest, char *menuName )
{
   char *gtkName = createMenuNameMnemonic( menuName );
   /* GtkJedMenuType *menu = g_hash_table_lookup( menuArray, dest ); */
   /* GtkJedMenuType *newMenu = ( GtkJedMenuType * ) SLmalloc( sizeof( GtkJedMenuType ) ); */

   /* newMenu->parent = menu; */
   /* newMenu->menuItem = gtk_menu_item_new_with_mnemonic( gtkName ); */
   SLfree( gtkName );
   newMenu->subMenu = gtk_menu_new();

   g_signal_connect( G_OBJECT( newMenu->menuItem ),
		     "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     &( newMenu->menuItem ) );

   g_signal_connect( G_OBJECT( newMenu->subMenu ),
		     "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     &( newMenu->subMenu ) );

   gtk_widget_show_all( newMenu->menuItem );
   gtk_widget_show_all( newMenu->subMenu );

   gtk_menu_item_set_submenu( GTK_MENU_ITEM( newMenu->menuItem ), newMenu->subMenu );

   /* printf( "gtkSimpleAppendPopupMenuCmd (File: %s, Line: %d): dest: |%s| name: |%s|\n", __FILE__, __LINE__, dest, menuName ); */

   if ( menu->parent )
     {
	gtk_menu_shell_append( GTK_MENU_SHELL( menu->subMenu ), newMenu->menuItem );
     }
   else
     {
	gtk_menu_bar_append( GTK_MENU_BAR( menu->menuItem ), newMenu->menuItem );
     }

   addNewMenuPath( dest, menuName, newMenu );
}

# endif

static void
jGtkSimpleInsertPopupMenuCmd( Menu_Popup_Type *m, int n )
{
   char *gtkName = createMenuNameMnemonic( m->name );

   m->menuItem = gtk_menu_item_new_with_mnemonic( gtkName );
   SLfree( gtkName );
   m->subMenu = gtk_menu_new();

   gtk_menu_item_set_submenu( GTK_MENU_ITEM( m->menuItem ), m->subMenu );

   g_signal_connect( G_OBJECT( m->menuItem ),
		     "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     &( m->menuItem ) );

   g_signal_connect( G_OBJECT( m->subMenu ),
		     "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     &( m->subMenu ) );

   g_signal_connect( G_OBJECT( m->menuItem ),
		     "activate",
		     G_CALLBACK( jGtkPopupMenuActivateCB ),
		     ( gpointer ) m );

   /* Dbp2( "Name: |%x|%s|\n", m, m->name ); */

   gtk_widget_show_all( m->menuItem );
   gtk_widget_show_all( m->subMenu );

   /* printf( "gtkSimpleAppendPopupMenuCmd (File: %s, Line: %d): dest: |%s| name: |%s|\n", __FILE__, __LINE__, dest, menuName ); */

   /* if ( m->parent->parent ) */
   /*   { */
   gtk_menu_shell_insert( GTK_MENU_SHELL( m->parent->subMenu ), m->menuItem, n );
   /*    } */
   /* else */
   /*   { */
   /*    gtk_menu_shell_insert( GTK_MENU_SHELL( m->parent->menuItem ), m->menuItem, n ); */
   /*    } */
}

# if 0

void
checkAndCreateMenuPath( char *path )
{
   /* char *savPath = path; */
   char *savPathBeg = path;
   char *savPathEnd = path;

   while ( *savPathEnd )
     {
	if ( *savPathEnd == '.' )
	  {
	 /* the char-sequence .. has occured */
	     if ( savPathBeg == savPathEnd )
	       {
		  savPathBeg++;
		  savPathEnd++;
	       }
	     else
	       {
		  *savPathEnd = '\0'; /* temporary End of String; */
		  if ( g_hash_table_lookup( menuArray, path ) )
		    {
	       /* Path exists */
		       *savPathEnd = '.';
		       ++savPathEnd;
		       savPathBeg = savPathEnd;
		    }
		  else
		    {
	       /* Path does not exist */
		       if ( savPathBeg == path )
			 {
		  /* First part does not exist ==> new Menu Bar */
			    gtkCreateTBCmd( path );
			    *savPathEnd = '.';
			    ++savPathEnd;
			    savPathBeg = savPathEnd;
			 }
		       else
			 {  /* a new submenu needs to be created */
			    char savChar;

			    --savPathBeg;
			    savChar = *savPathBeg;
			    *savPathBeg = '\0';
			    savPathBeg++;
			    jGtkSimpleAppendTBCmd( path, savPathBeg );
			    --savPathBeg;
			    *savPathBeg = savChar;
			    *savPathEnd = '.';
			    ++savPathEnd;
			    savPathBeg = savPathEnd;
			 }
		    }
	       }
	  }
	else
	  {
	     ++savPathEnd;
	  }
     }

   if ( savPathBeg != savPathEnd )
     {
	if ( !g_hash_table_lookup( menuArray, path ) )
	  {
	 /* Path does not exist */
	     if ( savPathBeg == path )
	       {
	    /* First part does not exist ==> new Menu Bar */
		  gtkCreateTBCmd( path );
	       }
	     else
	       {  /* a new submenu needs to be created */
		  char savChar;

		  --savPathBeg;
		  savChar = *savPathBeg;
		  *savPathBeg = '\0';
		  savPathBeg++;
		  gtkAppendTBCmd( path, savPathBeg );
		  --savPathBeg;
		  *savPathBeg = savChar;
	       }
	  }
     }
}

void
gtkAppendTBCmd( char *dest, char *menu )
{
   /* printf( "gtkAppendPopupMenuCmd (File: %s, Line: %d): dest: |%s| menu: |%s|\n", __FILE__, __LINE__, dest, menu ); */
   if ( !checkMenuPathExists( dest, menu ) )
     {
	checkAndCreateMenuPath( dest );
	jGtkSimpleAppendTBCmd( dest, menu );
     }
}

# endif

static void
jGtkInsertPopupMenuCmd( Menu_Popup_Type *m, int n )
{
   /* printf( "gtkAppendPopupMenuCmd (File: %s, Line: %d): dest: |%s| menu: |%s|\n", __FILE__, __LINE__, dest, menu ); */

   if ( m && !m->menuItem && !m->subMenu && m->parent &&
	( m->parent->type == 5 || m->parent->type == 3 ) )
     {
	jGtkSimpleInsertPopupMenuCmd( m, n );
     }
   /* m->flags &= ~JGTK_MENU_POPUP_PREPARED; */
   /* m->flags = 0; */
}

static void
jGtkInsertSeparatorCmd( Menu_Popup_Type *m, Menu_Node_Type *sep, int n )
{
   if ( m && m->subMenu && sep )
     {
	sep->menuItem = gtk_separator_menu_item_new();

	g_signal_connect( G_OBJECT( sep->menuItem ),
			  "destroy",
			  G_CALLBACK( jGtkDestroyWidget ),
			  &( sep->menuItem ) );

	gtk_menu_shell_insert( GTK_MENU_SHELL( m->subMenu ), sep->menuItem, n + 1 );

	gtk_widget_show_all( sep->menuItem );
     }
}

static void
jGtkMenuItemNew( Menu_Popup_Type *menu, Menu_Node_Type *newNode, int n )
{

   if ( menu && menu->subMenu && newNode )
     {
	char *gtkName = createMenuNameMnemonic( newNode->name );

	if ( isMnemonic( newNode->name ) )
	  {
	     newNode->menuItem = gtk_menu_item_new_with_mnemonic( gtkName );
	  }
	else
	  {
	     newNode->menuItem = gtk_menu_item_new_with_label( gtkName );
	  }

	g_signal_connect( G_OBJECT( newNode->menuItem ),
			  "destroy",
			  G_CALLBACK( jGtkDestroyWidget ),
			  &( newNode->menuItem ) );

	SLfree( gtkName );

	gtk_menu_shell_insert( GTK_MENU_SHELL( menu->subMenu ), newNode->menuItem, n );

	gtk_widget_show_all( newNode->menuItem );

      /********
      g_signal_connect_swapped( G_OBJECT( newNode->menuItem ), "activate",
				G_CALLBACK( menuCallback ),
				( gpointer ) newNode );
      **********/
	g_signal_connect( G_OBJECT( newNode->menuItem ), "activate",
			  G_CALLBACK( toolbarCallback ),
			  ( gpointer ) newNode );
     }

}

static void
jGtkMenuKeystringItemNew( Menu_Popup_Type *menu, Menu_Node_Type *newNode, int n )
{
   GtkWidget *l;
   GtkWidget *lb;

   if ( menu && menu->subMenu && newNode )
     {
	char *gtkName = createMenuNameMnemonic( newNode->name );

	newNode->menuItem = gtk_menu_item_new();

	lb = gtk_hbox_new( 0, 0 );

	gtk_container_add( ( GtkContainer * ) newNode->menuItem, lb );

	l = gtk_label_new( " " );

	gtk_box_pack_start( ( GtkBox * ) lb, l, True, True, 0 );

	if ( isMnemonic( newNode->name ) )
	  {
	     gtk_label_set_markup_with_mnemonic( ( GtkLabel * ) l, gtkName );
	 /* newNode->menuItem = gtk_menu_item_new_with_mnemonic( gtkName ); */
	  }
	else
	  {
	     gtk_label_set_markup( ( GtkLabel * ) l, gtkName );
	 /* newNode->menuItem = gtk_menu_item_new_with_label( gtkName ); */
	  }

	gtk_misc_set_alignment( ( GtkMisc * ) l, 0, 0 );
      /* gtk_label_set_justify( ( GtkLabel * ) l, GTK_JUSTIFY_LEFT ); */

	l = gtk_label_new( " " );

	gtk_box_pack_start( ( GtkBox * ) lb, l, False, False, 0 );

	g_signal_connect( G_OBJECT( newNode->menuItem ),
			  "destroy",
			  G_CALLBACK( jGtkDestroyWidget ),
			  &( newNode->menuItem ) );

	SLfree( gtkName );

	gtk_menu_shell_insert( GTK_MENU_SHELL( menu->subMenu ), newNode->menuItem, n );

	gtk_widget_show_all( newNode->menuItem );

      /********
      g_signal_connect_swapped( G_OBJECT( newNode->menuItem ), "activate",
				G_CALLBACK( menuCallback ),
				( gpointer ) newNode );
      **********/
	g_signal_connect( G_OBJECT( newNode->menuItem ), "activate",
			  G_CALLBACK( toolbarCallback ),
			  ( gpointer ) newNode );

	jGktDrawKeyStringAccelKey( ( Menu_Keystring_Fun_Type * ) newNode );
     }

}

# if 0
void
gtkMenuDeleteItemCmd( char *menu )
{
}

void
gtkMenuDeleteItemsCmd( char *menu )
{
}

# endif

void
gtkSetObjectAvailableCmd( char *name )
{
}

/********************************************************************
 ********************************************************************
 *****     Toolbar functions
 *******************************************************************
 *******************************************************************/

void
jgtk_initToolbarArray(void)
{
   /* menuArray = g_hash_table_new( g_str_hash, g_str_equal ); */
}

extern int execTBKeyFeedCmd( char * );

extern int split_window( void );

void
toolbarCallback( GtkWidget *w, gpointer data )
{
   Menu_Node_Type *menu = ( Menu_Node_Type * ) data;
   /* Dbp1( "Toolbar clicked\n", 1 ); */
   /* Dbp1( "Menu Item: %x selected!\n", menu ); */
   /* Dbp1( "Path: |%s| selected!\n", menu->name ); */

   if ( actTBCmdNode )
     {
	Menu_Keystring_Fun_Type *tmp = ( Menu_Keystring_Fun_Type * ) menu;

      /* Dbp1( "ActTBCmdNode: %x\n", actTBCmdNode ); */

	if ( tmp && tmp->type == MENU_NODE_TYPEDKEYS )
	  {
	 /* Dbp6( "keyStr[0]: %d, |%c|, keyStr[1]: %d, |%c|, keyStr[2]: %d, |%c|\n", tmp->keystring[0], tmp->keystring[0], tmp->keystring[1], tmp->keystring[1], tmp->keystring[2], tmp->keystring[2] ); */
	     jgtk_createKeyEvents( tmp->keystring );
	  }
	else
	  {
	     jgtk_createKeyEvents( SLang_process_keystring( "^M" ) );
	  }
     }
   else
     {
	actTBCmdNode = ( Menu_Node_Type * ) data;
      /* Dbp1( "ActTBCmdNode: %x\n", actTBCmdNode ); */

	if ( actTBCmdNode )
	  {
	 /* Dbp1( "ActTBCmdNode: %x\n", actTBCmdNode ); */
	     if ( actTBCmdNode->type == MENU_NODE_TYPEDKEYS )
	       {
		  jgtk_createKeyEvents( ( ( Menu_Keystring_Fun_Type * ) actTBCmdNode )->keystring );
		  actTBCmdNode = NULL;
	       }
	     else if ( !IN_MINI_WINDOW )
	       {
	    /* Dbp1( "IN_MINI_WINDOW: %d\n", IN_MINI_WINDOW ); */
		  jgtk_createKeyEvents( SLang_process_keystring( TB_SEL_KEY_SEQ ) );
	       }
	     else
	       {
		  actTBCmdNode = NULL;
	       }
	 /* Dbp1( "Dummy: %d\n", 1 ); */
	  }
      /* tbActive = 1; */
     }
}

void
gtkCreateToolbarCmd( Menu_Bar_Type *mb )
{
   /* char *newKey = SLmalloc( strlen( mb->name ) + 1 ); */
   GtkToolbar *tb = ( GtkToolbar * ) gtk_toolbar_new();

   mb->subMenu = ( GtkWidget * ) tb;
   mb->menuItem = gtk_handle_box_new();

   g_signal_connect( G_OBJECT( mb->subMenu ), "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     ( gpointer ) &( mb->subMenu ) );
   g_signal_connect( G_OBJECT( mb->menuItem ), "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     ( gpointer ) &( mb->menuItem ) );

   gtk_toolbar_set_tooltips( tb, TRUE );

   gtk_toolbar_set_style( tb, GTK_TOOLBAR_ICONS );

   /* strcpy( newKey, mb->name ); */

   gtk_container_add( GTK_CONTAINER( mb->menuItem ), ( GtkWidget * ) tb );
   gtk_widget_show_all( ( GtkWidget * ) tb );
}

/************************************
* jGtkInsertBufferToolbarCmd
*
* debug print: "Name: |%s|, where: %x, *where: %d\n", name, where, *where
*
************************************/

static void
jGtkInsertBufferToolbarCmd( char *name, int *where )  /*{{{*/
{
   Menu_Bar_Type *menu = menu_find_menu_bar( name, 1 );

   /* Dbp1( "Menu: %x\n", menu ); */

   jGtkAddToolbar( menu->menuItem, *where );
}

void
gtkAddBufferToolbarCmd( char *name )  /*{{{*/
{
   Menu_Bar_Type *menu = menu_find_menu_bar( name, 1 );

   jGtkAddToolbar( menu->menuItem, -1 );
}
/*}}}*/

#if GTK_HAS_TOOLTIPS
static gboolean
jGtkQueryTooltipCB( GtkWidget *w, gint x, gint y,
		    gboolean    kbM,
		    GtkTooltip *tooltip,
		    gpointer    ud )
{

   gtk_tooltip_set_text( tooltip, ( const gchar * ) ud );

   return( TRUE );
}
#endif

static void
gtkToolbarStockItemNew( Menu_Popup_Type *parent, Menu_Node_Type *nn, char *iconName, int where )
{
#if GTK_HAS_TOOLTIPS
   char *ttName;
#endif
   GtkWidget *stockW = ( GtkWidget * ) gtk_tool_button_new_from_stock( iconName );

   nn->subMenu = NULL;
   nn->menuItem = stockW;
#if GTK_HAS_TOOLTIPS
   ttName = SLmalloc( strlen( nn->name ) + 1 );
   if (ttName != NULL)
     {
	g_object_set( G_OBJECT( stockW ), "has-tooltip", TRUE, NULL );
	strcpy( ttName, nn->name );

	g_signal_connect( G_OBJECT( stockW ),
			  "query-tooltip",
			  G_CALLBACK( jGtkQueryTooltipCB ),
			  ttName );
     }
#endif

   g_object_set( G_OBJECT( stockW ), "has-tooltip", TRUE, NULL );

   gtk_toolbar_insert( ( GtkToolbar * ) parent->subMenu, ( GtkToolItem * ) stockW, where );

   gtk_widget_show_all( stockW );

   g_signal_connect( G_OBJECT( stockW ), "clicked",
		     G_CALLBACK( toolbarCallback ),
		     ( gpointer ) nn );
   g_signal_connect( G_OBJECT( stockW ), "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     ( gpointer ) &( nn->menuItem ) );

}

static void
gtkToolbarItemNew( Menu_Popup_Type *parent, Menu_Node_Type *nn, char *iconName, int where )
{
   gint iconW, iconH;
   GdkPixbuf *iconImagePixbuf;
   GtkWidget *stockW;

   gtk_icon_size_lookup(
			gtk_toolbar_get_icon_size( ( GtkToolbar * ) parent->subMenu ),
			&iconW, &iconH );

   iconImagePixbuf = gdk_pixbuf_new_from_file_at_size( iconName,
						       iconW, iconH,
						       NULL );

   stockW = ( GtkWidget * ) gtk_tool_button_new(
						gtk_image_new_from_pixbuf( iconImagePixbuf ),
						nn->name );

   nn->subMenu = NULL;
   nn->menuItem = stockW;
#if GTK_HAS_TOOLTIPS
   gtk_tool_item_set_tooltip_text( GTK_TOOL_ITEM( stockW ), nn->name );
#endif
   gtk_toolbar_insert( ( GtkToolbar * ) parent->subMenu, ( GtkToolItem * ) stockW, where );

   gtk_widget_show_all( stockW );

   g_signal_connect( G_OBJECT( stockW ), "clicked",
		     G_CALLBACK( toolbarCallback ),
		     ( gpointer ) nn );

   g_signal_connect( G_OBJECT( stockW ), "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     ( gpointer ) &( nn->menuItem ) );
}

static void
gtkToolbarAppendSeparatorCmd( Menu_Popup_Type *p, Menu_Node_Type *menu )
{
   char *name;
   Menu_Bar_Type *parent = ( Menu_Bar_Type * ) p;
   GtkWidget *sepW = ( GtkWidget * ) gtk_separator_tool_item_new();

   menu->subMenu = NULL;
   menu->menuItem = sepW;

   gtk_toolbar_insert( ( GtkToolbar * ) parent->subMenu, ( GtkToolItem * ) sepW, -1 );

   gtk_widget_show_all( sepW );

   g_signal_connect( G_OBJECT( sepW ), "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     ( gpointer ) &( menu->menuItem ) );
}

static void
gtkToolbarInsertSeparatorCmd( Menu_Popup_Type *p, Menu_Node_Type *menu, int where )
{
   char *name;
   Menu_Bar_Type *parent = ( Menu_Bar_Type * ) p;
   GtkWidget *sepW = ( GtkWidget * ) gtk_separator_tool_item_new();

   menu->subMenu = NULL;
   menu->menuItem = sepW;

   gtk_toolbar_insert( ( GtkToolbar * ) parent->subMenu, ( GtkToolItem * ) sepW, where );

   gtk_widget_show_all( sepW );

   g_signal_connect( G_OBJECT( sepW ), "destroy",
		     G_CALLBACK( jGtkDestroyWidget ),
		     ( gpointer ) &( menu->menuItem ) );
}

# ifdef HAS_MOUSE
/************************************
* select_menu_via_rc
*
* debug print: " Type: %d, r: %d, c: %d\n", type, r, c
*
************************************/

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

/************************************
* jed_menu_handle_mouse
*
* debug print: "Type: %d, x: %d, y: %d, button %d, shift: %d\n", type, x, y, button, shift
*
************************************/

int jed_menu_handle_mouse (unsigned int type,
			   int x, int y, int button, int shift)
{
   (void) shift; (void) button;

   if ((type != JMOUSE_UP) && (type != JMOUSE_DRAG))
     return -1;

   /* Dbp1( "################################################################\n", 1 ); */
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

#  ifndef IBMPC_SYSTEM
/************************************
* xterm_mouse_cmd
*
* debug print: "(void)\n"
*
************************************/

static int xterm_mouse_cmd (void)
{
   int x, y, b;

   b = my_getkey ();
   printf( "File: %s, Line: %d, ch: %d\n", __FILE__, __LINE__, b );
   x = (unsigned char) my_getkey () - 32;
   y = (unsigned char) my_getkey () - 32;

   /* We need to trigger on the button release event */

   b -= 32;
   if ((b & 3) != 3)
     return -1;

   return jed_menu_handle_mouse (JMOUSE_UP, x, y, 0, 0);
}

#  endif 				       /* !IBMPC_SYSTEM */
# endif				       /* HAS_MOUSE */
#endif				       /* JED_HAS_MENUS */
