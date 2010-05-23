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

#ifdef MSWINDOWS
# ifndef __WIN32__
#  include <toolhelp.h>
# else
#  include <windows.h>
# endif
#endif

#include <stdio.h>

#ifdef HAVE_SYS_WAIT_H
# include <sys/types.h>
# include <sys/wait.h>
#endif

#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#include <slang.h>
#include "jdmacros.h"

#include <string.h>

#ifdef IBMPC_SYSTEM
# include <fcntl.h>
#endif

#include <stdarg.h>

#include "buffer.h"
#include "keymap.h"
#include "file.h"
#include "ins.h"
#include "ledit.h"
#include "screen.h"
#include "window.h"
#include "display.h"
#include "search.h"
#include "misc.h"
#include "replace.h"
#include "paste.h"
#include "sysdep.h"
#include "cmds.h"
#include "text.h"
#include "hooks.h"
#include "undo.h"
#include "menu.h"
#include "userinfo.h"
#include "indent.h"

#ifdef __WIN32__
# include "win32.h"
#endif

#define JED_PROMPT "S-Lang>"

#if JED_HAS_SUBPROCESSES
# include "jprocess.h"
#endif


/*}}}*/

Buffer *MiniBuffer;
Buffer *The_MiniBuffer;     /* this never gets deleted since we may need it */

static Buffer *Last_Buffer;

/* extern char *get_cwd(void); */
int Ignore_Beep = 0;
int MiniBuffer_Active = 0;

/* Do not change this value without changing file_findfirst/next. */
#define JED_MAX_COMPLETION_LEN JED_MAX_PATH_LEN   

static int (*complete_open)(char *);
static int (*complete_next)(char *);
static int Buf_List_Len;
Buffer *Buf_Context;

static char *Expect_File_Error = "Expecting filename.";

static int next_bufflist(char *buf) /*{{{*/
{
   Buffer *tthis;
   while (1)
     {
	tthis = Buf_Context;
	if (tthis == MiniBuffer) return(0);
	Buf_Context = Buf_Context->next;

	if ((Buf_List_Len == 0)
#if JED_FILE_PRESERVE_CASE
	    || (0 == strncmp (buf, tthis->name, Buf_List_Len))
#else
	    || (0 == jed_case_strncmp (buf, tthis->name, Buf_List_Len))
#endif
	    )
	  {
	     if (*tthis->name == ' ') continue;   /* internal buffers */
	     safe_strcpy (buf, tthis->name, JED_MAX_COMPLETION_LEN);
	     return 1;
	  }
     }
}

/*}}}*/

static int open_bufflist(char *buf) /*{{{*/
{
   if ((Buf_Context = MiniBuffer) == NULL) return(0);
   Buf_Context = Buf_Context->next;
   Buf_List_Len = strlen(buf);
   return next_bufflist(buf);
}

/*}}}*/

char *what_buffer() /*{{{*/
{
   return (CBuf->name);
}

/*}}}*/

int bufferp(char *name) /*{{{*/
{
   if (NULL == find_buffer(name)) return(0); else return(1);
}

/*}}}*/

int insert_buffer_name(char *name) /*{{{*/
{
   Buffer *buf;
   
   if (NULL != (buf = find_buffer(name)))
     {
	insert_buffer(buf);
	return(1);
     }
   else msg_error("Unable to find buffer.");
   return(0);
}

/*}}}*/

static Buffer *find_scratch_buffer (void) /*{{{*/
{
   char *sname = "*scratch*";
   Buffer *scratch;
   
   scratch = find_buffer (sname);
   if (NULL == scratch)
     scratch = make_buffer (sname, CBuf->dir, NULL);
   return scratch;
}

/*}}}*/


/* Try to find any other buffer but kill_buf such that the buffer is not
 * visible, not scratch and not buried.
 */
static Buffer *find_non_visible_buffer (Buffer *kill_buf) /*{{{*/
{
   Buffer *buf, *scratch;

   buf = kill_buf->next;
   scratch = find_scratch_buffer ();
   
   while ((buf != kill_buf) &&
	  ((*buf->name == ' ') || (buf->flags & BURIED_BUFFER)
	   || (buf == scratch) || (buffer_visible(buf))))
     {
	buf = buf->next;
     }
   if ((buf == kill_buf) || (buf->flags & BURIED_BUFFER))
     buf = scratch;
   return buf;
}

/*}}}*/



int kill_buffer_cmd(char *name) /*{{{*/
{
   Buffer *this_buf, *kill_buf, *scratch, *buf;
   Window_Type *w;
   int do_kill;
   
   if (NULL == (kill_buf = find_buffer(name)))
     {
	msg_error("Buffer does not exist.");
	return(0);
     }

   /* Do not allow the minibuffer to be deleted.  Otherwise, the whole 
    * minibuffer creation/selection needs to be cleaned up.
    */
   if (kill_buf == The_MiniBuffer)
     return 0;

   this_buf = CBuf;
   switch_to_buffer(kill_buf);
   
#if JED_HAS_SUBPROCESSES
   if (kill_buf->locked)
     {
	erase_buffer ();
	switch_to_buffer (this_buf);
	return 0;
     }
#endif
   
   do_kill = 1;
   if ((*name != ' ') && (kill_buf->flags & BUFFER_MODIFIED))
     {
	jed_vmessage (1, "Buffer %s modified. (K)ill, (S)ave first, (A)bort:",
		      name);

	/* This does not go through keyboard macro routines
	 * on purpose! 
	 */
	switch (my_getkey())
	  {
	   case 'k': case 'K': do_kill = 1; break;
	   case 's': case 'S': do_kill = 2; break;
	   default: msg_error("Aborted."); return(0);
	  }
	clear_message ();
     }
   
   if (do_kill == 2)
     {
	jed_save_buffer_as_cmd ();
	if (SLang_get_error ())
	  {
	     switch_to_buffer(this_buf);
	     return(0);
	  }
     }
   
   /* if it is the scratch buffer, just erase it since we are going to 
      recreate it anyway. */
   
   scratch = find_scratch_buffer ();
   
   if (kill_buf == scratch)
     {
	erase_buffer();
#if JED_HAS_SUBPROCESSES
	if (kill_buf->subprocess) jed_kill_process (kill_buf->subprocess - 1);
	kill_buf->subprocess = 0;
#endif
	switch_to_buffer(this_buf);
	return 1;
     }

   buf = find_non_visible_buffer (kill_buf);

   /* search through windows looking for the buffer and replace it */
   w = JWindow;
   do
     {
	if (kill_buf == JWindow->buffer)
	  {
	     touch_window_hard (JWindow, 0);
	     window_buffer(buf); 
	     buf = find_non_visible_buffer (kill_buf);
	  }
	JWindow = JWindow->next;
     }
   while (w != JWindow);

   if (kill_buf == Last_Buffer) Last_Buffer = NULL;
   if (kill_buf == this_buf) this_buf = buf;
   switch_to_buffer(this_buf);
   delete_buffer(kill_buf);
   return 1;
}

/*}}}*/

