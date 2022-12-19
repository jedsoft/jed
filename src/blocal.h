/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#if JED_HAS_BUFFER_LOCAL_VARS

typedef struct _Jed_BLocal_Table_Type Jed_BLocal_Table_Type;

extern int jed_blocal_init (void);
extern void jed_delete_blocal_vars (Jed_BLocal_Table_Type *);

#endif				       /* JED_HAS_BUFFER_LOCAL_VARS */
