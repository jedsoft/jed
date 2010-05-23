/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

/*{{{ system include files */

#ifdef __WIN32__
/* This needs to go first before stdio is included. */
# include <windows.h>
# include <fcntl.h>
# include <io.h>
# include <sys/stat.h>
#endif

#ifdef __IBMC__
extern unsigned int sleep (unsigned int);
#endif

#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#include <string.h>
#include <limits.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef __unix__
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/file.h>
#endif

#ifdef HAVE_UTIME
# include <utime.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#endif

#ifdef __os2__
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
 
 typedef struct HOLDFEA *PHOLDFEA;
 PHOLDFEA QueryEAs (char *name);
 int WriteEAs (char *name, PHOLDFEA pHoldFEA);
#endif

#ifdef __MSDOS__
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#endif

#if defined(__DECC) && defined(VMS)
# include <unixio.h>
# include <unixlib.h>
#endif

#include <errno.h>
#include <signal.h>

#ifdef VMS
# include <stat.h>
# include <rms.h>
#endif

/* Was anything missed? */
#ifndef O_RDONLY
# ifdef VMS
#  include <file.h>
# else
#  include <fcntl.h>
# endif
#endif

#ifndef O_BINARY
# define O_BINARY	0
#endif

/*}}}*/
/*{{{ local inclue files */

#include "buffer.h"
#include "file.h"
#include "misc.h"
#include "sysdep.h"
#include "paste.h"
#include "ins.h"
#include "ledit.h"
#include "userinfo.h"
#include "hooks.h"
#include "screen.h"
#include "sig.h"

/*}}}*/

#if defined (SIXTEEN_BIT_SYSTEM)
#define MAX_LINE_LEN 1024
#else
#define MAX_LINE_LEN 64 * 1024
#endif

int Jed_Backup_By_Copying = 0;

#ifdef VMS
/*{{{ vms_stupid_open */

static int vms_max_rec_size;
static int VMS_write_rfm_fixed;
int vms_stupid_open(char *file)
{
   struct stat s;
   char rat_buf[80], rfm_buf[80], mrs_buf[40], *rfm = "var";
   unsigned short mode = 0, c;
   int ret;

   VMS_write_rfm_fixed = 0;

   strcpy(rfm_buf, "rfm=");
   
   
   if (0 == stat(file, &s))
     {
	strcpy(rat_buf, "rat");
	c = s.st_fab_rat;
	if (c & FAB$M_FTN)  strcat(rat_buf, ",ftn");
	if (c & FAB$M_CR)  strcat(rat_buf, ",cr");
	if (c & FAB$M_PRN)  strcat(rat_buf, ",prn");
	if (c & FAB$M_BLK)  strcat(rat_buf, ",blk");
	if (rat_buf[3] != 0) rat_buf[3] = '='; else *rat_buf = 0;

	c = s.st_fab_rfm;
	switch(c)
	  {
	   case FAB$C_UDF:  rfm = "udf"; break;
	   case FAB$C_FIX:  
	     rfm = "fix"; 
	     if (s.st_fab_rat & (FAB$M_CR | FAB$M_CR))
	       VMS_write_rfm_fixed = 1;
	     break;

	   case FAB$C_VAR:  rfm = "var"; break;
	   case FAB$C_VFC:  rfm = "vfc"; break;
	   case FAB$C_STM:  rfm = "stm"; break;
	   case FAB$C_STMLF:  rfm = "stmlf"; break;
	   case FAB$C_STMCR:  rfm = "stmcr"; break;
	  }
	mode = s.st_mode & 0777;
     }
   else strcpy (rat_buf, "rat=cr");
   
   strcat(rfm_buf, rfm);
   
   if (vms_max_rec_size <= 0) vms_max_rec_size = 255;
   sprintf(mrs_buf, "mrs=%d", vms_max_rec_size);
      
   if (*rfm == 's')		       /* stream */
     {
	ret = creat(file, mode, rfm_buf);
     }
   else
     {
	if (*rat_buf) ret = creat(file, mode, rfm_buf, mrs_buf, rat_buf);
	else ret = creat(file, mode, rfm_buf, mrs_buf);
     }
   if (ret >= 0) chmod(file, mode);
   return ret;
}

/*}}}*/
#endif

int Require_Final_Newline = 0;

#define _JED_OPEN_READ			0
#define _JED_OPEN_WRITE			1
#define _JED_OPEN_APPEND		2
#define _JED_OPEN_CREATE_EXCL		3

/* 0 = read, 1 = write , 2 = append... */
static int sys_open (char *file, int acces) /*{{{*/
{
   int flags;
   unsigned int perms;
#ifdef VMS
   char *p, neew[JED_MAX_PATH_LEN];
#endif

   flags = file_status (file);
   if ((flags < 0) || (flags > 1)) 
     return -1;			       /* directory? */
   
#ifdef VMS
   /* on VMS I cheat since I do not want to deal with RMS at this point */
   VMS_write_rfm_fixed = 0;
   safe_strcpy(neew, file, sizeof (neew));
   p = neew; while (*p) if (*p == ';') *p = 0; else p++;
   
   switch (acces)
     {
      case _JED_OPEN_READ:
	return open(file, O_RDONLY, "ctx=rec","mbf=8","mbc=32","rop=RAH","shr=upi,get,put");

      case _JED_OPEN_WRITE:
      case _JED_OPEN_CREATE_EXCL:      /* FIXME */
	return vms_stupid_open (neew);

      case _JED_OPEN_APPEND:
	return open (file, O_WRONLY | O_APPEND | O_CREAT | O_BINARY);
      default:
	return -1;
     }

#else

# ifdef IBMPC_SYSTEM
   perms = S_IREAD | S_IWRITE;
# else
   perms = 0666;
# endif

   switch (acces)
     {
      case _JED_OPEN_READ:
	flags = O_RDONLY; 
	break;
      case _JED_OPEN_WRITE: 
	flags = O_WRONLY | O_CREAT | O_TRUNC;
	break;
      case _JED_OPEN_APPEND: 
	flags = O_WRONLY | O_CREAT | O_APPEND; 
	break;
      case _JED_OPEN_CREATE_EXCL:
	flags = O_WRONLY | O_CREAT | O_EXCL;
#ifndef IBMPC_SYSTEM
	perms = 0600;
#endif
	break;
	
      default:
	return -1;
     }
   
   flags |= O_BINARY;
   
   return open(file, flags, perms);
#endif /* VMS */
}