int jed_save_buffer_to_file (char *dir, char *file)
{
   int status, state;
   char *dirfile, *canonical_file;

   if ((file == NULL) || (*file == 0))
     {
	file = CBuf->file;
	if (*file == 0)
	  {	     
	     msg_error ("Filename Required.");
	     return -1;
	  }
     }

   if (NULL == (dirfile = jed_dir_file_merge (dir, file)))
     return -1;

   state = 0;

   canonical_file = jed_get_canonical_pathname (dirfile);
   if (canonical_file == NULL)
     {
	SLfree (dirfile);
	return -1;
     }

   if (file_changed_on_disk (CBuf, dirfile) > 0)
     state |= BUFFER_MODIFIED;
   if (jed_file_is_readonly (canonical_file, 0))
     state |= READ_ONLY;
   if (0 != strcmp (canonical_file, CBuf->dirfile))
     state |= OVERWRITE_MODE;

   if (state)
     {
	char *prompt = NULL;

	switch (state)
	  {
	   default:
	   case OVERWRITE_MODE:
	     prompt = NULL;
	     break;

	   case BUFFER_MODIFIED:
	     prompt = "File changed on disk.  Save anyway?";
	     break;
	     
	   case READ_ONLY:
	     prompt = "No permission to write to file.  Try to save anyway?";
	     break;
	     
	   case BUFFER_MODIFIED|READ_ONLY:
	     prompt = "File changed on disk and no permission to write.  Save anyway?";
	     break;
	     
	   case BUFFER_MODIFIED|OVERWRITE_MODE:
	     prompt = "File exists and changed on disk.  Overwrite?";
	     break;
	     
	   case READ_ONLY|OVERWRITE_MODE:
	     prompt = "File exists and no permission to write.  Try to overwrite anyway?";
	     break;
	     
	   case READ_ONLY|OVERWRITE_MODE|BUFFER_MODIFIED:
	     prompt = "File changed on disk and no permission to write.  Overwrite anyway?";
	     break;
	  }
	
	if ((prompt != NULL) 
	    && (1 != jed_get_yes_no(prompt)))
	  {
	     SLfree (canonical_file);
	     SLfree (dirfile);
	     return -1;
	  }
     }

   SLfree (canonical_file);

   if (-1 == jed_va_run_hooks ("_jed_save_buffer_before_hooks",
			       JED_HOOKS_RUN_ALL, 1, dirfile))
     {
	auto_save_buffer (CBuf);
	SLfree (dirfile);
	return -1;
     }

   if (((status = write_file_with_backup (dirfile)) >= 0)
       && (Batch != 2))
     jed_vmessage (0, "Wrote %d lines to %s", status, dirfile);

   if (status < 0)
     {
	jed_verror ("Error writing file %s", dirfile);
	SLfree (dirfile);
	return -1;
     }

	
   CBuf->flags |= AUTO_SAVE_BUFFER;
   CBuf->hits = 0;

#ifdef UNDO_HAS_REDO
   update_undo_unchanged ();
#endif
   visit_file (dir, file);

   if (-1 == jed_va_run_hooks ("_jed_save_buffer_after_hooks", JED_HOOKS_RUN_ALL,
			       1, dirfile))
     {
	SLfree (dirfile);
	return -1;
     }
   
   SLfree (dirfile);
   return 0;
}


int jed_save_buffer_as (char *filestr)
{
   char *dir, *file;
   int status;

   filestr = jed_standardize_filename (filestr);
   
   if (-1 == jed_dirfile_to_dir_file (filestr, &dir, &file))
     {
	SLfree (filestr);
	return -1;
     }

   status = jed_save_buffer_to_file (dir, file);
   
   SLfree (filestr);
   SLfree (dir);
   SLfree (file);

   if (status == -1)
     return status;

   /* This function is an intrinsic and is documented to return the 
    * number of lines written out.
    * 
    */
   return 0;
}

int jed_save_buffer_cmd (void)
{
   if ((CBuf->file == NULL) || (CBuf->file[0] == 0))
     return jed_save_buffer_as_cmd ();
   
   return jed_save_buffer_to_file (CBuf->dir, CBuf->file);
}

#if defined(__BORLANDC__) && !defined(MSWINDOWS)
int show_memory() /*{{{*/
{
   struct farheapinfo hi;
   char *c;
   unsigned long total = 0, core, used = 0;
   unsigned long max = 0;
   int i = 0;
   
   hi.ptr = NULL;
   if (farheapcheck() == _HEAPCORRUPT) c = "corrupt"; else c = "ok";
   while (farheapwalk(&hi) == _HEAPOK)
     {
	if (hi.in_use)
	  used += hi.size;
	else
	  {
	     total += hi.size;
	     if (hi.size > max) max = hi.size;
	     i++;
	  }
     }
   core = farcoreleft();
   jed_vmessage (0, "used:%lu, core:%lu, free:%lu, grand:%lu, max:%lu, frag:%d (%s)",
		 used, core, total, core + total, max, i, c);
   return 0;
}

/*}}}*/

#endif

#ifdef MSWINDOWS
int show_memory (void) /*{{{*/
{
#ifndef __WIN32__
   MEMMANINFO mmi;
   if (MemManInfo(&mmi))
     {
 	jed_vmessage (0, "tot pages: %lu, free pages: %lu",
		      mmi.dwTotalLinearSpace, mmi.dwFreeLinearSpace);
     }
#else
   MEMORYSTATUS mst;
   GlobalMemoryStatus(&mst);
   jed_vmessage (0, "avail space: %lu, tot phys space: %lu, avail phys space: %lu",
		 mst.dwAvailPhys + mst.dwAvailPageFile, mst.dwTotalPhys, mst.dwAvailPhys);
#endif
   
   return 0;
}

/*}}}*/

#endif

#ifdef __WATCOMC__
#include <malloc.h>
int show_memory() /*{{{*/
{
   jed_vmessage (0, "avail mem: %lu, tot phys pgs: ???, free lin space: %lu",
		 _memavl (), _memmax ());
   return 0;
}

/*}}}*/

#endif

#ifdef __GO32__
#include <dpmi.h>

int show_memory() /*{{{*/
{
   unsigned long mem;
   _go32_dpmi_meminfo info;
   
   _go32_dpmi_get_free_memory_information(&info);
   
   if ((long)info.available_physical_pages != -1L)
     mem = info.available_physical_pages * 4096UL;
   else mem = info.available_memory;

   jed_vmessage (0, "avail mem: %lu, tot phys pgs: %lu, free lin space: %lu",
		 mem, info.total_physical_pages, info.free_linear_space);
   return 0;
}

/*}}}*/

#endif

int set_buffer(char *name) /*{{{*/
{
   Buffer *buf;
   
   if ((name == NULL) || (*name == 0))
     {
	msg_error("set_buffer: Buffer name is NULL");
	return (0);
     }
   
    /* Last_Buffer = CBuf; */
   
   if (NULL == (buf = find_buffer(name)))
     buf = make_buffer (name, CBuf->dir, NULL);
   switch_to_buffer(buf);
   
   return 1;
}

/*}}}*/

