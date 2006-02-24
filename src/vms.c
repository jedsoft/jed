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
#include <string.h>
#include "sysdep.h"

#include <ssdef.h>
#include <rmsdef.h>
#include <dvidef.h>
#include <jpidef.h>
#include <descrip.h>
#include <iodef.h>
#include <ttdef.h>
#include <rms.h>
#include <errno.h>
#include <tt2def.h>

#ifdef __DECC
#include <starlet.h>
#include <lib$routines.h>
#include <unixlib.h> 
#include <unixio.h>
#endif

#if (__VMS_VER >= 70000000)
# include <unistd.h>
#endif
/* #include <libdef.h>  */

typedef struct {                /* I/O status block     */
        short i_cond;           /* Condition value      */
        short i_xfer;           /* Transfer count     */
        long  i_info;           /* Device information     */
} Iosb_Type;

typedef struct {                /* Terminal characteristics   */
        char  t_class;          /* Terminal class     */
        char  t_type;           /* Terminal type      */
        short t_width;          /* Terminal width in characters   */
        long  t_mandl;          /* Terminal's mode and length   */
        long  t_extend;         /* Extended terminal characteristics  */
}  TermChar_Type;

TermChar_Type Old_Term_Char, New_Term_Char;

/* This serves to identify the channel we are reading input from.  */
static short This_Term;


typedef struct /*{{{*/
{
   short buflen;
   short item_code;
   int *buf_addr;
   int *len_addr;
} 
/*}}}*/
item_list_3;


static int TTY_Inited;

void vms_exit_handler(int not_used) /*{{{*/
{
   if (TTY_Inited == 0) exit(0);

   auto_save_all();
   jed_reset_display(); 
   reset_tty(); 
   exit(1);
}

/*}}}*/

/*
 *      Exit Handler Control Block
 */
static struct argument_block
  {
      int forward_link;
      int (*exit_routine)();
      int arg_count;
      int *status_address;
      int exit_status;
  }
exit_block =
  {
      0,
      NULL,
      1,
      &exit_block.exit_status,
      0
  };

void vms_cancel_exithandler() /*{{{*/
{
   sys$canexh(exit_block);
}

/*}}}*/

#undef USING_INPUT_BUFFER
#undef DONE_WITH_INPUT_BUFFER


int vms_input_buffer;

static struct vms_ast_iosb /*{{{*/
{
    short status;
    short offset;
    short termlen;
    short term;
} vms_ast_iosb;

/*}}}*/

extern void vms_que_key_ast();
static int Ast_Fired_Event_Flag;
static int Timer_Event_Flag;
static int Event_Flag_Mask;
static int Ast_Stop_Input;
static int Waiting_For_Ast;
static int Using_Keyboard_Buffer_Event_Flag;

#define USING_INPUT_BUFFER\
   sys$setast(0);
   
#define DONE_WITH_INPUT_BUFFER\
   sys$setast(1);

static int getkey_ast(int not_used) /*{{{*/
{
   unsigned int c = 1000;
   
   if (vms_ast_iosb.offset)
     {
	c = (unsigned int) vms_input_buffer;
     }

   if (c <= 255)
     {
	if (c == (unsigned int) Jed_Abort_Char)
	  {
	     if (Ignore_User_Abort == 0) 
	       SLang_set_error (SL_USER_BREAK);
	     SLKeyBoard_Quit = 1;
	  }

	USING_INPUT_BUFFER
	
	if (Input_Buffer_Len < MAX_INPUT_BUFFER_LEN - 3) 
	  Input_Buffer[Input_Buffer_Len++] = c;
	
	DONE_WITH_INPUT_BUFFER
     }
   if (Waiting_For_Ast)  sys$setef (Ast_Fired_Event_Flag);
   Waiting_For_Ast = 0;
   vms_que_key_ast();
   return (1);
}

/*}}}*/

void vms_que_key_ast() /*{{{*/
{
   static int trmmsk [2] = { 0, 0 };
   int status;

   if (Ast_Stop_Input) return;
   status = sys$qio (0, This_Term,
		     IO$_READVBLK | IO$M_NOECHO | IO$_TTYREADALL,
		     &vms_ast_iosb, getkey_ast, 1,
		     &vms_input_buffer, 1, 0, trmmsk, 0, 0);
}

/*}}}*/

static char TTY_Name[8];
static int This_Process_Pid;

