/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <stdio.h>

#if defined(__QNX__) && defined(__WATCOMC__)
# include <env.h>
#endif

#if JED_HAS_SUBPROCESSES
/* Everything else here is in this '#if' */

/*{{{ Include Files */

#include <string.h>
#include <errno.h>
#include <signal.h>

#ifdef __os2__
# include <io.h>
# include <process.h>
#endif

#if defined(__BORLANDC__) && (__BORLANDC__>>8)==5 /* [JMS:MISC] - needed for open/close in BC v5 */
# include <io.h>
# include <dos.h>
#endif

#include <slang.h>

#include "jdmacros.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
# include <fcntl.h>
#endif

#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifdef REALLY_HAVE_TERMIOS_H
# if !defined(__os2__) && !defined(__APPLE__)
#  define USE_PTY
# endif
#endif

#include "buffer.h"
#include "ins.h"
#include "ledit.h"
#include "misc.h"
#include "jprocess.h"
#include "paste.h"
#include "sysdep.h"
#include "sig.h"
#include "cmds.h"

/*}}}*/

#define MAX_TTY_SLAVE_NAME	256

int Num_Subprocesses;
int Max_Subprocess_FD;

/* This also servers as a lookup table for actual system pids to the 
 * pseudo-pids used here.  See, e.g., jed_get_child_status for usage in this
 * fashion.
 */
int Subprocess_Read_fds [MAX_PROCESSES][3];   
/* 0 is actual fd, 1 is our rep, 2 will be set to 1 if an EIO is present */

volatile int Child_Status_Changed_Flag;/* if this is non-zero, editor
					* should call the appropriate
					* function below to call slang
					* handlers.
					*/
typedef struct /*{{{*/
{
   int flags;			       /* This is zero if the process is gone
					* and the status is nolonger avail */
#define PROCESS_RUNNING		1
#define PROCESS_STOPPED		2
#define PROCESS_ALIVE		3
#define PROCESS_EXITED		4
#define PROCESS_SIGNALLED	8
   int return_status;		       /* This value depends on the flags */
   
   int status_changed;		       /* non-zero if status changed. */
   int rd, wd;			       /* read/write descriptors */
   int is_pty;
   int pid;			       /* real process pid */
   int output_type;
#define PROCESS_USE_BUFFER	1
#define PROCESS_USE_SLANG	2
#define PROCESS_SAVE_POINT	4
#define PROCESS_AT_POINT	8
   Buffer *buffer;		       /* buffer associated with process */
   SLang_Name_Type *slang_fun;	       /* function to pass output to */
   SLang_MMT_Type *umark;	       /* marks point of last output */
   int process_flags;
#define USE_CURRENT_BUFFER	0x1    /* use the current buffer instead of
					* the one associated with the process
					*/

   SLang_Name_Type *status_change_fun; /* call this if process status changes 
					* The function should be declared like
					* define fun (pid, flags, status);
					* The flags parameter corresponds to
					* the flags field in this struct and 
					* the pid is NOT the pid of this struct.
					* status depends upon flags.
					*/
   int quietly_kill_on_exit;
} 
/*}}}*/
Process_Type;

static Process_Type Processes[MAX_PROCESSES];

static int signal_safe_close (int fd)
{
   while (-1 == close (fd))
     {
#ifdef EINTR
	if (errno == EINTR)
	  continue;
#endif
	return -1;
     }
   return 0;
}

static int signal_safe_open (char *file, int mode)
{
   int fd;

   while (-1 == (fd = open (file, mode)))
     {
#ifdef EINTR
	if (errno == EINTR)
	  continue;
#endif
#ifdef EAGAIN
	if (errno == EAGAIN)
	  {
	     sleep (1);
	     continue;
	  }
#endif
	return -1;
     }
   return fd;
}

static int signal_safe_fcntl (int fd, int cmd, int arg)
{
   int ret;

   while (-1 == (ret = fcntl (fd, cmd, arg)))
     {
#ifdef EINTR
	if (errno == EINTR)
	  continue;
#endif
#ifdef EAGAIN
	if (errno == EAGAIN)
	  {
	     sleep (1);
	     continue;
	  }
#endif
	return -1;
     }
   return ret;
}

static int signal_safe_dup2 (int fd1, int fd2)
{
   while (-1 == dup2 (fd1, fd2))
     {
#ifdef EINTR
	if (errno == EINTR)
	  continue;
#endif
	return -1;
     }
   return 0;
}


static Process_Type *get_process (int fd) /*{{{*/
{
   Process_Type *p;
   
   if ((fd >= 0) && (fd < MAX_PROCESSES)
       && (p = &Processes[fd], p->flags != 0))
     return p;
   
   jed_verror ("process '%d' does not exist.", fd);
   return NULL;
}

/*}}}*/

