/* Copyright (c) 1992-2010 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef __JED_BUFFER_H_
#define  __JED_BUFFER_H_

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#include <slang.h>
#include "jedlimit.h"

#define JED_MAX_MULTIBYTE_SIZE SLUTF8_MAX_MBLEN
extern int Jed_UTF8_Mode;

typedef struct _Buffer Buffer;

#include "jdmacros.h"

#include "keymap.h"
#include "undo.h"
#include "blocal.h"
#if JED_HAS_MENUS
# include "menu.h"
#endif

/*
#define sprintf simple_sprintf
extern char *simple_sprintf(char *, char *, ...);
*/

typedef struct Line
{
   struct Line *next;               /* pointer to next line */
   struct Line *prev;               /* pointer to prev line */
   unsigned char *data;             /* data for the line */
   int len;                         /* actual length of line */
#ifdef KEEP_SPACE_INFO
   int space;		       /* space allocated for line */
#endif
#if JED_HAS_LINE_ATTRIBUTES
   /* The first n bits of the flags field represent an n bit integer that
    * is used by the syntax parsing routines.  The next m bits
    * control the various flags.  The last 8 bits specify the
    * color of the line.
    *
    * Finally, assuming at least 32 bit integers, the last 16 bits may be
    * used for the syntax state of the line. For instance, if the line is in
    * a string, the string character may be specified by the some of the bits,
    * which is useful if there are more than one.
    */
   unsigned int flags;
#endif
#define JED_LINE_IN_COMMENT		0x0001
#define JED_LINE_IN_STRING0		0x0002
#define JED_LINE_IN_STRING1		0x0004
#define JED_LINE_IN_HTML		0x0008
#define JED_LINE_HAS_EOL_COMMENT	0x0010
#define JED_LINE_SYNTAX_BITS		0x001F

#define JED_LINE_HIDDEN			0x0020
#define JED_LINE_IS_READONLY		0x0040
#define JED_LINE_FLAG_BITS		0x0060

#define JED_LINE_COLOR_BITS		0xFF00
#define JED_GET_LINE_COLOR(line) ((line->flags & JED_LINE_COLOR_BITS) >> 8)
#define JED_SET_LINE_COLOR(line,color) \
   (line)->flags &= ~JED_LINE_COLOR_BITS; \
   (line)->flags |= (((color) << 8) & JED_LINE_COLOR_BITS)
} Line;

#define LINE_HAS_NEWLINE(l) \
   ((l)->len && ((l)->data[(l)->len - 1] == '\n'))

/* This is the price we pay for a linked list approach.  With straight
   buffer gap, this would be an integer.  Sigh. */
typedef struct Mark
  {
     Line *line;                      /* line that marker points at */
     int point;                       /* offset from beginning */
     unsigned int n;		       /* line number in buffer */
     struct Mark *next;
     unsigned int flags;	       /* visible mark if non-zero */
#define MARK_COLOR_MASK		0x00FF
#define MARK_INVALID		0x0100
#define VISIBLE_MARK		0x0200
#define VISIBLE_COLUMN_MARK	0x0400
#define NARROW_REGION_MARK	0x0800
#define JED_LINE_MARK		0x1000

#define VISIBLE_MARK_MASK	(VISIBLE_MARK|VISIBLE_COLUMN_MARK)
  }
Mark;

#define JED_MAX_MARK_ARRAY_SIZE 5
typedef struct Jed_Mark_Array_Type
{
   struct Jed_Mark_Array_Type *next;
   unsigned int num_marks;
   Mark marks[JED_MAX_MARK_ARRAY_SIZE];
}
Jed_Mark_Array_Type;

extern unsigned int LineNum;	       /* current line number */
extern unsigned int Max_LineNum;       /* max line number */

typedef struct Narrow_Type
{
   struct Narrow_Type *next;
   unsigned int nup, ndown;	       /* (relative) lines above this narrow */
   Line *beg, *end;		       /* pointers to lines to linkup with */
   Line *beg1, *end1;		       /* beg and end before narrow */
   int is_region;
} Narrow_Type;

#if JED_HAS_SAVE_NARROW
typedef struct _Jed_Save_Narrow_Type
{
   Mark *beg, *end;
   struct _Jed_Save_Narrow_Type *next;
}
Jed_Save_Narrow_Type;
#endif

/* These are buffer local variables that slang can access */
typedef struct
{
   int tab;			       /* tab width */
   int case_search;
   int is_utf8;
   int wrap_column;
} Buffer_Local_Type;

extern Buffer_Local_Type Buffer_Local;

