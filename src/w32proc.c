/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#ifdef JED_HAS_SUBPROCESSES

# include <stdio.h>
# include <slang.h>

# include "jdmacros.h"

# include <stdlib.h>

# include "jprocess.h"
# include "buffer.h"
# include "ins.h"
# include "ledit.h"
# include "misc.h"
# include "paste.h"
# include "win32.h"

int Num_Subprocesses;
static CRITICAL_SECTION Critical_Section;

static int Is_GUI;

/* Note: The console version of jed uses Input_Events[0] for the 
 * handle to the keyboard.  So, we start numbering at 1.  This is ugly.
 */
HANDLE Input_Events[MAX_PROCESSES + 1];
static int Subprocess_Id[MAX_PROCESSES + 1];   /* maps input events to id */

volatile int Child_Status_Changed_Flag;/* if this is non-zero, editor
					* should call the appropriate
					* function below to call slang
					* handlers.
					*/

typedef struct
{
   int flags;			       /* This is zero if the process is gone
					* and the status is nolonger avail */
# define PROCESS_RUNNING		1
# define PROCESS_STOPPED		2
# define PROCESS_ALIVE		3
# define PROCESS_EXITED		4
# define PROCESS_SIGNALLED	8
   int return_status;		       /* This value depends on the flags */

   int status_changed;		       /* non-zero if status changed. */
   HANDLE rd, wd;		       /* read/write handles */
   HANDLE hprocess;		       /* real process handle */
   HANDLE hthread;
   
   int output_type;
# define PROCESS_USE_BUFFER	1
# define PROCESS_USE_SLANG	2
# define PROCESS_SAVE_POINT	4
# define PROCESS_AT_POINT	8

   int process_flags;
# define USE_CURRENT_BUFFER	0x1    /* use the current buffer instead of
					* the one associated with the process
					*/

   
   Buffer *buffer;		       /* buffer associated with process */
   SLang_Name_Type *slang_fun;	       /* function to pass output to */
   SLang_MMT_Type *umark;	       /* marks point of last output */

   SLang_Name_Type *status_change_fun; /* call this if process status changes
					* The function should be declared like
					* define fun (pid, flags);
					* The flags parameter corresponds to
					* the flags field in this struct and
					* the pid is NOT the pid of this struct
					*/
   HANDLE input_event;
   int quietly_kill_on_exit;
# define PROCESS_BUFSIZE 513
   unsigned char input_buf [PROCESS_BUFSIZE];
   int input_bufsize;
}
Process_Type;

static Process_Type Processes[MAX_PROCESSES];

static Process_Type *get_process (int fd)
{
   Process_Type *p;

   if ((fd >= 0) && (fd < MAX_PROCESSES)
       && (p = &Processes[fd], p->flags != 0)) return p;

   msg_error ("process does not exist.");
   return NULL;
}

static void call_slang_status_change_hook (Process_Type *p)
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
   if ((0 == (p->process_flags & USE_CURRENT_BUFFER))
       && (CBuf != cbuf))
     switch_to_buffer (cbuf);
   cbuf->locked--;
}
#if 0
static void close_rd_and_wd(Process_Type *p)
{
   char eof = '\x1a';		       /* ^Z */
   DWORD d;

   if (p->wd != INVALID_HANDLE_VALUE)
     {
	WriteFile(p->wd, &eof, 1, &d, NULL);
	CloseHandle(p->wd);
	p->wd = INVALID_HANDLE_VALUE;
     }

   if (p->rd != INVALID_HANDLE_VALUE)
     {
	CloseHandle(p->rd);
	p->rd = INVALID_HANDLE_VALUE;
     }
}
#endif
/* This routine is called to clean up after the process has exited.
 * After getting the exit status, we call a slang hook and if the
 * process is dead, adjust the process arrays to delete the process.
 */

