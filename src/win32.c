/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <windows.h>
#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#include <process.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
# include <direct.h>
#endif
#if defined(__BORLANDC__)
# include <dir.h>
# define _fdopen fdopen
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <time.h>

#include <assert.h>
#include <io.h>
#include <errno.h>

#include <fcntl.h>

#include "display.h"
#include "sysdep.h"
#include "screen.h"
#include "keymap.h"
#include "hooks.h"
#include "ins.h"
#include "ledit.h"
#include "misc.h"
#include "cmds.h"
#include "sig.h"
#include "window.h"

#include "win32.h"

#if JED_HAS_SUBPROCESSES
# include "jprocess.h"
#endif

#if SLANG_VERSION < 20000
# define SLw32_Hstdin _SLw32_Hstdin
extern HANDLE _SLw32_Hstdin;
#endif

static int x_insert_cutbuffer(void);
static void x_region_2_cutbuffer(void);
static void x_warp_pointer(void);
static int msw_system(char *, int *, int *);

static SLang_Intrin_Fun_Type Jed_WinCommon_Table[] =
{
   MAKE_INTRINSIC("x_warp_pointer", x_warp_pointer, VOID_TYPE, 0),
   MAKE_INTRINSIC("x_insert_cutbuffer", x_insert_cutbuffer, INT_TYPE, 0),
   /* Prototype: Integer x_insert_cut_buffer ();
    * Inserts cutbuffer into the current buffer and returns the number
    * of characters inserted.
    */
   MAKE_INTRINSIC("x_copy_region_to_cutbuffer", x_region_2_cutbuffer, VOID_TYPE, 0),
   /*Prototype: Void x_copy_region_to_cutbuffer();*/

   MAKE_INTRINSIC(NULL, NULL, 0,0)
};

static int init_intrinsics (void)
{
   if (-1 == SLadd_intrinsic_function ("msw_system", (FVOID_STAR) msw_system,
				       SLANG_INT_TYPE, 3, SLANG_STRING_TYPE,
				       SLANG_INT_TYPE, SLANG_INT_TYPE))
     return -1;

   if ((-1 == SLadd_intrin_fun_table (Jed_WinCommon_Table, "MSWINDOWS"))
       || (-1 == SLdefine_for_ifdef ("MOUSE")))
     return -1;
   
   return 0;
}

int Jed_W32_Is_GUI;

int init_tty(void)
{
   if (-1 == jed_add_init_slang_hook (init_intrinsics))
     return -1;

   if (X_Init_Term_Hook != NULL)
     return (*X_Init_Term_Hook) ();

   Jed_W32_Is_GUI = 0;
   if (-1 == SLang_init_tty (7, 1, 0))
     return -1;

   Input_Events[0] = SLw32_Hstdin;
   return 0;
}

void reset_tty (void)
{
   if (Batch) return;

   if (X_Init_Term_Hook != NULL)
     {
	if (X_Reset_Term_Hook != NULL) (*X_Reset_Term_Hook) ();
	return;
     }

   SLang_reset_tty();
}

int sys_System(char *command_line)
{
   return shell_command(command_line);
}

void sys_pause (int ms)
{
   Sleep(ms);
}

void init_signals()
{
}



/* Delete the file NAME.  returns 0 on failure, 1 on sucess
 * Under OS/2 and DOS, unlink()[UNIX] and remove()[ANSI] are equivalent.
 */

int sys_delete_file(char *name)
{
   return(1 + remove(name));
}

static WIN32_FIND_DATA findata;
static HANDLE hsearch = INVALID_HANDLE_VALUE;
static char Found_Dir[JED_MAX_PATH_LEN];
static char Found_File[JED_MAX_PATH_LEN];
static int Found_FLen;

