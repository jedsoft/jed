/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"

#define RGREP_MAJOR_VERSION 2
#define RGREP_MINOR_VERSION 1

#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#include <string.h>


#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* For isatty */
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef PATH_MAX
# define JED_MAX_PATH_LEN PATH_MAX
#else
# ifdef IBMPC_SYSTEM
#  define JED_MAX_PATH_LEN 256
# else
#  define JED_MAX_PATH_LEN 1024
# endif
#endif


#ifdef __MSDOS__
# include <io.h>
#endif

#include "vfile.h"

static int Binary_Option = 0;
static int Case_Sensitive = 1;
static int File_Name_Only;
static int Do_Recursive = 0;
static int Recursive_Match = 0;
static int Highlight = 0;
static int Output_Match_Only = 0;
static int Count_Matches = 0;
static int Line_Numbers = 0;
static int Follow_Links = 0;
static int Debug_Mode = 0;
static char *Match_This_Extension;
static int Print_Non_Matching_Lines = 0;
#ifdef __unix__
static int Stdout_Is_TTY;
#endif

#define HON_STR "\033[1m"
#define HON_STR_LEN 4
#define HOFF_STR "\033[0m"
#define HOFF_STR_LEN 4

#if SLANG_VERSION < 10410
int SLang_get_error (void)
{
   return SLang_Error;
}

int SLang_set_error (int e)
{
   SLang_Error = e;
   return 0;
}
#endif

extern int jed_handle_interrupt (void);
int jed_handle_interrupt (void)
{
   return 0;
}

static void version (void)
{
   fprintf (stdout, "rgrep version %d.%d\n",
	    RGREP_MAJOR_VERSION, RGREP_MINOR_VERSION);
   fprintf (stdout, "S-Lang version: %s\n", SLang_Version_String);
   if (SLang_Version != SLANG_VERSION)
     fprintf (stdout, "*** Compiled against S-Lang %d but linked to %d\n",
	      SLANG_VERSION, SLang_Version);

   exit (0);
}


static void usage(void)
{
   fputs("Usage: rgrep [options..] pattern [files ...]\n\
Options:\n\
  -?        additional help (use '-?' to avoid shell expansion on some systems)\n\
  -c        count matches\n\
  -h        highlight match (ANSI compatable terminal assumed)\n\
  -H        Output match instead of entire line containing match\n\
  -i        ignore case\n\
  -l        list filename only\n\
  -n        print line number of match\n\
  -F        follow links\n\
  -r        recursively scan through directory tree\n\
  -N        Do NOT perform a recursive search\n\
  -B        If file looks like a binary one, skip it.\n\
  -R 'pat'  like '-r' except that only those files matching 'pat' are checked\n\
              Here 'pat' is a globbing expression.\n\
  -v        print only lines that do NOT match the specified pattern\n\
  -x 'ext'  checks only files with extension given by 'ext'.\n\
  -D        Print all directories that would be searched.  This option is for\n\
             debugging purposes only.  No file is grepped with this option.\n\
  -W'len'   lines are 'len' characters long (not newline terminated).\n\
  --version Print version\n\
  --help    Print this help\n\
\n\
'pattern' is a valid 'ex' type of regular expression.  See the man page for ex.\n\
It is best enclosed in single quotes to avoid shell expansion.\n", stderr);
   
   exit(1);
}

