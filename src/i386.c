/* Copyright (c) 1992-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#ifndef __WATCOMC__
#include <sys/types.h>
#include <sys/stat.h>
#include <dir.h>
#include <pc.h>
#include <signal.h>
#else
#include <sys\types.h>
#include <sys\stat.h>
#include <direct.h>
#include <conio.h>
#include <io.h>
 /* for some reason _bios_keybrd seems to work better than kbhit()
  * even though the documentation indicates they do the same thing.
  */
#define kbhit() _bios_keybrd(_NKEYBRD_READY)
#endif /* NOT WATCOM */

#include <errno.h>

#include "sysdep.h"

#define BIOSKEY i386_bioskey

#include <dos.h>
#include <bios.h>
#include <process.h>

#include "dos_os2.c"

#ifndef __WATCOMC__
unsigned int i386_bioskey(int f)
{
   union REGS in, out;
   in.h.ah = f | 0x10;		       /* use enhanced kbd */
   int86(0x16, &in, &out);
   return(out.x.ax & 0xFFFF);
}
#endif

#include "pcscan.c"
static void read_and_buffer_keys (void)
{
   unsigned int scan;
   unsigned char keybuf[16];
   unsigned int shift;
   unsigned int num;

#ifdef __WATCOMC__
   scan = _bios_keybrd(_NKEYBRD_READ);
   shift = _bios_keybrd(_NKEYBRD_SHIFTSTATUS) & 0xF;
#else
   scan = (unsigned int) BIOSKEY(0);
   shift = BIOSKEY(2) & 0xF;
#endif

   num = jed_scan_to_key (scan & 0xFFFF, shift, keybuf);
   buffer_keystring (keybuf, num);
}

/* Here I also map keys to edt keys */

unsigned char sys_getkey()
{
   int wit = 300;

   if (
#ifdef __WATCOMC__
       (!_bios_keybrd(_NKEYBRD_READY))
#else
       (!kbhit())
#endif
       ) while (!sys_input_pending(&wit, 0))
	{
	   if (Display_Time)
	     {
		JWindow->trashed = 1;
		update((Line *) NULL, 0, 1, 0);
	     }
	}

#ifdef HAS_MOUSE
   /* This can only be set by the mouse */
   if (JMouse_Hide_Mouse_Hook != NULL) (*JMouse_Hide_Mouse_Hook) (0);
   if (Input_Buffer_Len)
     return my_getkey ();
#endif

   read_and_buffer_keys ();
   return my_getkey ();
}

void sys_pause (int ms)
{
   /* read one of the RTC registers to ensure delay will
    * work as designed */
   outportb(0x70,0x8c); inportb(0x71);
   delay (ms);
}

int sys_input_pending(int *tsecs, int unused)
{
   int count = *tsecs * 5;

   (void) unused;
   if (Batch || Input_Buffer_Len) return(Input_Buffer_Len);
   if (kbhit()) return 1;

   while (count > 0)
     {
	sys_pause (20);		       /* 20 ms or 1/50 sec */
	if (kbhit()
#ifdef HAS_MOUSE
	    || ((JMouse_Event_Hook != NULL)
		&& ((*JMouse_Event_Hook)() > 0))
#endif
	    ) break;
	count--;
     }

   return (count);
}

/* returns 0 if file does not exist, 1 if it is not a dir, 2 if it is */
int sys_chmod(SLFUTURE_CONST char *file, int what, mode_t *mode, uid_t *dum1, gid_t *dum2)
{
   struct stat buf;
   int m;
   *dum1 = *dum2 = 0;

   file = msdos_pinhead_fix_dir (file);

   if (what)
     {
	chmod(file, *mode);
	return(0);
     }

   if (stat(file, &buf) < 0) switch (errno)
     {
	case EACCES: return(-1); /* es = "Access denied."; break; */
	case ENOENT: return(0);  /* ms = "File does not exist."; */
	case ENOTDIR: return(-2); /* es = "Invalid Path."; */
	default: return(-3); /* "stat: unknown error."; break;*/
     }

   m = buf.st_mode;

/* AIX requires this */
#ifdef _S_IFDIR
#ifndef S_IFDIR
#define S_IFDIR _S_IFDIR
#endif
#endif

   *mode = m & 0777;

#ifndef __WATCOMC__
   if (m & S_IFDIR) return (2);
#else
   if (S_ISDIR(m)) return(2);
#endif
   return(1);
}