/*}}}*/

char *file_type (SLFUTURE_CONST char *file) /*{{{*/
{
   char *p, *psave;
   
   if ((file == NULL)
       || (*file == 0))
     return NULL;

   file = extract_file(file);
   p = (char *) file; while (*p != 0) p++;
   psave = p;
   while((p != file) && (*p != '.')) p--;
   if (*p == '.') p++;
   if (p == file) return psave;
   return p;
}

/*}}}*/


void jed_set_buffer_ctime (Buffer *b)
{
   char *file;

   if ((b->file == NULL)
       || (b->file[0] == 0))
     {
	b->c_time = 0;
	return;
     }

   if (NULL == (file = SLang_concat_slstrings (b->dir, b->file)))
     {
	b->c_time = 0;
	return;
     }
   b->c_time = sys_file_mod_time (file);
   
   SLang_free_slstring (file);
}

#if (defined(__MSDOS__) || defined(__WIN32__)) && !defined(W_OK)
# define W_OK 2
# define F_OK 0
#endif

#ifdef __GO32__
# define access i386_access
#endif

int jed_file_is_readonly (char *file, int respect_perms)
{
   int ro = 0;
   struct stat st;

   if (respect_perms)
     {
	/* respect the file's permissions.  If any write permission bit is
	 * is set, then consider it a writable candidate.
	 */
#if defined(S_IWGRP) && defined(S_IWOTH)
	if (0 == stat (file, &st))
	  ro = (0 == (st.st_mode & (S_IWUSR|S_IWGRP|S_IWOTH)));
#endif
     }

#ifdef W_OK
   if (ro == 0)
     {
	if (0 == access(file, F_OK))
	  {
	     if (-1 == access(file, W_OK))
	       ro = 1;
	  }
	else
	  {
	     /* file does not exist.  Can we write to the directory? */
	     char *dir;
	     
	     if (-1 == jed_dirfile_to_dir_file (file, &dir, NULL))
	       return -1;

# if defined(IBMPC_SYSTEM)
	     /* knock of slash since some primitive systems cannot handle the 
	      * final slash in a path name. 
	      */
	       {
		  unsigned int len = strlen (dir);
		  if (len > 3)   /* allow C:/, but not C:/xx/ */
		    dir[len-1] = 0;
	       }
# endif
	     if ((0 == access (dir, F_OK))
# ifdef VMS
		 && (-1 == access (dir, X_OK))
#else
		 && (-1 == access (dir, W_OK))
#endif
		)
	       ro = 1;
	     
	     SLfree (dir);
	  }
     }
#endif				       /* W_OK */
   return ro;
}

int jed_buffer_file_is_readonly (Buffer *b)
{
   char *file;
   int ro;

   if ((b->file == NULL)
       || (b->file[0] == 0))
     return 0;

   file = jed_dir_file_merge (b->dir, b->file);
   if (file == NULL)
     return -1;

   ro = jed_file_is_readonly (file, 1);

   SLfree (file);
   return ro;
}

void set_file_modes (void) /*{{{*/
{
   char *type;

   if (CBuf == NULL) return;

   jed_set_buffer_ctime (CBuf);
   if (1 == jed_buffer_file_is_readonly (CBuf))
     CBuf->flags |= READ_ONLY;

   type = NULL;

   if ((CBuf->file != NULL)
       && (CBuf->file[0] != 0))
     {
	CBuf->flags |= AUTO_SAVE_BUFFER;
	CBuf->hits = 0;

	if (type == NULL) 
	  type = file_type (CBuf->file);
     }

   CBuf->modes = NO_MODE;
#if JED_HAS_LINE_ATTRIBUTES
   CBuf->min_unparsed_line_num = 1;
   CBuf->max_unparsed_line_num = Max_LineNum + CBuf->nup;
#endif

   if (type != NULL)
     {
	if (1 != jed_va_run_hooks ("_jed_set_mode_hooks", 
				   JED_HOOKS_RUN_UNTIL_NON_0, 1, type))
	  (void) SLang_run_hooks("mode_hook", 1, type);
     }
}

/*}}}*/

/*{{{ reading/inserting files */

int read_file(char *file) /*{{{*/
{
   int n, status;

   status = jed_va_run_hooks ("_jed_read_file_hooks", JED_HOOKS_RUN_UNTIL_NON_0, 1, file);
   if (status < 0)
     return -1;
   
   if (status > 0)
     /* FIXME!! Is this true?  What if no lines have been read. */
     n = Max_LineNum;
   else
     {
	int fp;

	if ((fp = sys_open(file, _JED_OPEN_READ)) < 0)
	  {
	     status = file_status(file);
	     if (!status) return(-1);  /* file does not exist */
	     return(-2); /* exists but not readable */
	  }
	n = read_file_pointer(fp);
	close(fp);
     }

   eob();
   if ((Point < CLine->len)
       && ('\n' == *(CLine->data + Point)))
     make_line(2);

   VFile_Mode = VFILE_TEXT;
   return n;
}

/*}}}*/
int insert_file_pointer(VFILE *vp) /*{{{*/
{
   int n = 0;
   unsigned int num;
   unsigned char *vbuf;
   
   Suspend_Screen_Update = 1;
   while(NULL != (vbuf = (unsigned char *) vgets(vp, &num)))
     {
	n++;
	if (SLang_get_error ())
	  break;

	if (-1 == jed_quick_insert (vbuf, (int) num))
	  return -1;
     }
   return(n);
}

/*}}}*/
int insert_file(char *file) /*{{{*/
{
   VFILE *vp;
   int n;
   unsigned int un;

   un = Max_LineNum;
   if (1 == jed_va_run_hooks ("_jed_insert_file_hooks", JED_HOOKS_RUN_UNTIL_NON_0, 1, file))
     return (int) (Max_LineNum - un);

   if (NULL == (vp = vopen(file, 0, VFile_Mode))) return -1;
   n = insert_file_pointer(vp);
   vclose(vp);
   return(n);
}