static void additional_help (void)
{
   char buf[3];
   fputs("Supported Regular Expressions:\n\
   .                  match any character except newline\n\
   \\d                match any digit\n\
   \\e                match ESC char\n\
   *                  matches zero or more occurences of last single char RE\n\
   +                  matches one or more occurences of last single char RE\n\
   ?                  matches zero or one occurence of last single char RE\n\
   ^                  matches beginning of line\n\
   $                  matches end of line\n\
   [ ... ]            matches any single character between brackets.\n\
                      For example, [-02468] matches `-' or any even digit.\n\
		      and [-0-9a-z] matches `-' and any digit between 0 and 9\n\
		      as well as letters a through z.\n\
   \\<                Match the beginning of a word.\n\
   \\>                Match the end of a word.\n\
   \\{ ... \\}\n\
   \\( ... \\)\n\
   \\1, \\2, ..., \\9    matches match specified by nth \\( ... \\) expression.\n\
                      For example, '\\([ \\t][a-zA-Z]+\\)\\1[ \\t]' matches any\n\
		      word repeated consecutively.\n", 
	 stderr);
   
   if (isatty(fileno(stderr)) && isatty(fileno(stdin)))
     {
	fputs("\nPress RETURN for examples>", stderr);
	fgets(buf, 2, stdin);
	putc('\n', stderr);
     }
   fputs ("Examples:\n\
\n\
 Look in all files with a 'c' extension in current directory and all its\n\
 subdirectories looking for matches of 'int ' at the beginning of a line,\n\
 printing the line containing the match with its line number: (two methods)\n\
    rgrep -n -R '*.c' '^int ' .\n\
    rgrep -n -x c '^int ' .\n\
\n\
 Highlight all matches of repeated words in file 'paper.tex':\n\
    rgrep -h '\\<\\([a-zA-Z]+\\)\\>[ \\t]+\\<\\1\\>' paper.tex\n",
	  stderr);
   fputs ("\n\
 Search through all files EXCEPT .o and .a file below /usr/src/linux looking\n\
 for the string 'mouse' without regard to case:\n\
    rgrep -i -R '*.[^ao]' mouse /usr/src/linux\n\
\n\
 Search a fixed record length FITS file for the keyword EXTNAME:\n\
    rgrep -W80 ^EXTNAME file.fits\n\
   (Note that the regular expression '^[A-Z]+' will dump all fits headers.)\n",
	  stderr);
   
   exit (-1);
}

static FILE *File_Fp;
static VFILE *File_Vp;
static unsigned char *Fixed_Len_Buf;
static int Fixed_Len_Mode;
static int Fixed_Line_Len;

extern void msg_error (char *);

void msg_error (char *str)
{
   fputs(str, stderr);
   putc('\n', stderr);
}

static void exit_error(char *s)
{
   fprintf(stderr, "rgrep: %s\n", s);
   exit(1);
}

static void parse_flags(char *f)
{
   char ch;
   while ((ch = *f++) != 0)
     {
	switch (ch)
	  {
	   case 'i': Case_Sensitive = 0; break;
	   case 'l': File_Name_Only = 1; break;
	   case 'r': Do_Recursive = 1; break;
	   case 'N': Do_Recursive = -1; break;
	   case 'B': Binary_Option = 1; break;
	   case 'v': Print_Non_Matching_Lines = 1;
	   case 'H':
	     Highlight = 1;	       /* does not cause highlight for this case */
	     Output_Match_Only = 1;
	     break;
	   case 'h':
#ifndef IBMPC_SYSTEM
	     Highlight = 1;
#endif
	     break;
	   case 'c': Count_Matches = 1; break;
	   case 'n': Line_Numbers = 1; break;
	   case 'F': Follow_Links = 1; break;
	   case 'D': Debug_Mode = 1; break;
	   case '?': additional_help (); break;
	   case 'W':
	     Fixed_Line_Len = 0;
	     while (((ch = *f) != 0) && (ch >= '0') && (ch <= '9'))
	       {
		  Fixed_Line_Len = Fixed_Line_Len * 10 + (unsigned char) ch - '0';
		  f++;
	       }
	     if (Fixed_Line_Len == 0) usage ();
	     Fixed_Len_Buf = (unsigned char *) SLmalloc (Fixed_Line_Len);
	     if (Fixed_Len_Buf == NULL)
	       {
		  exit_error ("Malloc error.");
	       }
	     Fixed_Len_Mode = 1;
	     break;
	     
	   case '-':
	     if (!strcmp (f, "version"))
	       version ();
	     /* drop */
	   default: usage ();
	  }
     }
}


#if SLANG_VERSION < 20000
static SLRegexp_Type reg;
static SLRegexp_Type recurse_reg;
static unsigned char Recurse_Reg_Pattern_Buffer[JED_MAX_PATH_LEN];
#else
static SLRegexp_Type *reg;
static SLRegexp_Type *recurse_reg;
static unsigned char Recurse_Reg_Pattern_Buffer[JED_MAX_PATH_LEN];
#endif
static int Must_Match;
static int print_file_too;


