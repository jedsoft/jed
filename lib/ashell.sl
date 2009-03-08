%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% async shell for jed
!if (is_defined ("Shell_Default_Shell"))
{
#ifdef WIN32
   variable Shell_Default_Shell = getenv ("COMSPEC");
   if (Shell_Default_Shell == NULL)
     Shell_Default_Shell = "cmd.exe";
#else   
   variable Shell_Default_Shell = getenv ("SHELL");
   if (Shell_Default_Shell == NULL)
     Shell_Default_Shell = "sh";
#endif
}

!if (is_defined ("Shell_Default_Interactive_Shell"))
{
#ifdef WIN32
   variable Shell_Default_Interactive_Shell = "";
#else   
   variable Shell_Default_Interactive_Shell = "-i";
#endif  
}

variable AShell_Id = -1;

private variable Current_Working_Directory = ".";

public define ashell_expand_path (path)
{
   if ((path == NULL) or (path == "~"))
     path = "~/";
   
#ifdef UNIX
   if (path[0] == '~')
     {
	variable user = extract_element (path, 0, '/');
	if (strlen (user) > 1)
	  {
	     variable dir;
	     (dir,,,,) = get_passwd_info (user[[1:]]);
	     if (strlen (dir))
	       (path,) = strreplace (path, user, dir, 1);
	  }
     }
#endif

   variable cwd = getcwd ();
   () = chdir (Current_Working_Directory);
   path = expand_filename (path);
   () = chdir (cwd);
   return path;
}

public define ashell_getcwd ()
{
   return Current_Working_Directory;
}

public define ashell_chdir (dir)
{
   dir = ashell_expand_path (dir);
   if (-1 == chdir (dir))
     verror ("chdir %s failed: %s", dir, errno_string (errno));

   Current_Working_Directory = dir;
}

private define builtin_cd (cmd, argc, argv)
{
   variable arg = ashell_expand_path (argv[1]);

   if (0 == chdir (arg))
     Current_Working_Directory = arg;

   return cmd;
}

private define builtin_edit (cmd, argc, argv)
{
   variable dir = Current_Working_Directory;

   foreach (argv[[1:argc-1]])
     {
	variable file = ();
	
	() = find_file (expand_filename (dircat (dir, file)));
     }

   return "";
}

private define builtin_most (cmd, argc, argv)
{
   ERROR_BLOCK
     {
	_clear_error ();
	insert ("\nUnable to read file.\n");
	return "";
     }
	
   if (argc < 2)
     return cmd;

   () = read_file (argv[1]);
   pop2buf (whatbuf ());
   most_mode ();

   return "";
}

private variable Builtin_Cmds = Assoc_Type [Ref_Type];

public define ashell_add_builtin (cmd, fun)
{
   Builtin_Cmds[cmd] = fun;
}

ashell_add_builtin ("cd", &builtin_cd);
ashell_add_builtin ("edit", &builtin_edit);
ashell_add_builtin ("jed", &builtin_edit);
ashell_add_builtin ("most", &builtin_most);
ashell_add_builtin ("more", &builtin_most);
ashell_add_builtin ("less", &builtin_most);

private define parse_shell_cmd (cmd)
{
   variable argv, argc;
   
   cmd = strtok (cmd);
   argc = length (cmd);
   
   % This has the effect of NULL terminating the argv list
   argv = String_Type[argc+1];
   
   %  remember that in slang1 [0:-1] picks out elements in an array indexing 
   %  context.  So, only do this if argc is non-zero.  slang2 has different
   %  semantics and does not suffer from this problem.
   if (argc)
     argv[[0:argc-1]] = cmd;

   return (argc, argv);
}

private define try_builtin (cmd)
{
   variable argc, argv;
   
   (argc, argv) = parse_shell_cmd (cmd);
   !if (argc) return cmd;

   variable fun, command;

   command = argv[0];
   !if (assoc_key_exists (Builtin_Cmds, command))
     return cmd;

   fun = Builtin_Cmds[command];
   return @fun (cmd, argc, argv);
}

define ashell_send_input ()
{
   variable buf;
   variable this_line, mark_line;
   variable m, ch, prompt;

   m = process_mark (AShell_Id);

   this_line = what_line ();
   push_mark ();
   goto_user_mark (m);
   mark_line = what_line ();

   if (this_line >= mark_line)
     {
	pop_mark_0 ();
	push_mark_eob ();
	buf = bufsubstr ();
     }
   else
     {
	bskip_chars ("\t ");
	push_mark ();
	!if (bolp ())
	  {
	     go_left_1 ();
	     ch = what_char ();
	  }
	bol ();
	prompt = bufsubstr ();
	pop_mark_1 ();
	bol ();
	if (looking_at (prompt))
	  {
	     go_right (strlen (prompt));
	  }
	else if (ffind_char (ch))
	  {
	     go_right_1 ();
	  }
	push_mark_eol ();
	buf = bufsubstr ();
	eob ();
	insert (buf);
     }
   newline ();
   move_user_mark (m);
   
   buf = try_builtin (buf);

#ifdef WIN32
   send_process (AShell_Id, buf + "\n\r");
#else   
   send_process (AShell_Id, buf + "\n");
#endif
}