/*}}}*/

/*}}}*/

/*{{{ writing to files */

#ifdef __unix__
# define BUFSIZE 0x10000
#else
#ifdef VMS
# define BUFSIZE 0x3FFF
#else 
# define BUFSIZE 512
#endif
#endif

static int Output_Buffer_Size = BUFSIZE;
static char Output_Buffer[BUFSIZE];
static char *Output_Bufferp;
static char *Output_Bufferp_max;


/* definitely perform the write.  Return number of chars written */
static int jed_write1(int fd, char *b, unsigned int n) /*{{{*/
{
#if !defined(IBMPC_USE_ASM)
   int len;
   unsigned int total = 0;
#ifdef VMS
   register char *bmax;
#endif
   
   while (total < n)
     {
	int dlen;
	len = n - total;
#ifdef VMS
	if (VMS_write_rfm_fixed)
	  {
	  }   
	/* VMS wants to terminate a record with a cr so adjust for this 
	 * unfortunate fact.  The len - 1 stuff is so *bmax does not peek 
	 * beyond its boundary.
	 */
	bmax = b + (len - 1);
	while ((bmax > b) && (*bmax != '\n')) bmax--;
	if (bmax == b) bmax = b + (len - 1); /* cannot be helped */
	len = (int) (bmax - b) + 1;
#endif
	while (-1 == (dlen = write (fd, b, len)))
	  {
#ifdef EINTR
	     if (errno == EINTR)
	       {
		  if (0 == jed_handle_interrupt ())
		    continue;
	       }
#endif
#ifdef EAGAIN
	     if (errno == EAGAIN)
	       {
		  if (0 == jed_handle_interrupt ())
		    {
		       jed_sleep (1);
		       continue;
		    }
	       }
#endif
#ifdef ENOSPC
	     if (errno == ENOSPC)
	       {
		  msg_error ("Write Failed: Disk Full.");
		  return total;
	       }
#endif
	     jed_verror ("Write Failed: (%d bytes attemped, errno = %d)", len, errno);
	     return total;
	  }
	
	total += (unsigned int) dlen;
	b += dlen;
     }
   return total;
#else
   int num = -1;
   asm mov ah, 40h
   asm mov bx, fd
   asm mov cx, n
   asm push ds
   asm lds dx, dword ptr b
   asm int 21h
   asm pop ds
   asm jc L1
   asm mov num, ax		       /* number of bytes written */
   L1: 
   return(num);
#endif
}

/*}}}*/


/* RMS wants to start a NEW record after a write so just forget it! */
/* maybe do write-- return number of chars possibly written */
static int jed_write(int fd, char *b, unsigned int n) /*{{{*/
{
   int num, max, nl_flag = 0;
   unsigned int nsave = n;
   int cr_flag = CBuf->flags & ADD_CR_ON_WRITE_FLAG;
   
#ifdef MAP_CR_TO_NL_FLAG
   if (CBuf->flags & MAP_CR_TO_NL_FLAG)
     {
	char *bmax = b + n;
	char *p, *pmax, ch;
	p = Output_Bufferp;
	pmax = Output_Bufferp_max;
	
	while (b < bmax)
	  {
	     ch = *b++;
	     if ((ch == '\r') || (ch == '\n'))
	       {
		  if (cr_flag)
		    {
		       *p++ = ch;
		       if (p == pmax)
			 {
			    num = (int) (Output_Bufferp_max - Output_Bufferp);
			    if (num != jed_write1 (fd, Output_Bufferp, num))
			      return -1;
			    Output_Bufferp = Output_Buffer;
			    p = Output_Bufferp;
			 }
		    }
		  *p++ = '\n';
	       }
	     else *p++ = ch;
	     
	     if (p == pmax)
	       {
		  num = (int) (Output_Bufferp_max - Output_Bufferp);
		  if (num != jed_write1 (fd, Output_Buffer, num))
		    return -1;
		  Output_Bufferp = Output_Buffer;
		  p = Output_Bufferp;
	       }
	  }
	Output_Bufferp = p;
	return nsave;
     }
#endif		  
   /* amount of space left in buffer */
   /* copy whats in b to the output buffer */
   while (n > 0)
     {
	num = (int) (Output_Bufferp_max - Output_Bufferp);
	if ((int) n > num)
	  {
#ifdef VMS
	     max = (int) (Output_Bufferp - Output_Buffer);
	     if (max)
	       {
		  if (max != jed_write1(fd, Output_Buffer, max))
		    return(-1);
		  Output_Bufferp = Output_Buffer;
		  continue;
	       }
#endif		  
	     max = num;
	     SLMEMCPY(Output_Bufferp, b, max);
	     Output_Bufferp += max;
	  }

	else if (cr_flag && 
		 (*(b + (n - 1)) == '\n') && (VFile_Mode == VFILE_TEXT))
	  {
	     max = n - 1;
	     SLMEMCPY(Output_Bufferp, b, max);
	     Output_Bufferp += max;
	     *Output_Bufferp++ = '\r';
	     max++;

	     /* can only write the \r */
	     if (n == (unsigned int) num) nl_flag = 1; else *Output_Bufferp++ = '\n';
	  }
	else
	  {
	     max = n;
	     SLMEMCPY(Output_Bufferp, b, max);
	     Output_Bufferp += max;
	  }
	
	if (Output_Bufferp == Output_Bufferp_max)
	  {
	     Output_Bufferp = Output_Buffer;
	     if (Output_Buffer_Size != jed_write1(fd, Output_Buffer, Output_Buffer_Size)) return(-1);
	     if (nl_flag)
	       {
		  nl_flag = 0;
		  *Output_Bufferp++ = '\n';
	       }
	  }
	b += max;
	n -= max;
     }
   return(nsave);
}

/*}}}*/

