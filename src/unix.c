/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#include "config.h"
#include "jed-feat.h"
/*{{{ Include Files */

#include <stdio.h>
#include "sysdep.h"

#ifdef HAVE_GLOB_H
# include <glob.h>
#endif

#if JED_HAS_SUBPROCESSES
# include "jprocess.h"
#endif

#include <signal.h>
/* sequent support thanks to Kenneth Lorber <keni@oasys.dt.navy.mil> */
/* SYSV (SYSV ISC R3.2 v3.0) provided by iain.lea@erlm.siemens.de */

#include <sys/time.h>

#ifdef REALLY_HAVE_TERMIOS_H
# include <termios.h>
#endif

#ifdef sun
# ifndef PENDIN
#  include <sys/ioctl.h>
# endif
#else
# include <sys/ioctl.h>
#endif

#ifdef __QNX__
/* # include <termios.h> */
# include <sys/select.h>
#endif

#ifdef SYSV
# ifndef CRAY
#  include <sys/termio.h>
#  include <sys/stream.h>
#  include <sys/ptem.h>
#  include <sys/tty.h>
# endif
#endif

#include <sys/types.h>
#include <sys/time.h>

/* Another hack for AInt uniX (AIX) */
#if defined (_AIX) && !defined (FD_SET)
# include <sys/select.h>	/* for FD_ISSET, FD_SET, FD_ZERO */
#endif

#include <sys/stat.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#include <sys/file.h>
#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#endif

#ifndef F_GETFL
# include <fcntl.h>
#endif

#include "sig.h"

/*}}}*/

#define TTY_DESC 2
static int Read_FD = TTY_DESC;
static int Max_Fd = TTY_DESC;

static int Flow_Control;

#ifndef REALLY_HAVE_TERMIOS_H
typedef struct /*{{{*/
{
   struct tchars t;
   struct ltchars lt;
   struct sgttyb s;
}
/*}}}*/
TTY_Termio_Type;
#else
typedef struct termios TTY_Termio_Type;
#endif

static TTY_Termio_Type Old_TTY;

#ifdef REALLY_HAVE_TERMIOS_H
# define GET_TERMIOS(fd, x) tcgetattr(fd, x)
# if 0
#  define SET_TERMIOS(fd, x) tcsetattr(fd, TCSAFLUSH, x)
# else
#  define SET_TERMIOS(fd, x) tcsetattr(fd, TCSADRAIN, x)
# endif
#else
# ifdef TCGETS
#  define GET_TERMIOS(fd, x) ioctl(fd, TCGETS, x)
#  define SET_TERMIOS(fd, x) ioctl(fd, TCSETS, x)
# else
#  define X(x,m)  &(((TTY_Termio_Type *)(x))->m)
#  define GET_TERMIOS(fd, x)	\
    ((ioctl(fd, TIOCGETC, X(x,t)) || \
      ioctl(fd, TIOCGLTC, X(x,lt)) || \
      ioctl(fd, TIOCGETP, X(x,s))) ? -1 : 0)
#  define SET_TERMIOS(fd, x)	\
    ((ioctl(fd, TIOCSETC, X(x,t)) ||\
      ioctl(fd, TIOCSLTC, X(x,lt)) || \
      ioctl(fd, TIOCSETP, X(x,s))) ? -1 : 0)
# endif
#endif

#define NEED_BAUDRATES 0
#if NEED_BAUDRATES
static int Baud_Rates[20] = /*{{{*/
{
   0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
     9600, 19200, 38400, 0, 0, 0, 0
};

/*}}}*/
#endif

#ifdef HAS_MOUSE
static int JMouse_Fd = -1;
#endif

static int TTY_Inited = 0;

#ifdef ultrix   /* Ultrix gets _POSIX_VDISABLE wrong! */
# define NULL_VALUE -1
#else
# ifdef _POSIX_VDISABLE
#  define NULL_VALUE _POSIX_VDISABLE
# else
#  define NULL_VALUE 255
# endif
#endif

