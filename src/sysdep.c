/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

/*  This file is the interface to system specific files */
#include "config.h"
#include "jed-feat.h"
/*{{{ Include Files */

#include <stdio.h>
#include <string.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#define __USE_XOPEN_EXTENDED	       /* to get getpgid */
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef __WIN32__
/* windows.h must be included before slang.h */
# include <windows.h>
#endif

#include <slang.h>

#include "jdmacros.h"

#ifdef POSIX
# ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE
# endif
#endif

#ifdef __WIN32__
# ifdef VOID 
#  undef VOID
# endif
# if !defined(__CYGWIN32__)
#  define sleep Sleep
# endif
# undef _POSIX_SOURCE
# include <sys/stat.h>
# ifdef __BORLANDC__
#  include <dir.h>
# endif
# if defined(__MINGW32__) || defined(__IBMC__)
#  include <direct.h>
# endif
# ifdef __IBMC__
extern unsigned int sleep (unsigned int);
# endif
#endif

#include "buffer.h"
#include "sysdep.h"
#include "display.h"
#include "file.h"
#include "screen.h"
#include "misc.h"
#include "hooks.h"
/*}}}*/

/* These are hooks for porting to other systems */

int (*X_Read_Hook) (void);
int (*X_Input_Pending_Hook) (void);
int (*X_Init_Term_Hook) (void);
void (*X_Reset_Term_Hook) (void);

int Ignore_User_Abort = 1;	       /* Abort char triggers S-Lang error */
char *_Jed_Backspace_Key = "\x7F";

void (*Jed_Sig_Exit_Fun) (void);

#ifdef IBMPC_SYSTEM
int Jed_Filename_Case_Sensitive = 0;
#endif

unsigned char KeyBoard_Xlate[256] = /*{{{*/
{
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
     21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
     40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
     59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77,
     78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
     97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
     113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
     128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
     143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157,
     158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172,
     173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
     188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202,
     203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
     218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232,
     233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
     248, 249, 250, 251, 252, 253, 254, 255
};

/*}}}*/

#if defined(__WIN32__)
# define SLASH_CHAR '\\'
# include "win32.c"
#else
# if defined (__MSDOS__) || defined(MSWINDOWS)
#  define SLASH_CHAR '\\'
#  if defined (__WATCOMC__) || defined (__GO32__)
#   include	"i386.c"
#  else
#   if defined(MSWINDOWS)
#    include   "win16.c"
#   else
#    include	"ibmpc.c"
#   endif
#  endif
# else
#  if defined (__os2__)
#   define SLASH_CHAR '\\'
#   include "os2.c"
#  else
#   if defined (VMS)
#    define SLASH_CHAR ']'
#    include "vms.c"
#   else
#    define SLASH_CHAR '/'
#    include "unix.c"
#   endif
#  endif
# endif
#endif

int Input_Buffer_Len = 0;
unsigned char Input_Buffer[MAX_INPUT_BUFFER_LEN];

void map_character(int *fromp, int *top) /*{{{*/
{
   int from = *fromp, to = *top;
   if ((from > 255) || (to > 255) || (from < 0) || (to < 0)) return;
   KeyBoard_Xlate[from] = to;
}

/*}}}*/


/* if input char arrives with hi bit set, it is replaced by 2 characters:
 *   Meta_Char + char with hi bit off.  If Meta_Char is -1, then return 
 * full 8 bits which self inserts */

/* By default, 8 bit chars self insert. */	 
int Meta_Char = -1;
#if !defined(IBMPC_SYSTEM)
int DEC_8Bit_Hack = 64;
#else
int DEC_8Bit_Hack = 0;
#endif