int init_tty() /*{{{*/
{
   Iosb_Type iostatus;
   int tmp, name_len, status, lastppid, ppid;
   item_list_3 itmlst[3];
   $DESCRIPTOR ( term, TTY_Name);
   
   itmlst[0].buflen = sizeof(int);
   itmlst[0].item_code = JPI$_PID;
   itmlst[0].buf_addr = &This_Process_Pid;
   itmlst[0].len_addr = &tmp;
   
   itmlst[1].buflen = 7;
   itmlst[1].item_code = JPI$_TERMINAL;
   itmlst[1].buf_addr = (int *) TTY_Name;
   itmlst[1].len_addr = &name_len;
   
   itmlst[2].buflen = 0;
   itmlst[2].item_code = 0;
   itmlst[2].buf_addr = 0;
   itmlst[2].len_addr = 0;
   
   TTY_Inited = 1;
   ppid = 0, lastppid = -1;
   
   /* Here I get this process pid then I get the master process pid
      and use the controlling terminal of that process. */
   while (1)
     {
	status = sys$getjpiw(0,       /* event flag */
			     &ppid,   /* pid address */
			     0,       /* proc name address */
			     itmlst,
			     0, 0, 0);

	if (status != SS$_NORMAL) 
	  {
	     fprintf(stderr, "PID: %X, status: %X\n", This_Process_Pid, status);
	     return -1;
	  }

	if (lastppid == ppid) break;
	lastppid = ppid;

	itmlst[0].item_code =  JPI$_MASTER_PID;
	itmlst[0].buf_addr =  &ppid;
     }
   
   if (Batch) return 0;
   if (X_Init_Term_Hook != NULL)
     {
	(void) (*X_Init_Term_Hook) ();
	return 0;
     }
   
   term.dsc$w_length = name_len;
   status = sys$assign ( &term, &This_Term, 0, 0 );
   if (status != SS$_NORMAL)
     {
	fprintf(stderr,"Unable to assign input channel\n");
	fprintf(stderr,"PID: %X, DEV %s, status: %d\n", This_Process_Pid, TTY_Name, status);
	return -1;
     }

   if (NULL == exit_block.exit_routine)
     {
	exit_block.exit_routine = (int (*)()) vms_exit_handler;
	sys$dclexh(&exit_block);
     }

   /* allocate an event flag and clear it--- used by ast routines.  Since
    * I am only using a few local event flags, there is really no need to 
    * worry about freeing these.
    * 
    * The event flags are used to avoid timing problems with the getkey AST
    * as well as for a form of time out.
    */
   if (!Ast_Fired_Event_Flag) lib$get_ef (&Ast_Fired_Event_Flag);
   sys$clref (Ast_Fired_Event_Flag);
   
   if (!Timer_Event_Flag) lib$get_ef (&Timer_Event_Flag);
   sys$clref (Timer_Event_Flag);
   
   /* When no thread is using the keyboard buffer, this ev is set.  It 
    * gets cleared when the buffer is in use.
    */
   if (!Using_Keyboard_Buffer_Event_Flag) lib$get_ef (&Using_Keyboard_Buffer_Event_Flag);
   sys$setef (Using_Keyboard_Buffer_Event_Flag);
   
   /* The working assumption here is that the event flags are in the same
    * cluster.  They need not be but it is very likely that they are.
    */
   Event_Flag_Mask = ((unsigned) 1 << (Ast_Fired_Event_Flag % 32));
   Event_Flag_Mask |= ((unsigned) 1 << (Timer_Event_Flag % 32));

   Waiting_For_Ast = 0;
   Ast_Stop_Input = 0;
   
   /* Get the startup terminal characteristics */
   status = sys$qiow(0,		       /* Wait on event flag zero      */
		     This_Term,	       /* Channel to input terminal    */
		     IO$_SENSEMODE,    /* Get current characteristic   */
		     &iostatus,	       /* Status after operation       */
		     0, 0,	       /* No AST service               */
                     &Old_Term_Char,   /* Terminal characteristics buf */
		     sizeof(Old_Term_Char),/* Size of the buffer           */
		     0, 0, 0, 0);
   
   New_Term_Char = Old_Term_Char;
   New_Term_Char.t_mandl |= TT$M_EIGHTBIT | TT$M_NOECHO;
   New_Term_Char.t_extend |= TT2$M_PASTHRU | TT2$M_XON;
   
   status = sys$qiow(0,		       /* Wait on event flag zero      */
		     This_Term,	       /* Channel to input terminal    */
		     IO$_SETMODE,      /* Set current characteristic   */
		     &iostatus,	       /* Status after operation       */
		     0, 0,	       /* No AST service               */
                     &New_Term_Char,   /* Terminal characteristics buf */
		     sizeof(New_Term_Char),/* Size of the buffer           */
		     0, 0, 0, 0); 
      
   /* Enable the Application Keypad */
   (*tt_write_string) ("\033=\033[?1l");   /* application keys/cursor keys */
   vms_que_key_ast();   /* set up the key ast */
   return 0;
}