static void call_slang_status_change_hook (Process_Type *p) /*{{{*/
{
   Buffer *cbuf = CBuf;
   if ((p->status_change_fun == NULL) || (p->buffer == NULL)) return;
   
   cbuf->locked++;
   if (0 == (p->process_flags & USE_CURRENT_BUFFER))
     switch_to_buffer (p->buffer);
   SLang_push_integer ((int) (p - Processes));
   SLang_push_integer (p->flags);
   SLang_push_integer (p->return_status);
   SLexecute_function (p->status_change_fun);
   touch_screen ();
   if ((0 == (p->process_flags & USE_CURRENT_BUFFER)) && (CBuf != cbuf))
     switch_to_buffer (cbuf);
   cbuf->locked--;
}

/*}}}*/

#if 1
int jed_signal_process (int *fd, int *sig) /*{{{*/
{
   Process_Type *p;
   if (NULL == (p = get_process (*fd))) return -1;

   kill (p->pid, *sig);
   return 0;
}

int jed_signal_fg_process (int *fd, int *sig) /*{{{*/
{
   int pid;

   Process_Type *p;
   if (NULL == (p = get_process (*fd))) return -1;

#ifdef HAVE_TCGETPGRP
   pid = tcgetpgrp (p->rd);
   if (pid == -1)
#endif
     pid = p->pid;

   (void) kill (-pid, *sig);
   return 0;
}

/*}}}*/
#endif

static void close_rd_and_wd (Process_Type *p) /*{{{*/
{   
   if (p->rd != -1) 
     {
	signal_safe_close (p->rd);
	p->rd = -1;
     }
   if (p->wd != -1)
     {
	if (p->is_pty == 0) signal_safe_close (p->wd); 
	p->wd = -1;
     }
}

/*}}}*/

/* This routine is called to clean up after the process has exited.  
 * After getting the exit status, we call a slang hook and if the
 * process is dead, adjust the process arrays to delete the process.
 */
static void get_process_status (Process_Type *p) /*{{{*/
{
   int i;
   int fd, slfd;
   
   /* Call slang to let it know what happened.  Do it first before we 
    * really shut it down to give the hook a chance to query the state of
    * it before it returns.
    */
   call_slang_status_change_hook (p);
   if (p->flags & PROCESS_ALIVE) return;

   /* Process is dead.  So perform clean up. */
   close_rd_and_wd (p);
   
   if (p->buffer != NULL) p->buffer->subprocess = 0;
   slfd = (int) (p - Processes);

   if (p->umark != NULL) SLang_free_mmt (p->umark);

   SLang_free_function (p->slang_fun);
   SLang_free_function (p->status_change_fun);

   memset ((char *)p, 0, sizeof (Process_Type));
   p->rd = p->wd = -1;

   /* Adjust the array of read descriptors */

   i = 0;
   
   while (i < Num_Subprocesses)
     {
	if (Subprocess_Read_fds[i][1] == slfd)
	  break;
	  
	i++;
     }
   fd = Subprocess_Read_fds [i][0];

   Num_Subprocesses--;
   while (i < Num_Subprocesses)
     {
	Subprocess_Read_fds[i][0] = Subprocess_Read_fds[i + 1][0];
	Subprocess_Read_fds[i][1] = Subprocess_Read_fds[i + 1][1];
	Subprocess_Read_fds[i][2] = Subprocess_Read_fds[i + 1][2];
	i++;
     }
   
   
   if (Max_Subprocess_FD == fd)
     {
	i = 0;
	fd = -1;
	while (i < Num_Subprocesses)
	  {
	     if (Subprocess_Read_fds[i][0] > fd) 
	       fd = Subprocess_Read_fds[i][0];
	     i++;
	  }
	Max_Subprocess_FD = fd;
     }
}

/*}}}*/

int jed_close_process (int *fd) /*{{{*/
{
   Process_Type *p;
   
   if (NULL == (p = get_process (*fd))) return -1;
   
   close_rd_and_wd (p);
   
   kill (-p->pid, SIGINT);
   
   /* This is probably a bad idea.  It is better to check to see if it still 
    * around and the set a flag indicating that the user wants it killed.
    */
   
   /* Did we kill it? Make sure. */
   kill (-p->pid, SIGKILL);

   if (p->buffer != NULL) p->buffer->subprocess = 0;
   
   /* This next function wraps things up --- no need to.  Let handler do it. */
   /* get_process_status (p); */
   return 0;
}

/*}}}*/

void jed_kill_process (int fd) /*{{{*/
{
   /* This function is called when the buffer is going to be destroyed */
   Processes[fd].buffer = NULL;
   jed_close_process (&fd);
}

/*}}}*/

void jed_get_child_status (void) /*{{{*/
{
   Process_Type *p, *pmax;
   
   Ignore_User_Abort++;
   if (SLang_get_error ())
     {
	Ignore_User_Abort--;
	return;
     }

   /* FIXME: We should block SIGCHLD before trying to access this flag,
    * or anything in this function for that matter.
    */
   Child_Status_Changed_Flag--;
   
   get_process_input (&Number_Zero);
   p = Processes;
   pmax = p + MAX_PROCESSES;
   
   while (p < pmax)
     {
	if (p->flags && p->status_changed)
	  {
	     read_process_input ((int) (p - Processes));
	     p->status_changed--;
	     get_process_status (p);
	  }
	p++;
     }

   Ignore_User_Abort--;

}