/* returns -1 on failure */
int write_region_to_fp (int fp) /*{{{*/
{
   register int pnt, len;
   register Line *first, *last;
   int last_pnt, n = 0;
   char *err = "Write Failed!";

#ifndef VMS
   char nl = '\n';
#endif
   
   Output_Bufferp = Output_Buffer;
   Output_Bufferp_max = Output_Buffer + BUFSIZE;
   Output_Buffer_Size = BUFSIZE;

#ifdef VMS
   if (VMS_write_rfm_fixed && (vms_max_rec_size <= BUFSIZE))
     {
	Output_Buffer_Size = vms_max_rec_size;
     }
   else VMS_write_rfm_fixed = 0;
#endif   
   
   if (!check_region(&Number_One)) return(-1);
   last = CLine; last_pnt = Point;

   jed_pop_mark(1);
   first = CLine; pnt = Point;

   /* first should never be null without hitting last first.  If this
      ever happens, check_region failed. */
   while (first != last)
     {
	len = first->len - pnt;
	if (len != jed_write(fp, (char *) (first->data + pnt), len))
	  {
	     msg_error(err);
	  }
	
	/* This goes here inside the loop because it is possible for external
	   events to set error_buffer */
	pnt = 0;
	if (SLang_get_error ()) break;
	first = first->next;
	n++;
     }

   if (!SLang_get_error () && (last_pnt != 0))
     {
	len = last_pnt - pnt;
	if (len != jed_write(fp, (char *) (last->data + pnt), len))
	  {
	     msg_error(err);
	  }
	n++;
     }
#ifndef VMS
   if ((Require_Final_Newline) && (CBuf->end == last))
     {
	eob(); if (Point) jed_write(fp, &nl, 1);
     }
#endif
   

   /* Now flush output buffer if necessary */
   
   len = (int) (Output_Bufferp - Output_Buffer);
   if (!SLang_get_error () && len) if (len != jed_write1(fp, Output_Buffer, len))
     {
	msg_error(err);
     }
   
   Output_Bufferp = Output_Buffer;

   
   pop_spot();
   VFile_Mode = VFILE_TEXT;
   if (SLang_get_error ()) return(-1);
   return(n);
}

/*}}}*/

static int jed_close (int fp, int use_fsync) /*{{{*/
{
#ifdef HAVE_FSYNC
   if (use_fsync) while (-1 == fsync (fp))
     {
# ifdef EINTR
	if (errno == EINTR)
	  {
	     (void) jed_handle_interrupt ();
	     errno = 0;
	     jed_sleep (1);
	     continue;
	  }
# endif
# ifdef EIO
	if (errno == EIO)
	  {
	     msg_error ("Error fsyncing file.  File system may be full.");
	     return -1;
	  }
# endif
	break;
     }
#endif

   while (-1 == close(fp))
     {
#ifdef EINTR
#ifndef IBMPC_SYSTEM
	if (errno == EINTR) 
	  {
	     if (-1 == jed_handle_interrupt ())
	       {
		  errno = 0;
		  jed_sleep (1);
		  continue;
	       }
	  }
#endif
#endif
	msg_error ("Error closing file.  File system may be full.");
	return -1;
     }
   return 0;
}

/*}}}*/

static int write_region_internal (char *file, int omode, int use_fsync) /*{{{*/
{
   int fd;
   int n;
   unsigned int num_lines;

#if JED_HAS_EMACS_LOCKING
   if (-1 == jed_lock_file (file))
     return -1;
#endif
   if (!check_region(&Number_Zero)) 
     return -1;
   
   num_lines = jed_count_lines_in_region ();

   switch (omode)
     {
      case _JED_OPEN_WRITE:
      case _JED_OPEN_CREATE_EXCL:
	n = jed_va_run_hooks ("_jed_write_region_hooks",
			      JED_HOOKS_RUN_UNTIL_NON_0, 1, file);
	break;
	
      default:
	n = jed_va_run_hooks ("_jed_append_region_hooks",
			      JED_HOOKS_RUN_UNTIL_NON_0, 1, file);
	break;
     }

   if (n > 0)
     n = num_lines;
   else if (n < 0)
     n = -1;
   else if (n == 0)
     {
	if ((fd = sys_open(file, omode)) < 0)
	  {
	     jed_verror ("Unable to open %s for writing.", file);
	     return -1;
	  }

	n = write_region_to_fp (fd);
	if (n == -1)
	  (void) jed_close (fd, 0);
	else if (-1 == jed_close (fd, use_fsync))
	  n = -1;
     }
   
#if JED_HAS_EMACS_LOCKING
   if (n != -1) jed_unlock_file (file);
#endif
   return n;
}

/*}}}*/

int write_region (char *file)
{
   return write_region_internal (file, _JED_OPEN_WRITE, 1);
}


/* returns -1 on failure and number of lines on success */
static int write_file_internal (char *file, int omode, int use_fsync) /*{{{*/
{
   Mark *m;
   int n;
   int fnl;
      
#ifdef VMS
   register Line *l;
   register int len = 0, max = 0;
#endif
   
   push_spot();
   
#if JED_HAS_SAVE_NARROW
   jed_push_narrow ();
   jed_widen_whole_buffer (CBuf);
#endif
   
#ifdef VMS
   l = CBuf->beg;
   while (l != NULL)
     {
	len = l->len;
	if (len > max) max = len;
	l = l->next;
     }
   vms_max_rec_size = max;
#endif
   
   bob();
   jed_push_mark();  m = CBuf->marks;
   eob();
   fnl = Require_Final_Newline;
   if (CBuf->flags & BINARY_FILE) 
     {
	VFile_Mode = VFILE_BINARY;
	Require_Final_Newline = 0;

#ifdef VMS
	vms_max_rec_size = 512;
#endif
     }


   n = write_region_internal (file, omode, use_fsync);

   Require_Final_Newline = fnl;
   VFile_Mode = VFILE_TEXT;
   if (m == CBuf->marks) jed_pop_mark(0);
   
#if JED_HAS_SAVE_NARROW
   jed_pop_narrow ();
#endif   
   
   pop_spot();
   return(n);
}

/*}}}*/

int append_to_file(char *file) /*{{{*/
{
   int status;
   
   status = write_region_internal (file, _JED_OPEN_APPEND, 1);
   check_buffers();
   return status;
}

/*}}}*/

/*}}}*/