/*}}}*/


static void cancel_ast (void) /*{{{*/
{
   if (TTY_Inited == 0) return;
   
   /* stop the keyboard ast */
   sys$setast (0);		       /* disable AST delivery */
   sys$clref (Ast_Fired_Event_Flag);
   Waiting_For_Ast = 1;
   Ast_Stop_Input = 1;

   /* cancel all i/o on this channel.  This canels pending, as well as those
    * already in progress and queued.  In particular, according to the 
    * manuals, cancelling I/O on the channel will cause the getkey AST
    * to fire even though the SYS$QIO call was aborted.  This is crucial
    * because below we wait for the AST to set the event flag.
    */
   sys$cancel (This_Term);
   sys$setast (1);		       /* enable ASTs again */
   sys$waitfr (Ast_Fired_Event_Flag);  /* sleep until it fires */
   Waiting_For_Ast = 0;
}

/*}}}*/

static int keypad_state = 0;
void reset_tty() /*{{{*/
{
   Iosb_Type iostatus;
   if (Batch) return;
   if (!TTY_Inited) return;
   if (X_Init_Term_Hook != NULL)
     {
	if (X_Reset_Term_Hook != NULL) (*X_Reset_Term_Hook) ();
	TTY_Inited = 0;
	return;
     }
   cancel_ast ();
   TTY_Inited = 0;
   
   /* reset the terminal characteristics */
   
   sys$qiow(0,			       /* event flag 0 */
	    This_Term,		       /* Channel to input terminal    */
	    IO$_SETMODE,	       /* Set current characteristic   */
	    &iostatus,		       /* Status after operation       */
	    0, 0,		       /* No AST service               */
	    &Old_Term_Char,	       /* Terminal characteristics buf */
	    sizeof(Old_Term_Char),     /* Size of the buffer           */
	    0, 0, 0, 0); 	       /* unused */

   if (keypad_state) SLtt_write_string("\033=\033[?1l");
   else SLtt_write_string("\033>");
   
   SLtt_flush_output ();
}

/*}}}*/

unsigned char sys_getkey() /*{{{*/
{
   unsigned char c;
   int tsecs;

   if (SLKeyBoard_Quit) return(Jed_Abort_Char);
   
   /* Under DECWIndows, I do not know how to timeout so this will have to do
      for now */
   if (X_Read_Hook != NULL) return (c = X_Read_Hook ());

  /* On VMS, the keyboard ast routine should be stuffing the buffer, so
   do nothing except sleep */

   /* clear the flag which ast will set */
   Waiting_For_Ast = 0;
   if (Input_Buffer_Len) return(my_getkey());
   tsecs = 450;   /* 45 seconds */
   
   while (!sys_input_pending(&tsecs, 0))
     {
	if (Jed_Sig_Exit_Fun != NULL)
	  (*Jed_Sig_Exit_Fun) ();

	/* update status line incase user is displaying time */
	if (Display_Time)
	  {
	     JWindow->trashed = 1;
	     update((Line *) NULL, 0, 1, 0);
	  }
     }
   c = my_getkey();
   return(c);
}

/*}}}*/