static int Updating_Screen;

static void unix_update_open (void)
{
   Updating_Screen = 1;
}

static void unix_update_close (void)
{
   Updating_Screen = 0;
}

static void handle_interrupt (void)
{
   (void) jed_handle_interrupt ();

   if (Jed_Sig_Exit_Fun != NULL)
     (*Jed_Sig_Exit_Fun) ();
     
   if (Updating_Screen)
     return;

   while (Jed_Resize_Pending)
     jed_resize_display ();
}

static int unix_set_abort_char (unsigned char ch)
{
   if (Jed_Abort_Char == ch)
     return 0;

   reset_tty ();
   Jed_Abort_Char = ch;
   init_tty ();
   return 0;
}

#define HAS_GETSETPGID (defined(HAVE_GETPGID) && defined(HAVE_SETPGID))
#define HAS_TCGETSETPGRP (defined(HAVE_TCSETPGRP) && defined(HAVE_TCGETPGRP))

#if HAS_TCGETSETPGRP
static pid_t Terminal_PGID = -1;
#endif
#if HAS_GETSETPGID
static pid_t Startup_PGID = -1;
#endif

static void set_process_group (void)
{
#if HAS_GETSETPGID && HAS_TCGETSETPGRP
   if (-1 != (Terminal_PGID = tcgetpgrp (Read_FD)))
     {
	pid_t pid = getpid ();
	Startup_PGID = getpgid (pid);
	if (-1 == tcsetpgrp (Read_FD, pid))
	  {
	     fprintf (stderr, "tcsetpgrp failed\n");
	     Terminal_PGID = -1;
	     return;
	  }
	if (-1 == setpgid (pid, pid))
	  {
	     fprintf (stderr, "setpgid failed\n");
	     (void) tcsetpgrp (Read_FD, Startup_PGID);
	     Terminal_PGID = -1;
	     return;
	  }
     }
#endif
}

static void reset_process_group (void)
{
#if HAS_GETSETPGID && HAS_TCGETSETPGRP
   if (Terminal_PGID != -1)
     {
	(void) tcsetpgrp (Read_FD, Terminal_PGID);
	setpgid (0, Startup_PGID);
     }
#endif
}