static int make_autosave_filename(char *save, unsigned int buflen, char *dir, char *file) /*{{{*/
{
   char *s;

   if (*file == 0) return(0);
      
   
   if (1 == SLang_run_hooks ("make_autosave_filename", 2, dir, file))
     {
	if (SLang_pop_slstring(&s)) return(0);
	strncpy(save, s, buflen);
	save[buflen - 1] = 0;
	SLang_free_slstring (s);
     }
   else
     {
#if defined(IBMPC_SYSTEM)
	SLsnprintf (save, buflen, "%s#%s", dir, file);
#else
# ifdef VMS
	SLsnprintf (save, buflen, "%s_$%s;1", dir, file);
# else
	SLsnprintf (save, buflen, "%s#%s#", dir, file);
# endif
#endif
     }
   return 1;
}

/*}}}*/

#ifndef VMS
int jed_copy_file (char *from, char *to) /*{{{*/
{
   mode_t mode;
   uid_t uid;
   gid_t gid;
   FILE *fp0, *fp1;
   char buf[0x7FFF];
   unsigned int readlen;
   int ret;
   struct stat st;
#ifdef HAVE_UTIME
   struct utimbuf ut;
#endif

   if (1 != sys_chmod (from, 0, &mode, &uid, &gid))
     return -1;		       /* from does not exist as regular file */
   
   /* Need file modification times so that they can be preserved. */
   if (-1 == stat (from, &st))
     return -1;
   
   fp0 = fopen (from, "rb");
   if (fp0 == NULL) return -1;

#ifdef REAL_UNIX_SYSTEM
   (void) sys_delete_file (to);
   /* Try to avoid a race condition (code derived from Colin Phipps <crp22@cam.ac.uk>. */
   ret = sys_open (to, _JED_OPEN_CREATE_EXCL);
   if (ret == -1)
     {
	(void) fclose (fp0);
	return -1;
     }
   if (NULL == (fp1 = fdopen (ret, "wb")))
     {
	(void) close (ret);
	(void) sys_delete_file (to);
	/* Drop */
     }
#else
   fp1 = fopen (to, "wb");
#endif

   if (fp1 == NULL) 
     {
	(void) fclose (fp0);
	return -1;
     }
   
   (void) chmod (to, 0600);

   ret = 0;
   do
     {
	readlen = fread (buf, 1, sizeof(buf), fp0);
	if (readlen)
	  {
	     if (readlen != fwrite (buf, 1, readlen, fp1))
	       {
		  ret = -1;
		  break;
	       }
	  }
     }
   while (readlen == sizeof (buf));
   
   fclose (fp0);

   if (EOF == fclose (fp1))
     {
	ret = -1;
     }
   
   (void) sys_chmod (to, 1, &mode, &uid, &gid);

#ifdef HAVE_UTIME
   /* Set file modification times */
   ut.actime = st.st_atime;
   ut.modtime = st.st_mtime;
   (void) utime (to, &ut);
#endif

   return ret;
}
/*}}}*/
#endif				       /* NOT VMS */

#ifndef VMS   
static int perform_backup (char *from, char *to, int try_force_rename) /*{{{*/
{
   int ret = -1;
   int use_copy = Jed_Backup_By_Copying;
#ifdef REAL_UNIX_SYSTEM
   /* If the file has hardlinks, then backup by copying. */
   struct stat st;
   
   if (0 == stat (from, &st))
     {
	if (st.st_nlink > 1)
	  use_copy = 1;
     }
#endif

   if (try_force_rename
       || (use_copy == 0))
     {
	(void) unlink(to);
#ifdef __WIN32__
	/* Rename is broken on win32 */
	ret = jed_win32_rename (from, to);
#else
	ret = rename (from, to);
#endif
     }

   if (ret == -1)
     ret = jed_copy_file (from, to);

   return ret;
}
/*}}}*/
#endif

unsigned long sys_file_mod_time(char *file) /*{{{*/
{
   struct stat buf;
   
   if (stat(file, &buf) < 0) return 0;

   return (unsigned long) buf.st_mtime;
}

/*}}}*/

int write_file_with_backup(char *dirfile) /*{{{*/
{
   char autosave_file[JED_MAX_PATH_LEN];
   int n;
   int do_mode;
   uid_t uid;
   gid_t gid;
   mode_t mode;
#ifndef VMS
   char *old = NULL;
   int backup_went_ok;
#endif
#ifdef __os2__
   PHOLDFEA EAs;
#endif
   char *dir, *file;

   if (NULL == (dirfile = jed_expand_link (dirfile)))
     return -1;

   if (-1 == jed_dirfile_to_dir_file (dirfile, &dir, &file))
     {
	SLfree (dirfile);
	return -1;
     }

   if (*file == 0)
     {
	SLfree (dir);
	SLfree (dir);
	SLfree (dirfile);
	return -1;
     }

   *autosave_file = 0;
   if (CBuf->flags & AUTO_SAVE_BUFFER)
     (void) make_autosave_filename(autosave_file, sizeof (autosave_file), dir, file);

   do_mode = sys_chmod(dirfile, 0, &mode, &uid, &gid);
   if ((do_mode != 0) && (do_mode != 1)) 
     {
	SLfree (dir);
	SLfree (file);
	SLfree (dirfile);
	return -1;
     }

   /* IF do_mode == 0, then the file does not exist.  This means that 
    * there is nothing to backup.  If do_mode == 1, the file is a regular
    * file which we do want to backup.
    */

#ifndef VMS
   
# ifdef __os2__
   EAs = QueryEAs (dirfile);
# endif
   
   backup_went_ok = 0;
   if (do_mode
       && ((CBuf->flags & NO_BACKUP_FLAG) == 0)
       && (1 == SLang_run_hooks("make_backup_filename", 2, dir, file)))
     {
	if ((0 == SLang_pop_slstring(&old))
	    && (*old != 0))
	  {
	     backup_went_ok = (0 == perform_backup (dirfile, old, 0));
	  }
     }
#endif				       /* NOT VMS */

   n = write_file_internal (dirfile, _JED_OPEN_WRITE, 1);
   /* This is for NFS time problems.  Even if the write failed, modify the 
    * buffer's ctime because otherwise what is on the disk (a partial file) 
    * will appear newer than the buffer.
    */
   CBuf->c_time = sys_file_mod_time(dirfile);

   if (n != -1)
     {
	if (*autosave_file)
	  (void) sys_delete_file (autosave_file);

	if (do_mode) /* must be an existing file, so preserve mode */
	  {
#if defined(__MSDOS__)
	     /* Want the archive bit set since this is a new version */
	     mode |= 1 << 5;
#endif
	     sys_chmod (dirfile, 1, &mode, &uid, &gid);
#ifdef __os2__
	     WriteEAs (dirfile, EAs);
#endif
	  }
	
	/* Since we wrote the buffer to the file, it is not modified. */
	if (CBuf == find_file_buffer (dirfile))
	  CBuf->flags &= ~FILE_MODIFIED;
	
	mark_buffer_modified (CBuf, 0, 1);
     }
#ifndef VMS
   /* error -- put it back */
   else if (backup_went_ok) perform_backup (old, dirfile, 1);

   if (old != NULL) SLang_free_slstring (old);
#endif

#ifdef REAL_UNIX_SYSTEM
   (void) jed_get_inode_info (dirfile, &CBuf->device, &CBuf->inode);
#endif
   SLfree (dir);
   SLfree (file);
   SLfree (dirfile);

   return(n);
}