static void do_fwrite (unsigned char *s, int n, int nl)
{
   unsigned char *smax = s + n, ch = 0;
#if defined(__unix__) || defined(VMS)
   unsigned char ch1;
#endif
   
   while (s < smax)
     {
	ch = *s++;
#if defined(__unix__) || defined(VMS)
	ch1 = ch & 0x7F;
	if ((ch1 < ' ') || (ch1 == 0x7F))
	  {
	     if ((ch != '\n') && (ch != '\t'))
	       {
		  if (ch & 0x80) putc ('~', stdout);
		  putc ('^', stdout);
		  if (ch1 == 0x7F) ch = '?'; else ch = ch1 + '@';
	       }
	  }
#endif
	putc (ch, stdout);
     }
   if (nl && (ch != '\n')) putc ('\n', stdout);
}



static void output_line(unsigned char *s, unsigned int n, unsigned char *p, unsigned char *pmax)
{
   if (Highlight == 0)
     {
	do_fwrite(s, n, 1);
     }
   else
     {
	if (Output_Match_Only == 0)
	  {
	     do_fwrite (s, (int) (p - s), 0);
	     fwrite (HON_STR, 1, HON_STR_LEN, stdout);
	  }
	
	do_fwrite (p, (int) (pmax - p), 0);
	if (Output_Match_Only == 0)
	  {
	     fwrite (HOFF_STR, 1, HOFF_STR_LEN, stdout);
	     do_fwrite (pmax, (int) n - (int) (pmax - s), 1);
	  }
	else if (*(pmax - 1) != '\n') putc ('\n', stdout);
     }
}



static unsigned char *rgrep_gets (unsigned int *n)
{
   unsigned int nread;
   
   if (File_Vp != NULL) return (unsigned char *) vgets (File_Vp, n);
   
   nread = fread (Fixed_Len_Buf, 1, Fixed_Line_Len, File_Fp);
   if (nread == 0) return NULL;
   *n = nread;
   return Fixed_Len_Buf;
}


static int OSearch_Ok = 0;
static int Search_St_Key_Length = 0;
#if SLANG_VERSION < 20000
static SLsearch_Type Search_St;
# define SEARCH_FORWARD(a,b) SLsearch ((a), (b), &Search_St)
# define REGEXP_MATCH(r,b,n) SLang_regexp_match((unsigned char *)(b),(n), &r)
#else
static SLsearch_Type *Search_St;
# define SEARCH_FORWARD(a,b) SLsearch_forward (Search_St, (a), (b))
# define REGEXP_MATCH(r,b,n) (NULL != SLregexp_match(r,(char *) (b),(n)))
#endif