int init_tty (void) /*{{{*/
{
   TTY_Termio_Type newtty;
   static char bskey[2];

   bskey [1] = 0;

   if (Batch) return 0;
   if (X_Init_Term_Hook != NULL)
     {
	Read_FD = (*X_Init_Term_Hook) ();
	Max_Fd = Read_FD;
	if (Read_FD == -1) return -1;
	TTY_Inited = 1;
	return 0;
     }

   if (0 == isatty (Read_FD))
     return -1;

   if ((0 == isatty (fileno (stdin)))
       && (0 == isatty (fileno (stdout))))
     return -1;

   tt_enable_cursor_keys();
   while (-1 == GET_TERMIOS(Read_FD, &Old_TTY))
     {
	if (errno != EINTR)
	  return -1;
     }

   while (-1 == GET_TERMIOS(Read_FD, &newtty))
     {
	if (errno != EINTR)
	  return -1;
     }

   TTY_Inited = 1;

#ifndef REALLY_HAVE_TERMIOS_H
   newtty.s.sg_flags &= ~(ECHO);
   newtty.s.sg_flags &= ~(CRMOD);
/*   if (Flow_Control == 0) newtty.s.sg_flags &= ~IXON; */
   newtty.t.t_eofc = 1;
   newtty.t.t_intrc = Jed_Abort_Char;	/* ^G */
   newtty.t.t_quitc = 255;
   newtty.lt.t_suspc = 255;   /* to ignore ^Z */
   newtty.lt.t_dsuspc = 255;    /* to ignore ^Y */
   newtty.lt.t_lnextc = 255;
   newtty.s.sg_flags |= CBREAK;		/* do I want cbreak or raw????? */
   bskey[0] = (char) newtty.s.sg_erase;
   _Jed_Backspace_Key = bskey;
#else

# if NEED_BAUDRATES
   /* No need for this anymore since SLsmg is used */

   if (*tt_Baud_Rate == 0)
     {
/* Note:  if this generates an compiler error, simply remove
   the statement */
#  ifdef HAVE_CFGETOSPEED
	*tt_Baud_Rate = cfgetospeed (&newtty);
#  endif

	*tt_Baud_Rate = (*tt_Baud_Rate > 0) && (*tt_Baud_Rate < 19) ?
	  Baud_Rates[*tt_Baud_Rate] : -1;
     }
# endif

   newtty.c_iflag &= ~(ECHO | INLCR | ICRNL);
# ifdef ISTRIP
   /* allow 8 bit chars to pass through */
   /* newtty.c_iflag &= ~ISTRIP; */
# endif
   newtty.c_oflag &= ~OPOST;
   /* (ONLCR | OPOST);	*/       /* do not map newline to cr/newline on out */
   if (Flow_Control == 0) newtty.c_iflag &= ~IXON;
   newtty.c_cc[VMIN] = 1;
   newtty.c_cc[VTIME] = 0;
   newtty.c_cc[VEOF] = 1;
   newtty.c_lflag = ISIG | NOFLSH;

   newtty.c_cc[VINTR] = Jed_Abort_Char;   /* ^G */
   newtty.c_cc[VQUIT] = NULL_VALUE;
   newtty.c_cc[VSUSP] = NULL_VALUE;   /* to ignore ^Z */

   bskey [0] = (char) newtty.c_cc[VERASE];
   _Jed_Backspace_Key = bskey;

# ifdef VDSUSP
   newtty.c_cc[VDSUSP] = NULL_VALUE;   /* to ignore ^Y */
# endif
# ifdef VSWTCH
   newtty.c_cc[VSWTCH] = NULL_VALUE;   /* to ignore who knows what */
# endif
#endif /* NOT REALLY_HAVE_TERMIOS_H */
   
   while (-1 == SET_TERMIOS(Read_FD, &newtty))
     {
	if (errno != EINTR)
	  return -1;
     }

#ifdef HAS_MOUSE
   if ((X_Open_Mouse_Hook != NULL)
       && ((JMouse_Fd = (*X_Open_Mouse_Hook)()) >= 0))
     {
	if (JMouse_Fd > Read_FD) Max_Fd = JMouse_Fd;
     }
#endif

   X_Update_Open_Hook = unix_update_open;
   X_Update_Close_Hook = unix_update_close;
   X_Set_Abort_Char_Hook = unix_set_abort_char;
   
   set_process_group ();

   return 0;
}

/*}}}*/

void reset_tty (void) /*{{{*/
{
   if (Batch) return;
   if (!TTY_Inited) return;
   if (X_Init_Term_Hook != NULL)
     {
	if (X_Reset_Term_Hook != NULL) (*X_Reset_Term_Hook) ();
	return;
     }

   while (-1 == SET_TERMIOS(Read_FD, &Old_TTY))
     {
	if (errno != EINTR)
	  break;
     }
   
   reset_process_group ();

   /* This statement ensures init_tty will not try to change output_rate
      (when coming back from suspension) */
#if NEED_BAUDRATES
   if (*tt_Baud_Rate == 0) *tt_Baud_Rate = -1;
#endif

#ifdef HAS_MOUSE
   if (X_Close_Mouse_Hook != NULL) X_Close_Mouse_Hook ();
   JMouse_Fd = -1;
#endif
}

/*}}}*/

int Key_Wait_Time = 450;	       /* units of 1/10 sec */
int jed_set_default_key_wait_time (const int *tsecs)
{
   int tmp = Key_Wait_Time;
   if (*tsecs > 0)
     Key_Wait_Time = *tsecs;
   return tmp;
}