/*}}}*/

void auto_save_buffer(Buffer *b) /*{{{*/
{
   char tmp[JED_MAX_PATH_LEN];
   Buffer *old_buf;
   unsigned int vfm = VFile_Mode;
   char *dir, *file;

   if (b == NULL) return;
   b->hits = 0;
   if ((0 == (b->flags & BUFFER_MODIFIED))
       || (0 == (b->flags & (AUTO_SAVE_BUFFER | AUTO_SAVE_JUST_SAVE))))
     return;

   if (b->canonical_dirfile == NULL)
     return;

#if !JED_HAS_SAVE_NARROW
   if (b->narrow != NULL) return;
#endif

   if (-1 == jed_dirfile_to_dir_file (b->canonical_dirfile, &dir, &file))
     return;

   old_buf = CBuf;
   switch_to_buffer(b);
   
   if (b->flags & BINARY_FILE)  VFile_Mode = VFILE_BINARY; 
   else VFile_Mode = VFILE_TEXT;

   if (b->flags & AUTO_SAVE_BUFFER)
     {
	if (make_autosave_filename(tmp, sizeof (tmp), dir, file))
	  {
	     flush_message("autosaving..."); 
	     (void) sys_delete_file(tmp);
	     (void) write_file_internal (tmp, _JED_OPEN_CREATE_EXCL, 0);
	     message("autosaving...done");
	  }
      }
   else (void) write_file_with_backup(b->canonical_dirfile);

   SLfree (file);
   SLfree (dir);
   
   switch_to_buffer(old_buf);
   VFile_Mode = vfm;
}

/*}}}*/

void auto_save_all (void) /*{{{*/
{
    Buffer *b;

   if (NULL == (b = CBuf)) return;
   do
     {
	jed_widen_whole_buffer (b);
	if (CBuf == NULL) return;
	if (*b->file != 0) auto_save_buffer(b);
	if (CBuf == NULL) return;

	b = b->next;
     }
   while (b != CBuf);
}

/*}}}*/

#ifdef REAL_UNIX_SYSTEM
/*{{{ symbolic link stuff */

static int is_link(char *f, char *f1) /*{{{*/
{
   struct stat s;
   unsigned int l;
   int is_dir = 0;
   char work[JED_MAX_PATH_LEN];
   
   l = strlen(f);
   if (l == 0) return 0;
   l--;
   if ((l > 1) && (f[l] == '/'))
     {
	safe_strcpy(work, f, sizeof (work));
	is_dir = 1;
	f = work;
	f[l] = 0;
     }
   

   if (( lstat(f, &s) == -1 ) 
       /* || ((s.st_mode & S_IFMT)  S_IFLNK)) */
       || (((s.st_mode & S_IFMT) & S_IFLNK) == 0))
     return(0);
   
   l = JED_MAX_PATH_LEN - 1;
   if (is_dir)
     {
	/* This way, it will be expanded properly upon return */
	*f1++ = '.'; *f1++ = '.'; *f1++ = '/';
	l -= 4;			       /* 1 for final slash */
     }
	     
   if (-1 == (int) (l = readlink(f, f1, l))) return 0;
   if (is_dir) f1[l++] = '/';
   f1[l] = 0;
   
   /* If the link is an absolute pathname (starts with /) then the 
    * ../ prefix should not go there.
    */
   if (is_dir && (*f1 == '/'))
     {
	char ch;
	
	f = f1 - 3;
	while (0 != (ch = *f1++))
	  *f++ = ch;
	
	*f = 0;
     }

   return(1);
}

/*}}}*/

/*}}}*/
#endif


#ifdef REAL_UNIX_SYSTEM
static char *expand_link_2(char *f) /*{{{*/
{
   char work[JED_MAX_PATH_LEN], lnk[JED_MAX_PATH_LEN];
   char *d = jed_standardize_filename (f);
   unsigned int max_num = 20;

   while ((d != NULL) && is_link(d, lnk))
     {
	char *w, *w1, ch;
	
	if (*lnk == '/')	       /* absolute */
	  safe_strcpy (work, lnk, sizeof (work));
	else
	  {
	     safe_strcpy (work, d, sizeof (work));
	     *(extract_file(work)) = 0;
	     safe_strcat (work, lnk, sizeof (work));
	  }

	/* Collapse multiple // since jed_standardize_filename_static will interpret them
	 * differently.
	 */
	w = w1 = work;
	while ((ch = *w1++) != 0)
	  {
	     *w++ = ch;
	     if (ch == '/') while (*w1 == '/') w1++;
	  }
	*w = 0;
	
	SLfree (d);
	max_num--;
	if (max_num == 0)
	  {
	     jed_verror ("possible circular-link detected");
	     return NULL;
	  }
	d = jed_standardize_filename(work);
     }
   
   return (d);
}
/*}}}*/
#endif				       /* REAL_UNIX_SYSTEM */