int jed_get_mini_response (char *s)
{
   int ch;
   int len;

   if (Batch)
     {
	if (EOF == fputs (s, stdout))
	  exit_error ("Failed to write to stdout", 0);
	
	return jed_getkey ();
     }
	    
   len = strlen (s);
   if (len > Jed_Num_Screen_Cols)
     s += (len - Jed_Num_Screen_Cols);

   MiniBuf_Get_Response_String = s;
   update (NULL, 0, 0, 0);
   ch = jed_getkey ();
   MiniBuf_Get_Response_String = NULL;
   return ch;
}

static char *strcat_malloc (char *a, char *b)
{
   unsigned int len;
   char *c;

   len = strlen (a);

   if (NULL == (c = SLmalloc (len + strlen (b) + 1)))
     return NULL;

   strcpy (c, a);
   strcpy (c + len, b);
   return c;
}


int jed_get_y_n (char *question)
{
   int ans;
   char *yn_quest;

   if (NULL == (yn_quest = strcat_malloc (question, "? (y/n)")))
     return -1;

   ans = jed_get_mini_response (yn_quest);
   SLfree (yn_quest);

   if ((ans == 'y') || (ans == 'Y'))
     return 1;
   if ((ans == 'n') || (ans == 'N'))
     return 0;
   if (SLKeyBoard_Quit || (SLang_get_error () == SL_USER_BREAK))
     return -1;
   jed_beep ();
   flush_input ();
   return jed_get_yes_no (question);
}


int jed_vget_y_n (char *fmt, char *arg)
{
   char msg [1024];

   if (arg == NULL) 
     {
	arg = fmt;
	fmt = "%s";
     }
   
   SLsnprintf (msg, sizeof (msg), fmt, arg);
   
   return jed_get_y_n (msg);
}


int jed_get_yes_no (char *question) /*{{{*/
{
   char *yn_quest;
   char *tmp;
   int n;
   
   if (NULL == (yn_quest = strcat_malloc (question, "? (yes or no)")))
     return -1;

   while (1)
     {
	if (NULL == (tmp = read_from_minibuffer(yn_quest, 0, NULL, &n))) 
	  {
	     SLfree (yn_quest);
	     return -1;
	  }

	if (!strcmp(tmp, "yes"))
	  {
	     SLfree(tmp);
	     SLfree (yn_quest);
	     return 1;
	  }
	
	if (!strcmp(tmp, "no"))
	  {
	     SLfree(tmp);
	     SLfree (yn_quest);
	     return 0;
	  }
	msg_error("Answer `yes' or `no'.");
	SLfree(tmp);
     }
}

/*}}}*/

int find_file_cmd (char *filestr)       /* filestr is const ! */ /*{{{*/
{
   Buffer *buf;
   int status;
   char *dirfile, *dir, *file;
   char *name;

#ifdef REAL_UNIX_SYSTEM
   char *dir1;
 
   /* Handle the case where /foo/bar/../baz may not be the same as /foo/baz */
   if (-1 == jed_dirfile_to_dir_file (filestr, &dir, &file))
     return -1;
   
   if (NULL == (dir1 = jed_expand_link (dir)))
     {
	SLfree (dir);
	SLfree (file);
	return -1;
     }
   SLfree (dir);
   filestr = jed_dir_file_merge (dir1, file);
   SLfree (dir1);
   SLfree (file);
#else
   filestr = jed_standardize_filename (filestr);
#endif
   if (filestr == NULL)
     return -1;

   /* If this hook returns 1, then the file is considered to be found. */
   if (1 == jed_va_run_hooks ("_jed_find_file_before_hooks", JED_HOOKS_RUN_UNTIL_NON_0,
			      1, filestr))
     {
	SLfree (filestr);
	return 0;
     }

   dirfile = jed_expand_link(filestr);

   if (dirfile == NULL)
     {
	SLfree (filestr);
	return -1;
     }

   if (-1 == jed_dirfile_to_dir_file (filestr, &dir, &file))
     {
	SLfree (filestr);
	SLfree (dirfile);
	return -1;
     }

   status = -1;
   
   if (*file == 0)
     {
	jed_verror (Expect_File_Error);
	goto free_and_return;
     }

   check_buffers ();
   
   /* search for the file in current buffer list */
   
   if (NULL != (buf = find_file_buffer (dirfile)))
     {
	if ((file_changed_on_disk (buf, dirfile) > 0)
	    && (1 == jed_get_yes_no("File changed on disk.  Read from disk")))
	  {
	     int n = (int) buf->linenum;
	     mark_buffer_modified (buf, 0, 1);
	     kill_buffer_cmd (buf->name);
	     status = find_file_cmd (filestr);
	     goto_line (&n);
	     goto free_and_return;
	  }

	if (SLang_get_error ())
	  goto free_and_return;

	status = 1;
	switch_to_buffer (buf);
	goto free_and_return;
     }

   /* Make a buffer but do not attach it to a file yet. */
   name = extract_file (filestr);

   buf = make_buffer (name, dir, NULL);
   switch_to_buffer(buf);

   status = read_file (dirfile);
	
   switch (status)
     {
      case -2:
	jed_verror ("File %s not readable.", dirfile);
	status = -1;
	break;
	     
      case -1:
	if (Batch == 0)
	  message ("New file.");
	status = 0;
	break;
	     
      default:
	if (Batch == 0)
	  jed_vmessage (0, "%d lines read", status);
	status = 1;
     }

   if (buf == CBuf)		       /* check this in case the hook switches buffers */
     {
	mark_buffer_modified (CBuf, 0, 0);

	/* Now attach it to a file */
	visit_file (dir, file);
	uniquely_name_buffer (CBuf, name);

	(void) bob ();
	set_file_modes();
	CBuf->flags |= UNDO_ENABLED;
     }

   if (-1 == jed_va_run_hooks ("_jed_find_file_after_hooks", JED_HOOKS_RUN_ALL, 0))
     status = -1;

   free_and_return:

   SLfree (dirfile);
   SLfree (filestr);
   SLfree (dir);
   SLfree (file);

   return status;
}

/*}}}*/


int find_file_in_window(char *file) /*{{{*/
{
   int ret;
   Buffer *b = CBuf;
   
   ret = find_file_cmd (file);
   if ((b != CBuf) && (*CBuf->name != ' ')) Last_Buffer = CBuf;
   window_buffer(CBuf);
   return(ret);
}

/*}}}*/


/* create a minibuffer with window and switch to it. */
static void create_minibuffer(void) /*{{{*/
{
   Window_Type *w;
   MiniBuffer = The_MiniBuffer;
   
   /* We may get called before the keymap has been set up.  Make sure that
    * we have a keymap.
    */
   if ((The_MiniBuffer->keymap == NULL)
       || (The_MiniBuffer->keymap == Global_Map))
     jed_setup_minibuffer_keymap ();

   /* I want to make Mini_Window->next = Current Window so that scroll other
      window routines will scroll it. */
   
   w = JWindow;
   do other_window(); while (JWindow->next != w);
   JWindow->next = JMiniWindow;
   JMiniWindow->next = w;
   JMiniWindow->hscroll_column = 1;
   Mini_Info.action_window = w;
   other_window();    /* now in minibuffer window */
   window_buffer(MiniBuffer);
   switch_to_buffer(MiniBuffer);
   MiniBuffer_Active = 1;
   erase_buffer ();

#if JED_HAS_MENUS
   jed_notify_menu_buffer_changed ();
#endif

   /* allow kill region to kill to beginning of minibuffer. 
    * Suggested by stefen@uni-paderborn.de */
   jed_push_mark ();
}

