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

#ifdef __WIN32__
/* This needs to go first before stdio is included. */
# include <windows.h>
# if !defined(__CYGWIN32__)
#  define sleep Sleep
# endif
#endif

#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef REAL_UNIX_SYSTEM
# include <sys/types.h>
# include <sys/stat.h>
#endif

#include <errno.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <string.h>
#include <setjmp.h>
#ifdef __unix__
# ifdef SYSV
#   include <sys/types.h>
#   include <fcntl.h>
# endif
# include <sys/file.h>
#endif

#ifdef __MSDOS__
# include <io.h>
# if !defined(__WATCOMC__)
#  include <dir.h>
#  ifdef __BORLANDC__
extern unsigned _stklen = 10000U;
#  endif
# else
#  include <direct.h>
# endif
#endif

#if (defined(__WIN32__) && !defined(__CYGWIN32__)) || defined(__IBMC__)
# include <io.h>
# include <direct.h>
#endif

#if defined(__DECC) && defined(VMS)
#include <unixio.h>
#include <unixlib.h>
#endif

#ifndef O_RDWR
#ifndef VMS
# include <fcntl.h>
#endif
#endif

#include "file.h"
#include "buffer.h"
#include "display.h"
#include "sysdep.h"
#include "sig.h"
#include "misc.h"
#include "keymap.h"
#include "screen.h"
#include "ledit.h"
#include "search.h"
#include "text.h"
#include "hooks.h"
#include "paste.h"
#include "version.h"
#include "indent.h"
#include "colors.h"
#include "jprocess.h"

#ifdef __WIN32__
# include "win32.h"
#endif
/*}}}*/

int Jed_Secure_Mode;
int Jed_Load_Quietly = 0;
int Jed_UTF8_Mode;

char Jed_Root_Dir [JED_MAX_PATH_LEN];

typedef struct /*{{{*/
{
   jmp_buf b;
} 
/*}}}*/
jmp_buf_struct;
extern jmp_buf_struct Jump_Buffer, *Jump_Buffer_Ptr;

int Batch = 0;

#ifdef MSWINDOWS
static char *get_win32_root(void)
{
   static char base_path[JED_MAX_PATH_LEN];
   char *p;

   base_path[0] = 0;
   if (GetModuleFileName(NULL, base_path, JED_MAX_PATH_LEN-10) > 0)
     {
	/* drop file name */
	p = strrchr(base_path, '\\');
	if (p != NULL)
	  {
	     *p = '\0';
	     /* drop also 'bin' */
	     p = strrchr(base_path, '\\');
	     if (p != NULL) 
	       *p = '\0';
	  }
     }
   return base_path;
}
#endif
 

static void set_jed_root (char *pgm)
{
   char *jr, *jl;
   char jed_lib [JED_MAX_PATH_LEN + 1];

   *jed_lib = 0;

#ifdef VMS
   jr = "JED_ROOT:";
#else
   jr = (char *) getenv("JED_ROOT");
#endif
   
#ifdef JED_ROOT
   if ((jr == NULL) && (file_status(JED_ROOT) == 2))
     {
	jr = JED_ROOT;
     }
#endif
#ifdef MSWINDOWS
   if (jr == NULL)
     jr = get_win32_root ();
#endif

   if (jr != NULL) 
     {
	strcpy (Jed_Root_Dir, jr);
	strcpy (jed_lib, jr);
#ifndef VMS
	fixup_dir (jed_lib);
	strcat (jed_lib, "lib");
#else
	strcat(jed_lib, "[lib]");
#endif
     }
   
   jl = (char *) getenv("JED_LIBRARY");
	
   if (jl == NULL)
     {
	if (*jed_lib == 0)
	  {
	     unsigned int len;
	     
	     jl = extract_file (pgm);
	     len = (unsigned int) (jl - pgm);
	     strncpy (jed_lib, pgm, len);
	     jed_lib [len] = 0;
	  }
     }
   else strcpy(jed_lib, jl);
   
   if (-1 == SLpath_set_load_path (jed_lib))
     exit_error ("Out of memory", 0);
}

   
void (*X_Init_Global_Structures_Hook)(void);