#ifdef REAL_UNIX_SYSTEM
static char *expand_link_1(char *f) /*{{{*/
{
   char *d = SLmake_string (f);
   
   if (d == NULL)
     return d;

   f = d;

   while (1)
     {
	char *new_d, *d1;
	
	if (*f == '/')
	  f++;

	while (*f && (*f != '/'))
	  f++;
	
	if (*f == 0)
	  break;
	
	*f++ = 0;

	new_d = expand_link_2 (d);
	if (new_d == NULL)
	  {
	     SLfree (d);
	     return NULL;
	  }

	d1 = SLpath_dircat (new_d, f);
	f = d1 + strlen (new_d);
	SLfree (d);
	SLfree (new_d);
	if (d1 == NULL)
	  return NULL;
	d = d1;
     }
   
   /* Now get filename component */
   f = expand_link_2 (d);
   SLfree (d);
   return f;
}
#endif				       /* REAL_UNIX_SYSTEM */

/* Malloced string returned */
char *jed_expand_link (char *f)
{
#ifndef REAL_UNIX_SYSTEM
   return jed_standardize_filename (f);
#else
   char *f1, *f2;
   unsigned int len;
   
   /* Look for, e.g., /usr/local//tmp/foo and extract /tmp/foo.
    * Do this before attempting to expand a link since links could 
    * contain such constructs where they have a different meaning.
    * 
    * Do this only if // occurs because ../ constructs should be preserved
    * if a subdir is a link to another.  That is consider:
    * 
    *    /path/a/b/../c.txt
    * 
    * jed_standardize_filename will convert this to
    *  
    *    /path/a/c.txt
    * 
    * However, if b is a symlink to /tmp/a/b then we really want the file
    * /tmp/a/c.txt.
    */
#if 1   
   len = strlen (f);
   if (NULL == (f = SLmake_nstring (f, len)))
     return NULL;
   
   f2 = f + len;
   while (f2 > f)
     {
	if ((*f2-- == '/') && (*f2 == '/'))
	  {
	     f2++;
	     break;
	  }
     }
#else
   if (NULL == (f = jed_standardize_filename (f)))
     return NULL;
   f2 = f;
#endif

   if (NULL == (f1 = expand_link_1 (f2)))
     {
	SLfree (f);
	return NULL;
     }

   if (0 == strcmp (f, f1))
     {
	SLfree (f);
	return f1;
     }
   SLfree (f);

   f = f1;
   /* Keep expanding the link until nothing changes */
   while (1)
     {
	if (NULL == (f1 = expand_link_1 (f)))
	  {
	     SLfree (f);
	     return NULL;
	  }

	if (0 == strcmp (f, f1))
	  {
	     SLfree (f1);
	     return f;
	  }
	SLfree (f);
	f = f1;
     }
#endif
}


/*}}}*/

void visit_file (char *dir, char *file) /*{{{*/
{
#ifdef VMS
   char *ff;

   file = SLmake_string (file);
   if (file == NULL)
     return;

   ff = file; 
   while (*ff) 
     {
	if (*ff == ';')
	  {
	     *ff = 0; 
	     break;
	  }
	ff++;
     }
#endif

   if (
#if JED_FILE_PRESERVE_CASE
       strcmp(file, CBuf->file) || strcmp(dir, CBuf->dir)
#else
       jed_case_strcmp (file, CBuf->file) || jed_case_strcmp (dir, CBuf->dir)
#endif
       )
     {
	buffer_filename (CBuf, dir, file);
     }
   /* We have already taken care of this in write buffer function.
    */
   /* CBuf->c_time = sys_time(); */

   check_buffers();
}

/*}}}*/

char *jed_dir_file_merge(SLFUTURE_CONST char *dir, SLFUTURE_CONST char *file) /*{{{*/
/* 
 * returns result of merging dir with file. If file is empty, dir is
 * considered to be whole file.
 */
{
   char *dirfile;

   if ((file == NULL) || (*file == 0))
     return jed_standardize_filename (dir);

   
   dirfile = SLmalloc (strlen (dir) + strlen (file) + 2);
   if (dirfile == NULL)
     return NULL;
   strcpy (dirfile, dir);
   fixup_dir (dirfile);
   strcat (dirfile, file);
	
   file = jed_standardize_filename (dirfile);
   SLfree (dirfile);
   return (char *) file;
}

/*}}}*/


int file_status(SLFUTURE_CONST char *file) /*{{{*/
/*
 *  Returns a coded integer about file.  If the file does not exist, 0 is
 *  returned.  Meaning:
 *
 *     2 file is a directory
 *     1 file exists
 *     0 file does not exist.
 *    -1 no access.
 *    -2 path invalid
 *    -3 unknown error
 */
{
   mode_t mode = 0;
   uid_t uid;
   gid_t gid;
   return sys_chmod(file, 0, &mode, &uid, &gid);
}

/*}}}*/

/* Returns 0 if unchanged, 1 if changed, or -1 if it does not exist
 * and buffer is unmodified.
 * 
 * What does it mean for the file to be changed on the disk if the buffer
 * corresponds to a symlink?  That is, imagine editing foo.c which is a
 * symlink to bar.c.  If bar.c changes, then foo.c shold be flagged as
 * changed.  But, what if the link foo.c is changed such that it nolonger 
 * points to bar.c?  Now the buffer foo.c is nolonger connected with 
 * bar.c.
 * 
 * To handle both situations, do not use the dirfile field of the buffer
 * structure, since that corresponds to bar.c and not the link foo.c.
 * Since the file_changed_on_disk function uses stat and not lstat, if foo.c
 * is a link to bar.c, then bar.c will be checked.
 * 
 * If the file has changed, be sure to update the buffers dirfile field and
 * update the inode info.  If another buffer has the same inode info, then
 * the user has shot himself in the foot by making foo.c a hardlink.
 */