int my_getkey() /*{{{*/
{
   char buf[10];
   int i;
#if 0
   int imax;
#endif
   unsigned char ch;
   int eightbit_hack;

   if (Batch)
     {
	fgets(buf, 9 ,stdin);
	return (int) (unsigned char)*buf;
     }

   /* Apparantly this hack causes problems when dealing with a unicode-aware
    * terminal.
    */

   eightbit_hack = DEC_8Bit_Hack;
   if (Jed_UTF8_Mode)
     eightbit_hack = 0;
   if (!Input_Buffer_Len)
     {
	/* if (Batch) ch = (unsigned char) getc(stdin); else ch = sys_getkey(); */
	ch = (unsigned char) KeyBoard_Xlate[sys_getkey()];
	if (ch & 0x80)
	  {
	     i = (ch & 0x7F);
	     if ((i < ' ') && eightbit_hack)
	       {
		  i += eightbit_hack;
		  ungetkey((int *) &i);
		  ch = 27;
	       }
	     else if (Meta_Char != -1)
	       {
		  ungetkey((int *) &i);
		  ch = Meta_Char;		       /* escape char */
	       }
	  }
	return((int) ch);
     }
   
   ch = Input_Buffer[0];
   if ((ch & 0x80) && ((Meta_Char != -1) || ((ch < 160) && eightbit_hack)))
     {
	ch = (ch & 0x7F);
	if ((ch < ' ') && eightbit_hack)
	  {
	     ch += eightbit_hack;
	     i = 27;
	  }
	else i = Meta_Char;
	
	Input_Buffer[0] = ch;
	return ((int) (unsigned int) i);
     }
   
#ifdef USING_INPUT_BUFFER
   USING_INPUT_BUFFER
#endif

   Input_Buffer_Len--;
#if 1
   memmove (Input_Buffer, Input_Buffer+1, Input_Buffer_Len);
#else
   for (i = 0; i < imax; i++)
     Input_Buffer[i] = Input_Buffer[i+1];
#endif
   /* SLMEMCPY ((char *) Input_Buffer, (char *) (Input_Buffer + 1), imax); */
   
#ifdef DONE_WITH_INPUT_BUFFER
   DONE_WITH_INPUT_BUFFER
#endif     
     return((int) ch);
}

/*}}}*/

void ungetkey_string(char *s, int n) /*{{{*/
{
   /* int i; */
   register unsigned char *bmax, *b, *b1;
   
   /* FIXME!! This should affect the keyboard macro buffer. */
   if (Executing_Keyboard_Macro) return;

   if (n + Input_Buffer_Len > MAX_INPUT_BUFFER_LEN - 3) return;
   
#ifdef USING_INPUT_BUFFER
   USING_INPUT_BUFFER
#endif
     
     b = Input_Buffer;
   bmax = b + (Input_Buffer_Len - 1);
   b1 = bmax + n;
   while (bmax >= b) *b1-- = *bmax--;
   bmax = b + n;
   while (b < bmax) *b++ = (unsigned char) *s++;
   Input_Buffer_Len += n;
   
#ifdef DONE_WITH_INPUT_BUFFER
   DONE_WITH_INPUT_BUFFER
#endif
}

/*}}}*/

void buffer_keystring(char *s, int n) /*{{{*/
{
   if (n + Input_Buffer_Len > MAX_INPUT_BUFFER_LEN - 3) return;
   
#ifdef USING_INPUT_BUFFER
   USING_INPUT_BUFFER
# if 0
     ;
# endif
#endif
   SLMEMCPY ((char *) Input_Buffer + Input_Buffer_Len, s, n);
   Input_Buffer_Len += n;
#ifdef DONE_WITH_INPUT_BUFFER
   DONE_WITH_INPUT_BUFFER
#endif
}

/*}}}*/

void ungetkey(int *ci) /*{{{*/
{
   char ch;
   ch = (char) *ci;
   ungetkey_string(&ch, 1);
}

/*}}}*/