/* waits *secs tenth of seconds for input */
int sys_input_pending(int *secs, int unused) /*{{{*/
{
   unsigned long daytim[2];
   
   if (Batch) return(0);
   if (Input_Buffer_Len) return(Input_Buffer_Len);
   
   if (X_Input_Pending_Hook != NULL) 
     {
	if ((*X_Input_Pending_Hook) ()) return 1;
	/* I need to make this work with DECWIndows */
	return 0;
     }
   
   
   if (*secs) 
     {
	/* takes a quad word time.  If negative, use a relative time. */
	daytim[1] = 0xFFFFFFFF;
	daytim[0] = -(*secs * 1000 * 1000);   /* 1000 * 1000 is a tenth of a sec */
	
	sys$clref (Ast_Fired_Event_Flag);
	/* SYS$CLREF (Timer_Event_Flag);  SYS$SETIMR call clears this */

	/* set up a flag for the ast so it knows to set the event flag */
	Waiting_For_Ast = 1;
	
	sys$setimr(Timer_Event_Flag, daytim, 0, 1);
		   
	/* this will return when ast does its job or timer expires.
	 * The first argument simply serves to identify the cluster for 
	 * the event flag and that is all.  The second argument serves 
	 * to identify the event flags to wait for. 
	 */
	sys$wflor (Ast_Fired_Event_Flag, Event_Flag_Mask);
	
	Waiting_For_Ast = 0;
	
	/* cancel the timer */
	sys$cantim(1, 3);   /* 3 is user mode */
     }
   return (Input_Buffer_Len);
}

/*}}}*/

/* This is completely untested!! */
void sys_pause (int ms) /*{{{*/
{
   unsigned long daytim[2];

   /* takes a quad word time.  If negative, use a relative time. */
   daytim[1] = 0xFFFFFFFF;
   daytim[0] = -(ms * 10 * 1000);   /* 10 * 1000 is a milli sec */
	
	
   sys$setimr(Timer_Event_Flag, daytim, 0, 1);
		   
   /* cancel the timer */
   sys$cantim(1, 3);   /* 3 is user mode */
}

/*}}}*/
#if 0
/*  This is to get the size of the terminal  */
int get_term_dimensions(int *cols, int *rows) /*{{{*/
{
   int status, junk;
   Iosb_Type iostatus;
   $DESCRIPTOR(devnam, TTY_Name);
   item_list_3 itmlst[4];
   
   itmlst[0].buflen = sizeof (*rows);
   itmlst[0].item_code = DVI$_TT_PAGE;
   itmlst[0].buf_addr = rows;
   itmlst[0].len_addr = &junk;
   
   itmlst[1].buflen = sizeof(*cols);
   itmlst[1].item_code = DVI$_DEVBUFSIZ;
   itmlst[1].buf_addr = cols;
   itmlst[1].len_addr = &junk;
   
   itmlst[2].buflen = sizeof (keypad_state);
   itmlst[2].item_code = DVI$_TT_APP_KEYPAD;
   itmlst[2].buf_addr = &keypad_state;
   itmlst[2].len_addr = &junk;

   itmlst[3].buflen = 0;
   itmlst[3].item_code = 0;
   itmlst[3].buf_addr = 0;
   itmlst[3].len_addr = 0;

   if (*rows <= 0) *rows = Jed_Num_Screen_Rows;
   if (*cols <= 0) *cols = *tt_Screen_Cols;

   /* Get current terminal characteristics */
   status = sys$getdviw(0,           /* Wait on event flag zero  */
			0,           /* Channel to input terminal  */
			&devnam,     /* device name */
			&itmlst,	  /* Item descriptor List */
			&iostatus,   /* Status after operation */
			0, 0,        /* No AST service   */
			0);          /* nullarg */

   if (status&1) status = iostatus.i_cond;
   /* Jump out if bad status */
   if ((status & 1) == 0) exit(status);
   return 0;
}

/*}}}*/
#endif

/* returns 0 on failure, 1 on sucess */
int sys_delete_file(char *filename) /*{{{*/
{
    return (1 + delete(filename));   /* 0: sucess; -1 failure */
}

/*}}}*/


/* This routine converts unix type names to vms names */
int locate(char ch, char *string) /*{{{*/
{
    int i;
    char c;

    i = 0;
    while (c = string[i++], (c != ch) && (c != '\0'));
    if (c == ch) return(i); else return (0);
}

/*}}}*/

