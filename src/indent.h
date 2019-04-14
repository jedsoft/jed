/* Copyright (c) 1992-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef __JED_INDENT_H_
#define __JED_INDENT_H_

#if JED_HAS_DFA_SYNTAX
# include "dfasyntx.h"
#endif

typedef struct Syntax_Table_Type
{
   char *name;			       /* name of this table */
   unsigned char sgml_start_char;      /* < */
   unsigned char sgml_stop_char;       /* > */
#define JED_MAX_SYNTAX_STRING_CHARS (1+JED_LINE_IN_STRING_MAXVAL-JED_LINE_IN_STRING_MINVAL)
   /* do not change this without making other changes to buffer flags */
   unsigned char string_chars[JED_MAX_SYNTAX_STRING_CHARS];/* character for string delim */
   unsigned int num_string_chars;
   unsigned char char_char;	       /* char for char delim */
   unsigned int flags;
#define SYNTAX_NOT_CASE_SENSITIVE 	0x1
#define FORTRAN_TYPE 			0x2
#define C_COMMENT_TYPE			0x4
/* means if non-digit in first column, it is a comment */
#define TEX_LIKE_KEYWORDS 		0x8
#define EOL_COMMENT_NEEDS_WHITESPACE	0x10
#define PREPROCESS_COLOR_WHOLE_LINE	0x20
#define PREPROCESS_IGNORE_WHITESPACE	0x40
#define SINGLE_LINE_STRINGS		0x80

   /* Comments.  Each mode can have at most 2 comment styles: one for
    * multiline comments, and one for single line comments.
    * A single line comments ends at the end of the line.  Since some
    * comments must be followed by whitespace or a word delimeter, an
    * flag will be set for them.  For example, in BASIC, 'rem' denotes
    * a comment but 'remote' does not.  Sigh.
    *
    * Languages such as m4 are especially problematic.  m4 uses 'dnl' to
    * start an end-of-line comment.  However, it must occur as an isolated
    * word such that
    *    dnlsilly is not a comment
    *    and sillydnl is not a comment
    */
   char *comment_start;
   unsigned int comment_start_len;

   unsigned int comment_stop_len;
   char *comment_stop;

#define MAX_EOL_COMMENTS	2
   char *eol_comment_starts[MAX_EOL_COMMENTS];
   unsigned int eol_comment_lens[MAX_EOL_COMMENTS];
   unsigned int num_eol_comments;

   unsigned char quote_char;	       /* used for quoting in strings */
   unsigned char preprocess;	       /* start preprocessor lines */
   unsigned short char_syntax[256];     /* syntax type for characters */
   unsigned char matching_delim[256];  /* matching pairs */
#define MAX_KEYWORD_LEN 48
#define MAX_KEYWORD_TABLES 3	       /* must be less than 10 for a
					* 1-1 correspondence with keyword colors
					*/
   char *keywords[MAX_KEYWORD_TABLES][MAX_KEYWORD_LEN];
   struct Syntax_Table_Type *next;     /* pointer to next table */
#if JED_HAS_DFA_SYNTAX
   struct Highlight *hilite;
   SLang_Name_Type *init_dfa_callback;
#endif
   int use_dfa_syntax;
   unsigned char fortran_comment_chars[256];
}
Syntax_Table_Type;

#define WORD_SYNTAX		0x001
#define NUMBER_SYNTAX		0x002
#define DELIM_SYNTAX		0x004

#define SYNTAX_MASK		0xFFF8
#define QUOTE_SYNTAX		0x008
#define STRING_SYNTAX		0x010
#define OPEN_DELIM_SYNTAX	0x020
#define CLOSE_DELIM_SYNTAX	0x040
#define COMMENT_SYNTAX		0x080
#define OP_SYNTAX		0x100
#define HTML_START_SYNTAX	0x200
#define HTML_END_SYNTAX		0x400

extern Syntax_Table_Type *Default_Syntax_Table;

extern void init_syntax_tables (void);
extern void blink_match(void);
extern int goto_match(void);
extern Syntax_Table_Type *jed_find_syntax_table (char *, int);
extern int jed_init_syntax (void);

#if JED_HAS_LINE_ATTRIBUTES
extern void jed_syntax_parse_buffer (int);
#endif
extern int map_color_object_to_number (char *);

extern int _jed_is_eol_comment_start (Syntax_Table_Type *, Line *, unsigned char *, unsigned char *, unsigned int *);
#endif /* __JED_INDENT_H_ */