int input_pending (int *tsecs) /*{{{*/
{
   int n;
   int c;
   
   /* FIXME!!  This should affect the macro buffer */
   if (Executing_Keyboard_Macro) return 1;

   if (Jed_Sig_Exit_Fun != NULL)
     (*Jed_Sig_Exit_Fun) ();

   if (Input_Buffer_Len) return Input_Buffer_Len;
   
   n = sys_input_pending (tsecs, 0);
   if (n < 0) 
     {
	if (SLKeyBoard_Quit)
	  n = 1;
	else
	  n = 0;
     }
   
   if (n && (Input_Buffer_Len == 0))
     {
	c = my_getkey ();
	ungetkey (&c);
     }
   return n;
}

/*}}}*/

#if JED_HAS_SUBPROCESSES
void get_process_input (int *t) /*{{{*/
{
   (void) sys_input_pending (t, -1);
}

/*}}}*/

#endif

void flush_input () /*{{{*/
{
   int quit = SLKeyBoard_Quit;
   Input_Buffer_Len = 0;
   SLKeyBoard_Quit = 0;
   if (Executing_Keyboard_Macro == 0)
     {
#ifdef __MSDOS__
	while (input_pending(&Number_Zero)) if (!my_getkey()) my_getkey();
#else
# ifdef __os2__
	sys_flush_input();
# endif
	while (input_pending(&Number_Zero)) my_getkey();
#endif
#ifdef HAS_MOUSE
	jed_flush_mouse_queue ();
#endif
     }
   SLKeyBoard_Quit = quit;
}

/*}}}*/

#include <time.h>

unsigned long sys_time(void) /*{{{*/
{
   return((unsigned long) time((time_t *) 0));
}

/*}}}*/


char *slash2slash(char *dir) /*{{{*/
{
#ifndef VMS
   register char *p = dir, ch;
   
   while ((ch = *p) != 0)
     {
	if ((ch == '/') || (ch == '\\')) *p = SLASH_CHAR;
      	p++;
     }
#endif
   return(dir);
}

/*}}}*/

/* given a canonical filename, return pointer to its name.  
 * Note: If the file ends in a slash as in a/b/c/, then a pointer to
 * the END of the string is returned.
 */
char *extract_file(SLFUTURE_CONST char *file) /*{{{*/
{
   SLFUTURE_CONST char *f;
   
   f = file + strlen(file);
   while (f > file)
     {
	f--;
	if (*f == SLASH_CHAR) return (char *)f + 1;
     }
   return (char *) file;
}

/*}}}*/

/* given a canonical filename, return pointer to its name.  
 * Note: If the file ends in a slash as in a/b/c/, then a pointer to
 * the c/ is returned.
 */
char *jed_extract_file_or_dir (char *file) /*{{{*/
{
   char *f;
   
   f = file + strlen(file);
   if (f > file) 
     {
	f--;
	if (*f == SLASH_CHAR)
	  f--;
     }

   while (f > file)
     {
	f--;
	if (*f == SLASH_CHAR) return f + 1;
     }
   return (file);
}

/*}}}*/
#ifdef IBMPC_SYSTEM
/* I do not know how to do this in a portable fashion.  
 */
static int pcsystem_getdisk (void)
{
#if defined(__MINGW32__) || defined(__IBMC__)
   return _getdrive ();
#else
# if defined(__DJGPP__)
   int d;  _dos_getdrive (&d); return d;
# else
#  if defined(__BORLANDC__)
   return 1 + getdisk();
#  else
   /* This may not work */
   char *cwd = jed_get_cwd ();

   if ((cwd == NULL) || (*cwd == 0) || (cwd[1] != ':'))
     cwd = "C:";

   return 1 + (UPPER_CASE (*cwd) - 'A');
#  endif			       /* __BORLANDC__ */
# endif				       /* __DJGPP__ */
#endif				       /* __MINGW32__ */
}
#endif				       /* IBMPC_SYSTEM */

#ifndef VMS

static void str_move_bytes (char *a, char *b)
{
   while (*b != 0)
     *a++ = *b++;
   *a = 0;
}

