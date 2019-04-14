/* Copyright (c) 1992-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#if JED_HAS_BUFFER_LOCAL_VARS

typedef struct
{
   char *name;			       /* slstring */
   SLang_Any_Type *value;
}
Jed_BLocal_Type;

#define MAX_BLOCAL_VARS_PER_TABLE	10
typedef struct _Jed_BLocal_Table_Type
{
   unsigned int num;
   Jed_BLocal_Type local_vars[MAX_BLOCAL_VARS_PER_TABLE];
   struct _Jed_BLocal_Table_Type *next;
}
Jed_BLocal_Table_Type;

extern void jed_delete_blocal_vars (Jed_BLocal_Table_Type *);
extern void jed_make_blocal_var (char *);
extern void jed_set_blocal_var (char *);
extern void jed_get_blocal_var (char *);
extern int jed_blocal_var_exists (char *);

#endif				       /* JED_HAS_BUFFER_LOCAL_VARS */
