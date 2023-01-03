/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#if JED_HAS_BUFFER_LOCAL_VARS

#include <stdio.h>
#include <string.h>
#include <slang.h>

#include "jdmacros.h"
#include "buffer.h"
#include "misc.h"

typedef struct
{
   char *name;			       /* slstring */
   SLang_Any_Type *value;
}
BLocal_Type;

/* Each buffer can have a set of buffer-local variables.  The buffer-local
 * variables for a given buffer are stored in a linked list of tables
 * instead of one large table.
 */
struct _Jed_BLocal_Table_Type
{
   unsigned int num;
#define MAX_BLOCAL_VARS_PER_TABLE	10
   BLocal_Type local_vars[MAX_BLOCAL_VARS_PER_TABLE];
   Jed_BLocal_Table_Type *next;
};

static BLocal_Type *find_blocal_var (char *name, int err) /*{{{*/
{
   Jed_BLocal_Table_Type *table;
   BLocal_Type *lv, *lv_max;

   table = CBuf->blocal_table;

   while (table != NULL)
     {
	lv = table->local_vars;
	lv_max = lv + table->num;

	while (lv < lv_max)
	  {
	     if (name == lv->name)
	       return lv;

	     /* slstrings can be compared by pointer.
	      * if (0 == strcmp (name, lv->name))
	      * return lv;
	      */
	     lv++;
	  }
	table = table->next;
     }

   if (err) jed_verror ("buffer local variable '%s' does not exist in buffer '%s'",
			name, CBuf->name);

   return NULL;
}

/*}}}*/

static int blocal_var_exists (char *name)
{
   return (NULL != find_blocal_var (name, 0));
}

static void make_blocal_var (char *name) /*{{{*/
{
   Jed_BLocal_Table_Type *table;
   BLocal_Type *lv;

   if (NULL != find_blocal_var (name, 0))
     return;

   table = CBuf->blocal_table;

   if ((table == NULL)
       || (table->num == MAX_BLOCAL_VARS_PER_TABLE))
     {
	table = (Jed_BLocal_Table_Type *) jed_malloc0 (sizeof (Jed_BLocal_Table_Type));
	if (table == NULL)
	  return;

	table->next = CBuf->blocal_table;
	CBuf->blocal_table = table;
     }

   lv = table->local_vars + table->num;

   /* This should not fail since name already exists */
   if (NULL == (lv->name = SLang_create_slstring (name)))
     return;

   table->num += 1;
}

/*}}}*/

void jed_delete_blocal_vars (Jed_BLocal_Table_Type *table) /*{{{*/
{
   BLocal_Type *lv, *lv_max;
   Jed_BLocal_Table_Type *next;

   while (table != NULL)
     {
	lv = table->local_vars;
	lv_max = lv + table->num;

	while (lv < lv_max)
	  {
	     SLang_free_anytype (lv->value);
	     SLang_free_slstring (lv->name);
	     lv++;
	  }

	next = table->next;
	SLfree ((char *) table);
	table = next;
     }
}

/*}}}*/

static void set_blocal_var (char *name) /*{{{*/
{
   BLocal_Type *lv;

   lv = find_blocal_var (name, 1);
   if (lv == NULL)
     return;

   SLang_free_anytype (lv->value);
   lv->value = NULL;

   (void) SLang_pop_anytype (&lv->value);
}

/*}}}*/

static void get_blocal_var (char *name) /*{{{*/
{
   BLocal_Type *lv;

   lv = find_blocal_var (name, 1);
   if (lv == NULL)
     return;

   (void) SLang_push_anytype (lv->value);
}

/*}}}*/

static SLang_Intrin_Fun_Type BLocal_Intrinsics [] = /*{{{*/
{
   MAKE_INTRINSIC_S("blocal_var_exists", blocal_var_exists, SLANG_INT_TYPE),
   MAKE_INTRINSIC_S("set_blocal_var", set_blocal_var, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_S("_get_blocal_var", get_blocal_var, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_S("create_blocal_var", make_blocal_var, SLANG_VOID_TYPE),
   MAKE_INTRINSIC(NULL, NULL, 0, 0)
};
/*}}}*/

int jed_blocal_init (void)
{
   if (-1 == SLadd_intrin_fun_table (BLocal_Intrinsics, NULL))
     return -1;
   if (-1 == SLdefine_for_ifdef("HAS_BLOCAL_VAR"))
     return -1;

   return 0;
}

#endif				       /* JED_HAS_BUFFER_LOCAL_VARS */
