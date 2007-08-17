/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

/* This file should only be included if REALLY_HAVE_TERMIOS_H is defined. */

#include <termios.h>
#ifdef sun
# ifndef PENDIN
#  include <sys/ioctl.h>
# endif
#else
# include <sys/ioctl.h>
#endif

#include <errno.h>

#ifdef HAVE_GRANTPT
# if !defined (__linux__) && !defined(__CYGWIN__) && !defined(__FreeBSD__) && !defined(_AIX)
#  define USE_SYSV_PTYS
#  include <sys/types.h>
#  include <stropts.h>
# endif
#endif

#ifdef HAVE_PTY_H
# include <pty.h>
#endif

#ifdef HAVE_SYS_PTY_H
# include <sys/pty.h>
#endif

static int pty_setup_slave_term (int slave, int raw)
{
   struct termios slave_termios;

   while (-1 == tcgetattr (slave, &slave_termios))
     {
#ifdef EINTR
	if (errno == EINTR)
	  continue;
#endif
	return -1;
     }

   slave_termios.c_lflag = 0;

   if (raw == 0)
     slave_termios.c_lflag |= ICANON;

   slave_termios.c_iflag &= ~(ECHO | INLCR | ICRNL);
#ifdef ONLRET
   slave_termios.c_oflag |= ONLRET;
#endif
#ifdef ONLCR
   slave_termios.c_oflag &= ~ONLCR;
#endif
   slave_termios.c_cc[VEOF] = 4;
   slave_termios.c_cc[VMIN] = 1;
   slave_termios.c_cc[VTIME] = 0;

   /* while (-1 == tcsetattr (slave, TCSANOW, &slave_termios)) */
   while (-1 == tcsetattr (slave, TCSADRAIN, &slave_termios))
     {
#ifdef EINTR
	if (errno == EINTR)
	  continue;
#endif
	return -1;
     }

   return 0;
}

#ifndef USE_SYSV_PTYS

# include <grp.h>
# include <sys/types.h>
# include <sys/stat.h>

# ifdef HAVE_OPENPTY

static int pty_open_master_pty (int *master, char *slave_tty_name)
{
   int slave;
   char *s;

   if (-1 == openpty (master, &slave, NULL, NULL, NULL))
     return -1;

   if (NULL == (s = ttyname (slave)))
     {
	signal_safe_close (*master);
	signal_safe_close (slave);
	return -1;
     }

   safe_strcpy (slave_tty_name, s, MAX_TTY_SLAVE_NAME);

   (void) pty_setup_slave_term (slave, 1);
   signal_safe_close (slave);
   (void) pty_setup_slave_term (*master, 1);
   return 0;
}
# else				       /* NOT HAVE_OPENPTY */

static int pty_open_master_pty (int *master, char *slave_tty_name)
{
   char *a, *b;

   strcpy (slave_tty_name, "/dev/ptyab");

   a = "pqrstuvwxyz";
   while (*a != 0)
     {
	slave_tty_name [8] = *a++;

	b = "0123456789abcdef";
	while (*b != 0)
	  {
	     int slave;

	     slave_tty_name [9] = *b++;
	     if (-1 == (*master = signal_safe_open (slave_tty_name, O_RDWR)))
	       continue;

	     /* Make sure the slave can be opened.   I attempt to set up
	      * the master terminal also even though it is not a tty and will
	      * probably fail.
	      */
	     slave_tty_name [5] = 't';
	     if (-1 != (slave = open (slave_tty_name, O_RDWR)))
	       {
		  (void) pty_setup_slave_term (slave, 1);
		  signal_safe_close (slave);
		  (void) pty_setup_slave_term (*master, 1);
		  return 0;
	       }

	     signal_safe_close (*master);
	     slave_tty_name [5] = 'p';
	  }
     }
   return -1;
}

# endif				       /* HAVE_OPENPTY */

static int pty_open_slave_pty (char *slave_name, int *slave)
{
   int fd;
   struct group *g;

   *slave = -1;

   if (NULL != (g = getgrnam ("tty")))
     {
	int gid = g->gr_gid;
	(void) chown (slave_name, getuid (), gid);
# ifndef S_IRUSR
#  define S_IRUSR 0400
# endif
# ifndef S_IWUSR
#  define S_IWUSR 0200
# endif
# ifndef S_IWGRP
#  define S_IWGRP 0020
# endif
	(void) chmod (slave_name, S_IRUSR | S_IWUSR | S_IWGRP);
     }

   if (-1 == (fd = signal_safe_open (slave_name, O_RDWR)))
     return -1;

# if defined(TIOCSCTTY) && !defined(CIBAUD)
   /* Stevens says use CIBAUD to avoid doing this under SunOS.
    * This gives us a controlling terminal.
    */
   while ((-1 == ioctl (fd, TIOCSCTTY, NULL))
	  && (errno == EINTR));
# endif

   *slave = fd;

   return 0;
}
#else
static int pty_open_master_pty (int *master, char *slave_tty_name)
{
   int fd;
   char *slave_name;

   *master = -1;
   *slave_tty_name = 0;

   if (-1 == (fd = signal_safe_open ("/dev/ptmx", O_RDWR)))
     return -1;

   /* According to the solaris man page, this could fail if jed catches
    * SIGCHLD.  So lets block it.
    */
   jed_block_child_signal (1);
   if (-1 == grantpt (fd))
     {
	jed_block_child_signal (0);
	signal_safe_close (fd);
	return -1;
     }
   if (-1 == unlockpt (fd))
     {
	jed_block_child_signal (0);
	(void) signal_safe_close (fd);
	return -1;
     }

   jed_block_child_signal (0);

   if (NULL == (slave_name = ptsname (fd)))
     {
	(void) signal_safe_close (fd);
	return -1;
     }

   safe_strcpy (slave_tty_name, slave_name, MAX_TTY_SLAVE_NAME);
   *master = fd;

   return 0;
}

static int pty_open_slave_pty (char *slave_name, int *slave)
{
   int fd;

   *slave = -1;

   /* The open here will make this the controlling terminal */
   if (-1 == (fd = signal_safe_open (slave_name, O_RDWR)))
     return -1;

   if ((-1 == ioctl (fd, I_PUSH, "ptem"))
       || (-1 == ioctl (fd, I_PUSH, "ldterm")))
     {
	signal_safe_close (fd);
	return -1;
     }

   /* Some systems are reported not to have ttcompat.  Since it is a BSD
    * compatibility interface, let it silently fail if errno is EINVAL.
    */
   if (-1 == ioctl (fd, I_PUSH, "ttcompat"))
     {
# ifdef EINVAL
	if (errno != EINVAL)
	  {
	     signal_safe_close (fd);
	     return -1;
	  }
# endif
     }
   *slave = fd;
   return 0;
}

#endif