static void get_process_status (Process_Type *p)
{
   int i;
   
   /* Call slang to let it know what happened.  Do it first before we
    * really shut it down to give the hook a chance to query the state of
    * it before it returns.
    */
   call_slang_status_change_hook (p);
   if (p->flags & PROCESS_ALIVE) return;

   /* Process is dead.  So perform clean up. */
   CloseHandle(p->input_event);
   p->input_event = INVALID_HANDLE_VALUE;

   if (p->buffer != NULL) p->buffer->subprocess = 0;

   if (p->umark != NULL) SLang_free_mmt (p->umark);

   /* Adjust the array of input events. Note that first handle
    * is used for read descriptor for console jed */
   i = 1;
   while (i <= Num_Subprocesses)
     {
	if (Input_Events[i] == p->input_event) break;
	i++;
     }

   Num_Subprocesses--;
   
   while (i <= Num_Subprocesses)
     {
	Input_Events[i] = Input_Events[i + 1];
	Subprocess_Id[i] = Subprocess_Id[i + 1];
	i++;
     }
   
   SLang_free_function (p->slang_fun);
   SLang_free_function (p->status_change_fun);
   memset((char *) p, 0, sizeof(Process_Type));
}

int jed_close_process (int *fd)
{
   Process_Type *p;

   if (NULL == (p = get_process (*fd))) return -1;

   EnterCriticalSection(&Critical_Section);
   TerminateProcess(p->hprocess, 0);
   if (p->buffer != NULL) p->buffer->subprocess = 0;
   LeaveCriticalSection(&Critical_Section);

   return 0;
}

void jed_kill_process (int fd)
{
   /* This function is called when the buffer is going to be destroyed */
   Processes[fd].buffer = NULL;
   jed_close_process (&fd);
}

void jed_get_child_status (void)
{
   Process_Type *p, *pmax;

   p = Processes; pmax = p + MAX_PROCESSES;

   while (p < pmax)
     {
	if (p->flags && p->status_changed)
	  {
	     EnterCriticalSection(&Critical_Section);
	     Child_Status_Changed_Flag--;
	     LeaveCriticalSection(&Critical_Section);
	     p->status_changed--;
	     get_process_status (p);
	  }
	p++;
     }
}

static DWORD thread_func (DWORD fd)
{
   char buf[PROCESS_BUFSIZE];
   DWORD n;
   int i;
   Process_Type *p = get_process(fd);

   if (p == NULL)
     return 0;

   while (1)
     {
	EnterCriticalSection(&Critical_Section);
	i = PROCESS_BUFSIZE - p->input_bufsize - 1;
	LeaveCriticalSection(&Critical_Section);
	
	/* Check if there is free space in process buffer */
	if (i > 0)
	  {
	     if (!ReadFile(p->rd, buf, i, &n, NULL))
	       {
		  EnterCriticalSection(&Critical_Section);
		  /* read handle is closed, so mark process as terminated */
		  CloseHandle (p->rd);
		  p->rd = INVALID_HANDLE_VALUE;
		  CloseHandle (p->wd);
		  p->wd = INVALID_HANDLE_VALUE;

		  p->flags = PROCESS_EXITED;
		  p->status_changed++;
		  SetEvent(p->input_event);
		  LeaveCriticalSection (&Critical_Section);
		  return 0;
	       }

	     if (n == 0)
	       continue;

	     EnterCriticalSection(&Critical_Section);
	     buf[n] = 0;
	     memcpy(p->input_buf + p->input_bufsize, buf, n + 1);
	     p->input_bufsize += n;
	     SetEvent(p->input_event);
	     LeaveCriticalSection(&Critical_Section);
	  }
     }
}