define ashell_send_intr ()
{
   signal_fg_process (AShell_Id, 2);   %  SIGINT
}

define ashell_completion ()
{
   variable partial_completion;
   variable dir, file;

   push_spot ();
   push_mark ();
   bskip_chars ("^ \n\t'`\"><$");
   
   partial_completion = bufsubstr();
   pop_spot ();
   
   (dir, file) = parse_filename (partial_completion);
   dir = ashell_expand_path (dir);
   
   variable len = strlen (file);
   variable files = listdir (dir);
   files = files[where (0 == array_map (Int_Type, &strncmp, files, file, len))];
   
   variable num_matches = length (files);
   if (num_matches == 0)
     {
	message ("No completions");
	return;
     }

   variable match;

   variable i;
   _for (0, num_matches-1, 1)
     {
	i = ();
	match = files[i];
	if (2 == file_status (path_concat (dir, match)))
	  files[i] = path_concat (match, "");   %  add /
     }

   match = files[0];
   if (num_matches == 1)
     {
	insert (match[[len:]]);
	return;
     }

   % Complete as much as possible.  By construction, the first len characters
   % in the matches list are the same.  Start from there.
   _for (len, strlen (match)-1, 1)
     {
	i = ();
	variable try_match = match[[0:i]];
	if (num_matches != length (where (0==array_map (Int_Type, &strncmp,
							try_match, files, i+1))))
	  {
	     if (i != len)
	       insert (match[[len:i-1]]);
	     break;
	  }
     }

   variable cbuf = whatbuf ();
   pop2buf ("*completions*");
   erase_buffer ();
   foreach (files)
     {
	insert (()); 
	newline();
     }
   
   buffer_format_in_columns ();
   bob ();
   pop2buf (cbuf);
   message ("Ambiguous Completions");
}

$1 = "AShellMap";
!if (keymap_p ($1)) make_keymap ($1);
definekey ("ashell_send_input", "^M", $1);
undefinekey ("^C", $1);
definekey ("ashell_send_intr", "^C", $1);
definekey ("ashell_completion", "\t", $1);

define ashell_signal_handler (pid, flags, status)
{
   variable msg;

   eob ();
   msg = aprocess_stringify_status (pid, flags, status);
   vinsert ("\n\n----- %s ------\n\n", msg);
   AShell_Id = -1;
}

define ashell_insert_output (pid, str)
{
   goto_user_mark (process_mark (pid));
   variable use_overwrite = not eolp ();
   foreach (str) using ("bytes")
     {
	variable ch = ();
	if (ch == '\r')
	  {
	     bol ();
	     use_overwrite = 1;
	     continue;
	  }
	if (ch == 8)
	  {
	     ifnot (bolp ())
	       go_left(1);
	     use_overwrite = 1;
	     continue;
	  }
	if (ch == '\n')
	  {
	     eol ();
	     newline ();
	     use_overwrite = 0;
	     continue;
	  }

	if (use_overwrite) del ();
	insert_byte (ch);
     }
   variable col = what_column ();
   eol_trim ();
   goto_column (col);
   move_user_mark (process_mark (pid));
}

   
define ashell ()
{
   variable buf = "*ashell*";
   variable arg, nargs = 0;

   if ((AShell_Id != -1) and bufferp (buf))
     {
	pop2buf (buf);
	error ("Currently, only one shell process is supported.");
     }

   pop2buf (buf);
   (,Current_Working_Directory,,) = getbuf_info ();
   () = chdir (Current_Working_Directory);
   
   use_keymap ("AShellMap");
   run_mode_hooks ("ashell_mode_hook");
   erase_buffer ();
#iffalse
   AShell_Id = open_process (Shell_Default_Shell, 
			     Shell_Default_Interactive_Shell, 1);
#else
   % parse possible arguments
   forever
     {
	arg = extract_element (Shell_Default_Shell, nargs, ' ');
	if (arg == NULL)
	  break;

	nargs++;
	arg;		% push on stack
     }

   Shell_Default_Interactive_Shell; nargs;% push on stack  
   AShell_Id = open_process ();
#endif
   set_process (AShell_Id, "signal", "ashell_signal_handler");
   set_process (AShell_Id, "output", "ashell_insert_output");
}

provide ("ashell");