static char *strcat_malloc (char *a, char *b)
{
   char *c;
   unsigned int lena = strlen (a);
   unsigned int lenb = strlen (b);
   
   if (NULL == (c = SLmalloc (lena + lenb + 1)))
     return NULL;
   
   strcpy (c, a);
   strcpy (c+lena, b);
   return c;
}
   
/* file is assumed to be malloced here.  Its contents may be changed. */
static int remove_slash_slash (char *file)
{
   char *f, *g, ch;
   
   g = f = file;
   while ((ch = *f++) != 0)
     {
	*g++ = ch;
	if (ch != SLASH_CHAR)
	  continue;

	while (*f == SLASH_CHAR)
	  f++;
     }
   *g = 0;
   return 0;
}

/* /foo/blah/../bar --> /foo/bar
 * /foo/blah/./bar --> /foo/blah/bar
 */
static int remove_dot_dot_slashes (char *file)
{
   char *f = file;
   
   while (1)
     {
	char *f1;

	while (*f && (*f != SLASH_CHAR))
	  f++;
	if (*f == 0)
	  break;
	f++;
	if (*f != '.')
	  continue;

	/* foo/./bar -> foo/bar */
	if (f[1] == SLASH_CHAR)
	  {
	     str_move_bytes (f, f+2);
	     f--;
	     continue;
	  }
	/* foo/. --> foo/ */
	if (f[1] == 0)
	  {
	     *f = 0;
	     continue;
	  }

	if (f[1] != '.')
	  {
	     f++;
	     continue;
	  }

	f1 = f;
	if (f[2] == SLASH_CHAR)
	  f1 = f + 3;		       /* /../ */
	else if (f[2] == 0)
	  f1 = f + 2;		       /* /.. */
	else
	  continue;

	/* /foo/blah/../bar. 
	 * Move bar to /blah 
	 */
	f--;
	if (f != file)
	  f--;
	while ((f != file) && (*f != SLASH_CHAR))
	  f--;
	str_move_bytes (f+1, f1);
     }
   return 0;
}

/* This function may free its argument and returned a new malloced copy */
static char *make_abs_filename (char *file)
{
# if defined(IBMPC_SYSTEM)
   char buf[4];
# endif
   char *dir, *file1;

   if (*file != SLASH_CHAR)
     {
# if defined(IBMPC_SYSTEM)
	if (*file && (file[1] == ':'))
	  {
	     if (file[2] == SLASH_CHAR)
	       return file;
	     /* Insert a slash */
	     buf[0] = file[0]; buf[1] = ':'; buf[2] = SLASH_CHAR; buf[3] = 0;
	     file1 = strcat_malloc (buf, file+2);
	     SLfree (file);
	     return file1;
	  }
   
# endif
	/* Otherwise it looks like a relative path.  */
	/* get_cwd returns an absolute path with the trailing /. */
	dir = jed_get_cwd ();
	file1 = strcat_malloc (dir, file);
	SLfree (file);
	if (file1 == NULL)
	  return NULL;
	file = file1;
     }

# if defined(IBMPC_SYSTEM)
   /* If file is not //share/foo, then prepend drive specifier */
   if (file[0] == SLASH_CHAR)
     {
	if (file[1] != SLASH_CHAR)
	  {	     
	     buf[0] = 'A' + (pcsystem_getdisk ()-1); buf[1] = ':'; buf[2] = 0;
	     file1 = strcat_malloc (buf, file);
	     SLfree (file);
	     return file1;
	  }
	/* Make sure the //share ends with a slash. */
	file1 = file + 2;
	while (*file1 && (*file1 != SLASH_CHAR))
	  file1++;
	if (*file1 != SLASH_CHAR)
	  {
	     buf[0] = SLASH_CHAR; buf[1] = 0;
	     file1 = strcat_malloc (file, buf);
	     SLfree (file);
	     return file1;
	  }
     }
# endif
   return file;
}