/*}}}*/

static void child_signal_handler (int sig) /*{{{*/
{
   int status;
   int found;
   int save_errno = errno;
   
   (void) sig;

   do
     {
	Process_Type *p, *pmax;

	found = 0;

	p = Processes;
	pmax = p + MAX_PROCESSES;
	while (p < pmax)
	  {
	     int pid;
	     
	     if ((pid = p->pid) <= 0)
	       {
		  p++;
		  continue;
	       }

	     while ((-1 == (pid = (int) waitpid (p->pid, &status, WNOHANG | WUNTRACED)))
		    && (errno == EINTR))
	       ;

	     if (p->pid == pid)
	       {
		  int return_status;

		  found++;

		  if (WIFEXITED (status))
		    {
		       return_status = WEXITSTATUS (status);
		       status = PROCESS_EXITED;
		    }
		  else if (WIFSIGNALED (status))
		    {
		       return_status = WTERMSIG (status);
		       status = PROCESS_SIGNALLED;
		    }
#ifdef WIFSTOPPED
		  else if (WIFSTOPPED (status))
		    {
		       return_status = WSTOPSIG(status);
		       status = PROCESS_STOPPED;
		    }
#endif
		  p->flags = status;
		  p->status_changed++;
		  p->return_status = return_status;
	       }
	     p++;
	  }
     }
   while (found != 0);

   SLsignal_intr (SIGCHLD, child_signal_handler);
   errno = save_errno;
   Child_Status_Changed_Flag++;
}

/*}}}*/

#ifdef USE_PTY
# include "pty.c"
#endif

static int get_master_slave_fds (int *slave_read, int *slave_write,
				 int *master_read, int *master_write,
				 char *slave_tty_name, int *is_pty,
				 int want_pty)
{
#ifdef USE_PTY
   if (want_pty)
     {
	int master;
	
	if (-1 == pty_open_master_pty (&master, slave_tty_name))
	  return -1;

	*master_read = *master_write = master;
	*slave_read = *slave_write = -1;
	
	*is_pty = 1;
     }
   else
     {
#endif
	int fds0[2], fds1[2];
   
	if (-1 == pipe (fds0))
	  return -1;
	
	if (-1 == pipe (fds1))
	  {
	     signal_safe_close (fds0[0]);
	     signal_safe_close (fds0[1]);
	     return -1;
	  }
	*master_read = fds0[0];
	*slave_write = fds0[1];
	*master_write = fds1[1];
	*slave_read = fds1[0];
	*slave_tty_name = 0;
	*is_pty = 0;
#ifdef USE_PTY
     }
#endif
   return 0;
}


static void my_setenv (char *what, char *value)
{
#ifdef HAVE_SETENV
   (void) setenv (what, value, 1);
#else
# ifdef HAVE_PUTENV
   char buf[512];
   SLsnprintf (buf, sizeof (buf), "%s=%s", what, value);
   (void) putenv (buf);
# endif
#endif
}