/*}}}*/

char *Completion_Buffer = "*Completions*";

static char *Last_Completion_Buffer;
static int Last_Completion_Windows;

/* evaluate command in minibuffer and leave */
int exit_minibuffer() /*{{{*/
{
   if (IN_MINI_WINDOW)
     {
	if (Last_Completion_Buffer != NULL)
	  {
	     pop_to_buffer (Completion_Buffer);
	     mark_buffer_modified (CBuf, 0, 0);
	     switch_to_buffer_cmd (Last_Completion_Buffer);
	     kill_buffer_cmd (Completion_Buffer);
	     touch_window_hard (JWindow, 0);
	     if (Last_Completion_Windows == 1) one_window ();
	  }
        select_minibuffer ();
	Exit_From_MiniBuffer = 1;
     }
   Last_Completion_Buffer = NULL;
   return(0);
}

/*}}}*/

/* return 1 if minibuffer already exists otherwise returns 0 */
int select_minibuffer (void) /*{{{*/
{
   Window_Type *w;
   
    /* Try to find active minibuffer and go there */
   w = JWindow;
   while (MiniBuffer != NULL)
     {
	if (IN_MINI_WINDOW) return(1);
	other_window();
	if (w == JWindow) exit_error("Internal Error:  no window!", 1);
     }
   
    /* switchs to minibuffer too */
   create_minibuffer();
   return(0);
}

/*}}}*/

/* if cmd != NULL, insert it into minibuffer and then send the result to
   the appropriate routine. */

static int ledit(void) /*{{{*/
{
   int n;
   char *tmp;
   
   if (MiniBuffer == NULL) complete_open = NULL;
   if (NULL == (tmp = read_from_minibuffer(JED_PROMPT, 0, NULL, &n))) return(0);

   /* SLang_Error = 0; ??? Why?  0.99-16 and earlier */
   Suspend_Screen_Update = 1;
   
   SLang_load_string(tmp);
   SLfree(tmp);
   
   if (SLang_get_error () == SL_USER_BREAK)
     msg_error("Quit!");

   /* SLang_set_error (0); */
   
   return(1);
}

/*}}}*/

static int File_Expansion_Happened = 0;
static int file_findfirst (char *buf)
{
   char *file;

   File_Expansion_Happened = 1;
   if (NULL == (file = jed_expand_filename (buf)))
     return -1;
   safe_strcpy (buf, file, JED_MAX_COMPLETION_LEN);
   SLfree (file);
   return sys_findfirst (buf);
}

static int file_findnext (char *file)
{
   return sys_findnext (file);
}


static char *read_file_from_minibuffer(char *prompt, char *def, char *stuff) /*{{{*/
{
   int n;
   char *file, *file1;

   File_Expansion_Happened = 0;
   complete_open = file_findfirst;
   complete_next = file_findnext;
   
   if ((stuff == NULL) || (*stuff == 0))
     {
	if (*CBuf->dir == 0)
	  buffer_filename (CBuf, NULL, CBuf->file);
	stuff = CBuf->dir;
     }
   
   if (NULL == (file = read_from_minibuffer (prompt, def, stuff, &n)))
     return NULL;

   if (File_Expansion_Happened)
     file1 = jed_standardize_filename (file);
   else
     file1 = jed_expand_filename (file);

   SLfree (file);
   return file1;
}

/*}}}*/

static char *String_Completion_Str;
static char *String_Completion_Str_Next;
static int String_Completion_Str_Len;


static int next_string_list (char *buf) /*{{{*/
{
   register char *s = String_Completion_Str_Next;
   int len;
   while (*s)
     {
	while (*s && (*s != ',')) s++;
	len = (int) (s - String_Completion_Str_Next);
	
	if (*s == ',') s++;
	
	if (!len
	    || strncmp (buf, String_Completion_Str_Next, String_Completion_Str_Len))
	  {
	     String_Completion_Str_Next = s;
	     continue;
	  }
	if (len >= JED_MAX_PATH_LEN) len = JED_MAX_PATH_LEN - 1;
	strncpy (buf, String_Completion_Str_Next, len);
	buf[len] = 0;
	String_Completion_Str_Next = s;
	return 1;
     }
   String_Completion_Str_Next = s;
   return 0;
}

/*}}}*/

static int open_string_list (char *buf) /*{{{*/
{
   String_Completion_Str_Next = String_Completion_Str;
   String_Completion_Str_Len = strlen (buf);
   return next_string_list (buf);
}

/*}}}*/


void read_object_with_completion(char *prompt, char *dflt, char *stuff, int *typep) /*{{{*/
{
   int type = *typep, n;
   char buf[JED_MAX_COMPLETION_LEN], *tmp;
   char *str = NULL;

   *buf = 0;
   if (type == 'f')		       /* file */
     {
	char *file = read_file_from_minibuffer (prompt, dflt, stuff);
	if (file != NULL)
	  (void) SLang_push_malloced_string (file);
	return;
     }
   else if (type == 'b')	       /* buffer */
     {
	complete_open = open_bufflist;
	complete_next = next_bufflist;
     }
   else if (type == 'F')	       /* function */
     {
	complete_open = open_function_list;
	complete_next = next_function_list;
     }
   else if (type == 's')
     {
	complete_open = open_string_list;
	complete_next = next_string_list;
	if (SLpop_string (&str)) return;
	String_Completion_Str = str;
     }
   else
     {
	complete_open = NULL;
     }
   
   safe_strcat (buf, stuff, sizeof (buf));
   
   if (NULL != (tmp = read_from_minibuffer(prompt, dflt, buf, &n)))
     {
	(void) SLang_push_string(tmp);
	
	SLfree(tmp);
	if (str != NULL) SLfree (str);
     }
}

/*}}}*/

int insert_file_cmd (void) /*{{{*/
{
   char *f, *file;
   
   CHECK_READ_ONLY
     
   if (NULL == (file = read_file_from_minibuffer("Insert file:", NULL, NULL)))
     return -1;

   f = extract_file(file);
   if ((*file == 0) || (*f == 0))
     {
	SLfree (file);
	msg_error(Expect_File_Error);
	return(1);
     }
   
   if (insert_file(file) < 0) msg_error("Error inserting file.");
   SLfree (file);
   return(1);
}

/*}}}*/

int find_file (void) /*{{{*/
{
   char *file, *f;
   int status;

   if (NULL == (file = read_file_from_minibuffer("Find file:", NULL, NULL)))
     return 0;

   f = extract_file (file);
   if (*f == 0)
     {
	char *dirfile = jed_dir_file_merge (file, CBuf->file);
	SLfree (file);
	if (dirfile == NULL)
	  return -1;
	file = dirfile;
     }

   status = find_file_in_window (file);
   SLfree (file);
   return status;
}

