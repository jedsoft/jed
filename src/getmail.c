/*-------------------------------*- C -*--------------------------------*/
/* Copyright (c) 1992-2010 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

/* File:	getmail.c
 *
 * Usage:	getmail <infile> <outfile>
 *
 * typical usage:
 *	getmail	/usr/spool/mail/$USER	$HOME/Mail/#NewMail#
 *
 * -----------------------------------------------------------------------
 * Moving mail from a maildir.  Written in C to avoid a race condition.
 *
 * Two methods of file locking.
 *
 * 1) Write a lock file into the mail spool directory
 *	file = "/usr/mail/spool/username"
 *	lock = "/usr/mail/spool/username.lock"
 *
 *    * Requires sgid (possible security hole) or a world-writeable
 *       spool directory (minor security hole - but not serious)
 *
 * 2) use fcntl () call.
 *
 *    * Should always work provided the fcntl call supports file locking.
 *      This code was inspired by the Pine mailer implementation of flock.
 *
 * Depending on your system, you may need to define or comment out
 * the following line:
\*----------------------------------------------------------------------*/

/* #define USE_LOCK_FILE */		/* mail spool is writeable */

/*----------------------------------------------------------------------*/

#include "config.h"
#include "jed-feat.h"

#include <stdio.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/file.h>
#include <time.h>
#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#endif

#ifndef F_WRLCK
# define USE_LOCK_FILE
#endif

#ifndef JED_MAX_PATH_LEN
# define JED_MAX_PATH_LEN 1024
#endif

static char *Program_Name;
static void usage (void)
{
   fprintf (stderr, "%s: usage: %s infile outfile\n",
	    Program_Name, Program_Name);
   exit (-1);
}

#ifdef USE_LOCK_FILE
static char Lock_File [JED_MAX_PATH_LEN];
#endif

static void unlock_mail_file (void)
{
#ifdef USE_LOCK_FILE
   unlink (Lock_File);
#endif
}

static void exit_error (char *msg, int l)
{
   fprintf (stderr, "%s: %s\n", Program_Name, msg);
   if (l) unlock_mail_file ();
   exit (-1);
}

static int my_close (int fd)
{
   while (-1 == close(fd))
     {
#ifdef EINTR
	if (errno == EINTR)
	  {
	     errno = 0;
	     sleep (1);
	     continue;
	  }
#endif
	return -1;
     }
   return 0;
}

#ifdef USE_LOCK_FILE
static int lock_mail_file (char *file, int attempts)
{
   struct stat s;
   char buf[80];
   int fd;

   sprintf (Lock_File, "%s.lock", file);
   while (attempts-- > 0)
     {
	if (stat (Lock_File, &s) < 0)
	  {
	     if (errno != ENOENT) exit_error ("stat failed.", 0);
	     fd = open (Lock_File, O_EXCL | O_CREAT, 0666);
	     if (fd >= 0)
	       {
		  sprintf (buf, "%d", getpid());
		  write (fd, buf, strlen(buf));
		  if (0 == my_close (fd))
		    return (0);
		  exit_error ("Unable to lock file.  File system full.", 1);
	       }
	     perror (NULL);
	     fprintf (stderr, "Attempt %d: can\'t lock <%s>",
		      attempts, Lock_File);
	  }
	else if (time((time_t *) NULL) - s.st_ctime > 60)
	  {
	     unlock_mail_file ();
	  }
	sleep (1);
     }
   return (-1);
}
#endif

#ifndef USE_LOCK_FILE
/* This chunk was inspired by the way the pine mailer implemented flock. */
static void our_flock (int fd)
{
   struct flock f;

   f.l_type = F_WRLCK;

   f.l_pid = 0;			       /* not used for setting locks*/

   /* set up rest of structure to lock whole file */

   f.l_whence = 0;		       /* from beginning of file */
   f.l_start = f.l_len = 0;	       /* entire file */

   /* we are going to block */
   if (fcntl (fd, F_SETLKW, &f) == -1)
     {
	exit_error ("Unable to lock file.", 0);
     }
}
#endif

static void mv_mail (char *from, char *to)
{
   char buf[8192];
   int fdfrom, fdto;
   int n;
   int flags;

#ifdef USE_LOCK_FILE
   flags = O_RDONLY;
#else
   flags = O_RDWR;
#endif

   if ((fdfrom = open (from, flags, 0666)) < 0)
     exit_error ("Unable to open input file.", 1);

#ifndef USE_LOCK_FILE
   our_flock (fdfrom);
#endif

   if ((fdto = open (to, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
     exit_error ("Unable to create output file.", 1);

   while ((n = read (fdfrom, buf, sizeof(buf))) > 0)
     {
	if (n != write (fdto, buf, n))
	  {
	     exit_error ("write failed.", 1);
	  }
     }

   if (-1 == my_close (fdto))
     exit_error ("File system full.  write failed.", 1);

   my_close (fdfrom);
#ifndef TEST_DONT_UNLINK
   if ( unlink (from) )
     {
	if ((fdfrom = open (from, O_WRONLY | O_CREAT | O_TRUNC, 0600)) >= 0)
	  close (fdfrom);
     }
#endif
   unlock_mail_file ();
}

int main (int argc, char **argv)
{
   char *in = argv[1];
   char *out = argv[2];
   Program_Name = argv [0];
   if (argc != 3) usage ();

#ifdef USE_LOCK_FILE
   if (lock_mail_file (in, 60))
     exit_error ("Unable to lock file.", 0);
#endif
   mv_mail (in, out);
   return (0);
}
/* /////////////////////// end of file (c source) ///////////////////// */