static void grep(char *file)
{
   unsigned char *buf, *p, *pmax;
   unsigned int n;
   int line = 0, n_matches = 0;

   if (NULL == (buf = (unsigned char *) rgrep_gets(&n)))
     return;
   
   if (Binary_Option)
     {
	p = buf;
	
	if (File_Vp == NULL)
	  {
	     if (n < 32) pmax = p + n;
	     else pmax = p + 32;
	  }
	else
	  {
	     unsigned int vn;
	     
	     p = (unsigned char *) File_Vp->buf;
	     
	     if (File_Vp->eof != NULL)
	       vn = (unsigned char *) File_Vp->eof - p;
	     else vn = (unsigned char *) File_Vp->bmax - p;
	     
	     if (vn < 32) pmax = p + vn;
	     else pmax = p + 32;
	  }
	
	while (p < pmax)
	  {
	     if (*p == 0) return;
	     p++;
	  }
     }

   do
     {
	line++;
#if SLANG_VERSION < 20000
	if (reg.min_length > n)
	  {
	     if (Print_Non_Matching_Lines)
	       {
		  p = buf;
		  pmax = p + n;
		  goto match_found;
	       }
	     continue;
	  }
#endif
	if (Must_Match)
	  {
#if SLANG_VERSION < 20000
	     if (Search_St.key_len > (int) n)
	       {
		  if (Print_Non_Matching_Lines)
		    {
		       p = buf;
		       pmax = p + n;
		       goto match_found;
		    }
		  continue;
	       }
#endif
	     if (NULL == (p = SEARCH_FORWARD (buf, buf + n)))
	       {
		  if (Print_Non_Matching_Lines)
		    {
		       p = buf;
		       pmax = p + n;
		       goto match_found;
		    }
		  
		  continue;
	       }
	     if (OSearch_Ok)
	       {
		  if (Print_Non_Matching_Lines) continue;
		  pmax = p + Search_St_Key_Length;
		  goto match_found;
	       }
	  }
	
	if (!REGEXP_MATCH (reg, buf, n))
	  {
	     if (Print_Non_Matching_Lines)
	       {
		  p = buf;
		  pmax = p + n;
		  goto match_found;
	       }
	     continue;
	  }

	if (Print_Non_Matching_Lines) continue;
	
#if SLANG_VERSION < 20000
	p = buf + reg.beg_matches[0];
	pmax = p + reg.end_matches[0];
#else
	  {
	     unsigned int ofs, len;
	     (void) SLregexp_nth_match (reg, 0, &ofs, &len);
	     p = buf + ofs;
	     pmax = p + len;
	  }
#endif
	match_found:
	n_matches++;

	if (Count_Matches) continue;
	if (File_Name_Only)
	  {
	     if (Print_Non_Matching_Lines)
	       continue;

	     puts(file);
	     return;
	  }

	if (print_file_too)
	  {
	     fputs(file, stdout);
	     putc(':', stdout);
	  }
	if (Line_Numbers)
	  {
	     fprintf(stdout, "%d:", line);
	  }
	
	output_line(buf, n, p, pmax);
#ifdef __unix__
	if (Stdout_Is_TTY) fflush (stdout);
#endif	
     }
   while (NULL != (buf = (unsigned char *) rgrep_gets(&n)));

   if (Print_Non_Matching_Lines 
       && File_Name_Only
       && (n_matches == line))
     {
	puts (file);
#ifdef __unix__
	if (Stdout_Is_TTY) fflush (stdout);
#endif
     }

   if (n_matches && Count_Matches)
     {
	if (print_file_too || File_Name_Only)
	  {
	     fputs(file, stdout);
	     putc(':', stdout);
	  }
	fprintf(stdout, "%d\n", n_matches);
#ifdef __unix__
	if (Stdout_Is_TTY) fflush (stdout);
#endif
     }
}

#ifdef __MSDOS__
# include <dir.h>
#endif

#ifdef __unix__
# include <sys/types.h>
# include <sys/stat.h>
#endif


#ifdef IBMPC_USE_ASM
typedef struct Dos_DTA_Type
{
   unsigned char undoc[21];
   unsigned char attr;
   unsigned int time;
   unsigned int date;
   unsigned char low_size[2];
   unsigned char high_size[2];
   char name[13];
}
DOS_DTA_Type;
#endif

#ifdef __unix__
# if HAVE_DIRENT_H
#  include <dirent.h>
#  define NAMLEN(dirent) strlen((dirent)->d_name)
# else
#  define dirent direct
#  define NAMLEN(dirent) (dirent)->d_namlen
#  define NEED_D_NAMLEN
#  if HAVE_SYS_NDIR_H
#   include <sys/ndir.h>
#  endif
#  if HAVE_SYS_DIR_H
#   include <sys/dir.h>
#  endif
#  if HAVE_NDIR_H
#   include <ndir.h>
#  endif
# endif
#endif

typedef struct
{
   char dir[JED_MAX_PATH_LEN];
   int dir_len;
   char *file;			       /* pointer to place in dir */
   int isdir;
#ifdef IBMPC_USE_ASM
   DOS_DTA_Type *dta;
   char pattern[16];
#endif
#ifdef __unix__
   DIR *dirp;
#endif
}
Sys_Dir_Type;


#ifdef IBMPC_USE_ASM
void dos_set_dta (DOS_DTA_Type *dta)
{
   asm mov ah, 0x1A
     asm push ds
     asm lds dx, dword ptr dta
     asm int 21h
     asm pop ds
}