static void my_unsetenv (char *what)
{
#ifdef HAVE_UNSETENV
   unsetenv (what);
#endif
}

   
#ifdef __os2__
static int open_process (char *pgm, char **argv, int want_pty) /*{{{*/
{
    int val;
    int pd;
   int slave_read, slave_write, master_read, master_write, org_fd[3];
   int pid, i;
   Process_Type *p;
   SLang_MMT_Type *mmt;
   char slave_tty_name [MAX_TTY_SLAVE_NAME];
   int max_write_tries;
   SLSig_Fun_Type *sig_orig[NSIG];

   if (Jed_Secure_Mode)
     {
	msg_error ("Access to shell denied.");
	return -1;
     }
   
   pd = 0; while ((pd < MAX_PROCESSES) && Processes[pd].flags) pd++;
   if (pd == MAX_PROCESSES) return -1;
   p = &Processes[pd];

   SLMEMSET ((char *) p, 0, sizeof (Process_Type));

   if (NULL == (mmt = jed_make_user_object_mark ()))
     return -1;
   
   if (-1 == get_master_slave_fds (&slave_read, &slave_write,
				   &master_read, &master_write,
				   slave_tty_name, &p->is_pty, want_pty))
     {
	SLang_free_mmt (mmt);
	return -1;
     }

   SLsignal_intr (SIGCHLD, child_signal_handler);
   
     {
	char ch;
	
	/* At this point the slave tty is in raw mode.  Make sure that
	 * the read to synchronize with the parent blocks.
	 */
	val = signal_safe_fcntl (slave_read, F_GETFL, 0);
	signal_safe_fcntl (slave_read, F_SETFL, val & ~O_NONBLOCK);

	signal_safe_fcntl (slave_read, F_SETFL, val);

	
	org_fd[0] = dup(0);
	org_fd[1] = dup(1);
	org_fd[2] = dup(2);

	signal_safe_close (0);
	signal_safe_close (1);
	signal_safe_close (2);
	
	signal_safe_dup2(slave_read, 0);	/* stdin */
	signal_safe_dup2 (slave_write, 1);	/* stdout */
	signal_safe_dup2 (slave_write, 2);	/* stderr */

	signal_safe_close (slave_read);
	signal_safe_close (slave_write);
	setvbuf (stdout, NULL, _IONBF, 0);
	
	for (i=1 ; i<NSIG ; i++)
	  sig_orig[i] = SLsignal(i, SIG_DFL);
	
	pid = spawnvp (P_SESSION | P_MINIMIZE | P_BACKGROUND, pgm, argv);

	for (i=1 ; i<NSIG ; i++)
	  SLsignal(i, sig_orig[i]);
	signal_safe_close (0);
	signal_safe_close (1);
	signal_safe_close (2);
	signal_safe_dup2(org_fd[0], 0);
	signal_safe_dup2(org_fd[1], 1);
	signal_safe_dup2(org_fd[2], 2);
	signal_safe_close (org_fd[0]); 
	signal_safe_close (org_fd[1]); 
	signal_safe_close (org_fd[2]); 
	signal_safe_close (slave_read);
	signal_safe_close (slave_write);
	
	if (pid < 0)
	  {
	     signal_safe_close (master_read);
	     signal_safe_close (master_write); 
	     p->flags = 0;
	     SLang_free_mmt (mmt);
	     return -1;
	     
	  }
	p->pid = pid;
     }

   

   p->flags = PROCESS_RUNNING;
   p->rd = master_read;
   p->wd = master_write;
   
   Subprocess_Read_fds[Num_Subprocesses][0] = master_read;
   Subprocess_Read_fds[Num_Subprocesses][1] = pd;
   Subprocess_Read_fds[Num_Subprocesses][2] = 0;
   if (master_read > Max_Subprocess_FD) Max_Subprocess_FD = master_read;
   Num_Subprocesses += 1;
   
   val = signal_safe_fcntl (master_read, F_GETFL, 0);
   val |= O_NONBLOCK;
   signal_safe_fcntl (master_read, F_SETFL, val);

   CBuf->subprocess = pd + 1;

   /* Processing options */
   p->buffer = CBuf;
   p->output_type = PROCESS_USE_BUFFER;
   p->umark = mmt;
   SLang_inc_mmt (mmt);	       /* tell slang we are keeping a copy */
   
   return pd;
}
/*}}}*/
#else	/* unix and compatible OS */

static volatile int SigUsr_Flag;
static void sigusr_handler (int sig)
{
   (void) sig;

   SigUsr_Flag = 1;
}

static sigset_t Old_Signal_Mask, Zero_Signal_Mask;

static int init_child_parent_sync (void)
{
   sigset_t new_mask;

   if (SIG_ERR == SLsignal (SIGUSR1, sigusr_handler))
     return -1;
   
   /* Get a copy of the current signal mask */
   while ((-1 == sigprocmask (SIG_BLOCK, NULL, &Zero_Signal_Mask))
	  && (errno == EINTR))
     ;
   /* and remove SIGUSR1 from it. */
   sigdelset (&Zero_Signal_Mask, SIGUSR1);

   /* Add SIGUSR1 to the set of currently blocked signals */
   sigemptyset (&new_mask);
   sigaddset (&new_mask, SIGUSR1);

   while ((-1 == sigprocmask (SIG_BLOCK, &new_mask, &Old_Signal_Mask))
	  && (errno == EINTR))
     ;
   
   SigUsr_Flag = 0;
   return 0;
}

static void wait_for_parent (void)
{
   /* Suspend this process and wait for a signal.  All signals are allowed
    * here.  
    */
   while (SigUsr_Flag == 0)
     sigsuspend (&Zero_Signal_Mask);
}

static void tell_parent_to_go (pid_t pid)
{
   SigUsr_Flag = 0;
   kill (pid, SIGUSR1);
   
   while ((-1 == sigprocmask (SIG_SETMASK, &Old_Signal_Mask, NULL))
	  && (errno == EINTR))
     ;
}

static void tell_child_to_go (pid_t pid)
{   
   SigUsr_Flag = 0;
   kill (pid, SIGUSR1);

   /* Now wait for the child to setup the pty */
   while (SigUsr_Flag == 0)
     sigsuspend (&Zero_Signal_Mask);   

   while ((-1 == sigprocmask (SIG_SETMASK, &Old_Signal_Mask, NULL))
	  && (errno == EINTR))
     ;
}

static int set_non_blocking_io (int fd)
{   
   int val;
   
   val = signal_safe_fcntl (fd, F_GETFL, 0);
   return signal_safe_fcntl (fd, F_SETFL, val | O_NONBLOCK);
}

#ifdef SIGTRAP
static void sigtrap_handler (int sig)
{
   (void) sig;
}
#endif
   
