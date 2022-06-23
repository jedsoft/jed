/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1999-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

/*{{{ Include Files */
#include <stdio.h>
#include <string.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <slang.h>

#include "jdmacros.h"

#include "buffer.h"
#include "file.h"
#include "userinfo.h"
#include "ledit.h"
#include "misc.h"

/*}}}*/

#if JED_HAS_EMACS_LOCKING && !defined(HAVE_SYMLINK)
# undef JED_HAS_EMACS_LOCKING
# define JED_HAS_EMACS_LOCKING 0
#endif

#if !JED_HAS_EMACS_LOCKING /*{{{*/
int jed_lock_buffer_file (Buffer *b)
{
   (void) b;
   return 0;
}

int jed_unlock_buffer_file (Buffer *b)
{
   (void) b;
   return 0;
}
int jed_lock_file (char *file)
{
   (void) file;
   return 0;
}
int jed_unlock_file (char *file)
{
   (void) file;
   return 0;
}

/*}}}*/
#else  /* Rest of file */

/* The basic idea here is quite simple.  Whenever a buffer is attached to
 * a file, and that buffer is modified, then attempt to lock the
 * file. Moreover, before writing to a file for any reason, lock the
 * file. The lock is really a protocol respected and not a real lock.
 * The protocol is this: If in the directory of the file is a
 * symbolic link with name ".#FILE", the FILE is considered to be locked
 * by the process specified by the link.
 *
 * Here are the scenerios requiring a lock:
 *
 *    1.  When a buffer attached to a file becomes modified.
 *    2.  When appending or writing to a file.
 *
 * Suppose that a buffer has not been modified but one appends a
 * region of the buffer to some file.  Then while that file is being
 * written, it should be locked and then released when finished.
 * However, suppose another buffer has that file locked. Then in
 * order to write to it, either the lock must be stolen or ignored.
 * In either of these cases, the user is responsible to what happens
 * to the text in the other buffer.  In fact, if the user elects to
 * steal the lock from the other buffer, then that lock will be
 * deleted after the file has been modfied.  Of course these same comments
 * apply if another process owns the lock.
 *
 * Now consider the case when a buffer is modified and has the file locked.
 * When the buffer is saved, the file will already be locked and there is no
 * need to lock it again.
 */

typedef struct
{
   char *host;
   char *user;
   int pid;
}
Lock_Info_Type;

static char *make_lockfile_name (char *file)
{
   unsigned int len;
   char *buf, *dir, *name;

   file = jed_expand_link (file);
   if (file == NULL)
     return NULL;

   if (-1 == jed_dirfile_to_dir_file (file, &dir, &name))
     {
	SLfree (file);
	return NULL;
     }
   SLfree (file);

   len = strlen (dir) + strlen (name) + 3;
   if (NULL != (buf = SLmalloc (len)))
     sprintf (buf, "%s.#%s", dir, name);

   SLfree (dir);
   SLfree (name);
   return buf;
}

static void free_lock_info (Lock_Info_Type *l)
{
   SLang_free_slstring (l->host);
   SLang_free_slstring (l->user);
}

static int create_lock_info (Lock_Info_Type *l)
{
   char *host, *user;

   memset ((char *) l, 0, sizeof (Lock_Info_Type));

   if (NULL == (host = jed_get_hostname ()))
     return -1;

   if (NULL == (user = jed_get_username ()))
     {
	SLang_free_slstring (host);
	return -1;
     }
   l->host = host;
   l->user = user;
   l->pid = getpid ();
   return 0;
}

/*
 * If 0 is returned, then proceed with no lock.  If 1, then force
 * the lock, if -1 then abort with no lock.
 */
static int ask_about_lock (char *file, Lock_Info_Type *l)
{
   char *buf;
   unsigned int len;

   if (Batch)
     {
	jed_verror ("%s is locked by %s@%s.%d.", file, l->user, l->host, l->pid);
	return -1;
     }

   len = 64 + strlen (file)
     + strlen (l->host) + strlen (l->user);

   if (NULL == (buf = SLmalloc (len)))
     return -1;

   sprintf (buf, "%s is locked by %s@%s.%d.  (S)teal, (P)roceed, (A)bort?",
	    file, l->user, l->host, l->pid);

   while (1)
     {
	switch (jed_get_mini_response (buf, 1))   /* suspend macro */
	  {
	   case 's':
	   case 'S':
	     SLfree (buf);
	     return 1;

	   case 'P':
	   case 'p':
	   case 'q':
	   case 'Q':
	     SLfree (buf);
	     return 0;

	   case 'A':
	   case 'a':
	     SLfree (buf);
	     jed_verror ("%s is locked by another process", file);
	     return -1;
	  }
	if (SLang_get_error ())
	  {
	     SLfree (buf);
	     return -1;
	  }

	jed_beep ();
     }
}

/* Returns 0 if file does not exist, or 1 with info, or -1 for error */
static int get_lock_info (char *lockfile, Lock_Info_Type *l)
{
   struct stat st;
   char buf[1024];
   char *b;
   char *user, *host;
   int n;

   memset ((char *) l, 0, sizeof (Lock_Info_Type));

   if (-1 == lstat (lockfile, &st))
     {
	if (errno == ENOENT)
	  return 0;
	return -1;
     }

   if (((st.st_mode & S_IFMT) & S_IFLNK) == 0)
     return -1;

   n = readlink (lockfile, buf, sizeof (buf)-1);
   if ((n == -1)
       || (n >= (int)sizeof(buf)-1))
     return -1;

   buf[n] = 0;

   /* The format is: username@host.pid:boot_time */
   b = strchr (buf, '@');
   if (b == NULL)
     return -1;

   *b++ = 0;

   user = SLang_create_slstring (buf);
   if (user == NULL)
     return -1;

   host = b;
   b += strlen (b);
   while (b > host)
     {
	b--;
	if (*b == '.')
	  break;
     }
   if (b == host)
     {
	SLang_free_slstring (user);
	return -1;
     }
   *b++ = 0;

   if (NULL == (host = SLang_create_slstring (host)))
     {
	SLang_free_slstring (user);
	return -1;
     }

   l->pid = atoi (b);
   l->host = host;
   l->user = user;

   return 1;
}