int dos_is_dir (char *file)
{
   int n = strlen (file);
   if (n == 0) return 0;
   if (file[n - 1] == '\\') return 1;
   
   asm mov ah, 0x43
     asm mov al, 0
     asm push ds
     asm lds dx, dword ptr file
     asm int 21h
     asm pop ds
     asm mov n, cx
     asm jnc L1
     return 0;
   
   L1:
   if (n & 0x10) return 1;
   return 0;
}

#endif

#ifdef __unix__
static int unix_is_dir (char *dir, int force_link)
{
/* AIX requires this */
#ifdef _S_IFDIR
#ifndef S_IFDIR
#define S_IFDIR _S_IFDIR
#endif
#endif
   struct stat buf;
   int mode;

   return 0;

   if (stat(dir, &buf)) return -1;
   if ((buf.st_mode & S_IFMT) == S_IFREG)
     return 0;

#ifdef S_IFLNK
   if ((0 == Follow_Links)
       && (0 == force_link)
       && (-1 == lstat(dir, &buf)))
     return -1;
#endif
   
   mode = buf.st_mode & S_IFMT;
   
#ifdef S_IFLNK
   if (mode == S_IFLNK) 
     {
	return -2;
     }
#endif
   if (mode == S_IFDIR) return (1);
   if (mode != S_IFREG) return (-1);
   
   return(0);
}
#endif

static Sys_Dir_Type *sys_opendir(char *dir, Sys_Dir_Type *x)
{
#ifdef IBMPC_USE_ASM
   char slash = '\\';
   char *pat = "*.*";
   dos_set_dta (x->dta);
   
   if ((dir[1] == ':') && (dir[2] == '\\'))
     {
	strcpy (x->dir, dir);
     }
   else
     {
	/* must have drive/dirpath/filename */
	getcwd(x->dir, JED_MAX_PATH_LEN);
	if (*dir == slash)
	  {
	     strcpy (x->dir + 2, dir);
	  }
	else
	  {
	     if (x->dir[strlen (x->dir) - 1] != slash) strcat (x->dir, "\\");
	     strcat(x->dir, dir);
	  }
     }
   
   dir = x->dir + strlen (x->dir);
   /* check for a pattern already as part of the dirspec */
   while (dir > x->dir)
     {
	if (*dir == '\\') break;
	if (*dir == '*')
	  {
	     while (*dir != '\\') dir--;
	     *dir = 0;
	     pat = dir + 1;
	     break;
	  }
	dir--;
     }
   strcpy (x->pattern, pat);
   
#else
#ifdef __unix__
   char slash = '/';
   DIR *dirp;
   if (NULL == (dirp = (DIR *) opendir(dir)))
     {
	fprintf (stderr, "rgrep: dir %s not readable.\n", dir);
	return NULL;
     }
   x->dirp = dirp;
   strcpy(x->dir, dir);
#endif /* __unix__ */
#endif /* IBMPC_USE_ASM */
   x->dir_len = strlen(x->dir);
   if (x->dir[x->dir_len - 1] != slash)
     {
	x->dir[x->dir_len++] = slash;
	x->dir[x->dir_len] = 0;
     }
   return (x);
}



static void sys_closedir(Sys_Dir_Type *x)
{
#ifdef IBMPC_USE_ASM
   (void) x;
#else
#ifdef __unix__
   DIR *dirp;
   dirp = x->dirp;
   if (dirp != NULL) closedir(dirp);
   x->dirp = NULL;
#endif
#endif
}

#ifdef IBMPC_USE_ASM
char *dos_dta_fixup_name (Sys_Dir_Type *x)
{
   x->file = x->dir + x->dir_len;
   strcpy(x->file, x->dta->name);
   /* sub directory */
   if (x->dta->attr & 0x10) x->isdir = 1; else x->isdir = 0;
   return x->file;
}
#endif