/*}}}*/

int jed_save_buffer_as_cmd (void) /*{{{*/
{
   char *tmp;
   
   if (NULL == (tmp = read_file_from_minibuffer("Save as:", NULL, NULL))) return(0);
   (void) jed_save_buffer_as (tmp);
   SLfree(tmp);
   return(1);
}

/*}}}*/

void switch_to_buffer_cmd (char *name) /*{{{*/
{
   Buffer *tthis = CBuf;
   
   set_buffer(name);
   window_buffer(CBuf);
   if ((CBuf != tthis) && (*CBuf->name != ' ')) Last_Buffer = tthis;
}

/*}}}*/

static void get_last_buffer(void) /*{{{*/
{
   if ((Last_Buffer == CBuf) || (Last_Buffer == NULL)
       || (*Last_Buffer->name == ' ')
       || (Last_Buffer->flags & BURIED_BUFFER))
     {
	Last_Buffer = find_non_visible_buffer (CBuf);
     }
}

/*}}}*/

int get_buffer() /*{{{*/
{
   char *tmp;
   int n;
   complete_open = open_bufflist;
   complete_next = next_bufflist;
   
   get_last_buffer ();
   
   if (NULL == (tmp = read_from_minibuffer("Switch to buffer:", Last_Buffer->name, NULL, &n))) return(0);
   switch_to_buffer_cmd(tmp);
   SLfree(tmp);
   return(1);
}

/*}}}*/

int kill_buffer (void) /*{{{*/
{
   char *tmp;
   int n;
   
   complete_open = open_bufflist;
   complete_next = next_bufflist;
   tmp = read_from_minibuffer("Kill buffer:", (char *) CBuf->name, NULL, &n);
   if (tmp != NULL)
     {
#if JED_HAS_SUBPROCESSES
	Buffer *b = find_buffer(tmp);
	if ((b != NULL) && (b->subprocess))
	  {
	     if (0 == jed_get_yes_no("Buffer has a subprocess attached.  Delete anyway"))
	       {
		  SLfree(tmp);
		  return 0;
	       }
	  }
#endif
	kill_buffer_cmd(tmp);
	SLfree(tmp);
	return(1);
     }
   return 0;
}

/*}}}*/

int evaluate_cmd() /*{{{*/
{
   return(!ledit());
}

/*}}}*/


char *pop_to_buffer(char *name) /*{{{*/
{
   Window_Type *w, *action, *use_this;
   char *bname;
   Line *line, *oldline;
   int p, oldp, lnum, oldlnum;
   Buffer *b, *oldb;
   
   if (!strcmp(name, " <mini>"))
     {
	select_minibuffer ();
	return CBuf->name;
     }
   
   /* save position so we can pop back to it if buffer already exists in 
      window */
   oldb = CBuf; oldline = CLine; oldp = Point; oldlnum = LineNum;
   
   set_buffer(name);
   line = CLine; p = Point; lnum = LineNum;
   
   use_this = NULL;
   if (MiniBuffer != NULL)
     {
	action = Mini_Info.action_window;
     }
   else action = NULL;

   if (Batch) return CBuf->name;

   w = JWindow;
   /* find a window to use */
   do
     {
	if (0 == (w->flags & MINIBUFFER_WINDOW))
	  {
	     if (action != NULL)
	       {
		  if (w != action) use_this = w;
	       }
	     else if (w != JWindow) use_this = w;
	     
	     if (w->buffer == CBuf)
	       {
		  use_this = w;
		  break;
	       }
	  }
	w = w->next;
     }
   while (w != JWindow);
   
   b = CBuf;
   if (use_this != NULL)
     {
	while(JWindow != use_this) other_window();
	/* This is a good reason for haveing otherwindow avoid switching buffers */
	if (CBuf == oldb)
	  {
	     CLine = oldline; Point = oldp; LineNum = oldlnum;
	  }
     }
   else
     {
	if (action != NULL) while(JWindow != action) other_window();
	split_window();
	/* 
	 * doing it this way makes screen update look better
	 */
	w = JWindow;
	do
	  {
	     other_window();
	  }
	while (JWindow->buffer != w->buffer);
	JWindow->hscroll_column = 1;
     }
   
   bname = CBuf->name;
   switch_to_buffer(b);
   b->line = CLine = line;
   b->point = Point = p;
   b->linenum = LineNum = lnum;
   if (b != JWindow->buffer) window_buffer(b);
   return bname;
}

/*}}}*/

#if defined (REAL_UNIX_SYSTEM) || defined(__WIN32__) || (defined (__os2__) && !defined(__WATCOMC__))

# if defined (__WIN32__) /* defined (__BORLANDC__) || defined (_MSC_VER) */
#  undef popen
#  undef pclose
#  define popen w32_popen
#  define pclose w32_pclose
# endif

# ifdef	__IBMC__
extern FILE *popen(char *, char *);
extern int pclose(FILE *);
# endif

# if !JED_HAS_SUBPROCESSES
#  define jed_popen popen
#  define jed_pclose pclose
# endif

static char *Process_Error = "Unable to open process.";
int shell_command(char *cmd) /*{{{*/
{
   FILE *pp;
   VFILE *vp;
   int status;
   
   if (Jed_Secure_Mode)
     {
	msg_error ("Access denied.");
	return -1;
     }
   
   if (NULL == (pp = jed_popen(cmd, "r")))
     {
	msg_error(Process_Error);
	return -1;
     }
   
   if (NULL != (vp = vstream(fileno(pp), 0, VFILE_TEXT)))
     {
	(void) insert_file_pointer(vp);
	SLfree(vp->buf);
	SLfree ((char *)vp);
     }
   else	msg_error("Malloc Error.");
   
   status = jed_pclose (pp);
   
#if defined(WIFEXITED) && defined(WEXITSTATUS)
   if ((status != -1) && WIFEXITED(status))
     {
	status = WEXITSTATUS(status);
     }
#endif
   return status;
}

/*}}}*/
int pipe_region(char *cmd) /*{{{*/
{
   FILE *pp;
   int n;
   
   if (Jed_Secure_Mode)
     {
	msg_error ("Access denied.");
	return -1;
     }
   
   if (NULL == (pp = jed_popen (cmd, "w")))
     {
	msg_error(Process_Error);
	return(-1);
     }
   
   n = write_region_to_fp(fileno(pp));
   if (n == -1)
     msg_error ("pipe_region: write failed");
   
   return jed_pclose (pp);
}

/*}}}*/
#endif

/* 
 * Try to open a .slc then a .sl
 */
#ifdef SIXTEEN_BIT_SYSTEM
#define VFILE_BUF_SIZE 1024
#else
#define VFILE_BUF_SIZE 4096
#endif