void read_process_input(int input_event_number)
{
   Buffer *b = CBuf, *pbuf;
   int otype, n, status;
   Process_Type *p;
   char buf[PROCESS_BUFSIZE];
   
   /* Should never happen */
   if (NULL == (p = get_process (Subprocess_Id[input_event_number])))
     return;
   
   EnterCriticalSection(&Critical_Section);
   ResetEvent(p->input_event);
   n = p->input_bufsize;
   memcpy(buf, p->input_buf, n + 1);
   p->input_bufsize = 0;
   *p->input_buf = 0;
   status = p->status_changed;
   LeaveCriticalSection(&Critical_Section);
   
   if (n > 0)
     {
	otype = p->output_type;
	pbuf = p->buffer;
	
	if (pbuf != NULL)
	  {
	     if (0 == (p->process_flags & USE_CURRENT_BUFFER))
	       switch_to_buffer (pbuf);
	     pbuf->locked++;
	  }
	
	if (otype & PROCESS_SAVE_POINT) push_spot ();
	
	if (otype & PROCESS_USE_BUFFER)
	  {
	     if (0 == (otype & PROCESS_AT_POINT)) eob ();
	     jed_insert_nbytes ((unsigned char *)buf, n);
	     jed_move_user_object_mark (p->umark);
	  }
	else if (otype == PROCESS_USE_SLANG)
	  {
	     SLang_push_integer ((int) (p - Processes));
	     SLang_push_string (buf);
	     SLexecute_function (p->slang_fun);    /* function to pass output to */
	  }
	
	if (otype & PROCESS_SAVE_POINT) pop_spot ();
	else if (otype & PROCESS_USE_BUFFER) move_window_marks (0);
	
	if (p->buffer != NULL)
	  {
	     if ((b != CBuf) && (0 == (p->process_flags & USE_CURRENT_BUFFER)))
	       switch_to_buffer (b);
	     pbuf->locked--;
	  }
     }
   
   if (status)
     {
	p->status_changed--;
	get_process_status (p);
     }
   touch_screen ();
}

static int make_handle_inheritable (HANDLE p, HANDLE *h)
{
   HANDLE x;

   if (FALSE == DuplicateHandle (p, *h, p, &x, 0, TRUE, DUPLICATE_SAME_ACCESS))
     return -1;
   
   CloseHandle (*h);
   *h = x;
   return 0;
}

   

