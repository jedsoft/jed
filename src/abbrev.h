/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
extern void use_abbrev_table (char *);
extern void define_abbrev (char *, char *, char *);
extern void create_abbrev_table (char *, char *);
extern int jed_expand_abbrev (SLwchar_Type);
extern int abbrev_table_p (char *);
extern void dump_abbrev_table (char *);
extern void what_abbrev_table (void);
extern int list_abbrev_tables (void);
extern void delete_abbrev_table (char *);

