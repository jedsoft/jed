/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef JED_FEATURES_H
#define JED_FEATURES_H

/* If you want folding capibility, you MUST set JED_HAS_LINE_ATTRIBUTES to 1.
 * In addition, JED_HAS_LINE_ATTRIBUTES must also be set to 1 for
 * syntax highlighting to work properly. This adds an additional 4
 * bytes per line overhead.  You must also set JED_HAS_SAVE_NARROW to
 * 1.  It is also a good idea to set JED_HAS_BUFFER_LOCAL_VARS to 1
 * since that will allow fold marks to vary on a buffer-by-buffer
 * basis instead of a mode-by-mode basis. Summary: for folding or
 * syntax highlighting, set the next 3 variables to 1.
 */
#define JED_HAS_LINE_ATTRIBUTES		1
#define JED_HAS_BUFFER_LOCAL_VARS	1
#define JED_HAS_SAVE_NARROW		1

#define JED_HAS_DISPLAY_LINE_NUMBERS	1

/* Drop down menu support. */
#define JED_HAS_MENUS		1

/* See ../doc/filelock.txt --- requires symbolic links */
#define JED_HAS_EMACS_LOCKING		1

/* Double/Triple click support.  This is currently supported by:
 *
 *   X Windows
 *   GPM Mouse Driver (Linux)
 *   DJGPP version of jed
 */
#if defined(__unix__) || defined(VMS) || defined(__GO32__) || defined(MSWINDOWS)
# define JED_HAS_MULTICLICK		1
#else
# define JED_HAS_MULTICLICK		0
#endif

/*  Asynchronous subprocess support.  This is only available for Unix systems.
 */
#if defined(REAL_UNIX_SYSTEM) || defined(__WIN32__)
#  define JED_HAS_SUBPROCESSES		1
#else
# define JED_HAS_SUBPROCESSES		0
#endif

#ifdef NeXT
# undef JED_HAS_SUBPROCESSES
# define JED_HAS_SUBPROCESSES		0
#endif

/* Enhanced syntax highlighting support.  This is a much more sophisticated
 * approach based on regular expressions.  Experimental.
 */
#define JED_HAS_DFA_SYNTAX		1

/* Set JED_HAS_ABBREVS to 1 for the abbreviation feature. */
#define JED_HAS_ABBREVS			1
#define JED_HAS_COLOR_COLUMNS		1
#define JED_HAS_LINE_MARKS		1

/* The following only applies to systems that have case-insensitive file
 * systems.
 *
 * Are filenames case sensitive? If this is set to 1 Ma* will not match
 * makefile, even on systems that are not case sensitive by default.
 */
#define	JED_FILENAME_CASE_SENSITIVE	0

/* If compiled with XJED_SET_WM_COMMAND non-zero, then Xjed
 * will set the WM_COMMAND property to inform the
 * window manager or session manager of the command line used to
 * start the application.  Not all versions of X support this.
 */
#define XJED_SET_WM_COMMAND	0

#if JED_HAS_EMACS_LOCKING && !defined(HAVE_SYMLINK)
# undef JED_HAS_EMACS_LOCKING
# define JED_HAS_EMACS_LOCKING 0
#endif

/* Set JED_HAS_IMPORT if you want the ability to import modules into jed via
 * the slang import statement.  This assumes that slang was compiled with
 * such support.
 */
#define JED_HAS_IMPORT	1

#endif
