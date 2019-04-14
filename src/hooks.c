/* Copyright (c) 1992-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

/*{{{ #include files */

#include <stdio.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <string.h>
#include <slang.h>

#include "hooks.h"
#include "misc.h"

typedef struct Hook_Type
{
   SLang_Name_Type *nt;
   /* The locking and is_valid flags are necessary to prevent the list from
    * becoming corrupt if the hook is removed by the hook
    */
   unsigned int num_locks;
   int is_valid;		       /* non-zero if not deleted */
   struct Hook_Type *next;
   struct Hook_Type *prev;
}
Hook_Type;

typedef struct Hook_List_Type
{
   char *hooks_name;		       /* slstring */
   Hook_Type *hooks;
   struct Hook_List_Type *next;
}
Hook_List_Type;

static Hook_List_Type *Hook_List;

static Hook_Type *hook_already_exists (Hook_List_Type *h,
				       SLang_Name_Type *nt)
{
   Hook_Type *next;

   next = h->hooks;
   while (next != NULL)
     {
	if (next->nt == nt)
	  break;

	next = next->next;
     }

   return next;
}

static void free_hook (Hook_List_Type *l, Hook_Type *h)
{
   Hook_Type *n, *p;

   if (h == NULL)
     return;

   if (h->num_locks)
     return;

   p = h->prev;
   n = h->next;

   if (p != NULL)
     p->next = n;
   else
     l->hooks = n;

   if (n != NULL)
     n->prev = p;

   SLfree ((char *) h);
}

static void release_hook (Hook_List_Type *l, Hook_Type *h)
{
   if (h->num_locks >= 1)
     h->num_locks--;

   if (h->is_valid == 0)
     free_hook (l, h);
}

static void lock_hook (Hook_Type *h)
{
   h->num_locks++;
}

static Hook_List_Type *new_hook_list (char *name)
{
   Hook_List_Type *h;

   h = (Hook_List_Type *) jed_malloc0 (sizeof (Hook_List_Type));
   if (h == NULL)
     return NULL;

   if (NULL == (h->hooks_name = SLang_create_slstring (name)))
     {
	SLfree ((char *) h);
	return NULL;
     }

   h->next = Hook_List;
   Hook_List = h;
   return h;
}

static Hook_List_Type *find_hook_list (char *name)
{
   Hook_List_Type *l;
   char ch;

   l = Hook_List;
   ch = name[0];
   while (l != NULL)
     {
	if ((ch == l->hooks_name[0])
	    && (0 == strcmp (l->hooks_name, name)))
	  break;

	l = l->next;
     }
   return l;
}

static int insert_hook_1 (char *hook, SLang_Name_Type *nt, int append)
{
   Hook_List_Type *l;
   Hook_Type *h, *p;

   if (NULL == (l = find_hook_list (hook)))
     {
	if (NULL == (l = new_hook_list (hook)))
	  return -1;
     }

   h = hook_already_exists (l, nt);
   if (h != NULL)
     {
	h->is_valid = 1;
	return 0;
     }

   if (NULL == (h = (Hook_Type *) jed_malloc0 (sizeof (Hook_Type))))
     return -1;

   h->nt = nt;
   h->is_valid = 1;

   p = l->hooks;
   if (append && (p != NULL))
     {
	while (p->next != NULL)
	  p = p->next;

	p->next = h;
	h->prev = p;
     }
   else
     {
	h->next = p;
	l->hooks = h;
	if (p != NULL)
	  p->prev = h;
     }
   return 0;
}

static int pop_hooks_info (char **s, SLang_Name_Type **nt)
{
   if (NULL == (*nt = SLang_pop_function ()))
     return -1;

   if (-1 == SLang_pop_slstring (s))
     return -1;

   return 0;
}

static int insert_hook (int append)
{
   SLang_Name_Type *nt;
   char *name;
   int status;

   if (-1 == pop_hooks_info (&name, &nt))
     return -1;

   status = insert_hook_1 (name, nt, append);

   SLang_free_slstring (name);
   return status;
}

static void add_hook_cmd (void)
{
   (void) insert_hook (0);
}
static void append_hook_cmd (void)
{
   (void) insert_hook (1);
}
static void remove_hook_cmd (void)
{
   char *name;
   SLang_Name_Type *nt;
   Hook_Type *h;
   Hook_List_Type *l;

   if (-1 == pop_hooks_info (&name, &nt))
     return;

   if ((NULL == (l = find_hook_list (name)))
       || (NULL == (h = hook_already_exists (l, nt))))
     {
	SLang_free_slstring (name);
	return;
     }

   h->is_valid = 0;
   free_hook (l, h);
   SLang_free_slstring (name);
}