static int open_process (char *cmd)
{
   int pd;
   Process_Type *p;
   HANDLE pid;
   SLang_MMT_Type *mmt;
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   DWORD id_thread;
   HANDLE h_stdin, h_stdout, h_stderr; /* For subprocess */
   HANDLE x_stdin, x_stdout; /* Our copies */

   pd = 0; while ((pd < MAX_PROCESSES) && Processes[pd].flags) pd++;
   if (pd == MAX_PROCESSES) return -1;
   p = &Processes[pd];

   SLMEMSET ((char *) p, 0, sizeof (Process_Type));
   
   /* Create stdin/out/err handles */
   pid = GetCurrentProcess ();

   if (FALSE == CreatePipe(&h_stdin, &x_stdin, NULL, 0))
     return -1;

   if (FALSE == CreatePipe(&x_stdout, &h_stdout, NULL, 0))
     {
	CloseHandle(h_stdin);
	CloseHandle(x_stdin);
	return -1;
     }

   if ((-1 == make_handle_inheritable (pid, &h_stdin))
       || (-1 == make_handle_inheritable (pid, &h_stdout))
       || (FALSE == DuplicateHandle (pid, h_stdout, pid, &h_stderr, 0, TRUE, DUPLICATE_SAME_ACCESS)))
     {	
	CloseHandle (h_stdin);
	CloseHandle (h_stdout);
	CloseHandle (x_stdin);
	CloseHandle (x_stdout);
	return -1;
     }
   
   if (NULL == (mmt = jed_make_user_object_mark ()))
     {
	CloseHandle (h_stderr);
	CloseHandle (h_stdin);
	CloseHandle (h_stdout);
	CloseHandle (x_stdin);
	CloseHandle (x_stdout);
	return -1;
     }

   si.cb = sizeof(STARTUPINFO);
   si.lpReserved = NULL;
   si.lpReserved2 = NULL;
   si.cbReserved2 = 0;
   si.lpDesktop = NULL;
   si.lpTitle = NULL;
   si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

   /* This is necessary to work around a windows 95 bug where HIDE has no
    * effect.
    */
   if (Is_GUI)
     si.wShowWindow = SW_MINIMIZE;
   else 
     si.wShowWindow = SW_HIDE;

   si.hStdInput = h_stdin;
   si.hStdOutput = h_stdout;
   si.hStdError = h_stderr;
   
   if (FALSE == CreateProcess(NULL, cmd, NULL, NULL, TRUE,
			      CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
     {
	jed_verror ("open_process: exec of %s failed.", cmd);

	CloseHandle (h_stderr);
	CloseHandle (h_stdin);
	CloseHandle (h_stdout);
	CloseHandle (x_stdin);
	CloseHandle (x_stdout);
	SLang_free_mmt(mmt);
	
	return -1;
     }
   
   CloseHandle(h_stdin);
   CloseHandle(h_stdout);
   CloseHandle(h_stderr);

   p->flags = PROCESS_RUNNING;
   p->rd = x_stdout;
   p->wd = x_stdin;

   p->hprocess = pi.hProcess;
	
   Num_Subprocesses += 1;
	
   CBuf->subprocess = pd + 1;
	
   /* Processing options */
   p->buffer = CBuf;
   p->output_type = PROCESS_USE_BUFFER;
   p->umark = mmt;
   p->input_bufsize = 0;
   SLang_inc_mmt (mmt);	       /* tell slang we are keeping a copy */
   
   /* If we wait on this event, it will block until set */
   if (NULL == (p->input_event = CreateEvent(NULL, TRUE, FALSE, NULL)))
     {
     }
   
   /* Note that Input_Events[0] is used by the console version.  So, make
    * sure that we start at 1.
    */
   Input_Events[Num_Subprocesses] = p->input_event;
   Subprocess_Id[Num_Subprocesses] = pd;

   p->hthread =
     CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thread_func, (LPVOID) pd, 0, &id_thread);
   
   return pd;
}

int jed_send_process (int *fd, char *str)
{
   DWORD d;
   
   Process_Type *p = get_process (*fd);
   if ((p == NULL) || (p->wd == INVALID_HANDLE_VALUE)) return -1;
   WriteFile(p->wd, str, strlen(str), &d, NULL);
   return 0;
}

void jed_send_process_eof (int *fd)
{
   Process_Type *p = get_process (*fd);
   if (p == NULL) return;
   
   if (p->wd != INVALID_HANDLE_VALUE)
     {
	CloseHandle(p->wd);
	p->wd = INVALID_HANDLE_VALUE;
     }
}

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

void jed_get_process_mark (int *fd)
{
   Process_Type *p;
   if (NULL == (p = get_process (*fd))) return;
   
   SLang_push_mmt (p->umark);
}

int jed_open_process (int *np)
{
   int fd = -1;
   char *argv[501];
   unsigned int num, i;
   char *cmd;

   if (CBuf->subprocess)
     {
	msg_error ("There is already a process attached to this buffer.");
	return -1;
     }

   num = (unsigned int) *np;
   if (num > 500)
     {
	msg_error ("open process: too many arguments");
	return -1;
     }

   memset ((char *) argv, 0, sizeof (argv));

   num += 1;			       /* allow for argv[0] */

   i = num;
   while (i--)
     {
	if (SLang_pop_slstring (&argv[i]))
	  goto free_return;
     }

   if (NULL == (cmd = w32_build_command (argv, num)))
     goto free_return;

   if ((fd = open_process(cmd)) < 0)
     msg_error ("Unable to open process.");

   SLfree (cmd);
   /* drop */

   free_return:
   for (i = 0; i < num; i++)
     {
	SLang_free_slstring (argv[i]); /* NULL ok */
	i++;
     }

   return fd;
}

int jed_signal_process (int *fd, int *sig)
{
   (void) fd;  (void) sig;
   return -1;
}

int jed_signal_fg_process (int *fd, int *sig)
{
   return jed_signal_process (fd, sig);
}

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

#if defined (__WIN32__) /* defined (__BORLANDC__) || defined (_MSC_VER) */
# undef popen
# undef pclose
# define popen w32_popen
# define pclose w32_pclose
#endif

FILE *jed_popen (char *cmd, char *type) /*{{{*/
{
   return popen(cmd, type);
}

/*}}}*/

int jed_pclose (FILE *fp)
{
   return pclose (fp);
}

int w32_init_subprocess_support (int is_gui)
{
   Is_GUI = is_gui;
   InitializeCriticalSection(&Critical_Section);
   return 0;
}

#endif