int i386_access (char *file, int mode)
{
   struct stat buf;

   if (stat(file, &buf) < 0) return -1;
   if (mode == W_OK)
     {
        if (buf.st_mode & S_IWRITE) return 0;
	return -1;
     }
   return 0;
}

#ifndef __WATCOMC__
# ifdef __GO32__
static int cbreak;
# endif
#endif

void reset_tty()
{
#ifndef __WATCOMC__
# ifdef __GO32__
#  if __DJGPP__ > 1
   signal (SIGINT, SIG_IGN);	       /* was sig_dfl */
#  endif
   setcbrk(cbreak);
# endif
#endif

#ifdef HAS_MOUSE
   if (X_Close_Mouse_Hook != NULL) (*X_Close_Mouse_Hook) ();
#endif
}

int init_tty()
{
#ifndef __WATCOMC__
# if __DJGPP__ > 1
   signal (SIGINT, SIG_IGN);
# endif
   cbreak = getcbrk();
   setcbrk(0);
#endif

#ifdef HAS_MOUSE
   if (X_Open_Mouse_Hook != NULL) (*X_Open_Mouse_Hook) ();
#endif

   return 0;
}

#ifndef __WATCOMC__
static struct ffblk Dos_DTA;
#else
static struct find_t fileinfo;
#endif

static char Found_Dir[JED_MAX_PATH_LEN], *Found_File;
/* found_File is a pointer into found_Dir such that the
 * full pathname is stored in the following form
 * "c:/dir/path/\0filename.ext\0"
 */

#if JED_FILENAME_LOWERCASE
#define lcase(x) ((((x) >= 'A') && ((x) <= 'Z')) ? ((x) |= 0x20) : (x))
static void lowercase_filename (char *p)
{
   char ch;
   while (0 != (ch = *p))
     *p++ = lcase (ch);
}
#endif

static void dta_fixup_name (char *file)
{
#ifndef __WATCOMC__
   strcpy (Found_File, Dos_DTA.ff_name);
#else
   strcpy (Found_File, fileinfo.name);
#endif

#if JED_FILENAME_LOWERCASE
   lowercase_filename (Found_File);
#endif

   strcpy(file, Found_Dir);
   strcat(file, Found_File);

#ifndef __WATCOMC__
   if (Dos_DTA.ff_attrib & FA_DIREC) strcat(file, "\\");
#else
   if (fileinfo.attrib & _A_SUBDIR) strcat(file, "\\");
#endif
}

int sys_findfirst(char *file)
{
   char *f;

   strcpy(Found_Dir, jed_standardize_filename_static(file) );
   Found_File = extract_file( Found_Dir );

   f = Found_File;

   while (*f && (*f != '*')) f++;
   if (! *f)
     {
	f = Found_File;
	while (*f && (*f != '.')) f++;
	if (*f) strcat(Found_Dir, "*");
	else strcat(Found_Dir, "*.*");
     }

#ifndef __WATCOMC__
   if (findfirst(Found_Dir, &Dos_DTA, FA_RDONLY | FA_DIREC))
#else
   if (_dos_findfirst(Found_Dir, _A_RDONLY | _A_SUBDIR, &fileinfo))
#endif
     {
	*Found_File++ = 0;
	return 0;
     }
   *Found_File++ = 0;

   dta_fixup_name(file);
   return(1);
}

int sys_findnext(char *file)
{
#ifndef __WATCOMC__
   if (findnext(&Dos_DTA)) return(0);
#else
   if (_dos_findnext (&fileinfo)) return(0);
#endif
   dta_fixup_name(file);
   return(1);
}

/* This routine is called from S-Lang inner interpreter.  It serves
   as a poor mans version of an interrupt 9 handler */

void i386_check_kbd()
{
   while (kbhit())
     {
	read_and_buffer_keys ();
     }
}
