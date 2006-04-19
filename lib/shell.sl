% -*- mode: slang; mode: fold; -*-
% 
% 
%% system specific routines

if (_jed_secure_mode) %{{{
{
   error ("Shell not available");
}

%}}}

variable Shell_Last_Shell_Command = Null_String;
variable Shell_Prompt;
#ifdef UNIX
Shell_Prompt = "% ";
#endif
#ifdef IBMPC_SYSTEM
Shell_Prompt = "> ";
#endif
#ifdef VMS
Shell_Prompt = "$ ";
#endif



#ifdef MSDOS
% pass "(cmd) 2>&1" contructs to the system as "(cmd) > tmpfile 2>&1"
define run_shell_cmd (cmd) %{{{
{
   variable msg = "Executing shell cmd...";
   variable tmp, dir;

   dir = getenv ("TMP");
   if (dir == NULL) dir = "";

   if (2 != file_status (dir))
     (,dir,,) = getbuf_info ();	% get directory
   
   tmp = dircat (dir, "_jed_shl.cmd");
   
   flush (msg);
   
%%    if ( is_substr (cmd, "2>&1") )
%%      both;
%%   	argv = extract_element (cmd, 1, ' ');

   if (system (sprintf ("%s &> %s", cmd, tmp)) < 0)
     error ("system failed.");
   () = insert_file (tmp);
   flush (strcat (msg, "done"));
   delete_file (tmp);		       %  value returned
}

%}}}
#endif
#ifdef VMS
define run_shell_cmd (cmd) %{{{
{
   variable cfile, file = "_jed_shell.cmd_";
   variable tmpdir;
   tmpdir = getenv ("SYS$SCRATCH");
   if (tmpdir == NULL) tmpdir = "SYS$LOGIN:";
   file = dircat (tmpdir, file);

   cfile = expand_jedlib_file ("vms_shell.com");
   !if ( strlen (cfile) ) error ("Unable to open vms_shell.com");

   flush ("starting process ...");
   () = system ((sprintf ("@%s/output=%s \"%s\"", cfile, file, cmd)));
   () = insert_file (file);
   delete_file (file);		       %  value returned
}

%}}}
#endif

private define shell_set_output_buffer () %{{{
{
   variable dir, file, name, flags;
   
   (,dir,,) = getbuf_info ();
   if ( change_default_dir (dir) ) 
     error ("Unable to chdir!");
   
   pop2buf ("*shell-output*"); 
   erase_buffer ();
   
   (file,,name, flags) = getbuf_info ();
   setbuf_info (file, dir,name, flags);
}

%}}}

public define shell_perform_cmd (cmd, same_buf) %{{{
{
   variable status;
   
   !if (same_buf) 
     shell_set_output_buffer ();

   push_spot ();
   status = run_shell_cmd (cmd);
   pop_spot ();

   !if (same_buf)
     set_buffer_modified_flag (0);

   vmessage ("Exit Status: %d", status);
}

%}}}
      
public define do_shell_cmd () %{{{
{
   variable cmd, dir;
   variable same_buf = (-9999 != prefix_argument (-9999));

   if (_NARGS)
     cmd = ();
   else
     {
	(,dir,,) = getbuf_info ();
	cmd = read_mini (sprintf ("(%s) Shell Cmd:", dir),
			 "", Shell_Last_Shell_Command);
	!if ( strlen (cmd) ) return;
	Shell_Last_Shell_Command = cmd;
     }

   shell_perform_cmd (sprintf (
# ifdef OS2
			       "(%s) 2>&1",
# elifdef UNIX
			       "(%s) 2>&1 < /dev/null",
# else
			       "%s",
# endif
			       cmd),
		      same_buf);
}

%}}}

public define shell () %{{{
{
   variable dir, buf = "*shell*";

   !if ( keymap_p (buf) )
     {
	make_keymap (buf);
	definekey ("shell_input", "^M", buf);
     }

   (,dir,,) = getbuf_info ();		% get directory
   if ( change_default_dir (dir) ) error ("Unable to chdir!");

   pop2buf (buf);
   use_keymap (buf);

   vinsert ("\n(%s)\n%s", dir, Shell_Prompt);

   % no backup (0x100), no save ~(0x80), no undo ~(0x20), no autosave ~(0x2)
   % unmodified ~(0x1)
   getbuf_info ();
   setbuf_info ((() | 0x100) & ~(0xA3));

   run_mode_hooks ("shell_mode_hook");
}

%}}}