unsigned char sys_getkey (void) /*{{{*/
{
   int n = Key_Wait_Time;
   unsigned char c;
#if JED_HAS_SUBPROCESSES
   int all = Num_Subprocesses;
#else
   int all = 0;
#endif
   int kb;

   while (1)
     {
	handle_interrupt ();

	kb = SLKeyBoard_Quit;
	if (kb)
	  break;

	/* sleep for 45 second and try again */
	if (sys_input_pending(&n, all) > 0)
	  break;
	
	kb = SLKeyBoard_Quit;
	if (kb)
	  break;

	/* update status line in case user is displaying time */
	if (Display_Time || all)
	  {
	     check_buffers ();
	     JWindow->trashed = 1;
	     update((Line *) NULL, 0, 1, 0);
	  }
     }

   kb = SLKeyBoard_Quit;
   if (kb)
     {
	SLKeyBoard_Quit = 0;
	return Jed_Abort_Char;
     }

   if (X_Read_Hook != NULL) return (X_Read_Hook ());

   /* Something may have stuffed it, e.g., mouse */

   if (Input_Buffer_Len) return my_getkey ();

#ifdef HAS_MOUSE
   if (JMouse_Hide_Mouse_Hook != NULL) (*JMouse_Hide_Mouse_Hook) (0);
#endif

   while (1)
     {
	int nread;

	if (SLKeyBoard_Quit)
	  {
	     SLKeyBoard_Quit = 0;
	     return Jed_Abort_Char;
	  }

	if (-1 == (nread = read (Read_FD, (char *) &c, 1)))
	  {
#ifdef EINTR
	     if (errno == EINTR)
	       {
		  handle_interrupt ();
		  continue;
	       }
#endif
	     exit_error ("getkey: read failed.", 0);
	  }

	if (nread > 0)
	  return c;

	if (nread == 0)
	  exit_error ("getkey: EOF seen!", 0);
     }
}

/*}}}*/

#ifndef FD_SET
# define FD_SET(fd, tthis) *(tthis) |= 1 << fd
# define FD_ZERO(tthis)    *(tthis) = 0
# define FD_ISSET(fd, tthis) (*(tthis) & (1 << fd))
typedef int fd_set;
#endif

static fd_set Read_FD_Set;

#ifdef __BEOS__
static int beos_tty_select(int fd, fd_set *mask, struct timeval *wait)
{
   int ret;
   int howmany;
   long long usecs;
   extern void snooze(long long);

   if (wait == NULL)
     usecs = 1000000LL * 60 * 60 * 24 * 365;
   else
     usecs = wait->tv_usec + wait->tv_sec * 1000000LL;

   while (usecs >= 0)
     {
	if (wait == NULL)
	  howmany = 1;
	else
	  howmany = 0;

	ret = ioctl(fd, 'ichr', &howmany);
	if (ret < 0)
	  {
	     FD_CLR(fd, mask);
	     return -1;
	  }

	if ((ret >= 0) && (howmany > 0))
	  return 1;

	snooze(1000LL);
	usecs -= 1000;
     }

   FD_CLR(fd, mask);
   return 0;
}
#endif				       /* __BEOS__ */

/* If all is zero, only check for mouse or keyboard input.  If all is -1,
 * check for only subprocess input. Otherwise, check all input.
 */
