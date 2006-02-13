/* Copyright (c) 1992, 1998, 2000, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
extern int wrap_line(int);
extern int forward_paragraph(void);
extern void set_paragraph_separator (char *);
extern int backward_paragraph(void);
extern int text_format_paragraph(void);
extern int narrow_paragraph(void);
extern int center_line(void);
extern int text_smart_quote(void);
extern void define_word(char *);
extern void transform_region(int *);
extern void skip_word_chars(void);
extern void bskip_word_chars(void);
extern void skip_non_word_chars(void);
extern void bskip_non_word_chars(void);
extern void jed_skip_chars (char *);
extern void jed_bskip_chars (char *);

extern char Jed_Word_Range[256];
extern char *jed_get_word_chars (void);