int file_changed_on_disk (Buffer *buf, char *dirfile) /*{{{*/
{
   unsigned long t;

   /* If we are saving this to a different file, then skip the test --
    * unless dirfile is a link.
    */
   if ((buf->canonical_dirfile != NULL) 
#if JED_FILE_PRESERVE_CASE
       && (0 != strcmp (buf->canonical_dirfile, dirfile))
#else
       && (0 != jed_case_strcmp (buf->canonical_dirfile, dirfile))
#endif
       )
     {
#ifdef REAL_UNIX_SYSTEM
	struct stat s;

	if ((-1 == lstat(dirfile, &s))
	    || (0 == ((s.st_mode & S_IFMT) & S_IFLNK)))
	  return 0;
#else	
	return 0;
#endif
     }

#ifdef REAL_UNIX_SYSTEM
   /*
    * The file may still have been changed via a rename operation.
    * Update (or even invalidate) inode information since directories may have
    * been renamed, etc...
    */
     {
	int device, inode;
	(void) jed_get_inode_info (dirfile, &device, &inode);
	if ((buf->device != -1) && (buf->inode != -1))
	  {
	     if ((device != buf->device) || (inode != buf->inode))
	       buf->flags |= FILE_MODIFIED;
	  }
	buf->device = device;
	buf->inode = inode;
     }
#endif

   t = sys_file_mod_time(dirfile);
   if (t == 0)
     {
	/* File does not exist.  If the buffer has not been modified since
	 * the last save, then the file has changed on the disk.
	 */
	if (buf->c_time == 0)
	  return 0;

	if (buf->flags & BUFFER_MODIFIED)
	  return 0;
	return -1;		       /* does not exist */
     }

   if (t > buf->c_time)
     {
#if 0  /* Commented out because if the file did not exist when the buffer was created
	* then c_time will be 0.  But if t>0, then this would indicate that an
	* external process created/modified the file.
	*/
	
	if (buf->c_time == 0)	       /* new buffer, writing to existing file */
	  return 0;
#endif
	return 1;
     }

   return 0;
}

/*}}}*/
int file_time_cmp(char *file1, char *file2) /*{{{*/
{
   unsigned long t1, t2;
   
   t1 = sys_file_mod_time(file1);
   t2 = sys_file_mod_time(file2);
    /*These times are modification  times from 1970.  Hence, the bigger 
     * number represents the more recent change.  Let '>' denote 
     * 'more recent than'.  This function returns:
     *	   1:  file1 > file2
     *	   0:  file 1 == file2
     *	   -1: otherwise. 
     *	So:
     */
   if (t1 == t2) return(0);
   if (t1 > t2) return(1); 
   return(-1);
}

/*}}}*/

void auto_save(void) /*{{{*/
{
   auto_save_buffer(CBuf);
}

/*}}}*/

void check_buffer(Buffer *b) /*{{{*/
{
   if (*b->file)
     {
	char *dirfile = jed_dir_file_merge (b->dir, b->file);

	if (dirfile == NULL)
	  return;

	if (file_changed_on_disk (b, dirfile))
	  b->flags |= FILE_MODIFIED;

	SLfree (dirfile);
     }
   else
     b->flags &= ~FILE_MODIFIED;
}

/*}}}*/
void check_buffers() /*{{{*/
{
   Buffer *b = CBuf;
   do
     {
	check_buffer(b);
	b = b->next;
     }
   while (b != CBuf);
}

/*}}}*/

void set_file_trans(int *mode) /*{{{*/
{
   if (*mode) VFile_Mode = VFILE_BINARY; else VFile_Mode = VFILE_TEXT;
}

/*}}}*/

int read_file_pointer(int fp) /*{{{*/
{
   int n = 0;
   unsigned int num;
   unsigned char *vbuf;
   VFILE *vp;

   if (SLang_get_error () || (vp = vstream(fp, MAX_LINE_LEN, VFile_Mode)) == NULL) return(-1);
   
   if (NULL != (vbuf = (unsigned char *) vgets(vp, &num)))
     {
	n++;
#ifdef KEEP_SPACE_INFO
	if (CLine->space < num)
#endif
	  remake_line(CLine->len + num + 1);
	
	SLMEMCPY((char *) CLine->data, (char *) vbuf, (int) num);
	CLine->len = num;
   
	while(NULL != (vbuf = (unsigned char *) vgets(vp, &num)))
	  {
	     n++;
	     if ((num == 1) && (*vbuf == '\n')) make_line(num); else make_line(num + 1);
	     SLMEMCPY((char *) CLine->data, (char *) vbuf, (int) num);
	     CLine->len = num;
	     if (SLang_get_error () || SLKeyBoard_Quit) break;
	  }
	if (vp->cr_flag) CBuf->flags |= ADD_CR_ON_WRITE_FLAG;
	else CBuf->flags &= ~ADD_CR_ON_WRITE_FLAG;
     }
   
   if (vp->buf != NULL) SLfree(vp->buf);
   SLfree ((char *)vp);
   return n;
}

/*}}}*/

#ifdef REAL_UNIX_SYSTEM

int jed_get_inode_info (char *dirfile, int *device, int *inode)
{
   struct stat st;
   
   *inode = *device = -1;
   if ((dirfile == NULL)
       || (-1 == stat (dirfile, &st)))
     return -1;
   
   *inode = st.st_ino;
   *device = st.st_dev;

   return 0;
}

#endif

void jed_set_umask (int mask)
{
#ifdef REAL_UNIX_SYSTEM
   static int default_umask;
#endif

   if (mask == 0) 
     {
#ifdef REAL_UNIX_SYSTEM
	if (default_umask != 0)
	  (void) umask (default_umask);
#endif
     }
   else
     {
#ifdef REAL_UNIX_SYSTEM
	int u;
	
	u = umask (mask & 07777);
	if (default_umask == 0) default_umask = u;
#endif
     }
}

int jed_dirfile_to_dir_file (char *dirfile, char **dirp, char **filep)
{
   char *dir, *file;
   unsigned int len;

   if (dirfile == NULL)
     return -1;

   file = extract_file (dirfile);
   len = (unsigned int) (file - dirfile);
   if (NULL == (dir = SLmake_nstring (dirfile, len)))
     return -1;
   
   if (filep != NULL)
     {
	if (NULL == (file = SLmake_string (file)))
	  {
	     SLfree (dir);
	     return -1;
	  }
	*filep = file;
     }

   *dirp = dir;
   return 0;
}


char *jed_get_canonical_pathname (char *file)
{
   return jed_expand_link (file);
}

