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

#if defined(__BORLANDC__)
# include <dir.h>
# include <dos.h>
#endif

#include <time.h>

#include <assert.h>
#include <io.h>
#include <errno.h>

#include "display.h"
//#include "buffer.h"
#include "sysdep.h"
#include "screen.h"
#include "keymap.h"
#include "hooks.h"
#include "ins.h"
#include "ledit.h"
#include "misc.h"
#include "cmds.h"
#include "sig.h"

extern void process_message(void);
BOOL CALLBACK EnumWndProc(HWND, LPARAM);

extern HINSTANCE hPrevInst;
extern HINSTANCE hInstance;

extern HINSTANCE _hPrev, _hInstance;

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

   SLANG_END_TABLE
};

int init_tty(void)
{
   SLadd_intrinsic_function ("msw_system", (FVOID_STAR) msw_system,
			     SLANG_INT_TYPE, 3, SLANG_STRING_TYPE,
			     SLANG_INT_TYPE, SLANG_INT_TYPE);

   if ((-1 == SLadd_intrin_fun_table (Jed_WinCommon_Table, "MSWINDOWS"))
       || (-1 == SLdefine_for_ifdef ("MOUSE")))
     return -1;

   return 0;
}

void reset_tty(void)
{
}

typedef struct _Process_Info
{
   HINSTANCE hInstance;
   HWND hWnd;
   HTASK hTask;
}
Process_Info;

int msw_system(char *command_line, int *nCmdShow, int *wait)
{
   UINT retcode;
   HCURSOR hcur, hcur_old;
   MSG msg;
   WNDENUMPROC enumproc;
   Process_Info pi;

   retcode = WinExec(command_line, *nCmdShow);

   if (retcode < 32)
     {
	switch (retcode)
	  {
	   case 0:
	   case 11:
	   case 12:
	   case 14:
	   case 15:
	     msg_error("Invalid EXE file");
	     break;

	   case 2:
	     msg_error("File not found");
	     break;

	   case 3:
	     msg_error("Path not found");
	     break;

	   case 8:
	     msg_error("Out of memory");
	     break;

	   case 10:
	     msg_error("Incorrect MS Windows version");
	     break;

	   case 16:
	     msg_error("Cannot run more that one instance of application");
	     break;

	   case 20:
	     msg_error("Cannot find one of required DLL's");
	     break;

	   case 21:
	     msg_error("Application requires MS Windows 32-bit extension");
	     break;

	   default:
	     msg_error("Unknown error");
	  }
	return retcode;
     }

   if (!*wait) return 0;

   pi.hInstance = (HINSTANCE) retcode;
   pi.hWnd = NULL;
   pi.hTask = NULL;

   enumproc = (WNDENUMPROC) MakeProcInstance((FARPROC)EnumWndProc,hInstance);
   EnumWindows(enumproc, (LPARAM) &pi);
   FreeProcInstance((FARPROC)enumproc);

   hcur = LoadCursor(NULL, IDC_WAIT);
   hcur_old = SetCursor(hcur);
   while (1)
     {
	if (!IsWindow(pi.hWnd) || !IsTask(pi.hTask)) break;

	if ((HINSTANCE) GetWindowWord(pi.hWnd, GWW_HINSTANCE) != pi.hInstance) break;
	if (GetWindowTask(pi.hWnd) != pi.hTask) break;

	if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
	  {
	     process_message();
	     SetCursor(hcur);
	  }
     }
   SetCursor(hcur_old);
   return 0;
}

BOOL CALLBACK EnumWndProc(HWND hWnd, LPARAM lParam)
{
   Process_Info *pi = (Process_Info *) lParam;

   if ((HINSTANCE) GetWindowWord(hWnd, GWW_HINSTANCE) == pi->hInstance)
     {
	pi->hWnd = hWnd;
	pi->hTask = GetWindowTask(hWnd);
	return FALSE;
     }

   return TRUE;
}

static int dos_system(char *command_line)
{
   char *tmp_file = "jedshell.tmp";
   char buf[JED_MAX_PATH_LEN];
   FILE *f;
   int ret;
   int nCmdShow = SW_SHOWMINIMIZED;
   int wait = TRUE;

   SLsnprintf(buf, sizeof(buf), "%s\\bin\\mswshell.pif %s", Jed_Root_Dir, command_line);

   sys_delete_file(tmp_file);

   if (msw_system(buf, &nCmdShow, &wait) != 0) return 1;

   if (NULL == (f = fopen(tmp_file, "r"))) return 1;

   fgets(buf, sizeof(buf), f);
   ret = (int) atol(buf);
   *buf = 0;
   fgets(buf, sizeof(buf), f);
   fclose(f);
   sys_delete_file(tmp_file);

   if (*buf != 0)
     msg_error(buf);

   return ret;
}

int sys_System(char *command_line)
{
   return dos_system(command_line);
}

/* to make slang.lib happy
 * There is no definition of function system for MS Windows
 */