int sys_input_pending(int *tsecs, int all) /*{{{*/
{
   struct timeval wait;
   long usecs, secs;
   int ret, maxfd;
#if JED_HAS_SUBPROCESSES
   int i;
#endif
   static int bad_select;

   if ((all >= 0) && (Input_Buffer_Len || Batch)) return(Input_Buffer_Len);

   if (bad_select) return -1;

   top:

   FD_ZERO(&Read_FD_Set);
   secs = *tsecs / 10;
   usecs = (*tsecs % 10) * 100000;
   wait.tv_sec = secs;
   wait.tv_usec = usecs;

   if (all >= 0)
     {
	if ((X_Input_Pending_Hook != NULL)
	    && (*X_Input_Pending_Hook) ())
	  return 1;

	FD_SET(Read_FD, &Read_FD_Set);
#ifdef HAS_MOUSE
	if ((JMouse_Fd >= 0) && (JMouse_Event_Hook != NULL))
	  {
	     FD_SET (JMouse_Fd, &Read_FD_Set);
	  }
#endif
	maxfd = Max_Fd;
     }
   else maxfd = -1;

#if JED_HAS_SUBPROCESSES
   if (all)
     {
	i = 0;
	while (i < Num_Subprocesses)
	  {
	     
	     if (Subprocess_Read_fds[i][2] == 0)   /* If non-0, fd has an EIO error */
	       {
		  int fd = Subprocess_Read_fds[i][0];
		  FD_SET(fd, &Read_FD_Set);
		  if (fd > maxfd) maxfd = fd;
	       }
	     i++;
	  }
     }
#endif

#ifdef __BEOS__
   ret = beos_tty_select (Read_FD, &Read_FD_Set, &wait);
#else
   ret = select(maxfd + 1, &Read_FD_Set, NULL, NULL, &wait);
#endif

   if (ret == 0) return 0;

   if ((ret == -1) && (errno == EINTR))
     handle_interrupt ();

#ifdef ENODEV
   if ((ret == -1) && (errno == ENODEV))
     {
	bad_select = 1;
	return -1;
     }
#endif

#ifdef EBADF
   if ((ret < 0) && (errno == EBADF))
     {
	/* Oh Geez.  We have an invalid file descriptor.  Well, there are
	 * several options.  Here is what I am choosing: If we are selecting
	 * on subprocesses and the bad descriptor appears to be the subprocess
	 * then ignore select on suprocesses.  Hopefully the subprocess child
	 * handlers will fire and illiminate this situation.  If it is the
	 * mouse, then turn off the mouse.  If it is the keyboard, then exit.
	 */
	FD_ZERO (&Read_FD_Set);
	if (all >= 0)
	  {
# ifdef HAS_MOUSE
	     if ((JMouse_Fd >= 0) && (JMouse_Event_Hook != NULL))
	       {
		  if ((-1 == fcntl (JMouse_Fd, F_GETFL, 0)) && (errno == EBADF))
		    {
		       JMouse_Event_Hook = NULL;
		       goto top;
		    }
	       }
# endif
	     if ((-1 == fcntl (Read_FD, F_GETFL, 0)) && (errno == EBADF))
	       {
		  exit_error ("select: keyboard file descriptor is bad!!!", 1);
	       }
	  }
	if (all == -1) return 0;
	all = 0;
	goto top;
     }
#endif				       /* EBADF */

   if (all >= 0)
     {
	if ((X_Input_Pending_Hook != NULL) && (ret > 0)
	    && (FD_ISSET(Read_FD, &Read_FD_Set)))
	  {
	     if ((*X_Input_Pending_Hook) ()) return 1;

	     /* Nothing there so try to time out again --- too bad select does
	      * not inform of of how far we got.
	      */
	     goto top;
#if 0
	     FD_SET(Read_FD, &Read_FD_Set);
	     (void) select(Read_FD + 1, &Read_FD_Set, NULL, NULL, &wait);
	     /* try again, it could be more bogus Xpackets.  Event driven systems
	      * are not always the way to go. */
	     return (*X_Input_Pending_Hook) ();
#endif
	  }

	if (ret < 0) return ret;

	if (FD_ISSET(Read_FD, &Read_FD_Set)) return ret;

	/* Nothing to read from the terminal so while we are waiting
	 * for tty input, read from those that are available.  TTY input
	 * always gets preference.
	 */
#ifdef HAS_MOUSE
	if ((JMouse_Event_Hook != NULL)
	    && (JMouse_Fd >= 0) && (FD_ISSET (JMouse_Fd, &Read_FD_Set)))
	  {
	     if ((*JMouse_Event_Hook) () > 0)
	       {
		  return 1;
	       }
	     /* This is ugly. */
	     goto top;
	  }
#endif
     }
   /* all >= 0 */

   if (ret <= 0) return ret;

#if JED_HAS_SUBPROCESSES
   i = 0;
   while (i < Num_Subprocesses)
     {
	if ((Subprocess_Read_fds[i][2] == 0)   /* If non-0, fd has an EIO error */
	    && FD_ISSET(Subprocess_Read_fds[i][0], &Read_FD_Set))
	  read_process_input (Subprocess_Read_fds[i][1]);
	i++;
     }
#endif
   if (all < 0) return ret;
   return 0;			       /* no keyboard input */
}