static VFILE *jed_open_lib_file (SLFUTURE_CONST char *file, char **dirfile) /*{{{*/
{
   char libfsl[JED_MAX_PATH_LEN], libfslc[JED_MAX_PATH_LEN];
   SLFUTURE_CONST char *lib, *type, *libf;
   unsigned int n;
   VFILE *vp = NULL;
   int free_lib;
   int delimiter;

   libf = file;
   lib = NULL;
   free_lib = 0;

   /* If file begins with ./,then read it from the current directory */
   if ((0 == strncmp (file, "./", 2))
#ifdef IBMPC_SYSTEM
       || (0 == strncmp (file, ".\\", 2))
       || (0 == strncmp (file, "..\\", 3))
#endif
       || (0 == strncmp (file, "../", 3)))
     {
	if (NULL == (lib = jed_get_cwd ()))
	  lib = "";
     }
   else if (SLpath_is_absolute_path (file))
     lib = "";
   else
     {
	lib = SLpath_get_load_path ();      /* SLstring */
	free_lib = 1;
	if ((lib == NULL) || (*lib == 0))
	  exit_error("The JED_ROOT environment variable needs set.", 0);
     }

   if ((NULL != (type = file_type(file)))
       && ((*type == 0) 
	   || ((0 != strcmp (type, "sl")) && (0 != strcmp (type, "slc")))))
     type = NULL;

   delimiter = SLpath_get_delimiter ();
   n = 0;
   while (0 == SLextract_list_element (lib, n, delimiter, libfsl, sizeof(libfsl)))
     {
	n++;

	fixup_dir(libfsl);
	safe_strcat (libfsl, file, sizeof (libfsl));
	safe_strcpy (libfsl, jed_standardize_filename_static(libfsl), sizeof (libfsl));

	libf = libfsl;

	if ((1 == file_status (libf))
	    && (NULL != (vp = vopen(libf, VFILE_BUF_SIZE, VFILE_TEXT))))
	  break;

	if (type == NULL)
	  {
#ifdef VMS
	     int vmsn;
	     /* remove trailing '.' */
	     if (0 != (vmsn = strlen(libfsl)))
	       {
		  vmsn--;
		  if (libfsl[vmsn] == '.') libfsl[vmsn] = 0;
	       }
#endif
	     
	     /* Try .sl and .slc */
	     safe_strcat (libfsl, ".sl", sizeof (libfsl));
	     safe_strcpy (libfslc, libfsl, sizeof (libfslc));
	     safe_strcat (libfslc, "c", sizeof (libfslc));

	     if (file_time_cmp(libfslc, libfsl) >= 0)
	       libf = libfslc;

	     if ((1 == file_status (libf))
		 && (NULL != (vp = vopen(libf, VFILE_BUF_SIZE, VFILE_TEXT))))
	       break;
	  }
     }
   
   if (free_lib)
     SLang_free_slstring ((char *) lib);

   if (vp == NULL)
     {
	*dirfile = NULL;
	return NULL;
     }

   if (NULL == (*dirfile = SLang_create_slstring (libf)))
     {
	vclose (vp);
	return NULL;
     }

   if (0 == Jed_Load_Quietly)
     jed_vmessage (1, "loading %s", libf);

   return (vp);
}

/*}}}*/

static char *jed_read_from_file(SLang_Load_Type *x) /*{{{*/
{
   char *s;
   unsigned int n;

   if ((s = vgets((VFILE *) x->client_data, &n)) != NULL)
     {
	if (s[n - 1] != '\n') s[n] = 0;
     }
   
   return s;
}

/*}}}*/

int jed_ns_load_file (SLFUTURE_CONST char *file, SLFUTURE_CONST char *ns)
{
   VFILE *vp;
   SLang_Load_Type *x;
   int ret;
   char *dirfile;

   if (NULL == (vp = jed_open_lib_file (file, &dirfile)))
     {
	SLang_verror (SL_OBJ_NOPEN, "Unable to open %s.  Check the value of the S-Lang load path.", file);
	return -1;
     }

   x = SLns_allocate_load_type (dirfile, ns);
   SLang_free_slstring (dirfile);
   
   if (x == NULL)
     {
	vclose (vp);
	return -1;
     }

   x->client_data = (VOID_STAR) vp;
   x->read = jed_read_from_file;
   ret = SLang_load_object (x);
   SLdeallocate_load_type (x);
   vclose (vp);
   return ret;
}

typedef struct
{
   Line *l;
   char buf[256];
}
Buffer_Client_Type;

static char *jed_read_from_buffer (SLang_Load_Type *x) /*{{{*/
{
   Buffer_Client_Type *b;
   char *buf;
   Line *l;
   unsigned int len;

   b = (Buffer_Client_Type *)x->client_data;

   if (NULL == (l = b->l))
     return NULL;

   len = (unsigned int) l->len;
   if (len > 255)
     {
	SLang_verror (SL_BUILTIN_LIMIT_EXCEEDED, "Line too long");
	return NULL;
     }

   buf = b->buf;
   SLMEMCPY(buf, (char *) l->data, len);
   buf [len] = 0;
   b->l = l->next;

   return buf;
}
/*}}}*/

void jed_load_buffer (char *ns) /*{{{*/
{
   SLang_Load_Type *x;
   Buffer_Client_Type client_data;
   Buffer *cbuf = CBuf;
   int flags = CBuf->flags;
   Line *l, *lwant;
   char *file;

   if (cbuf->file[0] == 0)
     file = cbuf->name;
   else
     file = cbuf->file;
   
   file = jed_dir_file_merge (CBuf->dir, file);
   if (file == NULL)
     return;
	
   x = SLns_allocate_load_type (file, ns);
   SLfree (file);
   
   if (x == NULL)
     return;

   x->read = jed_read_from_buffer;
   x->client_data = (VOID_STAR) &client_data;
   client_data.l = CBuf->beg;

   cbuf->flags |= READ_ONLY;
   SLang_load_object(x);
   SLdeallocate_load_type (x);
   
   if (buffer_exists (cbuf))
     cbuf->flags = flags;
   else cbuf = NULL;

   if (SLang_get_error () == 0)
     return;

   if (cbuf == NULL)
     return;

   pop_to_buffer (cbuf->name);
   lwant = client_data.l;

   if (lwant == NULL)
     {
	eob();
	return;
     }
   
   bob();
   while (1)
     {
	l = CLine->next;
	if ((l == NULL) || (l == lwant)) break;
	(void) jed_down(1);
     }
   (void) jed_skip_whitespace();
}

/*}}}*/

static SLang_Name_Type *Expand_File_Hook;
void set_expansion_hook (char *s) /*{{{*/
{
   if (NULL == (Expand_File_Hook = SLang_get_function (s)))
     {
	msg_error ("The expansion hook has not been defined.");
     }
}

/*}}}*/