int system(const char *command)
{
   return sys_System((char *) command);
}

void sys_pause (int ms)
{
   DWORD t = GetTickCount() + ms;

   while (GetTickCount() < t)
     process_message();
}

/* Delete the file NAME.  returns 0 on failure, 1 on sucess
 * Under OS/2 and DOS, unlink()[UNIX] and remove()[ANSI] are equivalent.
 */

int sys_delete_file(char *name)
{
   return(1 + remove(name));
}

static struct ffblk The_ffblk;
static int File_Attr;

#define HIDDEN FA_HIDEN
#define SYSTEM FA_SYSTEM
#define SUBDIR FA_DIREC
#define READON FA_RDONLY

static char Found_Dir[JED_MAX_PATH_LEN];

#define lcase(x) if (((x) >= 'A') && ((x) <= 'Z')) (x) |= 0x20

static void fixup_name(char *file)
{
   int dir;
   char name[JED_MAX_PATH_LEN];
   char *p;

   strcpy (file, Found_Dir);

   strcpy (name, The_ffblk.ff_name);
   dir = (The_ffblk.ff_attrib & SUBDIR);

   p = name;
   while (*p)
     {
	lcase(*p);
	p++;
     }

   strcat(file, name);
   if (dir) strcat(file, "\\");
}

int sys_findfirst(char *thefile)
{
   char *f, the_path[JED_MAX_PATH_LEN], *file, *f1;
   char *pat;

   File_Attr = READON | SUBDIR;

   file = jed_standardize_filename_static(thefile);
   f1 = f = extract_file(file);
   strcpy (Found_Dir, file);

   Found_Dir[(int) (f - file)] = 0;

   strcpy(the_path, file);

   while (*f1 && (*f1 != '*')) f1++;
   if (! *f1)
     {
	while (*f && (*f != '.')) f++;
 	if (*f) strcat(the_path, "*");
	else
	  strcat(the_path, "*.*");
     }
   pat = the_path;

   if (!findfirst (pat, &The_ffblk, File_Attr))
     {
 	fixup_name(file);

 	strcpy(thefile, file);
 	return(1);
     }
   else
     return 0;
}

int sys_findnext(char *file)
{
   if (!findnext (&The_ffblk))
     {
 	fixup_name(file);

	return 1;
     }

   return 0;
}



int sys_chmod(char *file, int what, mode_t *mode, uid_t *dum1, gid_t *dum2)
{
   int flag, m = *mode;
   (void) dum1; (void) dum2;

   file = msdos_pinhead_fix_dir (file);
   if ((m = _chmod(file, what, m)) == -1)
     {
 	flag = errno;
 	/* Here if carry flag is set */
 	if (flag == ENOENT) return(0);/* file not found */
 	return -1;
     }
   if (what == 0)
     {
	*mode = m;
     }

   if (m & 0x10)
     {
	/* msg_error("File is a directory."); */
	return(2);
     }

   return(1);
}

extern int main(int, char **);

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
   OpenClipboard(This_Window.w);
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

	     jed_insert_chars((unsigned char *) dat, nbytes);
	     SLfree (dat);
	  }
	GlobalUnlock(hBuf);
     }

   return nbytes;
}

#if 0
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow)
{
   char **argv;
   int  argc;
   int count;
   char *pt;
   int ret;
   char *command_line;

   /* add 6 for "wjed", the separating space, and a terminating '\0' */
   if (NULL == (command_line = SLMALLOC (strlen(lpszCmdLine) + 8)))
     return 0;

   (void) hInstance;
   (void) nCmdShow;

   strcpy(command_line, "wjed ");
   strcat(command_line, lpszCmdLine);
   _hPrev = hPrevInst;
   _hInstance = hInstance;

   while ( (*command_line != '\0') && (*command_line == ' '))
     command_line++;		       /* start on 1st non-space */

   pt = command_line;
   count = 0;
   while ( *pt != '\0' )
     {
	count++;			       /* this is an argument */
	while ((*pt != '\0') && (*pt != ' '))
	  pt++;			       /* advance until a space */
	while ( *pt == ' '  )
	  pt++;			       /* advance until a non-space */
     }

   argv = (char **) SLMALLOC( (count+3) * sizeof(char *) );
   if (argv == NULL )
     return 0;			       /* malloc error */

   argc = 0;
   pt = command_line;
   while ((argc < count) && (*pt != '\0'))
     {
	argv[ argc ] = pt;
	argc++;
	while ( *pt != '\0' && *pt != ' ' )
	  pt++;			       /* advance until a space */
	if ( *pt != '\0' )
	  *(pt++) = '\0';		       /* parse argument here */
	while ( *pt == ' ')
	  pt++;			       /* advance until a non-space */
     }
   argv [ argc ] = (char *) NULL;      /* NULL terminated list */

   ret = main(argc, argv);

   SLfree(command_line);
   return ret;
}
#endif