static char *sys_dir_findnext(Sys_Dir_Type *x)
{
   char *file;
#ifdef IBMPC_USE_ASM
   asm mov ah, 0x4F
     asm int 21h
     asm jnc L1
     return NULL;
   
   L1:
   file = dos_dta_fixup_name (x);
#else
   
#ifdef __unix__
#  ifdef NEED_D_NAMLEN
#    define dirent direct
#  endif
   struct dirent *dp;
   DIR *d;
   d = x->dirp;
   
   if (NULL == (dp = readdir(d))) return(NULL);
#  ifdef NEED_D_NAMLEN
   dp->d_name[dp->d_namlen] = 0;
#  endif
   file = dp->d_name;
   x->file = x->dir + x->dir_len;
   strcpy (x->file, dp->d_name);
   x->isdir = unix_is_dir (x->dir, 0);
#endif /* __unix__ */
#endif /* IBMPC_USE_ASM */
   /* exclude '.' and '..' */
   if (*file++ == '.')
     {
	if ((*file == 0) ||
	    ((*file == '.') && (*(file + 1) == 0))) x->isdir = -1;
     }
   return (x->dir);
}

static char *sys_dir_findfirst(Sys_Dir_Type *x)
{
#ifdef IBMPC_USE_ASM
   unsigned int attr = 0x1 | 0x10;     /* read only + sub directory */
   char pat[JED_MAX_PATH_LEN], *patp, *file;
   
   attr |= 0x2 | 0x4;		       /* hidden and system */
   
   strcpy (pat, x->dir);
   strcat (pat, x->pattern);
   patp = pat;
   
   asm mov ah, 0x4e
     asm mov cx, attr
     asm push ds
     asm lds dx, dword ptr patp
     asm int 21h
     asm pop ds
     asm jc L1
     
     file = dos_dta_fixup_name (x);
   /* exclude '.' and '..' */
   if (*file++ == '.')
     {
	if ((*file == 0) ||
	    ((*file == '.') && (*(file + 1) == 0))) x->isdir = -1;
     }
   return x->dir;
   
   L1:
   return NULL;
#else
#ifdef __unix__
   return (sys_dir_findnext(x));
#endif
#endif
}

#define BUF_SIZE 4096

static void grep_file(char *file, char *filename)
{
   char *p;
   if (Debug_Mode) return;
   if (Recursive_Match)
     {
	if (Match_This_Extension != NULL)
	  {
	     p = filename + strlen(filename);
	     while ((p >= filename) && (*p != '.')) p--;
	     if ((*p != '.') ||
#ifdef __MSDOS__
		 stricmp(Match_This_Extension, p + 1)
#else
		 strcmp(Match_This_Extension, p + 1)
#endif
		 )
	       return;
	  }
	else if (!REGEXP_MATCH(recurse_reg, filename, strlen(filename)))
	  return;
     }
   
   File_Fp = NULL;
   File_Vp = NULL;
   
   if (Fixed_Len_Mode)
     {
	File_Fp = fopen (file, "rb");
     }
   else File_Vp = vopen (file, BUF_SIZE, 0);
   
   if ((File_Vp == NULL) && (File_Fp == NULL))
     {
	fprintf(stderr, "rgrep: unable to read %s\n", file);
     }
   else
     {
	grep(file);
	if (File_Fp == NULL) vclose(File_Vp);
	else fclose (File_Fp);
     }
}

#define MAX_DEPTH 25
static void grep_dir(char *dir)
{
   static int depth;
   Sys_Dir_Type x;
   char *file;
#ifdef IBMPC_USE_ASM
   DOS_DTA_Type dta;
   x.dta = &dta;
#endif
   
   if (NULL == sys_opendir(dir, &x)) return;
   if (depth >= MAX_DEPTH)
     {
	fprintf(stderr, "Maximum search depth exceeded.\n");
	return;
     }
   
   depth++;
   if (Debug_Mode) fprintf(stderr, "%s\n", dir);
   
   for (file = sys_dir_findfirst(&x);
	file != NULL; file = sys_dir_findnext(&x))
     {
	if (x.isdir == 0) grep_file(file, x.file);
	else if ((Do_Recursive > 0) && (x.isdir == 1)) grep_dir(file);
	
#ifdef IBMPC_USE_ASM
	dos_set_dta (&dta);	       /* something might move it */
#endif
     }
   
   sys_closedir(&x);
   depth--;
}