int mini_complete (void) /*{{{*/
{
   char *pl, *pb;
   char last[JED_MAX_PATH_LEN], buf[JED_MAX_PATH_LEN], *tmp;
   static char prev[JED_MAX_PATH_LEN];
   int n, last_key_char = SLang_Last_Key_Char;
   static int flag = 0;  /* when flag goes 0, we call open */
   
   if (complete_open == NULL) return ins_char_cmd();
   
   bol ();
   jed_push_mark();
   eob();
   if (NULL == (tmp = make_buffer_substring(&n))) return(1);
   
   safe_strcpy(buf, tmp, sizeof (buf));
   SLfree(tmp);
   
   if ((last_key_char == ' ') && ((long) Last_Key_Function == (long) mini_complete))
     {
	if (flag)
	  flag = (*complete_next)(buf);
	if (flag == 0)
	  {
	     safe_strcpy(buf, prev, sizeof (buf));
	     flag = (*complete_open)(buf);
	  }
	safe_strcpy(last, buf, sizeof (last));
	n = -1;
     }
   else
     {
	n = 0;
	safe_strcpy (prev, buf, sizeof (prev));  /* save this search context */
     }
   
   if (!n)
     {
	if ((Repeat_Factor != NULL)
	    || (complete_open != file_findfirst) || (Expand_File_Hook == NULL))
	  flag = (*complete_open)(buf);
	else
	  {
	     int do_free;
	     SLang_push_string (buf);
	     SLexecute_function (Expand_File_Hook);
	     if (SLang_get_error () == 0) SLang_pop_integer (&do_free);
	     if (SLang_get_error () == 0)
	       {
		  if (do_free == 0)
		    {
		       flag = (*complete_open) (buf);
		       goto start_loop;
		    }
	       }
	     
	     if (SLang_get_error () || SLang_pop_slstring (&tmp))
	       {
		  msg_error ("Error encounter during expansion.  Disabling expansion hook.");
		  Expand_File_Hook = NULL;
		  return 1;
	       }
	     safe_strcpy (last, tmp, sizeof (last));
	     safe_strcpy (prev, last, sizeof (prev));
	     SLang_free_slstring (tmp);
	     flag = 0;		       /* So we do not call complete_next */
	     n = -1;
	  }
     }
   
   start_loop:
   
   if (!n && flag)
     {
	safe_strcpy(last, buf, sizeof (last));
	
	/* This do loop tests all values from complete_next and returns the
	   smallest length match of initial characters of buf */
	do
	  {
	     if ((n == 0) && (last_key_char == '\t'))
	       {
		  set_buffer (Completion_Buffer);
		  erase_buffer ();
		  CBuf->flags |= BURIED_BUFFER;
		  (void) jed_insert_string ("!!! Use Page Up/Down keys to scroll this window. !!!\n");
	       }
	     
	     n++;
	     pl = last;
	     pb = buf;

#if !JED_FILE_PRESERVE_CASE
 	     if ((complete_open == open_bufflist)
#if !JED_FILENAME_CASE_SENSITIVE
		 || (complete_open == file_findfirst)
#endif
		 )
	       while (*pl && (UPPER_CASE(*pl) == UPPER_CASE(*pb)))
		 {
		    pl++;
		    pb++;
		 }
	     else  /* next statement */
#endif
	       while (*pl && (*pl == *pb)) 
	       {
		  pl++;
		  pb++;
	       }
	     

	     *pl = 0;
	     
	     if (last_key_char == '\t')
	       {
		  if (complete_open == file_findfirst)
		    {
		       char *file = jed_extract_file_or_dir (buf);
		       (void) jed_quick_insert ((unsigned char *) file, strlen(file));
		    }
		  else
		    {
		       while (*pb) pb++;
		       (void) jed_quick_insert ((unsigned char *)buf, (int) (pb - buf));
		    }
		  newline ();
	       }
	  }
	while(0 != (flag = (*complete_next)(buf)));
	
#if JED_FILE_PRESERVE_CASE
	/* OS/2 uses case-insensitive search on buffer-names. Set the 
	 * flag if there is an exact match, so that completion will
	 * cycle without repeats through all the matches. 
	 */
	if (complete_open == open_bufflist) 
	  {
	     safe_strcpy(buf, last, sizeof (buf));
	     (*complete_open)(buf);
	     do 
	       {
		  if (!strcmp(buf, last)) 
		    {
		       flag = 1; break;
		    }
	       }
	     while ((*complete_next)(buf));
	  }
#endif
     }
   
   if ((n > 1) && (last_key_char == '\t') && (Last_Completion_Buffer == NULL))
     {
	Last_Completion_Windows = jed_num_windows () - 1;   /* not including mini */
	Last_Completion_Buffer = pop_to_buffer (Completion_Buffer);
	bob ();
     }
   
   while ((CBuf != MiniBuffer) || !IN_MINI_WINDOW) other_window ();
   
   if (n)
     {
	erase_buffer();
	(void) jed_insert_string(last);
	if ((n == 1) && ((long) Last_Key_Function == (long) mini_complete))
	  message("[Sole Completion.]");
     }
   else msg_error("No Match!");
   
   return(1);
}

/*}}}*/


void copy_region_cmd(char *name) /*{{{*/
{
   Buffer *buf;
   
   if (NULL != (buf = find_buffer(name)))
     {
	copy_region_to_buffer(buf);
     }
   else msg_error("Unable to find buffer.");
}

/*}}}*/

#ifndef IBMPC_SYSTEM

void screen_w80 (void) /*{{{*/
{
   tt_narrow_width ();
   jed_resize_display ();
}

/*}}}*/
void screen_w132 (void) /*{{{*/
{
   tt_wide_width();
   jed_resize_display ();
}

/*}}}*/
#endif

char *make_line_string(char *string, unsigned int buflen) /*{{{*/
{
   unsigned char *tmp, *p1, *p2;
   unsigned int n;
   
   if (CBuf->marks == NULL)
     {
	p1 = CLine->data + Point;
	p2 = CLine->data + CLine->len;
     }
   else
     {
	p1 = CLine->data + CBuf->marks->point;
	p2 = CLine->data + Point;
	if (p2 < p1)
	  {
	     tmp = p1; p1 = p2; p2 = tmp;
	  }
	jed_pop_mark(0);
     }
   n = (unsigned int) (p2 - p1);
   if (n >= buflen) n = buflen - 1;
   SLMEMCPY(string, (char *) p1, n);
   string[n] = 0;
   return(string);
}

/*}}}*/

char *make_buffer_substring(int *np) /*{{{*/
{
   Line *tthis, *beg;
   int n = 1, dn, thisp;
   unsigned char *buf;
   
   if (!check_region(&n)) return (NULL);      /* spot pushed */
   /* Point now at end of the region */
   
   beg = tthis = CBuf->marks->line;
   thisp = CBuf->marks->point;
   n = 0;
   jed_pop_mark(0);
   
   while (tthis != CLine)
     {
	n += tthis->len;
	tthis = tthis->next;
     }
   n -= thisp;
   n += Point;
   
   if (NULL == (buf = (unsigned char *) SLmalloc (n + 1)))
     {
	pop_spot();
	return (NULL);
     }
   
   if (CLine == (tthis = beg))
     {
	SLMEMCPY((char *)buf, (char *) (tthis->data + thisp), n);
     }
   else
     {
	n = 0;
	while (tthis != CLine)
	  {
	     dn = tthis->len - thisp;
	     SLMEMCPY((char *)(buf + n), (char *) (tthis->data + thisp), dn);
	     tthis = tthis->next;
	     thisp = 0;
	     n += dn;
	  }
	SLMEMCPY((char *)(buf + n), (char *) tthis->data, Point);
	n += Point;
     }
   buf[n] = 0;
   *np = n;
   pop_spot();
   return ((char *) buf);
}

