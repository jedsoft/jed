% Asynchronous compilation

private variable Compile_Last_Compile_Cmd = "";
public variable Compile_Process_Id = -1;

private define compile_set_status_line (state)
{
   set_mode ("compile: " + state, 0);
}

private define compile_signal_handler (pid, flags, status)
{
   variable str = aprocess_stringify_status (pid, flags, status);

   push_spot ();
   eob ();
   vinsert ("\n\nProcess no longer running: %s\n", str);
   pop_spot ();
   
   compile_set_status_line (str);
   
   if (flags != 2) Compile_Process_Id = -1;
}

	
private define compile_start_process (cmd)
{
   variable dir, name, file, flags;
   variable shell, shopt;

   if (cmd == NULL)
     cmd = read_mini ("Compile command:", "", Compile_Last_Compile_Cmd);
   !if (strlen (cmd))
     return;

   (,dir,,) = getbuf_info ();
   if (change_default_dir (dir)) 
     error ("Unable to chdir.");
   
   pop2buf (Compile_Output_Buffer);

   set_readonly (0);
   erase_buffer ();
   (file,,name,flags) = getbuf_info ();
   setbuf_info (file, dir, name, flags);
   Compile_Line_Mark = 0;
   
   compile_set_status_line ("");
   insert (cmd); newline ();

#ifdef WIN32
   shopt = "";
   shell = "";
#else
   shell = getenv ("SHELL");
   if (shell == NULL) shell = "sh";
   shopt = "-c";   
#endif

   Compile_Process_Id = open_process (shell,shopt, cmd, 2);

   if (Compile_Process_Id == -1)
     error ("Unable to start subprocess.");
   
   compile_set_status_line ("run");
   Compile_Last_Compile_Cmd = cmd;
   
   set_process (Compile_Process_Id, "signal", &compile_signal_handler);
   set_process (Compile_Process_Id, "output", "@");
}



public define compile ()
{
   variable b, n;
   variable cmd = NULL;

   if (_NARGS == 1)
     cmd = ();

   Compile_Output_Buffer = "*compile*";
   
   if (Compile_Process_Id != -1)
     {
	if (bufferp (Compile_Output_Buffer))
	  error ("A compile process is already running.");
	try kill_process (Compile_Process_Id); 
	catch RunTimeError;
	Compile_Process_Id = -1;
     }
	
   
   b = whatbuf();
   call ("save_some_buffers");
   
   compile_start_process (cmd);
   
   pop2buf(b);
   
   %compile_parse_errors ();
}