%!%+
%\function{shell_builtin}
%\synopsis{shell_builtin}
%\description
% rudimentary `builtin' shell commands:
% 	`cd [dir]'	change the default directory
% 	`exit'		exit the subshell
% 	`pwd'		Print Working Directory
% 
% functions to eliminate some jed/shell vs. real shell problems
% 	`clear'		erase the *shell* buffer
% 	`e'		simulate ^X^F keybinding
% 	`jed'		simulate ^X^F keybinding
% 
% 
% returns one of the following on the stack
% 	Null_String	- builtin dispatched, no prompt
% 	"pwd"		- builtin dispatched, give prompt
% 	cmd		- use shell to execute CMD
%!%-
define shell_builtin (cmd) %{{{
{
   variable argv, dir, buf, flag, pwd = "pwd";
   variable cmd1 = strcompress (cmd, " \t");

   (,dir,buf,flag) = getbuf_info ();		% cwd info

   argv = extract_element (cmd, 0, ' ');	% parse cmd
   switch (argv)
   % simple "aliases"
     {case pwd: argv; }			% 'pwd'='pwd' (no args)
#ifdef UNIX
     {case "dir": 	% 'dir'='ls -Al' [dir [dir]]
	() = str_replace (cmd, argv, "ls -Al");
     }
#endif
     {case "clear": erase_buffer; pwd;}	% 'clear'=erase_buffer()
     {case "exit":			% 'exit'
	set_buffer_modified_flag (0);
	delbuf (buf);
	Null_String;
     }
     {case "cd":			% 'cd' [dir]
	argv = extract_element (cmd1, 1, ' ');
	if (argv == NULL)
#ifdef MSDOS OS2
	  return pwd;
#else
	argv = "~/";
#endif
	argv = expand_filename (argv);
	if ( change_default_dir (argv) ) insert ("Unable to chdir!\n");
	else setbuf_info (Null_String, argv, buf, flag);
	pwd;			% only "pwd" pending
     }
     {case "jed" or case "e":	% jed
	argv = extract_element (cmd1, 1, ' ');
	if (argv != NULL)
	  () = find_file (expand_filename (dircat (dir, argv)));
	Null_String;		% nothing pending
     }
     {cmd;}		% default = failure, cmd still pending
}

%}}}

define shell_input () %{{{
{
   variable cmd, dir, tmp;

   bol (); skip_chars (Shell_Prompt); skip_white ();
   push_mark_eol ();
   cmd = bufsubstr ();
   eob ();
   bol (); skip_chars (Shell_Prompt); skip_white ();
   !if ( looking_at (cmd) ) insert (cmd);
   eol (); newline ();
   update (0);			% Update now so user see that things are ok

   if ( strlen (cmd) )
     {
	cmd = shell_builtin (cmd);
	!if ( strlen (cmd) )
	  {
	     update (0);
	     return;
	  }

	(,dir,,) = getbuf_info ();	% get directory
	if ( change_default_dir (dir) ) error ("Unable to chdir!");

	if ( strcmp (cmd, "pwd") )	% not "pwd"
	  {
#ifdef VMS
	     tmp = make_tmp_file ("sys$login:_jed_shell_.");
	     variable file = expand_jedlib_file ("vms_shell.com");
	     !if ( strlen (file) ) error ("Unable to find vms_shell.com");
	     () = system (sprintf ("@%s/output= %s %s", file, tmp, cmd));
	     () = insert_file (tmp);
	     () = delete_file (tmp);
#else
#ifdef MSDOS MSWINDOWS WIN32
	     shell_cmd (cmd);
#else
	     shell_cmd (sprintf ("(%s) 2>&1", cmd));
#endif
#endif
	  }
	vinsert ("\n(%s)\n%s", dir, Shell_Prompt);
     }
   else
     {
	!if ( bolp () ) newline ();
	insert (Shell_Prompt);
     }
   set_buffer_modified_flag (0);

#ifdef OS2
   update (1);			% Update due to problems with Borland C
#endif
}

%}}}


%%%%%%%%%%%%%%%%%%%%%%%%%%
