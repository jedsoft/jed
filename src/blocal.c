/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2010 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "jed-feat.h"

#if JED_HAS_BUFFER_LOCAL_VARS

#include <slang.h>

#include "jdmacros.h"
#include "buffer.h"
#include "misc.h"

static Jed_BLocal_Type *find_blocal_var (char *name, int err) /*{{{*/
{
   Jed_BLocal_Table_Type *table;
   Jed_BLocal_Type *lv, *lv_max;

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

int jed_blocal_var_exists (char *name)
{
   return (NULL != find_blocal_var (name, 0));
}

void jed_make_blocal_var (char *name) /*{{{*/
{
   Jed_BLocal_Table_Type *table;
   Jed_BLocal_Type *lv;

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
   Jed_BLocal_Type *lv, *lv_max;
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

void jed_set_blocal_var (char *name) /*{{{*/
{
   Jed_BLocal_Type *lv;

   lv = find_blocal_var (name, 1);
   if (lv == NULL)
     return;

   SLang_free_anytype (lv->value);
   lv->value = NULL;

   (void) SLang_pop_anytype (&lv->value);
}

/*}}}*/

void jed_get_blocal_var (char *name) /*{{{*/
{
   Jed_BLocal_Type *lv;

   lv = find_blocal_var (name, 1);
   if (lv == NULL)
     return;

   (void) SLang_push_anytype (lv->value);
}

/*}}}*/

#endif				       /* JED_HAS_BUFFER_LOCAL_VARS */