/*}}}*/

void sys_pause (int ms) /*{{{*/
{
   struct timeval wait;
   long usecs, secs;

   secs = ms / 1000;
   usecs = (ms % 1000) * 1000;

   wait.tv_sec = secs;
   wait.tv_usec = usecs;

   (void) select (0, NULL, NULL, NULL, &wait);
}

/*}}}*/

/* returns 0 on failure, 1 on sucess */
int sys_delete_file(char *filename) /*{{{*/
{
   return(1 + unlink(filename));
}

/*}}}*/

#ifdef __QNX__
# include <process.h>
#endif
void sys_suspend (void) /*{{{*/
{
#ifdef __QNX__
   char *shell= getenv("SHELL");
   if (shell == NULL) shell = "sh";
   spawnlp(P_WAIT, shell, shell, 0);
#else
   /* (void) SLsignal_intr (SIGTSTP, SIG_DFL); */
   if (Signal_Sys_Spawn_Flag) kill(0, SIGSTOP); else kill(0, SIGTSTP);
   /* (void) SLsignal_intr (SIGTSTP, sig_sys_spawn_cmd); */
#endif
}

/*}}}*/

/* returns 0 if file does not exist, 1 if it is not a dir, 2 if it is */
int sys_chmod(SLFUTURE_CONST char *file, int what, mode_t *mode, uid_t *uid, gid_t *gid) /*{{{*/
{
   struct stat buf;
   mode_t m;

   if (what)
     {
	if (-1 == chmod(file, *mode))
	  goto return_error;

	if (-1 == chown(file, *uid, *gid))
	  {
	     if (-1 == stat (file, &buf))
	       goto return_error;

	     (void) chown(file, buf.st_uid, *gid);
	  }
	return 0;
     }

   if (-1 == stat(file, &buf))
     goto return_error;

   m = buf.st_mode;
   *uid = buf.st_uid;
   *gid = buf.st_gid;

/* AIX requires this */
#ifdef _S_IFDIR
# ifndef S_IFDIR
#  define S_IFDIR _S_IFDIR
# endif
#endif

#ifndef S_ISDIR
# ifdef S_IFDIR
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
# else
#  define S_ISDIR(m) 0
# endif
#endif

   *mode = m & 07777;

   if (S_ISDIR(m)) return (2);
   return(1);

   return_error:

   switch (errno)
     {
      case EACCES: return(-1); /* es = "Access denied."; break; */
      case ENOENT: return(0);  /* ms = "File does not exist."; */
      case ENOTDIR: return(-2); /* es = "Invalid Path."; */
      default: return(-3); /* "stat: unknown error."; break;*/
     }
}

/*}}}*/

#ifdef HAVE_GLOB_H
static glob_t Globbuf;
static int Glob_Count = -1;

int sys_findnext(char *file) /*{{{*/
{
   while ((unsigned int) Glob_Count < (unsigned int) Globbuf.gl_pathc)
     {
	char *f = Globbuf.gl_pathv[Glob_Count++];
	if (strlen (f) < JED_MAX_PATH_LEN)
	  {
	     strcpy (file, f);
	     return 1;
	  }
     }

   Glob_Count = -1;
   globfree(&Globbuf);
   return 0;
}

