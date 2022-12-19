/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

/* For undo, these flags assumed to be <= 0xFF */
#define CDELETE		0x1
#define CINSERT		0x2
#define LDELETE		0x4
#define NLINSERT	0x8   /* causes marks to be moved to next line */
#define NLDELETE	0x10    /* opposite of above */
#define UNDO_POSITION	0x20	       /* records position */

extern void jed_update_marks(int, int);
extern int No_Screen_Update;
extern int jed_del_nbytes (int);
extern int jed_generic_del_nbytes (int);
extern int jed_del_wchar(void);
extern int jed_del_through_eol(void);
extern int _jed_ins_byte (unsigned char);   /* \n will not split the line */
extern int jed_insert_wchar (SLwchar_Type);
extern int jed_insert_byte (unsigned char);
extern void insert_buffer(Buffer *);
extern int jed_quick_insert(register unsigned char *, int);
extern int jed_insert_nbytes(unsigned char *, int);
extern int jed_insert_wchar_n_times (SLwchar_Type, unsigned int);
extern int jed_del_newline(void);
extern int _jed_replace_wchar (SLwchar_Type);   /* \n not allowed */
extern int jed_insert_string (SLFUTURE_CONST char *s);
extern int jed_insert_newline (void);
extern int Suspend_Screen_Update;
extern int jed_prepare_for_modification (int);