static char *skip_drive_specifier (char *file)
{
#ifdef __QNX__
   /* QNX pathnames look like //<node number>/rest_of_path 
    *  -- we need to leave on the <node number> and return /rest_of_path
    */
   if ((file[0] == SLASH_CHAR) && (file[1] == SLASH_CHAR)
       && (file[2] >= '1') && (file[2] <= '9'))
     {
	file += 2;
	while (*file && (*file != SLASH_CHAR))
	  file++;
     }
   return file;
#endif			       /* __QNX__ */

#ifdef IBMPC_SYSTEM
   if (((file[0] != 0) && (file[1] == ':'))
       || ((file[0] == SLASH_CHAR) && (file[1] == SLASH_CHAR)))
     {	
	file += 2;
	while (*file && (*file != SLASH_CHAR))
	  file++;
     }
#endif

   return file;
}

static char *prep_filename (SLFUTURE_CONST char *filename)
{
   unsigned int len;
   char *file;

   len = strlen (filename);

#ifdef MSWINDOWS
   /* Strip quotes from around the filename, if any */
   if ((len >= 2) && (filename[0] == '"') && (filename[len - 1] == '"'))
     {
	len -= 2;
	filename++;
     }
#endif

   if (NULL == (file = SLmake_nstring (filename, len)))
     return NULL;

#ifndef __unix__
   slash2slash (file);
#endif
   return file;
}

char *jed_standardize_filename (SLFUTURE_CONST char *filename)
{
   char *file1, *file;
   
   if (NULL == (file = prep_filename (filename)))
     return NULL;

   if (NULL == (file = make_abs_filename (file)))
     return NULL;
   
   file1 = skip_drive_specifier (file);

   (void) remove_slash_slash (file1);

   (void) remove_dot_dot_slashes (file1);
   
   return file;
}

   
/* this routine returns a Static pointer which is considered volatile */
char *jed_standardize_filename_static (char *file) /*{{{*/
{
   static char filebuf [JED_MAX_PATH_LEN];

   *filebuf = 0;
   if (NULL != (file = jed_standardize_filename (file)))
     {
	safe_strcpy (filebuf, file, JED_MAX_PATH_LEN);
	SLfree (file);
     }
   return filebuf;
}
/*}}}*/

#endif /* ! VMS */

#ifdef sequent
char *my_strstr(char *a, char *b) /*{{{*/
{
   register char *bb, *aa, *amax;
   
   if (*b == 0) return(a);
   
   bb = b; while (*bb) bb++;
   aa = a; while (*aa++);
   
   amax = aa - (bb - b);
   
   while (a < amax)
     {
	bb = b;
	while ((a < amax) && (*a != *bb)) a++;
	if (a == amax) return((char *) NULL);
	
	aa = a;
	while (*aa && (*aa == *bb)) aa++, bb++;
	if (! *bb) return(a);
	
	a++;
     }
   return((char *) NULL);
}

/*}}}*/
#endif


void deslash(char *dir) /*{{{*/
{
#ifndef VMS
   int n;
   
   if ((n = strlen(dir)) > 1) 
     {
	n--;
# if defined (IBMPC_SYSTEM)
	if ( (dir[n] == '\\' || dir[n] == '/') && dir[n - 1] != ':' )
	  dir[n] = '\0';
# else
	if ( dir[n] == '/' )
	  dir[n] = '\0';
# endif
     }
#endif /* !VMS */
}

/*}}}*/

/* add trailing slash to dir */
void fixup_dir(char *dir) /*{{{*/
{
#ifndef VMS
   int n;
   
   if ((n = strlen(dir)) >= 1)
     {
	n--;
# if defined(IBMPC_SYSTEM)
	if ( dir[n] != '/' && dir[n] != '\\' )
	  strcat(dir, "\\" );
# else
	if ( dir[n] != '/' )
	  strcat(dir, "/" );
# endif
     }
#endif /* !VMS */
}

/*}}}*/

 /* ch_dir routine added during OS/2 port in order to
    simplify script writing. */