char *unix2vms(char *file) /*{{{*/
{
    int i,device,j,first,last;
    static char vms_name[80];
    char ch;

    if (locate('[',file)) return(file); /* vms_name syntax */
    if (!locate('/',file)) return(file); /* vms_name syntax */

    /* search for the ':' which means a device is present */
    device = locate(':',file);

    i = 0;
    if (device)
      {
          while (ch = file[i], i < device) vms_name[i++] = ch;
      }
    j = i;

    /* go from the  end looking for a '/' and mark it */
    i = strlen(file) - 1;
    while(ch = file[i], ch != '/' && i-- >= 0);
    if (ch == '/')
      {
          file[i] = ']';
          last = 0;
      }
    else last = 1;

    i = j;
    vms_name[j++] = '[';
    vms_name[j++] = '.';
    first = 0;
    while(ch = file[i++], ch != '\0')
      {
          switch (ch)
            {
              case '.':
                if (last) vms_name[j++] = '.';
                if (last) break;
                ch = file[i++];
                if (ch == '.')
                  {
                      if (!first) j--;  /* overwrite the dot */
                      vms_name[j++] = '-';
                  }
                else if (ch == '/'); /*  './' combinations-- do nothing */
                else if (ch == ']')
                  {
                      last = 1;
                      if (vms_name[j-1] == '.') j--;
                      vms_name[j++] = ']';
                  }

                else vms_name[j++] = '.';
                break;
              case '/':
                if (first)
                  {
                      vms_name[j++] = '.';
                  }
                else
                  {
                      first = 1;
                      /* if '/' is first char or follows a colon do nothing */
                      if ((i!=1) && (file[i-2] != ':'))
                        {
                            vms_name[j++] = '.';
                        }
                      else j--; /* overwrite the '.' following '[' */
                  }
                break;
              case ']':
                last = 1;
                if (vms_name[j-1] == '.') j--;
                vms_name[j++] = ']';
                break;
              default:
                vms_name[j++] = ch;
            }
      }
    return (vms_name);
}

/*}}}*/

void define_logical_name (char *varname, char *string) /*{{{*/
{
   struct dsc$descriptor_s strdsc, envdsc, lnmdsc;
   
   strdsc.dsc$w_length = strlen (string);
   strdsc.dsc$b_dtype = DSC$K_DTYPE_T;
   strdsc.dsc$b_class = DSC$K_CLASS_S;
   strdsc.dsc$a_pointer = string;
   envdsc.dsc$w_length = strlen (varname);
   envdsc.dsc$b_dtype = DSC$K_DTYPE_T;
   envdsc.dsc$b_class = DSC$K_CLASS_S;
   envdsc.dsc$a_pointer = varname;
   lnmdsc.dsc$w_length = 7;
   lnmdsc.dsc$b_dtype = DSC$K_DTYPE_T;
   lnmdsc.dsc$b_class = DSC$K_CLASS_S;
   lnmdsc.dsc$a_pointer = "LNM$JOB";
   
   lib$set_logical (&envdsc, &strdsc, &lnmdsc, 0, 0);
}

/*}}}*/

void delete_logical_name (char *varname) /*{{{*/
{
   struct dsc$descriptor_s envdsc, lnmdsc;
   
   envdsc.dsc$w_length = strlen (varname);
   envdsc.dsc$b_dtype = DSC$K_DTYPE_T;
   envdsc.dsc$b_class = DSC$K_CLASS_S;
   envdsc.dsc$a_pointer = varname;
   lnmdsc.dsc$w_length = 7;
   lnmdsc.dsc$b_dtype = DSC$K_DTYPE_T;
   lnmdsc.dsc$b_class = DSC$K_CLASS_S;
   lnmdsc.dsc$a_pointer = "LNM$JOB";

   lib$delete_logical (&envdsc, &lnmdsc);
}

/*}}}*/

int do_attach_cmd() /*{{{*/
{
   unsigned long pid;
   char *pidstr;

   if((pidstr = getenv("JED_ATTACH_TO")) != NULL)
     {
	delete_logical_name("JED_ATTACH_TO");
	(void) sscanf(pidstr,"%X",&pid);
	if (lib$attach(&pid) == SS$_NORMAL)
	  return(1);
        else
          return(0);
     }
   else return(0);
}

/*}}}*/

unsigned long SHELL_PID = 0;