static int open_process (char *pgm, char **argv, int want_pty) /*{{{*/
{
   int pd;
   int slave_read, slave_write, master_read, master_write;
   int pid, jed_pid, i;
   Process_Type *p;
   SLang_MMT_Type *mmt;
   char slave_tty_name [MAX_TTY_SLAVE_NAME];

   if (Jed_Secure_Mode)
     {
	msg_error ("Access to shell denied.");
	return -1;
     }
   
   pd = 0; while ((pd < MAX_PROCESSES) && Processes[pd].flags) pd++;
   if (pd == MAX_PROCESSES) return -1;
   p = &Processes[pd];

   SLMEMSET ((char *) p, 0, sizeof (Process_Type));

   if (NULL == (mmt = jed_make_user_object_mark ()))
     return -1;
   
   if (-1 == get_master_slave_fds (&slave_read, &slave_write,
				   &master_read, &master_write,
				   slave_tty_name, &p->is_pty, want_pty))
     {
	SLang_free_mmt (mmt);
	return -1;
     }

   SLsignal_intr (SIGCHLD, child_signal_handler);

   jed_pid = getpid ();		       /* used by slave */

   if ((-1 == init_child_parent_sync ())
       || ((pid = fork ()) < 0))
     {
	signal_safe_close (master_read);
	if (p->is_pty == 0)
	  {
	     signal_safe_close (slave_read);
	     signal_safe_close (master_write); 
	     signal_safe_close (slave_write);
	  }
	p->flags = 0;
	SLang_free_mmt (mmt);
	return -1;
     }
   p->pid = pid;
   
   /* Make the child its own process group leader.  Do it here too because
    * we are not sure which one will run first.  We have to do this because
    * if not, a ^G will be sent to ALL child subprocesses possibly killing 
    * them unless they catch the signal.  This call means that the INTR signal
    * will not be sent to any child processes sent by this fork.
    */
   if (p->is_pty == 0) 
     setpgid(pid, pid);

   if (pid == 0)
     {
	/* child code */
	wait_for_parent ();
	
	if (p->is_pty == 0)
	  signal_safe_close (master_write);	       /* close write end of 0 */
	signal_safe_close (master_read);	       /* close read end of 1 */

#ifdef USE_PTY	
	/* Call set setsid so that the child will become the session leader.
	 * This has the side effect that we will loose the controlling
	 * terminal.  For this reason, the pty slave is opened after setsid
	 * and then for good luck, the controlling terminal is set 
	 * via the TIOCSCTTY ioctl.
	 */
	if (p->is_pty)
	  {
	     if (-1 == setsid ())
	       fprintf (stderr, "child: setsid failed.\n");
	
	     if (-1 == pty_open_slave_pty (slave_tty_name, &slave_read))
	       {
		  tell_parent_to_go (jed_pid);
		  fprintf (stderr, "child: failed to open slave.");
		  _exit (1);
	       }
	     slave_write = slave_read;

	     /* (void) pty_setup_slave_term (slave_read, 0); */
	  }
#endif	/* USE_PTY */
	tell_parent_to_go (jed_pid);

#ifdef USE_PTY
	/* Put tty back into cbreak mode. */
	if (p->is_pty) (void) pty_setup_slave_term (slave_read, 0);
#endif
	if ((signal_safe_dup2(slave_read, 0) < 0)     /* stdin */
	    || (signal_safe_dup2 (slave_write, 1) < 0) /* stdout */
	    || (signal_safe_dup2 (slave_write, 2) < 0)) /* stderr */
	  {
	     fprintf (stderr, "dup2 failed. errno = %d\n", errno);
	     _exit (1);
	  }

	my_setenv ("TERM", "unknown");
	my_unsetenv ("TERMCAP");

	for (i = 0; i < 32; i++) 
	  {
#ifdef SIGTRAP
	/* Under Linux, subprocesses fail if run under the debugger because
	 * something sends this child a SIGTRAP.  Let's try ignoring it.
	 */
	     if (i == SIGTRAP)
	       SLsignal (i, sigtrap_handler);
	     else
#endif
	       SLsignal (i, SIG_DFL);
	  }
#ifdef SIGTRAP
	SLsignal (SIGTRAP, SIG_DFL);
#endif
	if (execvp (pgm, argv) < 0)
	  {
	     fprintf (stderr, "execvp of %s failed!\r\n", pgm);
	     _exit (1);
	  }
     }
   
   /* parent */
   if (p->is_pty == 0) 
     {
	signal_safe_close (slave_read);
	signal_safe_close (slave_write);
     }

   p->flags = PROCESS_RUNNING;
   p->rd = master_read;
   p->wd = master_write;
   
   Subprocess_Read_fds[Num_Subprocesses][0] = master_read;
   Subprocess_Read_fds[Num_Subprocesses][1] = pd;
   Subprocess_Read_fds[Num_Subprocesses][2] = 0;
   if (master_read > Max_Subprocess_FD) Max_Subprocess_FD = master_read;
   Num_Subprocesses += 1;
   
   CBuf->subprocess = pd + 1;

   /* Processing options */
   p->buffer = CBuf;
   p->output_type = PROCESS_USE_BUFFER;
   p->umark = mmt;
   SLang_inc_mmt (mmt);	       /* tell slang we are keeping a copy */
   
   set_non_blocking_io (master_read);
   set_non_blocking_io (master_write);

   /* Tell child it is ok to go. */
   tell_child_to_go (pid);
   return pd;
}