/*}}}*/
int sys_findfirst(char *the_file) /*{{{*/
{
   char *f, the_path[JED_MAX_PATH_LEN+2], *file;
   unsigned int len;

   file = jed_standardize_filename_static(the_file);
   f = extract_file(file);
   len = strlen (file);
   if (len >= JED_MAX_PATH_LEN)
     {
	jed_verror ("path name is too long");
	return 0;
     }
   safe_strcpy (the_path, file, sizeof (the_path));

   if (NULL == strchr(f, '*'))
     safe_strcat (the_path, "*", sizeof (the_path));

   if (Glob_Count != -1)
     {
	Glob_Count = -1;
	globfree(&Globbuf);
     }

   if (glob(the_path, GLOB_MARK, NULL, &Globbuf) != 0) return 0;

   Glob_Count = 0;
   return sys_findnext (the_file);
}

/*}}}*/

#else

static char Found_Dir[JED_MAX_PATH_LEN];
static char Found_File[JED_MAX_PATH_LEN];
static int File_Len;

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

static DIR *Dirp;

/* These routines should be fixed to work with wildcards like the DOS and VMS
 * versions do.  Unfortunately, it is a sad fact of life that Unix does NOT
 * support wildcards.  Strangely enough, many Unix users are totally unaware
 * of this fact.
 *
 * To add wild card support, I need to use the regular expression package.
 * Specifically, in sys_findfirst, I would need to scan the file spec for
 * wild cards making the replacements: . --> \., ? -> ., and * -> .* in that
 * order.  I should also quote the special characters ($, etc...) but I will
 * not worry about this.  Finally, I will have to prefix it with '^' since
 * the match will start at the beginning of the string.
 *
 *    Remark: to implement something like a real wildcard file renaming routine
 *    would require even more processing.  Consider something like:
 *       rename *.c~ *.bak
 *    This mean that the first expression (*.c~) would have to be converted to:
 *      \(.*\)\.c~
 *    and the second (*.bak) would be determined to be: \1.bak  where \1 is
 *    the expression matched by the first wildcard.
 *
 * After converting the wildcard file name to a regexp file name, I would then
 * have to compile it and save the compiled expression for use in sys_findnext.
 * There, I would use the regexp string matching routine instead of the
 * strcmp which is now used.
 *
 * Mainly, the findfirst/findnext are used by the completion routines.  This
 * means that there is an implicit '.*' attached to the end of the filespec.
 * This will have to be dealt with.
 */
int sys_findnext(char *file) /*{{{*/
{
   struct dirent *dp;

   if (Dirp == NULL)
     return 0;

   while (1)
     {
	if (NULL == (dp = readdir(Dirp)))
	  {
	     closedir (Dirp); Dirp = NULL;
	     return 0;
	  }
# ifdef NEED_D_NAMLEN
	dp->d_name[dp->d_namlen] = 0;
# endif
	if (!strncmp(Found_File, dp->d_name, File_Len)) break;
     }
   safe_strcpy (file, Found_Dir, JED_MAX_PATH_LEN); 
   safe_strcat (file, dp->d_name, JED_MAX_PATH_LEN);
   if (2 == file_status(file)) safe_strcat (file, "/", JED_MAX_PATH_LEN);
   return(1);
}

/*}}}*/
int sys_findfirst(char *the_file) /*{{{*/
{
   char *f, *file;

   file = jed_standardize_filename_static(the_file);
   safe_strcpy (Found_Dir, file, sizeof (Found_Dir));
   f = extract_file(Found_Dir);
   safe_strcpy (Found_File, f, sizeof (Found_File));
   File_Len = strlen(Found_File);
   *f = 0;			       /* truncate Found_Dir to dirname */

   if (Dirp != NULL) closedir(Dirp);

   if (NULL == (Dirp = (DIR *) opendir(Found_Dir))) return(0);
   safe_strcpy(the_file, file, JED_MAX_PATH_LEN);
   return sys_findnext(the_file);
}

/*}}}*/

#endif				       /* GLOB_H */

/* if non-zero, Flow control is enabled */
void enable_flow_control (int *mode) /*{{{*/
{
   /* This kills X windows.  For the time being, work around it as follows */
   if (X_Init_Term_Hook != NULL) return;
   Flow_Control = *mode;
   reset_tty();
   init_tty();
}

/*}}}*/