int Stdin_Is_TTY;

int _Jed_Startup_Argc;
char **_Jed_Startup_Argv;

static void script_usage (void)
{
   (void) fputs ("\
Usage: Running jed in script mode requires one of the following forms:\n\
  jed-script SCRIPT_FILE [args...]\n\
  jed --script SCRIPT_FILE [args...]\n\
\n\
If the script contains a public function called `jedscript_main`, then it\n\
will be called after the script has been loaded.  The value of __argv[0] will\n\
be the name of the script, and __argv[[1:]] will be set to the script\n\
arguments.\n\
", 
		 stderr);
}


static int main_initialize (int argc, char **argv)
{
   char *cmd_hook = ".()command_line_hook";
   int i, fd;
   char *script_file;
   int read_stdin_to_buffer = 0;

   _Jed_Startup_Argv = argv;
   _Jed_Startup_Argc = argc;

#ifdef __EMX__
   _response(&argc, &argv);
   _wildcard(&argc, &argv);
#endif

#ifdef WINDOWS
   if (NULL == getenv ("JED_ROOT"))
     {
	MessageBox(NULL,
		   "RTFM: You must set the JED_ROOT environment variable.\nAborting\n",
		   "Jed",
		   MB_ICONEXCLAMATION | MB_TASKMODAL | MB_SETFOREGROUND | MB_OK);
	exit(1);
     }
#endif
   
#if JED_HAS_DISPLAY_TABLE
   for (i = 0; i < 256; i++) Output_Display_Table[i] = i;
#endif

   if ((argc > 1) 
       && ((0 == strcmp (argv[1], "-secure"))
	   || (0 == strcmp (argv[1], "--secure"))))
     {
	Jed_Secure_Mode = 1;
	argc--;
	argv[1] = argv[0];
	argv++;
     }
   
   /* If this hook is defined, let it peel off what ever arguments
      it wants.  It should return the number of remaining arguments */
   if (X_Argc_Argv_Hook != NULL) 
     {
	i = (*X_Argc_Argv_Hook)(argc, argv) - 1;
	argv[i] = argv[0];
	argc -= i;
	argv += i;
     }

   /* If jed-script is a symbolic link to jed, then treat this as equivalent
    * to jed -script args...
    */
   script_file = NULL;
   if (0 == strcmp (extract_file (argv[0]), "jed-script"))
     {
	Batch = 2;
	SLang_Traceback = 1;
	Jed_Load_Quietly = 1;
	if (argc > 1)
	  script_file = argv[1];
	else
	  {
	     script_usage ();
	     return -1;
	  }
	argv++;			       /* make argv[0] the name of the script */
	argc--;
     }
   else if (argc > 1)
     {
	char *argv1 = argv[1];

	/* Allow, e.g., --batch */
	if ((argv1[0] == '-') && (argv1[1] == '-'))
	  argv1++;

	if (!strcmp(argv1, "-batch"))
	  {
	     Batch = 1;
	     SLang_Traceback = 1;
	  }
	else if (!strcmp(argv1, "-script"))
	  {
	     Batch = 2;
	     SLang_Traceback = 1;
	     Jed_Load_Quietly = 1;
	     if (argc > 2)
	       {
		  script_file = argv[2];
		  argv += 2;			       /* make argv[0] the name of the script */
		  argc -= 2;
	       }
	  }
	else if (!strcmp (argv1, "-help"))
	  {
	     Batch = 1;
	     SLang_Traceback = 1;
	     Jed_Load_Quietly = 1;
	  }
     }

   Jed_UTF8_Mode = SLutf8_enable (-1);
   if (Jed_UTF8_Mode == 0)
     {
	char *u = getenv ("JED_UTF8");
	if (u != NULL)
	  {
	     if ((0 == strcmp (u, "1")) || (0 == strcmp (u, "FORCE")))
	       {
		  Jed_UTF8_Mode = 1;		       /* force jed to use UTF-8 internally */
		  SLsmg_utf8_enable (1);
		  SLinterp_utf8_enable (1);
	       }
	     if (0 == strcmp (u, "FORCE"))
	       SLtt_utf8_enable (1);
	  }
     }

   if (Jed_UTF8_Mode)
     define_word ("\\w");
   else
     {
#ifdef __MSDOS__
	define_word("0-9a-zA-Z\200-\232\240-\245\341-\353");
#else
	define_word("0-9a-zA-Z\300-\326\330-\366\370-\377");
#endif
     }
   (*tt_get_terminfo) ();

   fd = 2;			       /* 2 is stderr, assume it is ok */

#if defined(REAL_UNIX_SYSTEM) && !defined(__APPLE__)
   if (Batch == 0)
     {
	int dev_tty_fd;

	if ((dev_tty_fd = open("/dev/tty", O_RDWR)) >= 0)
	  {
	     fd = dev_tty_fd;
	     while (-1 == dup2(fd, 2))  /* tty uses 2 as the descriptor */
	       {
# ifdef EINTR
		  if (errno == EINTR)
		    continue;
# endif		  
		  fd = 2;
		  break;
	       }
	     close (dev_tty_fd);
	  }
     }
#endif				       /* REAL_UNIX_SYSTEM */

#if defined(__WIN32__)
   if (-1 == jed_init_w32_support ())
     return -1;
#endif

   if (Stdin_Is_TTY == 0) Stdin_Is_TTY = isatty (0);
   if ((Stdin_Is_TTY == 0) && (Batch != 2))
     {
	read_stdin_to_buffer = 1;
#ifdef REAL_UNIX_SYSTEM
	/* Since stdin is not a terminal, jed is being used in a pipe such
	 * as `ls | jed`.  A problem occurs when when `ls` exits and
	 * the user suspends `jed`.  To deal with signals such as
	 * SIGINT generated at the controlling terminal, jed puts
	 * itself into its own process group. This way pressing ^G
	 * will cause SIGINT to get sent only to jed and not a
	 * program that spawned it (e.g., `git`).  However, to play
	 * nicely with the shell's job control, jed puts itself back
	 * into the original process group prior to suspending.
	 * However, that group will disappear if the other processes
	 * in the pipe have exited.  The only way I know how to deal
	 * with this is to fork another process.
	 * 
	 * Better ideas are welcome.
	 */
	(void) jed_fork_monitor (); 
#endif 
     }

   if (-1 == init_tty ())
     exit_error ("Unable to initialize tty.", 0);

   jed_init_display ();         /* sets up virtual screen */

   (*tt_set_mono) (JMESSAGE_COLOR, NULL, 0);
   (*tt_set_mono) (JERROR_COLOR, NULL, SLTT_BOLD_MASK);
   (*tt_set_mono) (JDOLLAR_COLOR, NULL, SLTT_BOLD_MASK);
   (*tt_set_mono) (JDOTS_COLOR, NULL, SLTT_BOLD_MASK);

   (*tt_set_mono) (JMENU_CHAR_COLOR, NULL, SLTT_BOLD_MASK);
   (*tt_set_mono) (JMENU_SELECTION_COLOR, NULL, SLTT_REV_MASK|SLTT_BOLD_MASK);
   (*tt_set_mono) (JMENU_POPUP_COLOR, NULL, 0);
   (*tt_set_mono) (JMENU_SELECTED_CHAR_COLOR, NULL, SLTT_BOLD_MASK);
 
   (void) jed_set_color (JMENU_SHADOW_COLOR, "blue", "black");

   (void) jed_set_color (JMENU_POPUP_COLOR, "lightgray", "blue");

   (*tt_set_mono) (JMENU_SHADOW_COLOR, NULL, 0);
   (void) jed_set_color (JMENU_SHADOW_COLOR, "blue", "black");

   init_syntax_tables ();

   /* This order here is crucial! */
   init_keymaps();

   if (NULL == (CBuf = make_buffer(NULL, NULL, NULL)))
     {
	exit_error("main: Allocation Failure", 0);
     }
   CLine = NULL;
   
   init_minibuffer();

   set_file_modes();
   
   /* what if someone pipes something to jed, allow it unless if is a
    * jed script.
    */
   
   if (read_stdin_to_buffer)
     /* 1 if stdin is a terminal, 0 otherwise */
     {
	set_buffer("*stdin*");
	read_file_pointer(fileno(stdin));
	bob();
	fclose(stdin);
	dup2(fd, 0);
     }
   
   if (CLine == NULL) make_line(25);
   bol ();
   
   window_buffer(CBuf);
#if !defined(IBMPC_SYSTEM) && !defined(VMS)
   init_signals();
#endif

   (void) SLang_set_argc_argv (argc, argv);

   if ((0 != jed_ns_load_file("site", NULL))
       && Batch)
     return -1;

   if (Batch) Ignore_User_Abort = 0;
   
   if (Batch == 2)
     {
	if (script_file == NULL)
	  {
	     script_usage ();
	     return -1;
	  }
	  {
	     char *file = SLpath_find_file_in_path (".", script_file);
	     if (file == NULL)
	       {
		  fprintf (stderr, "%s not found.\n", script_file);
		  return -1;
	       }
	     if (0 != SLns_load_file (file, NULL))
	       {
		  SLfree (file);
		  return -1;
	       }
	     SLfree (file);
	  }
	if (-1 == SLang_run_hooks ("jedscript_main", 0))
	  return -1;
     }
   else
     {     
	if (SLang_is_defined(cmd_hook + 3))
	  {
	     SLang_run_hooks(cmd_hook + 3, 0);

	     if (SLang_get_error () && Batch)
	       return -1;

	     /* This has the effect of removing the memory that cmd_hook
	      * is using.
	      */
	     SLang_load_string (cmd_hook);
	  }
	else if (!SLang_get_error () && (argc > 2))
	  {
	     find_file_in_window(argv[2]);
	  }
     }

   if (SLang_get_error () && Batch)
     return -1;

   /* after we have possible loaded key definitions, we can fix up
    * the minibuffer map. This way user definitions are used. 
    */
   jed_setup_minibuffer_keymap ();

   return 0;
}

int main(int argc, char **argv) /*{{{*/
{
   int err;

   if (SLang_Version < SLANG_VERSION)
     {
	fprintf (stderr, "***Warning: Executable compiled against S-Lang %d but linked to %d\n",
		 SLANG_VERSION, SLang_Version);
	fflush (stderr);
	sleep (2);
     }

   set_jed_root (argv[0]);

   if (argc > 1)
     {
	if (!strcmp (argv[1], "--version"))
	  {
	     jed_show_version (stdout);
	     fprintf (stdout, "\nUsing JED_ROOT=%s\n", Jed_Root_Dir);
	     exit (0);
	  }
     }

   Jump_Buffer_Ptr = &Jump_Buffer;
   /* incase something goes wrong before we even get started... */
   if (setjmp(Jump_Buffer_Ptr->b) != 0)
     {
	/* hmm.... just exit I guess */
	exit_error("main: Fatal Error", 0);
	exit (1);
     }
   
   if ((0 == main_initialize (argc, argv))
       && (Batch == 0))
     jed (); /* edit_loop -- never returns */
   
   jed_reset_display();
   reset_tty();

   if (0 != (err = SLang_get_error ()))
     SLang_restart (1);
   return err;
}


/*}}}*/