static void fixup_name(char *file)
{
   int dir;
   char name[JED_MAX_PATH_LEN];

   strcpy (file, Found_Dir);

   strcpy (name, findata.cFileName);
   dir = (findata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

   strcat(file, name);
   if (dir) strcat(file, "\\");
}

int sys_findfirst (char *thefile)
{
   char *f, the_path[JED_MAX_PATH_LEN], *file, *f1;
   char *pat;

   file = jed_standardize_filename_static(thefile);
   f1 = f = extract_file(file);
   strcpy (Found_Dir, file);

   strcpy (Found_File, file);
   Found_FLen = strlen(Found_File);

   Found_Dir[(int) (f - file)] = 0;

   strcpy(the_path, file);

   while (*f1 && (*f1 != '*')) f1++;
   if (! *f1)
     {
	while (*f && (*f != '.')) f++;
	strcat(the_path, "*");
     }
   pat = the_path;

   if (hsearch != INVALID_HANDLE_VALUE) FindClose (hsearch);

   if ((INVALID_HANDLE_VALUE != (hsearch = FindFirstFile (pat, &findata))))
     {
 	fixup_name(file);

	/* The windows file system is case-insensitive, so if one 
	 * searches for makefi*, the OS will return Makefile.in
	 * between the others, so check for wrong values and reject
	 * them
	 */
	if (Jed_Filename_Case_Sensitive
	    && (0 != strncmp (file, Found_File, Found_FLen))
	    && !sys_findnext (file))
	  return 0;

 	strcpy (thefile, file);
 	return(1);
     }
   else
     return 0;
}

int sys_findnext(char *file)
{
   while (FindNextFile (hsearch, &findata))
     {
 	fixup_name(file);

	if (Jed_Filename_Case_Sensitive
	    && (0 != strncmp (file, Found_File, Found_FLen)))
	  continue;

	return 1;
     }

   FindClose (hsearch);
   hsearch = INVALID_HANDLE_VALUE;
   return 0;
}


/* returns 0 if file does not exist, 1 if it is not a dir, 2 if it is */
int sys_chmod (SLFUTURE_CONST char *file, int what, mode_t *mode, uid_t *uid, gid_t *gid)
{
#ifdef _MSC_VER
   struct _stat buf;
#else
   struct stat buf;
#endif
   char ourname[256];
   int len;

   if (what)
     {
#ifdef _MSC_VER
	_chmod(file, *mode);
#else
	chmod(file, *mode);
#endif
	return(0);
     }

   /* strip the trailing backslash if necessary */
   strcpy(ourname, file);
   len = strlen(ourname);
   if (len>1 && ourname[len-1] == SLASH_CHAR && ourname[len-2] != ':')
     ourname[strlen(ourname)-1] = '\0';

   if (
#ifdef _MSC_VER
       _stat(ourname, &buf) < 0
#else
       stat(ourname, &buf) < 0
#endif
       )
     {
	if (ourname[0] == SLASH_CHAR && ourname[1] == SLASH_CHAR)
	  {
	     int at = GetFileAttributes(ourname);

	     if (at >= 0)
	       {
		  if (at & FILE_ATTRIBUTE_DIRECTORY)
		    return 2;
		  else
		    return 1;
	       }
	  }
	return 0;
     }


   *mode = buf.st_mode & 0777;

   if (buf.st_mode & S_IFDIR) return (2);
   return(1);
}

static void x_warp_pointer (void)
{
}


static void 
x_region_2_cutbuffer (void)
{
   int i, x, nbytes;
   char *dat, *buf;
   HGLOBAL hBuf;

   dat = make_buffer_substring(&nbytes);
   if (dat == NULL) return;

   /* space for LF -> CR/LF translation(s) */
   for (i = x = 0; i < nbytes; i++)
     if (dat[i] == '\n') x++;

   hBuf = GlobalAlloc(GHND, x + nbytes + 1);	/* space for trailing nul */
   buf = (char *) GlobalLock(hBuf);

   for (i = x = 0; i < nbytes; i++)
     {
	if (dat[i] == '\n') buf[x++] = '\r';	/* LF -> CR/LF */
	buf[x++] = dat[i];
     }
   /* since GlobalAlloc() is like calloc(), trailing nul is 'automatic' */

   /* tranfer data to clipboard */
   OpenClipboard(NULL); /* This_Window.w); */
   EmptyClipboard();

   GlobalUnlock(hBuf);

   SetClipboardData(CF_TEXT, hBuf);
   CloseClipboard();

   SLfree (dat);	/* discard string */
}

static int x_insert_cutbuffer (void)
{
   int nbytes = 0;
   char *dat;
   int i, x;
   HGLOBAL hBuf;
   char *buf;

   CHECK_READ_ONLY;

   OpenClipboard(NULL);

   hBuf = GetClipboardData(CF_TEXT);
   CloseClipboard();

   if (hBuf)
     {
	buf = (char *)GlobalLock(hBuf);
	for(i = x = 0; buf[i] != 0; i++)
	  if (buf[i] != '\r') x++;

	nbytes = x;
	dat = SLmalloc (x + 1);
	if (dat != NULL)
	  {
	     for(i = x = 0; buf[i] != 0; i++)
	       if (buf[i] != '\r')
		 dat[x++] = buf[i];

	     dat[x] = 0;

	     jed_insert_nbytes((unsigned char *) dat, nbytes);
	     SLfree (dat);
	  }
	GlobalUnlock(hBuf);
     }

   return nbytes;
}

static int msw_system(char *command_line, int *nCmdShow, int *wait)
{
   UINT retcode;
   HCURSOR hcur, hcur_old;
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   memset ((char *) &si, 0, sizeof (STARTUPINFO));
   si.wShowWindow = *nCmdShow;
   si.dwFlags = STARTF_USESHOWWINDOW;
   si.cb = sizeof(si);
   retcode = CreateProcess (NULL, command_line, NULL, NULL,
			    0, NORMAL_PRIORITY_CLASS, NULL,
			    NULL, &si, &pi);
   if (!retcode)
     {
	msg_error ("Unable to create process");
	return 1;
     }

   if (!*wait) return 0;

   hcur = LoadCursor(NULL, IDC_WAIT);
   hcur_old = SetCursor(hcur);
   WaitForSingleObject(pi.hProcess, INFINITE);
   SetCursor(hcur_old);
   return 0;
}

/* popen, pclose function for Win32 -- it seems that standard ones work
 * only for console applications.
 */
# define MAX_POPEN 10
typedef struct
{
   FILE *fp;
   HANDLE hprocess;
}
Popen_Type;

static int Popen_Ptr = 0;
static Popen_Type Popen_Buf[MAX_POPEN];

static char *get_helper_app (void)
{
   char *s;
   
   if (1 == SLang_execute_function ("_win32_get_helper_app_name"))
     {
	if (-1 == SLang_pop_slstring (&s))
	  return NULL;
	
	return s;
     }
   msg_error ("call to _win32_get_helper_app_name failed");
   return NULL;
}

char *w32_build_command (char **argv, unsigned int num)
{
   unsigned int len;
   unsigned int i;
   char *helper;
   char *cmd;

   helper = get_helper_app ();
   if (NULL == helper)
     return NULL;
   len = strlen (helper);
   if (len) len++;

   for (i = 0; i < num; i++)
     len += 1 + strlen (argv[i]);
   
   cmd = SLmalloc (len + 1);
   if (cmd == NULL)
     {
	SLang_free_slstring (helper);
	return NULL;
     }
   
   len = strlen (helper);
   if (len)
     {
	strcpy (cmd, helper);
	cmd[len] = ' ';
	len++;
     }
   
   for (i = 0; i < num; i++)
     {
	strcpy (cmd + len, argv[i]);
	len += strlen(argv[i]);
	cmd[len] = ' ';
	len++;
     }
   cmd[len] = 0;

   SLang_free_slstring (helper);
   return cmd;
}

FILE *w32_popen(char *cmd, char *mode)
{
   int fd;
   HANDLE rd, wr, er, h, p;
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   FILE *fp;

   if (Popen_Ptr == MAX_POPEN)
     {
	msg_error("too many popens");
	return NULL;
     }

   if (((*mode != 'r') && (*mode != 'w')) || (*(mode + 1) != 0))
     {
	errno = EINVAL;
	return(NULL);
     }
   if (*mode == 'w')
     {
	msg_error ("popen mode `w' not implemented");
	return NULL;
     }

   p = GetCurrentProcess ();

   if (FALSE == CreatePipe(&rd, &wr, NULL, 0))   /* NOT inherited */
     return NULL;

   if (FALSE == DuplicateHandle (p, wr, p, &h, 0, TRUE, DUPLICATE_SAME_ACCESS))
     {
	CloseHandle (rd);
	CloseHandle (wr);
     }
   CloseHandle (wr);
   wr = h;

   /* Get stderr handle from stdout */
   if (FALSE == DuplicateHandle (p, wr, p, &er, 0, TRUE, DUPLICATE_SAME_ACCESS))
     {
	CloseHandle (rd);
	CloseHandle (wr);
	return NULL;
     }

   if (NULL == (cmd = w32_build_command (&cmd, 1)))
     {
	CloseHandle (rd);
	CloseHandle (wr);
	return NULL;
     }

   memset ((char *) &si, 0, sizeof (STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.lpReserved = NULL;
   si.lpReserved2 = NULL;
   si.cbReserved2 = 0;
   si.lpDesktop = NULL;
   si.lpTitle = NULL;
   si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
   /* si.wShowWindow = SW_MINIMIZE; */
   si.wShowWindow = SW_HIDE;

   si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
   si.hStdOutput = wr;
   si.hStdError = er;

   if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE,
		      CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
     {
	jed_verror ("popen failed (cmd=%s)", cmd);
	SLfree (cmd);
	CloseHandle (rd);
	CloseHandle (wr);
	CloseHandle (er);
	return NULL;
     }
   CloseHandle (wr);
   CloseHandle (er);
   SLfree (cmd);

   fd = _open_osfhandle((long) rd, O_RDONLY|O_TEXT);
   if ((fd < 0)
       || (NULL == (fp = _fdopen(fd, mode))))
     {
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	if (fd < 0)
	  CloseHandle (rd);
	else
	  _close (fd);
	return NULL;
     }
   CloseHandle (pi.hThread);
   CloseHandle (pi.hProcess);

   Popen_Buf[Popen_Ptr].hprocess = pi.hProcess;
   Popen_Buf[Popen_Ptr].fp = fp;
   Popen_Ptr++;

   return fp;
}

int w32_pclose(FILE *fp)
{
   int i;

   i = Popen_Ptr - 1;
   while (i >= 0)
     {
	if (Popen_Buf[i].fp == fp)
	  {
	     DWORD d = 0;
	     /* send EOF */
	     (void) fputs ("\x1a", fp); (void) fflush (fp);
#ifndef __BORLANDC__		  
	     if (fclose(fp) == EOF) return -1;
#else		  
 /* [JMS] Something odd with Borland C possibly, as the fclose attempt seems */
 /* [JMS] to fail every time, resulting in a 'too many popens' error as */
 /* [JMS] the failure causes the function to abort without removing the */
 /* [JMS] entry from the table. */
	     if (fclose(fp) == EOF) d = -1;
#endif
#if 0
	     if (WaitForSingleObject(Popen_Buf[i].hprocess, 1000) == WAIT_TIMEOUT)
	       {
		  d = TerminateProcess(Popen_Buf[i].hprocess, -1);
	       }

	     GetExitCodeProcess(Popen_Buf[i].hprocess, &d);
#endif
	     i++;
	     while (i < Popen_Ptr)
	       {
		  Popen_Buf[i - 1] = Popen_Buf[i];
		  i++;
	       }

	     Popen_Ptr--;
	     return d;
	  }
	i--;
     }

   return -1;
}