static int execute_fun_with_args (SLang_Name_Type *nt, unsigned int argc,
				  char **argv)
{
   unsigned int i;

   (void) SLang_start_arg_list ();
   for (i = 0; i < argc; i++)
     (void) SLang_push_string (argv[i]);
   (void) SLang_end_arg_list ();

   return SLexecute_function (nt);
}

int jed_hook_exists (char *name)
{
   Hook_List_Type *l;

   return ((NULL != (l = find_hook_list (name)))
	   && (l->hooks != NULL));
}

static int jed_run_hooks (char *name, int method, unsigned int argc, char **argv)
{
   Hook_List_Type *l;
   Hook_Type *h;

   if (NULL == (l = find_hook_list (name)))
     h = NULL;
   else
     h = l->hooks;

   while (h != NULL)
     {
	int status;
	Hook_Type *next;

	if (0 == h->is_valid)
	  {
	     h = h->next;
	     continue;
	  }

	lock_hook (h);
	status = execute_fun_with_args (h->nt, argc, argv);
	next = h->next;
	release_hook (l, h);
	h = next;

	if (status == -1)
	  return -1;

	if (method == JED_HOOKS_RUN_ALL)
	  continue;

	if (-1 == SLang_pop_integer (&status))
	  return -1;

	if (method == JED_HOOKS_RUN_UNTIL_0)
	  {
	     if (status == 0)
	       return 0;
	     continue;
	  }

	/* else method = JED_HOOKS_RUN_UNTIL_NON_0 */
	if (status)
	  return 1;
     }

   if (method == JED_HOOKS_RUN_UNTIL_0)
     return 1;

   return 0;
}

static char **build_arg_list (unsigned int n, va_list ap)
{
   char **s;
   unsigned int i;

   s = (char **)SLmalloc ((n+1) * sizeof (char *));
   if (s == NULL)
     return NULL;

   for (i = 0; i < n; i++)
     s[i] = va_arg(ap, char *);
   s[n] = NULL;
   return s;
}

int jed_va_run_hooks (char *name, int method, unsigned int nargs, ...)
{
   va_list ap;
   char **arglist;
   int status;

   va_start (ap, nargs);
   if (NULL == (arglist = build_arg_list (nargs, ap)))
     {
	va_end (ap);
	return -1;
     }
   va_end (ap);

   status = jed_run_hooks (name, method, nargs, arglist);
   SLfree ((char *) arglist);
   return status;
}

static void run_hooks_cmd (void)
{
   unsigned int n;
   SLang_Array_Type *at;
   int method;
   char *hook;

   n = (unsigned int) SLang_Num_Function_Args;

   at = NULL;
   hook = NULL;
   switch (n)
     {
      case 3:
	if (-1 == SLang_pop_array_of_type (&at, SLANG_STRING_TYPE))
	  return;
	/* drop */
      case 2:
	if (-1 == SLang_pop_integer (&method))
	  goto the_return;
	if (-1 == SLang_pop_slstring (&hook))
	  goto the_return;
	break;

      default:
	SLang_verror (SL_USAGE_ERROR, "usage: expecting 2 or 3 arguments");
	return;
     }

   switch (method)
     {
      case JED_HOOKS_RUN_ALL:
      case JED_HOOKS_RUN_UNTIL_0:
      case JED_HOOKS_RUN_UNTIL_NON_0:
	break;

      default:
	SLang_verror (SL_INVALID_PARM, "run method %d is not supported", method);
	goto the_return;
     }

   if (at == NULL)
     (void) jed_run_hooks (hook, method, 0, NULL);
   else
     (void) jed_run_hooks (hook, method, at->num_elements, (char **) at->data);

   the_return:
   SLang_free_slstring (hook);
   SLang_free_array (at);
}

static SLang_Intrin_Fun_Type Hooks_Intrinsics [] =
{
   MAKE_INTRINSIC_0("add_to_hook", add_hook_cmd, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("append_to_hook", append_hook_cmd, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("remove_from_hook", remove_hook_cmd, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("_jed_run_hooks", run_hooks_cmd, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0(NULL, NULL, 0)
};

static SLang_IConstant_Type Hook_IConstants [] =
{
   MAKE_ICONSTANT("JED_HOOKS_RUN_ALL", JED_HOOKS_RUN_ALL),
   MAKE_ICONSTANT("JED_HOOKS_RUN_UNTIL_NON_0", JED_HOOKS_RUN_UNTIL_NON_0),
   MAKE_ICONSTANT("JED_HOOKS_RUN_UNTIL_0", JED_HOOKS_RUN_UNTIL_0),
   SLANG_END_ICONST_TABLE
};

int jed_init_user_hooks (void)
{
   if ((-1 == SLadd_intrin_fun_table (Hooks_Intrinsics, NULL))
       || (-1 == SLadd_iconstant_table (Hook_IConstants, NULL)))
     return -1;

   return 0;
}