/*}}}*/

void buffer_substring() /*{{{*/
{
   char *buf;
   int n;
   if (NULL == (buf = make_buffer_substring(&n))) return;
   SLang_push_malloced_string((char *)buf);
}

/*}}}*/

int markp(void) /*{{{*/
{
   return (CBuf->marks != NULL);
}

/*}}}*/

int dup_mark(void) /*{{{*/
{
   if (CBuf->marks == NULL) return(0);
   
   push_spot();
   jed_goto_mark(CBuf->marks);
   jed_push_mark();
   pop_spot();
   return(1);
}

/*}}}*/

void mini_read(char *prompt, char *def, char *stuff) /*{{{*/
{
   char *buf;
   int n;
   
   complete_open = NULL;
   if (NULL == (buf = read_from_minibuffer(prompt, def, stuff, &n)))
     SLang_push_string ("");
   else SLang_push_malloced_string(buf);
}

/*}}}*/

void make_buffer_list(void) /*{{{*/
{
   int n = 0;
   Buffer *b;
   
   b = CBuf;
   
   do
     {
	SLang_push_string(b->name);
	b = b->next;
	n++;
     }
   while (b != CBuf);
   SLang_push_integer(n);
}

/*}}}*/

static void jed_traceback (SLFUTURE_CONST char *s) /*{{{*/
{
   char *n;

   if (Batch)
     {
	(void) fputs (s, stderr);
	return;
     }
   
   n = CBuf->name;
   set_buffer("*traceback*");
   CBuf->flags |= BURIED_BUFFER;
   eob();
   (void) jed_insert_string(s);
   set_buffer(n);
}

/*}}}*/

#if 0
static struct /*{{{*/
{
   int depth = 0;
   char *name[20];
   int marks[20];
}

/*}}}*/
FName_Stack;

void enter_function(char *name) /*{{{*/
{
   if (depth > 20)
     {
	msg_error("Function Stack too deep.");
	return;
     }
   FName_Stack->name[depth] = name;
   FName_Stack->marks[depth] = 0;
}

/*}}}*/
void exit_function(char *name) /*{{{*/
{
   int n = FName_Stack->marks[depth];
   
}

/*}}}*/
#endif

static void jed_clear_error(void) /*{{{*/
{
   *Error_Buffer = 0;
   SLKeyBoard_Quit = 0;
}

/*}}}*/

typedef struct _Init_SLang_Hook_Type
{
   int (*hook)(void);
   struct _Init_SLang_Hook_Type *next;
}
Init_SLang_Hook_Type;

static Init_SLang_Hook_Type *Init_SLang_Hooks;

int jed_add_init_slang_hook (int (*hook)(void))
{
   Init_SLang_Hook_Type *h;

   if (hook == NULL)
     return 0;
   
   h = (Init_SLang_Hook_Type *)SLmalloc (sizeof (Init_SLang_Hook_Type));
   if (h == NULL)
     return -1;
   
   h->hook = hook;
   h->next = Init_SLang_Hooks;
   Init_SLang_Hooks = h;
   return 0;
}

static int run_init_slang_hooks (void)
{
   Init_SLang_Hook_Type *h;
   
   h = Init_SLang_Hooks;

   while (h != NULL)
     {
	if (-1 == (*h->hook)())
	  return -1;
	h = h->next;
     }

   while (Init_SLang_Hooks != NULL)
     {
	h = Init_SLang_Hooks->next;
	SLfree ((char *) Init_SLang_Hooks);
	Init_SLang_Hooks = h;
     }
   
   return 0;
}

static void slang_exit_error_handler (SLFUTURE_CONST char *fmt, va_list ap)
{
   char buf [2048];
   
   SLvsnprintf (buf, sizeof (buf), fmt, ap);
   exit_error (buf, 0);
}

static void vmsg_hook (SLFUTURE_CONST char *fmt, va_list ap)
{
   char buf [2048];
   SLvsnprintf (buf, sizeof (buf), fmt, ap);
   message (buf);
}

void jed_setup_minibuffer_keymap (void)
{
   SLKeyMap_List_Type *m;

   if (NULL == (m = SLang_find_keymap ("Mini_Map")))
     {
	if (NULL == (m = SLang_create_keymap("Mini_Map", Global_Map))) 
	  exit_error("Unable to create minibuffer keymap", 0);

	SLang_undefine_key("\r", m);
	SLkm_define_key ("\r", (FVOID_STAR) exit_minibuffer, m);
	SLkm_define_key ("\t", (FVOID_STAR) mini_complete, m);
	SLkm_define_key (" ", (FVOID_STAR) mini_complete, m);
     }
   The_MiniBuffer->keymap = m;
}

void init_minibuffer() /*{{{*/
{
   Buffer *tmp;
   
   tmp = CBuf;
   
   The_MiniBuffer = make_buffer (" <mini>", NULL, NULL);
   The_MiniBuffer->modes = 0;
   /* do some initializing */
   switch_to_buffer(The_MiniBuffer);
   remake_line(132);
   JMiniWindow = jed_create_minibuffer_window ();
   JMiniWindow->buffer = CBuf;
   Buffer_Local.tab = 0;
   switch_to_buffer(tmp);
   SLang_Dump_Routine = jed_traceback;
   SLang_Exit_Error_Hook = slang_exit_error_handler;

#ifdef __GO32__
   SLang_Interrupt = i386_check_kbd;
#endif
   
#if 0
   SLang_Enter_Function = enter_function;
   SLang_Exit_Function = exit_function;
#endif

   if ((-1 == SLang_init_slang ())
#ifndef SIXTEEN_BIT_SYSTEM
       || (-1 == SLang_init_slmath ())
       || (-1 == SLang_init_array ())
#endif
       || (-1 == SLang_init_posix_process ())
       || (-1 == SLang_init_posix_dir ())
       /* || (-1 == SLang_init_slassoc ()) -- handled by SLang_init_slang */
       || (-1 == SLang_init_stdio ())
       || (-1 == SLang_init_posix_io ())
       || (-1 == SLang_init_ospath ())
#if JED_HAS_IMPORT
       || (-1 == SLang_init_import ())
#endif
       || (-1 == init_jed_intrinsics ())
       || (-1 == jed_init_userinfo ())
#if JED_HAS_MENUS
       || (-1 == jed_init_menus ())
#endif
       || (-1 == jed_init_syntax ())
       || (-1 == register_jed_classes ())
       || (-1 == jed_init_user_hooks ())
       || (-1 == run_init_slang_hooks ()))
     {
	exit_error("Unable to initialize S-Lang!", 0);
     }
   
   /* use jed rouotines instead of default slang ones */
   SLang_Error_Hook = jed_error_hook;
   SLang_VMessage_Hook = vmsg_hook;
   SLang_User_Clear_Error = jed_clear_error;
   SLns_Load_File_Hook = jed_ns_load_file;
}

/*}}}*/
