variable Last_Process_Command = Null_String;

define process_region ()
{
   variable cmd, tmp_file;
   cmd = read_mini ("Pipe to command:", Last_Process_Command, Null_String);
   !if (strlen (cmd)) return;
   
   Last_Process_Command = cmd;
   
   tmp_file = make_tmp_file ("/tmp/jedpipe");
   cmd = strncat (cmd, " > ", tmp_file, " 2>&1", 4);
   
   !if (dupmark ()) error ("Mark not set.");
   
   if (pipe_region (cmd)) 
     {
	error ("Process returned a non-zero exit status.");
     }
   del_region ();
   () = insert_file (tmp_file);
   () = delete_file (tmp_file);
}

   
   