/* here we try to attach to the parent otherwise just spawn a new one */
void sys_suspend() /*{{{*/
{
   unsigned long parent_pid;
   unsigned long status = 0;

   cancel_ast ();
   parent_pid = getppid();

   /* try to attach to different process */
   if (do_attach_cmd()) status = SS$_NORMAL;

   else if (parent_pid && parent_pid != 0xffffffff)
   /* we attach to parent */
      status = lib$attach(&parent_pid);

   else if (SHELL_PID && SHELL_PID != 0xffffffff)
   /* try to attach to previous shell */
      status = lib$attach (&SHELL_PID);

   if (status != SS$_NORMAL)		/* others fail so spawn a new shell */
     {
	status = 0;
	SLtt_write_string("Spawning Subprocess...\n");
	flush_output ();
	status = lib$spawn(0,0,0,0,0,&SHELL_PID,0);
	/* if we attach back, status may come back unchanged */
	if (!(status & 1))  /* Thanks to Hunter Goatley for this suggestion */
	  {              
	     jed_verror ("Unable to spawn subprocess. Error = X%X (%d)", status, status);
	     return;
	  }
     }

/*   if((file = getenv("JED_FILE_NAME")) != NULL)
     {
	find_file_cmd(file);
	delete_logical_name ("JED_FILE_NAME");
     } */
}

/*}}}*/

/* returns 0 on success, -1 on syntax error
   -2 on bad dir, -3 on bad device, 1 if something else */
int vms_parse_file(char *old) /*{{{*/
{
   struct FAB fab = cc$rms_fab;
   struct NAM nam = cc$rms_nam;
   char neew[JED_MAX_PATH_LEN];
   int status;

   fab.fab$l_fna = old;
   fab.fab$b_fns = strlen(old);
   fab.fab$b_dns = 0;
   fab.fab$w_ifi = 0;
   fab.fab$l_nam = &nam;
   nam.nam$l_esa = neew;
   nam.nam$b_ess = JED_MAX_PATH_LEN - 1;
   nam.nam$b_nop = NAM$V_SYNCHK;   /* syntax only */
   nam.nam$l_rlf = 0;

   status = sys$parse(&fab);
   neew[nam.nam$b_esl] = 0;
   strcpy(old, neew);
   while(*old != 0)
     {
	if ((*old == ';') && (*(old + 1) == 0)) *old = 0;
	else
	  {
	     if ((*old >= 'A') && (*old <= 'Z')) *old |= 0x20;
	     old++;
	  }
     }

   switch(status)
     {
	case RMS$_NORMAL: return 0;
	case RMS$_SYN: return -1;
	case RMS$_DNF: return -2;
	case RMS$_DEV: return -3;
	return 1;
     }
}

/*}}}*/

char *jed_standardize_filename_static(char *file) /*{{{*/
{
   char *p, *p1, *dir;
   static char work [1024];
   int len;

   if (NULL != strstr (file, "::"))
     goto ugly;

   strcpy(work, jed_get_cwd()); strcat(work, file); file = work;

    /*  start at end and look for ']' then look for ':' */
    if (0 == (len = strlen(file))) return(file);
    p = file + (len - 1);
    while (p >= file) if (*p-- == ':')
      {
	 while((p >= file) && (*p != ':') && (*p != ']')) p--;
	 p++;
	 p1 = file;
         while(*p) *p1++ = *p++;
	 *p1 = 0;
	 break;
      }
   
   dir = p = p1 = file;
   
   /* look for the start of the path */
   while (*p != 0)
     {
	if (*p == ':')
	  {
	     p++;
	     if (*p == ':')
	       {
		  p++;
	       }
	     dir = p;
	     break;
	  }
	if (*p == '[') 
	  {
	     dir = p++;
	     break;
	  }
	p++;
     }
   
   p1 = p = dir;
   
    while (*p != 0)
      {
	 if (*p == '[')
	   {
	      /* if (*(p + 1) == '-') */
	      p1 = dir;
	   }
	 
	 if (p1 != p) *p1 = *p;
	 
	 p1++;
	 p++;
      }
   *p1 = 0;

   ugly:
   
   switch(vms_parse_file(file))
      {
       case 0:
	 break;
	 case -1: msg_error("Filename syntax error."); break; 
	 case -2: msg_error("Directory does not exist!"); 
	          msg_error (file); break;
	 case -3: msg_error("Bad device name."); break;
       default: msg_error ("Unknown directory error:");
	 msg_error (file);
      }
   
   p = file;
   while(*p != 0) if (*p++ == ']')
     {
	 if ((*p == '.') && (*(p + 1) == 0)) *p = 0;
         break;
      }
   return(file);

}

/*}}}*/

char *jed_standardize_filename (char *file)
{
   return SLmake_string (jed_standardize_filename_static (file));
}

