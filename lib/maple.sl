%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
custom_variable ("Maple_Program", "maple");
custom_variable ("Maple_Prompt_Regexp", "^[a-zA-Z]*>");

variable Maple_Pid = -1;

define maple_send_input ()
{
   variable buf;
   variable this_line, mark_line;
   variable m, len;

   m = process_mark (Maple_Pid);

   this_line = what_line ();
   push_mark ();
   goto_user_mark (m);
   mark_line = what_line ();

   if (this_line >= mark_line)
     {
	% Cursor is after point of last output.  Just send everything
	% from last output to end of buffer.
	pop_mark_0 ();
	push_mark_eob ();
	buf = bufsubstr ();
     }
   else
     {
	% it looks like user wants to re-run this line.  Fine.
	pop_mark_1 ();
	eol ();
	push_mark ();
	len = re_bsearch (Maple_Prompt_Regexp);	
	!if (len)
	  {
	     pop_mark_0 ();
	     return;
	  }
	go_right (len - 1);
	buf = bufsubstr ();
	eob ();
	insert (buf);
     }
   newline ();
   move_user_mark (m);
   send_process (Maple_Pid, strcat (buf, "\n"));
}

$1 = "MapleMap";
!if (keymap_p ($1)) make_keymap ($1);
definekey ("maple_send_input", "^M", $1);

define maple_signal_handler (pid, flags, status)
{
   variable msg;
   eob ();
   msg = aprocess_stringify_status (pid, flags, status);
   vinsert ("\n\n----- %s ------\n\n", msg);
   if (flags != 2)
     Maple_Pid = -1;
}

define maple_insert_output (pid, str)
{
   eob ();
   push_spot ();
   insert (str);
   pop_spot ();
   bol ();
   replace ("\r", Null_String);
   eob ();
   move_user_mark (process_mark (pid));
}

   
define maple ()
{
   variable buf = "*maple*";
   variable arg, nargs = 0;

   if ((Maple_Pid != -1) and bufferp (buf))
     {
	error ("Currently, only one maple process is supported.");
     }

   pop2buf (buf);
   use_keymap ("MapleMap");
   run_mode_hooks ("maple_mode_hook");
   erase_buffer ();

   % parse possible arguments
   forever
     {
	arg = extract_element (Maple_Program, nargs, ' ');
	if (arg == NULL)
	  break;

	nargs++;
	arg;		% push on stack
     }

   nargs - 1;			% push on stack
   Maple_Pid = open_process ();
   set_process (Maple_Pid, "signal", "maple_signal_handler");
   set_process (Maple_Pid, "output", "maple_insert_output");
}