static unsigned char *fixup_filename (unsigned char *name)
{
   unsigned char *pat = Recurse_Reg_Pattern_Buffer;
   unsigned char ch;
   
   *pat++ = '^';
   while ((ch = *name++) != 0)
     {
	if (ch == '*')
	  {
	     *pat++ = '.'; *pat++ = '*';
	  }
	else if (ch == '?') *pat++ = '.';
	else if ((ch == '.') || (ch == '$'))
	  {
	     *pat++ = '\\'; *pat++ = ch;
	  }
	else *pat++ = ch;
     }
   *pat++ = '$';
   *pat = 0;
   return Recurse_Reg_Pattern_Buffer;
}



int main(int argc, char **argv)
{
#if SLANG_VERSION < 20000
   unsigned char buf[2 * JED_MAX_PATH_LEN];
   unsigned char recurse_buf[JED_MAX_PATH_LEN];
#else
   unsigned int flags;
#endif
   char *pattern;

#ifdef __unix__
   Stdout_Is_TTY = isatty (fileno(stdout));
#endif
   argv++;
   argc--;
   
   SLang_init_case_tables ();
   while (argc && (**argv == '-') && *(*argv + 1))
     {
	if (!strcmp(*argv, "-R"))
	  {
	     argc--;
	     argv++;
	     if (!argc) usage();
	     
#if SLANG_VERSION < 20000
	     recurse_reg.pat = fixup_filename ((unsigned char *) *argv);
	     recurse_reg.buf = recurse_buf;
	     recurse_reg.buf_len = JED_MAX_PATH_LEN;
# ifdef __MSDOS__
	     recurse_reg.case_sensitive = 0;
# else
	     recurse_reg.case_sensitive = 1;
# endif
	     if (SLang_regexp_compile (&recurse_reg)) exit_error("Error compiling pattern.");
#else
	     flags = 0;
# ifdef __MSDOS__
	     flags |= SLREGEXP_CASELESS;
# endif
	     if (NULL == (recurse_reg = SLregexp_compile ((char *)fixup_filename((unsigned char *) *argv), flags)))
	       exit_error ("Error compiling pattern");
#endif
	     Do_Recursive = 1;
	     Recursive_Match = 1;
	  }
	else if (!strcmp(*argv, "-x"))
	  {
	     argc--;
	     argv++;
	     if (!argc) usage();
	     Recursive_Match = 1;
	     Match_This_Extension = *argv;
	  }
	else
	  {
	     parse_flags(*argv + 1);
	  }
	argv++; argc--;
     }
   
   if (!argc) usage();
   
   pattern = *argv;

#if SLANG_VERSION < 20000
   reg.pat = (unsigned char *) pattern;
   reg.buf = buf;
   reg.buf_len = sizeof (buf);
   reg.case_sensitive = Case_Sensitive;
   
   if (SLang_regexp_compile (&reg)) exit_error("Error compiling pattern.");
#else
   if (NULL == (reg = SLregexp_compile (pattern, Case_Sensitive ? 0 : SLREGEXP_CASELESS)))
     exit_error ("Error compiling pattern");
#endif
   argc--; argv++;
   
   Must_Match = 1;
   
#if SLANG_VERSION < 20000
   OSearch_Ok = reg.osearch;
#else
   (void) SLregexp_get_hints (reg, &flags);
   OSearch_Ok = (0 != (flags & SLREGEXP_HINT_OSEARCH));
#endif
   
   if (OSearch_Ok)
     {
#if SLANG_VERSION < 20000
	SLsearch_init (pattern, 1, Case_Sensitive, &Search_St);
#else
	if (NULL == (Search_St = SLsearch_new ((unsigned char *)pattern, Case_Sensitive ? 0 : SLSEARCH_CASELESS)))
	  exit_error ("SLsearch_new failed");
#endif
	Search_St_Key_Length = strlen (pattern);
     }
#if SLANG_VERSION < 20000
   else if (reg.must_match)
     {
	SLsearch_init ((char *) reg.must_match_str, 1, Case_Sensitive, &Search_St);
	Search_St_Key_Length = strlen ((char *) reg.must_match_str);
     }
   else
#endif
     Must_Match = 0;

   if (argc == 0)
     {
	if (Fixed_Len_Mode) File_Fp = stdin;
	else File_Vp = vstream(fileno(stdin), BUF_SIZE, 0);
	if ((File_Fp == NULL) && (File_Vp == NULL))
	  {
	     exit_error("Error vopening stdin.");
	  }
	grep("stdin");
	if (File_Vp != NULL) vclose(File_Vp);
	else fclose (File_Fp);
     }
   else
     {
	if ((Do_Recursive > 0) || (argc != 1)) print_file_too = 1;
	while (argc--)
	  {
#ifdef __unix__
	     int ret;
#endif
#ifdef __MSDOS__
	       {
		  char *p = *argv;
		  while (*p)
		    {
		       if (*p == '/') *p = '\\';
		       p++;
		    }
	       }
#endif
	     if (
#ifdef IBMPC_USE_ASM
		 dos_is_dir (*argv)
/*		 
 *		 && (('\\' == (*argv)[strlen(*argv) - 1])
 *		     || (!strcmp (*argv, "."))
 *		     || (!strcmp (*argv, ".."))) */
#else
#ifdef __unix__
		 (1 == (ret = unix_is_dir (*argv, 
					   (strlen(*argv) && ('/' == (*argv)[strlen(*argv) - 1])))))
#endif
#endif
		 )
	       {
		  print_file_too = 1;
		  if (Do_Recursive >= 0) grep_dir (*argv);
	       }
	     
	     else
#ifdef __MSDOS__
	       {
		  char *file = *argv;
		  while (*file && (*file != '*')) file++;
		  if (*file == '*')
		    {
		       print_file_too = 1;
		       grep_dir (*argv);
		    }
		  else grep_file(*argv, *argv);
	       }
#else
#ifdef __unix__
	     if (ret == 0)
#endif
	       grep_file(*argv, *argv);
#endif
	     argv++;
	  }
     }
   return (0);
}


