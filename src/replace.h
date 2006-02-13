/* Copyright (c) 1992, 1998, 2000, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
extern int Replace_Preserve_Case;
extern SLang_Intrin_Fun_Type Jed_Other_Intrinsics [];
#ifndef SIXTEEN_BIT_SYSTEM
extern void append_region_to_kill_array (int *);
extern void prepend_region_to_kill_array (int *);
extern void insert_from_kill_array (int *);
extern void copy_region_to_kill_array (int *);
extern int Kill_Array_Size;
#endif