/*}}}*/
#endif				       /* __os2__ */

static int flag_fd_as_eio_error (int fd)
{
   unsigned int i, n;
   
   n = Num_Subprocesses;
   for (i = 0; i < n; i++)
     {
	if (Subprocess_Read_fds[i][0] == fd)
	  {
	     Subprocess_Read_fds[i][2] = 1;
	     return 0;
	  }
     }
   return -1;
}

/* This function is only called when we are reading characters from the 
 * keyboard.  Keyboard input has the highest priority and this is called only
 * if there is no input ready.
 * 
 * It might also get called by the hooks that it calls.  As a result, it must
 * be reentrant.
 * 
 * It is possible for the user to press Ctrl-G while this routine is executing.
 * This could cause slang hooks to fail.  It would probably be better to 
 * block SIGINT for the duration of this routine.
 */
void read_process_input (int fd) /*{{{*/
{
   unsigned char buf[513];	       /* last byte for 0 char */
   int n;
   Buffer *b = CBuf, *pbuf;
   Process_Type *p;
   int otype, total;
   
   /* Should never happen */
   if (NULL == (p = get_process (fd))) 
     {
	return;
     }
   
   Ignore_User_Abort++;
   if (SLang_get_error ())
     {
	Ignore_User_Abort--;
	return;
     }
   
   otype = p->output_type;
   pbuf = p->buffer;
   
   if (pbuf != NULL)
     {
	if (0 == (p->process_flags & USE_CURRENT_BUFFER))
	  switch_to_buffer (pbuf);
	pbuf->locked++;
     }

   total = 0;
   if (otype & PROCESS_SAVE_POINT) push_spot ();
   while ((n = read (p->rd, buf, 512)) > 0)
     {
	total += n;
	if (p->buffer == NULL) continue;
	
	if (otype & PROCESS_USE_BUFFER) 
	  {
	     if (0 == (otype & PROCESS_AT_POINT)) eob ();
	     (void) jed_insert_nbytes (buf, n);
	     jed_move_user_object_mark (p->umark);
	  }
	else if (otype == PROCESS_USE_SLANG)
	  {
	     buf[n] = 0;
	     if ((-1 == SLang_push_integer ((int) (p - Processes)))
		 || (-1 == SLang_push_string ((char *) buf))
		 || (-1 == SLexecute_function (p->slang_fun)))
	       {
		  /* It looks like the contents of buf may be lost */
		  break;
	       }
	  }
     }
   
   if (n == -1)
     {
#ifdef EIO
	if (errno == EIO)
	  (void) flag_fd_as_eio_error (p->rd);
#endif
     }

   if (otype & PROCESS_SAVE_POINT) pop_spot ();
   else if (otype & PROCESS_USE_BUFFER) move_window_marks (0);
   
   if (p->buffer != NULL)
     {
	if ((b != CBuf) && (0 == (p->process_flags & USE_CURRENT_BUFFER)))
	  switch_to_buffer (b);
     }
   
   /* Since it was locked, it cannot be deleted.  So pbuf is still a good 
    * pointer
    */
   if (pbuf != NULL)
     pbuf->locked--;

   if (total)
     {
#ifdef __os2__
	/* Why does OS/2 need this?? -- JED */
	JWindow->trashed = 1;
	update ((Line *)NULL, 0, 1, 0);
#endif
	touch_screen ();
     }
   
   Ignore_User_Abort--;
}


/*}}}*/

static int write_to_process (int fd, char *buf, unsigned int len)
{
   int n, ilen;
   
   if (len == 0) 
     return 0;
   
   ilen = (int) len;
   n = 0;
   while (n < ilen)
     {
	int dn;
	int count = 0;

	while (-1 == (dn =  write (fd, buf + n, ilen - n)))
	  {
	     if (errno == EINTR)
	       continue;
	     
#ifdef EAGAIN
	     if (errno == EAGAIN)
	       {
		  get_process_input (&Number_Zero);
		  count++;
		  if ((count % 10) == 0)
		    {
		       sleep (1);
		       if (count > 20)
			 return n;
		    }
		  continue;
	       }
#endif
	     return -1;
	  }

	if (dn == 0)
	  break;

	n += dn;
     }
  
   return n;
}

int jed_send_process (int *fd, char *str) /*{{{*/
{
   unsigned int len;
   Process_Type *p = get_process (*fd);
   if ((p == NULL) || (p->wd == -1)) return -1;
   
   len = strlen (str);
   
   if (len != (unsigned int) write_to_process (p->wd, str, len))
     jed_verror ("write to process failed");
   
   return len;
}

/*}}}*/

void jed_send_process_eof (int *fd) /*{{{*/
{
   Process_Type *p = get_process (*fd);
   
   if (p == NULL) 
     return;
   
   if (p->wd == -1)
     return;
   
   if (p->is_pty)
     write_to_process (p->wd, "\004", 1);
   else
     {
	signal_safe_close (p->wd);
	p->wd = -1;
     }
}