/* ------------------------------------------------------------ */

#ifdef VMS

int vms_expand_filename(char *file,char *expanded_file)
{
   unsigned long status;
   static int context = 0;
   static char inputname[JED_MAX_PATH_LEN] = "";
   $DESCRIPTOR(file_desc,inputname);
   $DESCRIPTOR(default_dsc,"SYS$DISK:[]*.*;");
   static struct dsc$descriptor_s  result =
     {0, DSC$K_DTYPE_T, DSC$K_CLASS_D, NULL};
   
   if (strcmp(inputname, file))
     {
	if (context)
	  {
	     lib$find_file_end(&context);
	  }
	context = 0;
	strcpy(inputname, file);
	file_desc.dsc$w_length = strlen(inputname);
     }
   
   if (RMS$_NORMAL == lib$find_file(&file_desc,&result,&context,
				    &default_dsc,0,0,&Number_Zero))
     {
	MEMCPY(expanded_file, result.dsc$a_pointer, result.dsc$w_length);
	expanded_file[result.dsc$w_length] = '\0';
	return (1);
     }
   else
     {
          /* expanded_file[0] = '\0'; */      /* so file comes back as zero width */
	return(0);
     }
}

static int context = 0;

static char inputname[JED_MAX_PATH_LEN] = "";
$DESCRIPTOR(file_desc,inputname);
$DESCRIPTOR(default_dsc,"SYS$DISK:[]*.*;");

int sys_findnext(char *file)
{
   unsigned long status;
   static struct dsc$descriptor_s  result =
     {
	0, DSC$K_DTYPE_T, DSC$K_CLASS_D, NULL
     };
   
   if (RMS$_NORMAL == lib$find_file(&file_desc,&result,&context,
				    &default_dsc,0,0,&Number_Zero))
     {
	MEMCPY(file, result.dsc$a_pointer, result.dsc$w_length);
	file[result.dsc$w_length] = 0;
	return (1);
     }
   else return(0);
}

int sys_findfirst(char *file)
{
   char *file;
   strcpy(inputname, file);
   file_desc.dsc$w_length = strlen(inputname);
   if (context) lib$find_file_end(&context);
   context = 0;
   return sys_findnext(file);
}
#endif
/* VMS */
