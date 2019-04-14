/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
/*{{{ Include Files */

#include "config.h"
#include "jed-feat.h"
#include <stdio.h>

#if defined(__unix__) || defined(VMS)

#include <errno.h>
#include <slang.h>

#include "jdmacros.h"

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <signal.h>
#if !defined(VMS) || (__VMS_VER >= 70000000)
# include <sys/types.h>
#endif

#include "sig.h"
#include "file.h"
#include "sysdep.h"
#include "misc.h"
#include "screen.h"
#include "cmds.h"
#include "hooks.h"

/*}}}*/

#ifdef SIGTSTP
int Jed_Handle_SIGTSTP = 1;
#endif

#if !defined(VMS) || (__VMS_VER >= 70000000)

static void sigwinch_handler (int sig) /*{{{*/
{
   sig = errno;
   Jed_Resize_Pending = 1;
# ifdef SIGWINCH
   SLsignal_intr (SIGWINCH, sigwinch_handler);
# endif
   errno = sig;
}

/*}}}*/
#endif

static volatile int Signal_In_Progress = 0;

#if !defined(VMS) || (__VMS_VER >= 70000000)

/* Handle SIGHUP-like signals differently.  The reason is that we might get
 * such a signal when messing with a linked list pointer, etc.  We do not want
 * to exit at that point because the exit routines may need that pointer
 * intact.  So, set a hook that should be called by the editor at a
 * convenient moment.
 */
static int Sig_Hup_Sig;
static void hup_exit_jed (void)
{
   char buf[64];
   Jed_Sig_Exit_Fun = NULL;
   auto_save_all ();
   sprintf (buf, "Killed by signal %d.", Sig_Hup_Sig);
   exit_error (buf, 0);
   exit (1);
}

static void sig_hup_exit_jed (int sig)
{
   if (Sig_Hup_Sig == 0)
     {
	Sig_Hup_Sig = sig;
	Jed_Sig_Exit_Fun = hup_exit_jed;
     }
}

static void sig_exit_jed(int sig) /*{{{*/
{
   char buf[48];

   if (Signal_In_Progress)
     return;

   Signal_In_Progress = 1;
   SLsig_block_signals ();

   auto_save_all ();
   sprintf (buf, "Killed by signal %d.", sig);
   exit_error (buf, (sig == SIGSEGV) || (sig == SIGBUS));
   exit (1);
}

/*}}}*/

/* a control-G puts us here */
static void my_interrupt(int sig) /*{{{*/
{
   sig = errno;

   SLKeyBoard_Quit = 1;
   if (Ignore_User_Abort == 0) SLang_set_error (SL_USER_BREAK);
   SLsignal_intr (SIGINT, my_interrupt);

   errno = sig;
}

/*}}}*/

# if defined( SIGTSTP ) || (defined( VMS) && ( __VMS_VER >= 70000000))
int Signal_Sys_Spawn_Flag = 0;
/* This should only be called from outside disturbance */
void sig_sys_spawn_cmd(int sig) /*{{{*/
{
   sig = errno;
   Signal_Sys_Spawn_Flag = 1;
   SLsignal_intr (SIGTSTP, sig_sys_spawn_cmd);
   errno = sig;
}

/*}}}*/

# endif
#endif /* NOT VMS */

#define CATCH_SIGTTIN 0

#if CATCH_SIGTTIN
#ifdef SIGTTIN
static void background_read (int sig) /*{{{*/
{
   sig = errno;
   if (Stdin_Is_TTY == 0)
     {
	if (Signal_In_Progress)
	  return;

	Signal_In_Progress = 1;
	SLsig_block_signals ();
	exit_error ("Attempt to read from background-- exiting.", 0);
	exit (1);
     }
   sig_sys_spawn_cmd (0);
   errno = sig;
}

/*}}}*/
#endif
#endif

static void (*Old_Sigint_Handler)(int);
static void (*Old_Sigtstp_Handler)(int);

void jed_reset_signals (void)
{
   SLsignal (SIGINT, Old_Sigint_Handler);
#ifdef SIGTSTP
   SLsignal (SIGTSTP, Old_Sigtstp_Handler);
#endif
   /* return 0; */
}

void init_signals (void) /*{{{*/
{
#if !defined(VMS) || (__VMS_VER >= 70000000)

#ifdef SIGWINCH
   (void) SLsignal_intr(SIGWINCH, sigwinch_handler);
#endif

   Old_Sigint_Handler = SLsignal_intr (SIGINT, my_interrupt);

   SLsignal (SIGHUP, sig_hup_exit_jed);
   SLsignal (SIGQUIT, sig_hup_exit_jed);
   SLsignal (SIGTERM, sig_hup_exit_jed);

   SLsignal (SIGILL, sig_exit_jed);
#if 0 && defined(SIGTRAP)
   SLsignal (SIGTRAP, sig_exit_jed);
#endif
#if 0
   SLsignal (SIGIOT, sig_exit_jed);  /* used by abort */
#endif
#ifdef SIGPIPE
   SLsignal_intr (SIGPIPE, SIG_IGN);
#endif
   /* SIGNAL (SIGFPE, sig_exit_jed); */
#ifdef SIGBUS
   SLsignal (SIGBUS, sig_exit_jed);
#endif
#if 0
   SLsignal (SIGSEGV, sig_exit_jed);
#endif
#ifdef SIGSYS
    SLsignal (SIGSYS, sig_exit_jed);
#endif

#ifdef SIGTSTP
   if (Jed_Handle_SIGTSTP)
     {
	Old_Sigtstp_Handler = SLsignal_intr (SIGTSTP, sig_sys_spawn_cmd);
	if (SIG_DFL != Old_Sigtstp_Handler)
	  Jed_Suspension_Not_Allowed = 1;
     }
#endif

#if CATCH_SIGTTIN
# ifdef SIGTTOU
   SLsignal_intr (SIGTTOU, background_read);
# endif
# ifdef SIGTTIN
   SLsignal_intr (SIGTTIN, background_read);
# endif
#endif

#endif /* VMS */
}

/*}}}*/

#endif				       /* __unix__ || VMS */