char *jed_expand_filename (char *file)
{
   return jed_standardize_filename (file);
}

int vms_expand_filename(char *file,char *expanded_file) /*{{{*/
{
    static int context = 0;
    static char inputname[JED_MAX_PATH_LEN] = "";
    $DESCRIPTOR(file_desc,inputname);
    $DESCRIPTOR(default_dsc,"SYS$DISK:[]*.*;");
    static struct dsc$descriptor_s  result =
	    {0, DSC$K_DTYPE_T, DSC$K_CLASS_D, NULL};

    if (strcmp(inputname, file))
      {
	  if (context)
	    {
		lib$find_file_end(&context);
	    }
	  context = 0;
	  strcpy(inputname, file);
	  file_desc.dsc$w_length = strlen(inputname);
      }

    if (RMS$_NORMAL == lib$find_file(&file_desc,&result,&context,
	           		     &default_dsc,0,0,&Number_Zero))
      {
	  SLMEMCPY(expanded_file, result.dsc$a_pointer, result.dsc$w_length);
	  expanded_file[result.dsc$w_length] = '\0';
          return (1);
      }
    else
      {
          /* expanded_file[0] = '\0'; */      /* so file comes back as zero width */
          return(0);
      }
}

/*}}}*/

static int context = 0;

static char inputname[JED_MAX_PATH_LEN] = "";
$DESCRIPTOR(file_desc,inputname);
$DESCRIPTOR(default_dsc,"SYS$DISK:[]*.*;");

static char *VMS_Star;

int sys_findnext(char *file) /*{{{*/
{
   char *f;
   static struct dsc$descriptor_s  result = {0, DSC$K_DTYPE_T, DSC$K_CLASS_D, NULL};

   if (RMS$_NORMAL == lib$find_file(&file_desc,&result,&context,
				    &default_dsc,0,0,&Number_Zero))
     {
	SLMEMCPY(file, result.dsc$a_pointer, result.dsc$w_length);
	file[result.dsc$w_length] = 0;
	f = &file[result.dsc$w_length];
	if (*VMS_Star)
	  {
	     while ((f > file) && (*f != ';')) f--;
	     if (*f == ';') *f = 0;
	  }

	return (1);
     }
   else return(0);
}

/*}}}*/

int sys_findfirst(char *thefile) /*{{{*/
{
   char *f, *f1, *s2 = "*.*";
   char *file;

   file = thefile;
   f = extract_file(file);

   while (*f && (*f != '*')) f++;

   VMS_Star = "";

   /* let user choose what to match with or take mine */
   if (! *f)
     {
	f = extract_file(file);
	while (*f && (*f != '.')) f++;
	if (*f) VMS_Star = "*"; else VMS_Star = s2;

	file = jed_standardize_filename_static(thefile);
	f = f1 = extract_file(file);
	if (VMS_Star == s2)
	  {
	     while (*f && (*f != '.')) f++;
	     *f = 0;
	  }
	else
	  {
	     while (*f1 && (*f1 != ';')) f1++;
	     *f1 = 0;
	  }
	strcpy(thefile, file);
     }

   strcpy(inputname, file);
   if (*VMS_Star) strcat(inputname, VMS_Star);
   file_desc.dsc$w_length = strlen(inputname);

   if (context) lib$find_file_end(&context);
   context = 0;
   return sys_findnext(thefile);
}

/*}}}*/

#include <stat.h>

/* returns 0 if file does not exist, 1 if it is not a dir, 2 if it is */
int sys_chmod(char *file, int what, mode_t *mode, uid_t *uid, gid_t *gid) /*{{{*/
{
   struct stat buf;
   int m;

   if (stat(file, &buf) < 0) switch (errno)
     {
	case EACCES: return(-1); /* es = "Access denied."; break; */
	case ENOENT: return(0);  /* ms = "File does not exist."; */
	case ENOTDIR: return(-2); /* es = "Invalid Path."; */
	default: return(-3); /* "stat: unknown error."; break;*/
     }

   m = buf.st_mode;
   (void) uid; (void) gid;

   *mode = m & 0777;

   if (m & S_IFDIR) return (2);
   return(1);
}

/*}}}*/

#if (__VMS_VER < 70000000)
int rmdir (const char *d) /*{{{*/
{
   return -1;
}

/*}}}*/

#endif