/* Returns 1 upon success, 0 if lock exists, or -1 upon failure.  If force
 * is non-zero, then this function will not return 0.
 */
static int perform_lock (char *lockfile, Lock_Info_Type *l, int force)
{
   unsigned int len;
   char *buf;
   int status;
   int not_supported = 0;

   len = 32 + strlen (l->host) + strlen (l->user);
   if (NULL == (buf = SLmalloc (len)))
     return -1;

   sprintf (buf, "%s@%s.%d", l->user, l->host, l->pid);
   if (0 == symlink (buf, lockfile))
     {
	SLfree (buf);
	return 1;
     }

   if (not_supported
# ifdef EPERM
       || (errno == EPERM)
# endif
# ifdef EOPNOTSUPP
       || (errno == EOPNOTSUPP)
# endif
# ifdef ENOSYS
       || (errno == ENOSYS)
# endif
       )
     {
	jed_vmessage (0, "File system does not support symbolic links: file not locked.");
	SLfree (buf);
	return 1;
     }

# ifdef EACCES
   if (errno == EACCES)
     {
	jed_vmessage (0, "No permission to lock file");
	SLfree (buf);
	return 1;
     }
#endif
   if (force == 0)
     {
	SLfree (buf);
	if (errno == EEXIST)
	  return 0;

	return -1;
     }

   (void) unlink (lockfile);
   status = symlink (buf, lockfile);
   if (status == -1)
     jed_vmessage (0, "Unable to create lockfile %s", lockfile);
   else
     status = 1;

   SLfree (buf);
   return status;
}

/* Return 0 if same, -1 if same host, 1 if different host */
static int compare_lock_info (Lock_Info_Type *a, Lock_Info_Type *b)
{
   if (a->host != b->host)
     return 1;

   if ((a->pid != b->pid)
       || (a->user != b->user))
     return -1;

   return 0;
}

int jed_lock_file (char *file)
{
   char *lockfile;
   Lock_Info_Type l0;
   int force;
   int status;

   lockfile = make_lockfile_name (file);
   if (lockfile == NULL)
     return -1;

   if (-1 == create_lock_info (&l0))
     {
	SLfree (lockfile);
	return -1;
     }

   force = 0;
   while (1)
     {
	Lock_Info_Type l1;

	status = perform_lock (lockfile, &l0, force);
	if (status == 1)	       /* lock succeeded */
	  break;

	if (status == -1)	       /* error */
	  break;

	/* Lock already exists.  Let's see who owns it */
	status = get_lock_info (lockfile, &l1);
	if (status == -1)
	  break;

	if (status == 0)	       /* lock nolonger exists, try again */
	  continue;

	status = compare_lock_info (&l0, &l1);
	if (status == 0)
	  {
	     /* We already own this lock */
	     free_lock_info (&l1);
	     status = 1;
	     break;
	  }

	if (status == -1)
	  {
	     /* Some process on this machine owns it.  See if the process is
	      * alive.
	      */
	     if (l1.pid <= 0)
	       status = 2;
	     else if (-1 == kill (l1.pid, 0))
	       {
# ifdef ESRCH
		  if (errno == ESRCH) /* Doesn't exist */
		    status = 2;
# endif
	       }
	  }

	if (status != 2)
	  status = ask_about_lock (file, &l1);

	free_lock_info (&l1);

	if (status <= 0)
	  break;

	force = 1;
     }

   SLfree (lockfile);
   free_lock_info (&l0);

   if (status == -1)
     return -1;

   return 0;
}

int jed_unlock_file (char *file)
{
   Lock_Info_Type l0, l1;
   char *lockfile;
   int status;

   lockfile = make_lockfile_name (file);
   if (lockfile == NULL)
     return -1;

   if (-1 == create_lock_info (&l0))
     {
	SLfree (lockfile);
	return -1;
     }

   status = get_lock_info (lockfile, &l1);
   if (status <= 0)
     {
	free_lock_info (&l0);
	SLfree (lockfile);
	return status;
     }

   if (0 == compare_lock_info (&l0, &l1))
     (void) unlink (lockfile);

   free_lock_info (&l0);
   free_lock_info (&l1);
   SLfree (lockfile);

   return 0;
}

int jed_lock_buffer_file (Buffer *b)
{
   int status;
   char *file;

   if ((b->file[0] == 0) || (b->flags & BUFFER_NON_LOCKING))
     return 0;

   file = jed_dir_file_merge (b->dir, b->file);
   status = jed_lock_file (file);
   SLfree (file);

   return status;
}

int jed_unlock_buffer_file (Buffer *b)
{
   int status;
   char *file;

   if ((b->file[0] == 0) || (b->flags & BUFFER_NON_LOCKING))
     return 0;

   file = jed_dir_file_merge (b->dir, b->file);
   status = jed_unlock_file (file);
   SLfree (file);

   return status;
}

int jed_unlock_buffer_files (void)
{
   int status;
   Buffer *b;

   if (NULL == (b = CBuf)) return 0;

   status = 0;
   do
     {
	b = b->next;

	if (-1 == jed_unlock_buffer_file (b))
	  status = -1;
     }
   while (b != CBuf);

   return status;
}

#endif				       /* JED_HAS_EMACS_LOCKING */
