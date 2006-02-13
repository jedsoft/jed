% This File is OBSOLETE

%
%  Example of menus for JED.  This is designed to aid in learning
%  the editor since after a menu item has been chosen, the user is 
%  informed of the keybinding of the function.
%

enable_top_status_line (1);
if (TOP_WINDOW_ROW == 1) error ("Menu bar has been disabled.  It must be present to use the menus.");

define menu_build_display_string (list)
{
   variable i = 0, cmd, s = Null_String;
   
   forever 
     {
	cmd = extract_element (list, i, ',');
	if (cmd == NULL) break;
	if (i) s += ", ";
	s = sprintf("%s(%d)%s", s, i, cmd);
	++i;
     }
   s;
}


define menu_list_length (list)
{
   variable i = 0;
   
   i - 1;
}

variable Menu_Last_Cmd;
variable Menu_Abort;  Menu_Abort = 0;

define menu_select_cmd (s_list, cmd_list)
{
   variable cmd, i, s, n;
   i = 0;
  
   n = 0; 
   while (NULL != extract_element(cmd_list, n, ',')) n++;
   n--;
  
   s = strcat ("Menu: ", menu_build_display_string(s_list));
   
   
   do
     {
	Menu_Abort = 0;

	forever 
	  {
	     message ("Pick a number or press Ctrl-G to abort or SPACE to go back to previous level.");
	     set_top_status_line (s); pop (); update_sans_update_hook(1);
	     i = getkey();
	     if (i == 7) call ("kbd_quit");
	     if (i == ' ')
	       {
		  Menu_Abort = 1;
		  return;
	       }

	     i -= '0';
  
	     if ((i >= 0) and (i <= n)) break;
    

	     beep();
	     flush_input();
	     % flush("Pick a number or press Ctrl-G to abort or SPACE to go back.");
	     % pop(input_pending(20));
	  }
   
	cmd = extract_element(cmd_list, i, ',');
  
	if (cmd [0] == '@')
	  {	
	     Menu_Last_Cmd = substr(cmd, 2, strlen(cmd));
	     call (Menu_Last_Cmd);
	  }
	else
	  {
	     Menu_Last_Cmd = cmd;
	     eval(strcat (".", cmd));  %  Use RPN--- no need to parse it
	  }
     }
   while(Menu_Abort);
}

define menu_window_cmds ()
{
   menu_select_cmd ("One window,Split Window,Other Window,Del Window,Redraw",
		    "@one_window,@split_window,@other_window,@delete_window,@redraw");
}


define search_replace ()
{
   menu_select_cmd ("Search Forward,Search Backward,Replace",
		    "search_forward,search_backward,replace_cmd");
}


define menu_deletion_cmd ()
{
   menu_select_cmd ("Delete Char,Delete Word",
		    "del,delete_word");
}



define menu_cut_paste ()
{
   menu_select_cmd ("Deleting,Begin Region,Cut Region,Copy Region,Paste",
		    "menu_deletion_cmd,@set_mark_cmd,@kill_region,@copy_region,@yank");
}



define menu_movement ()
{
   menu_select_cmd ("Page Up,Page Down,Top,Bottom,Page Left,Page Right",
		    "@page_up,@page_down,@beg_of_buffer,@end_of_buffer,@scroll_right,@scroll_left");
}



define menu_buffer_modes ()
{
   menu_select_cmd ("C-Mode,Text-Mode,No-Mode,Fortran-Mode",
		    "c_mode,text_mode,no_mode,fortran");
}

define menu_basics ()
{
   menu_select_cmd ("Search/Replace,Movement,Cut/Paste,Undo,Formatting",
		    "search_replace,menu_movement,menu_cut_paste,@undo,@format_paragraph");
}


define menu_buffers ()
{
   menu_select_cmd ("Kill Buffer,Switch To Buffer,List Buffers,Set buffer modes",
		    "@kill_buffer,@switch_to_buffer,list_buffers,menu_buffer_modes");
}


define menu_files ()
{
   menu_select_cmd ("Open File,Save to File,Save Buffers,Insert File",
		    "@find_file,@write_buffer,@save_some_buffers,@insert_file");
}



define menu_help ()
{
   menu_select_cmd ("Pop up help,Browse Documentation,Show Key,Where is Command",
		    "help,info_mode,showkey,where_is");
}


define menu_misc ()
{
#ifdef UNIX VMS
   menu_select_cmd ("compose mail,send mail,ispell,shell command,suspend",
		    "mail,mail_send,ispell,do_shell_cmd,@sys_spawn_cmd");
#else
   menu_select_cmd ("Shell command,Suspend",
		    "do_shell_cmd,@sys_spawn_cmd");
#endif
}



define menu_main_cmds ()
{
   variable n, key, len, next, nlen;
   
   ERROR_BLOCK 
     {
	set_top_status_line (Global_Top_Status_Line);
	pop ();
     }
   
   menu_select_cmd ("File,Edit,Buffers,Windows,Help,Misc,Exit",
		    "menu_files,menu_basics,menu_buffers,menu_window_cmds,menu_help,menu_misc,@exit_jed");

   EXECUTE_ERROR_BLOCK ();
   
     %
     %  Show user keybinding of the function.
     %  The problem with this is that any function which leaves a message
     %  in the message buffer, the message will be destroyed.  Here I 
     %  update screen and sleep for a second before continuing.  Menus are
     %  intrinsically inefficient anyway.
     %
  
   if (strlen(MESSAGE_BUFFER))
     { 
	update_sans_update_hook (0);
	() = input_pending(20);
     }
   
   if (Menu_Abort)
     {
	return; 
     }
   
   if (n = which_key (Menu_Last_Cmd), n)
     {
	key = (); n--;
	
	len = strlen (key);
	loop (n)
	  {
	     next = ();
	     nlen = strlen (next);
	     if (nlen < len)
	       {
		  len = nlen;
		  key = next;
	       }
	  }

	sprintf ("%s is on key %s", Menu_Last_Cmd, expand_keystring (key));
     }
   else strcat (Menu_Last_Cmd, " is not on any keys.");
   message (());
}