/*}}}*/

/* If s is NULL, then f != NULL, or visa-versa */
static int set_process (Process_Type *p, char *what, char *s, SLang_Name_Type *f) /*{{{*/
{
   if (!strcmp (what, "output"))
     {
	if (s != NULL)
	  {
	     if (0 == strcmp (s, "."))
	       {
		  p->output_type = PROCESS_AT_POINT | PROCESS_USE_BUFFER;
		  return 0;
	       }
	     
	     if (0 == strcmp (s, "@"))
	       {
		  p->output_type = PROCESS_SAVE_POINT | PROCESS_USE_BUFFER;
		  return 0;
	       }
	     
	     if (*s == 0)
	       {
		  p->output_type = PROCESS_USE_BUFFER;
		  return 0;
	       }

	     if (NULL == (f = SLang_get_function (s)))
	       return -1;
	  }
	
	p->output_type = PROCESS_USE_SLANG;
	p->slang_fun = f;
	return 0;
     }
   else if (!strcmp (what, "signal"))
     {
	if ((s != NULL)
	    && (NULL == (f = SLang_get_function (s))))
	  return -1;

	p->status_change_fun = f;
	return 0;
     }
   
   jed_verror ("set_process: %s not supported", what);
   return -1;
}

/*}}}*/

void jed_set_process (void)
{
   int pd;
   char *what, *s;
   Process_Type *p;
   SLang_Name_Type *f;

   f = NULL;
   what = NULL;
   s = NULL;

   if (SLang_peek_at_stack () == SLANG_REF_TYPE)
     {
	if (NULL == (f = SLang_pop_function ()))
	  return;
     }
   else if (-1 == SLang_pop_slstring (&s))
     return;
   
   if ((0 == SLang_pop_slstring (&what))
       && (0 == SLang_pop_integer (&pd))
       && (NULL != (p = get_process (pd)))
       && (0 == set_process (p, what, s, f)))
     f = NULL;
     
   SLang_free_slstring (what);
   SLang_free_slstring (s);
   SLang_free_function (f);
}


void jed_set_process_flags (int *fd, int *oflags)
{
   Process_Type *p;
   if (NULL == (p = get_process (*fd))) 
     return;
   
   p->process_flags = *oflags;
}
 
int jed_get_process_flags (int *fd)
{
   Process_Type *p;
   if (NULL == (p = get_process (*fd))) 
     return -1;

   return p->process_flags;
}

void jed_get_process_mark (int *fd) /*{{{*/
{
   Process_Type *p;
   if (NULL == (p = get_process (*fd))) return;
   
   SLang_push_mmt (p->umark);
}

/*}}}*/

static int open_process_of_type (int nargs, int wantpty) /*{{{*/
{
   int fd = -1;
   char *argv[502];
   int n = nargs;

   if (CBuf->subprocess)
     {
	msg_error ("There is already a process attached to this buffer.");
	return -1;
     }
   
   if ((n > 500) || (n < 0))
     {
	msg_error ("Arguments out of range.");
	return -1;
     }
   
   n++;				       /* for argv0 since *np does not include
					* it. 
					*/
   argv[n] = NULL;
   while (n--)
     {
	if (SLang_pop_slstring (&argv[n]))
	  {
	     n++;
	     goto free_return;
	  }
     }
   n = 0;

   if ((fd = open_process(argv[0], argv, wantpty)) < 0)
     {
	jed_verror ("Unable to open %s process.", argv[0]);
     }
   
   /* free up the argument strings */
   free_return:
   
   while (n <= nargs)
     {
	SLang_free_slstring (argv[n]);
	n++;
     }
   
   return fd;
}

/*}}}*/

int jed_open_process (int *np)
{
   return open_process_of_type (*np, 1);
}

int jed_open_process_pipe (int *np)
{
   return open_process_of_type (*np, 0);
}


void jed_block_child_signal (int block) /*{{{*/
{
   static sigset_t new_mask, old_mask;
   
   if (block)
     {
	sigemptyset (&new_mask);
	sigaddset (&new_mask, SIGCHLD);
	(void) sigprocmask (SIG_BLOCK, &new_mask, &old_mask);
	return;
     }
   
   (void) sigprocmask (SIG_SETMASK, &old_mask, NULL);
}

/*}}}*/

/* Jed only calls these in pairs so that this should be fine. */
FILE *jed_popen (char *cmd, char *type) /*{{{*/
{
   FILE *pp;
   
   jed_block_child_signal (1);
   pp = popen (cmd, type);
   if (pp == NULL)
     jed_block_child_signal (0);
   return pp;
}

/*}}}*/

int jed_pclose (FILE *fp) /*{{{*/
{
   int ret;
   
   if (fp == NULL)
     return -1;
   
   ret = pclose (fp);
   jed_block_child_signal (0);
   
   return ret;
}

/*}}}*/