int ch_dir(char *path) /*{{{*/
{
#if defined(IBMPC_SYSTEM) || defined(__os2__)
   char work[JED_MAX_PATH_LEN];
   
   safe_strcpy(work, path, sizeof (work));
   deslash(work);
   return chdir(work);
#else
   return chdir(path);
#endif
}

/*}}}*/

/* generate a random number */
int make_random_number (int *seed, int *max) /*{{{*/
{
   static unsigned long s;
   int mmax;

   if (*seed == -1)		       /* generate seed */
     s = (unsigned long) (time(0) + getpid());
   else if (*seed != 0)
     s = *seed;

   if ((mmax = *max) < 2) mmax = 2;

   s = s * 69069UL + 1013904243UL;

   s = s & 0xFFFFFFFFUL;
   return (int) (mmax * (double)s/4294967296.0);
}

/*}}}*/


#if defined(IBMPC_SYSTEM)
/* This routine converts  C:\  --> C:\ and C:\subdir\  -> C:\subdir */
char *msdos_pinhead_fix_dir(char *f) /*{{{*/
{
   static char file[JED_MAX_PATH_LEN];
   register char ch;
   int n;
   
   if (*f == 0) return f;
   strncpy (file, f, JED_MAX_PATH_LEN); file[JED_MAX_PATH_LEN - 1] = 0;
   f = file;
   /* skip past colon */
   while (((ch = *f) != 0) && (ch != ':')) f++;
   
   if (ch == 0)			       /* no colon */
     {
	n = (int) (f - file);
	f = file;
     }
   else
     {
	f++;
	n = strlen (f);
     }
   if (n == 0)
     {
	*f++ = '\\'; *f = 0;
	return file;
     }
   if ((n == 1) && (*f == '\\')) return file;
   
   f += n - 1;
   if (*f == '\\') *f = 0;
   return file;
}

/*}}}*/

#endif

#ifdef __WIN32__
int jed_win32_rename (char *old_name, char *new_name)
{
   /* Some versions of win95 have a bug in rename
    * Nick Tatham discovered this and provided the patch:
    *  C:\> echo try1 >temp.txt
    *  C:\> ren temp.txt temp.txt~
    *  C:\> echo try2 >temp.txt
    */
   
   /* The work-around is to rename the file in two stages:
    *   file.typ ==> file.~ ==> file.typ~
    */
   
   char tmp_name[1024];
   char *p;
   unsigned int n, extlen;
   unsigned int num_tries;

   p = extract_file (old_name);
   while (*p && (*p != '.')) p++;      /* find extension */
   /* create temporary filename */
   /* everything up to potential dot */
   
   n = (unsigned int) (p - old_name);
   extlen = strlen (p);

   if ((n + 10 + extlen) > sizeof (tmp_name))
     {
#ifdef ENAMETOOLONG
	errno = ENAMETOOLONG;
#endif
	return -1;
     }
   strncpy (tmp_name, old_name, n);

   num_tries = 0xFFF;
   while (num_tries != 0)
     {
	struct stat st;

	sprintf (tmp_name + n, "~~~~~~%03X%s", num_tries, p);
	if (0 == stat (tmp_name, &st))
	  {
	     num_tries--;
	     continue;
	  }
	if (-1 == rename (old_name, tmp_name))
	  return -1;
	
	if (-1 == rename (tmp_name, new_name))
	  {
	     (void) rename (tmp_name, old_name);
	     return -1;
	  }
	
	return 0;
     }
#ifdef EFAULT
   /* Any better idea? */
   errno = EFAULT;
#endif
   return -1;
}

#endif
void jed_pause (int *ms) /*{{{*/
{
   if (*ms < 0)
     return;
   
   sys_pause (*ms);
}

/*}}}*/



int jed_handle_interrupt (void)
{
   return SLang_handle_interrupt ();
}