typedef struct
{
   SLang_Name_Type *mark_paragraph_hook;
   SLang_Name_Type *format_paragraph_hook;
   SLang_Name_Type *forward_paragraph_hook;
   SLang_Name_Type *backward_paragraph_hook;
   SLang_Name_Type *par_sep;		       /* paragraph sep function */
   SLang_Name_Type *indent_hook;
   SLang_Name_Type *newline_indent_hook;
   SLang_Name_Type *wrap_hook;
   SLang_Name_Type *wrapok_hook;
   SLang_Name_Type *bob_eob_error_hook;
#ifdef HAS_MOUSE
   SLang_Name_Type *mouse_down_hook;
   SLang_Name_Type *mouse_up_hook;
   SLang_Name_Type *mouse_drag_hook;
# if JED_HAS_MULTICLICK
   SLang_Name_Type *mouse_2click_hook;
   SLang_Name_Type *mouse_3click_hook;
# endif
#endif
   SLang_Name_Type *update_hook;
   SLang_Name_Type *color_region_hook;
}
Jed_Buffer_Hook_Type;

struct _Buffer
{
   Line *beg;			       /* Top line of buffer */
   Line *end;			       /* Bottom line */
   Line *line;		       /* current line */
   int point;			       /* current offset */
   unsigned int linenum;	       /* current line number */
   unsigned int max_linenum;	       /* lines in buffer */
   char *name;			       /* name of this buffer (slstring) */
   char *file;			       /* filename sans dir (slstring) */
   char *dir;			       /* directory of file (slstring) */
   char *dirfile;		       /* dir+file sans link expansion */
   char *canonical_dirfile;	       /* canonical filename with expanded links */
#ifdef REAL_UNIX_SYSTEM
   int device;			       /* inode and device of canonical_dirfile */
   int inode;
#endif
   int umask;
   unsigned int flags;	       /* flags  (autosave, etc...) */
   Narrow_Type *narrow;	       /* info for use by widen */
   unsigned int nup;		       /* lines above narrow (absolute) */
   unsigned int ndown;	       /* lines below narrow */
   Mark *marks;
   Mark *spots;
   Mark *user_marks;
   unsigned int modes;	       /* c-mode, wrap, etc... */
   SLKeyMap_List_Type *keymap;       /* keymap attached to this buffer */
   struct _Buffer *next;	       /*  */
   struct _Buffer *prev;
   char *mode_string;		       /* (slstring) */
   int hits;			       /* number of hits on buffer since
					* last autosave.  A hit is the number
					* of times the buffer was hit on at top level  */

   /* 0 or time when buffer first associated with a file */
   unsigned long c_time;

   Undo_Type *undo;		       /* pointer to undo ring */
   Buffer_Local_Type local_vars;

#define SPOT_ARRAY_SIZE 4

   Jed_Mark_Array_Type *spot_array;
   Jed_Mark_Array_Type *mark_array;
   int vis_marks;		       /* number of visible marks */
   char status_line[80];
   Jed_Buffer_Hook_Type *buffer_hooks;

#if JED_HAS_COLOR_COLUMNS
   unsigned int coloring_style;
   unsigned char *column_colors;
   unsigned int num_column_colors;
#endif
#if JED_HAS_ABBREVS
   struct Abbrev_Table_Type *abbrev_table;
#endif
   struct Syntax_Table_Type *syntax_table;
#if JED_HAS_SUBPROCESSES
   int subprocess;		       /* 1 + subprocess id */
   int locked;
#endif
#if JED_HAS_SAVE_NARROW
   Jed_Save_Narrow_Type *save_narrow;
#endif
#if JED_HAS_BUFFER_LOCAL_VARS
   Jed_BLocal_Table_Type *blocal_table;
#endif
#if JED_HAS_LINE_ATTRIBUTES
   unsigned int max_unparsed_line_num;
   unsigned int min_unparsed_line_num;
#endif
#if JED_HAS_MENUS
   Menu_Bar_Type *menubar;
#endif
#if JED_HAS_DISPLAY_LINE_NUMBERS
   int line_num_display_size;
#endif
};

extern char Default_Status_Line[80];

/* flags */
#define BUFFER_MODIFIED			0x0001

/* This flag cannot be used with the AUTO_SAVE_JUST_SAVE flag */
#define AUTO_SAVE_BUFFER		0x0002
/* these two flags are to tell user that the buffer and the file on disk
   have been modified--- see update_marks and main editor loop */
#define FILE_MODIFIED			0x0004
#define READ_ONLY			0x0008
#define OVERWRITE_MODE			0x0010
#define UNDO_ENABLED			0x0020

/* skip this buffer if looking for a pop up one. */
#define BURIED_BUFFER			0x0040

/* Instead of autosaving saving the buffer, just save it.  This flag
 * is only used when SIGHUP or something like that hits.  It is also
 * used when exiting the editor.  It will cause the buffer to be silently
 * saved.  It is possible that I need another flag for this.
 */