#if 0
/* These are my versions of popen/pclose.  For some reason, the popen/pclose 
 * do not work on SunOS when there are subprocesses.  I think it has 
 * something to do with the way pclose is waiting.
 * See Steven's book for more information.
 */
#ifndef OPEN_MAX
#define OPEN_MAX 256
#endif
static pid_t Popen_Child_Pids[OPEN_MAX];
FILE *jed_popen(char *cmd, char *type) /*{{{*/
{
   int i, pfd[2], fd;
   pid_t pid;
   FILE	*fp;

   if (((*type != 'r') && (*type != 'w')) || (*(type + 1) != 0))
     {
	errno = EINVAL;		/* required by POSIX.2 */
	return(NULL);
     }

   if (pipe(pfd) < 0) return(NULL);	/* errno set by pipe() or fork() */
   if ((pid = fork()) < 0) return(NULL);
   
   if (pid == 0) 
     {				       /* child */
	if (*type == 'r') 
	  {
	     signal_safe_close(pfd[0]);
	     if (pfd[1] != 1) signal_safe_dup2(pfd[1], 1);
	     signal_safe_close(pfd[1]);
	  }
	else
	  {
	     signal_safe_close(pfd[1]);
	     if (pfd[0] != 0)
	       {
		  signal_safe_dup2(pfd[0], STDIN_FILENO);
		  signal_safe_close(pfd[0]);
	       }
	  }
	
	/* POSIX requires that all streams open by previous popen
	 * be closed.
	 */
	for (i = 0; i < OPEN_MAX; i++)
	  {
	     if (Popen_Child_Pids[i] > 0) signal_safe_close(i);
	  }

	execl("/bin/sh", "sh", "-c", cmd, (char *) 0);
	_exit(127);
     }
   
   /* parent */
   if (*type == 'r') 
     {
	signal_safe_close(pfd[1]);
	if (NULL == (fp = fdopen(pfd[0], type)))
	  return(NULL);
     } 
   else 
     {
	signal_safe_close(pfd[0]);
	if (NULL == (fp = fdopen(pfd[1], type)))
	  return(NULL);
     }
   
   fd = fileno (fp);
   if (fd >= OPEN_MAX)
     {
#ifdef EMFILE
	errno = EMFILE;
#endif
	fclose (fp);
	return NULL;
     }
   Popen_Child_Pids [fd] = pid;
   return(fp);
}

/*}}}*/
int jed_pclose(FILE *fp) /*{{{*/
{
   int	fd, stat;
   pid_t pid;
   int ret;

   fd = fileno(fp);
   if ((fd >= OPEN_MAX)
       || (fd < 0)
       || (0 == (pid = Popen_Child_Pids[fd])))
     return -1;

   Popen_Child_Pids [fd] = 0;
   
   if (fclose(fp) == EOF) return(-1);

   /* This is the part that the SunOS pclose was apparantly screwing up. */
   while (-1 == waitpid(pid, &stat, 0))
     {
	if (errno != EINTR)
	  return -1;

	(void) SLang_handle_interrupt ();
     }
   
   ret = WEXITSTATUS(stat);
   if (WIFEXITED (stat))
     return ret;
   
   return -1;
}

/*}}}*/
#endif


void jed_query_process_at_exit (int *pid, int *query)
{
   Process_Type *p;

   if (NULL == (p = get_process (*pid)))
     return;
   
   p->quietly_kill_on_exit = (*query == 0);
}

   
int jed_processes_ok_to_exit (void)
{     
   Process_Type *p, *pmax;
   int num;
   char buf[64];

   if (Num_Subprocesses == 0)
     return 1;

   num = 0;
   p = Processes;
   pmax = p + MAX_PROCESSES;
   
   while (p < pmax)
     {
	if ((p->flags & PROCESS_ALIVE)
	    && (0 == p->quietly_kill_on_exit))
	  num++;
	p++;
     }
   
   if (num == 0)
     return 1;
   
   sprintf (buf, "%d Subprocesses exist.  Exit anyway", num);
   return jed_get_y_n (buf);
}

int jed_fork_monitor (void)
{
   pid_t pid;
   int i;

   while (-1 == (pid = fork ()))
     {
	if (errno == EINTR)
	  continue;

	fprintf (stderr, "Unable to fork: errno=%d\n", errno);
	return -1;
     }
   
   if (pid != 0)
     return 0;
   
   pid = getppid ();

   for (i = 1; i < 32; i++)
     (void) SLsignal (i, SIG_IGN);

   SLsignal (SIGQUIT, SIG_DFL);
   SLsignal (SIGTERM, SIG_DFL);
   SLsignal (SIGCONT, SIG_DFL);
   SLsignal (SIGTSTP, SIG_DFL);
   
   for (i = 0; i < 256; i++)
     {
	while ((-1 == close (i)) 
	       && (errno == EINTR))
	  ;
     }

   while (1)
     {
	if ((-1 == kill (pid, 0))
	    && (errno == ESRCH))
	  _exit (0);
	
	sleep (10);
     }
}

#endif 				       /* JED_HAS_SUBPROCESSES */