void jed_sleep (unsigned int n)
{
   sleep (n);
}


char *jed_get_cwd (void) /*{{{*/
{
   static char cwd[JED_MAX_PATH_LEN];
   char *c;

#ifdef HAVE_GETCWD
# if defined (__EMX__)
   c = _getcwd2(cwd, sizeof(cwd));		       /* includes drive specifier */
# else
   c = getcwd(cwd, sizeof(cwd));		       /* djggp includes drive specifier */
# endif
#else
   c = (char *) getwd(cwd);
#endif
   
   if (c == NULL)
     {
#ifndef REAL_UNIX_SYSTEM
	exit_error ("Unable to get the current working directory.", 0);
#else
	struct stat st1, st2;
	
	if ((NULL == (c = getenv ("PWD")))
	    || (-1 == stat (c, &st1))
	    || (-1 == stat (".", &st2))
	    || (st1.st_dev != st2.st_dev)
	    || (st1.st_ino != st2.st_ino))
	  {
	     jed_vmessage (1, "Unable to get CWD.  Assuming /");
	     if (-1 == chdir ("/"))
	       exit_error ("Unable to change directory to /", 0);
	     c = "/";
	  }

	strncpy (cwd, c, sizeof (cwd));
	cwd [sizeof (cwd) - 1] = 0;
#endif
     }

#ifndef VMS
   slash2slash(cwd);
   fixup_dir(cwd);
#endif
   return(cwd);
}

/*}}}*/

#ifndef VMS

static char *prepend_home_dir (char *file)
{
   char *home;

   home = getenv ("HOME");
   if ((home == NULL) || (*home == 0))
     home = "/";
   
   file = strcat_malloc (home, file);
   if (file == NULL)
     return file;
   
   home = jed_standardize_filename (file);
   SLfree (file);
   return home;
}


/* This function makes the following transformations:
 * 
 *     /foo/bar//root/baz   --> /root/baz
 *     /foo/bar/~/foo       --> $HOME/foo
 * 
 * and then converts the result to standard form.
 * Possible cases include:
 * 
 *    /foo/bar//tmp/x.c    --> /tmp/x.c
 *    /foo/bar/~/me/baz    --> $HOME/me/baz
 *    /foo/bar//tmp/~/bar  --> /tmp/~/bar
 *    ~/bar                --> $HOME/bar
 *    /tmp/~/bar//home/me  --> /home/me
 * 
 * DOS/Windows:
 * 
 *    C:/foo/bar/D:/foo     --> D:/foo
 *    //foo/bar///share/baz --> //share/baz
 * 
 * For the case: /tmp/~/bar/~/foo, assume ~/foo.
 */
char *jed_expand_filename (char *file)
{
   char *f;
   char *drive;

   if (NULL == (file = prep_filename (file)))
     return NULL;

   drive = NULL;
   f = skip_drive_specifier (file);

   /* Now look for // */
   while (1)
     {
	char *f1;

	while (*f && (*f != SLASH_CHAR))
	  f++;
	if (*f == 0)
	  break;
	f++;
	if (*f != SLASH_CHAR)
	  {
	     f1 = skip_drive_specifier (f);
	     if (f1 == f)
	       continue;
	  }
	drive = f;
	break;
     }
   
   if (drive != NULL)
     {
	f = jed_standardize_filename (drive);
	SLfree (file);
	return f;
     }
   
   /* Now look for /~/ */
   f = file + strlen (file);
   while (f != file)
     {
	while (f > file)
	  {
	     f--;
	     if (*f == '~')
	       break;
	  }
	if (*f != '~')
	  break;
	if (f[1] != SLASH_CHAR)
	  continue;
	if ((f != file) && (*(f-1) != SLASH_CHAR))
	  continue;
	
	f = prepend_home_dir (f + 1);
	SLfree (file);
	return f;
     }

   f = jed_standardize_filename (file);
   SLfree (file);
   return f;
}

#endif				       /* !VMS */