#define AUTO_SAVE_JUST_SAVE		0x0080
#define NO_BACKUP_FLAG			0x0100
#define BINARY_FILE			0x0200
#define ADD_CR_ON_WRITE_FLAG		0x0400
#define ABBREV_MODE			0x0800
#define SMG_EMBEDDED_ESCAPE		0x1000

#define BUFFER_NON_LOCKING		0x8000
#ifndef VMS
# define MAP_CR_TO_NL_FLAG		0x10000
#endif

extern char *Read_Only_Error;
extern char *Line_Read_Only_Error;

#if JED_HAS_LINE_ATTRIBUTES
#define CHECK_READ_ONLY\
    if (CBuf->flags & READ_ONLY) { msg_error(Read_Only_Error); return(1);}\
    if (CLine->flags & JED_LINE_IS_READONLY) {msg_error(Line_Read_Only_Error); return 1;}
#else
#define CHECK_READ_ONLY\
    if (CBuf->flags & READ_ONLY) { msg_error(Read_Only_Error); return(1);}
#endif

#if JED_HAS_LINE_ATTRIBUTES
#define CHECK_READ_ONLY_VOID\
    if (CBuf->flags & READ_ONLY) { msg_error(Read_Only_Error); return;}\
    if (CLine->flags & JED_LINE_IS_READONLY) {msg_error(Line_Read_Only_Error); return;}
#else
#define CHECK_READ_ONLY_VOID\
    if (CBuf->flags & READ_ONLY) { msg_error(Read_Only_Error); return;}
#endif

#define NO_MODE 0x00
#define WRAP_MODE 0x01

extern Buffer *CBuf;
extern Line *CLine;

extern void jed_set_point (int point);
extern void jed_position_point (unsigned char *);
extern unsigned char *jed_eol_position (Line *);

extern unsigned char *
  jed_multibyte_chars_forward (unsigned char *p, unsigned char *pmax,
			       unsigned int n, unsigned int *dn, int skip_combining);
extern unsigned char *
  jed_multibyte_chars_backward (unsigned char *pmin, unsigned char *p,
				unsigned int n, unsigned int *dn, int skip_combining);

/* This is roughly equivalent to (UPPERCASE(*a++) - UPPERCASE(*b++)) */
extern int jed_multibyte_charcasecmp (unsigned char **ap, unsigned char *amax,
				      unsigned char **bp, unsigned char *bmax);

/* Wide char equiv of *buf++ = c; */
unsigned char *jed_wchar_to_multibyte (SLwchar_Type c, unsigned char *buf);

extern int jed_what_char (SLwchar_Type *);
extern void jed_count_chars (void);

extern int bob(void);
extern int eob(void);                  /* point to end of buffer */
extern int bol(void);
extern int eol(void);

extern int bobp(void);
extern int eobp(void);
extern int eolp(void);
extern int bolp(void);

extern unsigned int jed_up (unsigned int);
extern unsigned int jed_down (unsigned int);

extern unsigned int jed_right (unsigned int);
extern unsigned int jed_left (unsigned int);
extern unsigned int jed_right_bytes (unsigned int);
extern void goto_line(int *);

extern Line *make_line1(unsigned int);
extern unsigned char *make_line(unsigned int);
extern unsigned char *remake_line(unsigned int);

extern Buffer *make_buffer(char *, char *, char *);
extern void uniquely_name_buffer(Buffer *, SLFUTURE_CONST char *);
extern void buffer_filename(Buffer *, SLFUTURE_CONST char *, SLFUTURE_CONST char *);
extern Buffer *find_file_buffer(char *);
extern Buffer *find_buffer(char *);
extern int delete_line(void);
extern void delete_buffer(Buffer *);
extern int switch_to_buffer(Buffer *);
extern int get_percent(void);
extern int what_line(void);
extern int erase_buffer(void);
extern void mark_buffer_modified (Buffer *, int, int);
extern Line *dup_line(Line *);
extern void free_line(Line *);
extern void check_buffers(void);
extern int buffer_exists(Buffer *);
extern int Point;
extern int Number_Zero;
extern int Number_One;
extern int Number_Two;
extern int Number_Ten;
extern void mark_undo_boundary(Buffer *);
extern void delete_undo_ring(Buffer *);

extern int Batch;		       /* JED used in batch mode. */
extern void touch_screen(void);
extern void check_line(void);

extern int jed_set_buffer_hook (Buffer *, char *, SLang_Name_Type *);
extern int jed_unset_buffer_hook (Buffer *, char *);
extern SLang_Name_Type *jed_get_buffer_hook (Buffer *, char *);

extern void jed_set_buffer_flags (Buffer *, unsigned int);
extern Buffer *MiniBuffer;
#endif
