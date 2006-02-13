%%
%%  Most/more/less file viewing
%%  

define most_edit ()
{
   set_readonly(0);
   getbuf_info(); pop(); pop(); pop(); 
   file_type(); runhooks("mode_hook");
   set_status_line("", 0);
}

variable Most_N_Windows = 1;

define most_exit_most ()
{
   variable flags;
   most_edit ();
   !if (buffer_modified ())
     delbuf (whatbuf ());
   otherwindow ();
   if (Most_N_Windows == 2) onewindow ();
   message ("Done.");
}

define most_help ()
{
   message (
	    "SPC:pg dn, DEL:pg up, /:search forw, ?:search back, q:quit, e:edit, h:this help"
	    ); 
}

variable Most_Map = "Most";

!if (keymap_p (Most_Map))
{  
   make_keymap (Most_Map);
   _for ('A', 'z', 1) 
     {
	undefinekey (char (), Most_Map);
     }
   
   definekey ("page_up", char (127), Most_Map);   %  delete
   definekey ("page_down", char (' '),  Most_Map);
   definekey ("most_exit_most", char ('q'),  Most_Map);
   definekey ("most_edit", char ('e'),	Most_Map);
   definekey ("most_find_f", char ('s'), Most_Map);
   definekey ("most_find_f", char ('S'), Most_Map);
   definekey ("most_find_f", char ('/'), Most_Map);
   definekey ("most_find_b", char ('?'), Most_Map);
   definekey ("most_find_n", char ('N'), Most_Map);
   definekey ("most_help", char ('h'),	Most_Map);
   definekey ("eob", char ('B'), Most_Map);
   definekey ("bob", char ('T'), Most_Map);
}


%!%+
%\function{most_mode}
%\synopsis{most_mode}
%\description
% Emulates MOST fileviewer
% The following keys are defined:
%#v+
% SPACE            next screen
% DELETE           previous screen
% /                search_forward
% ?                search_backward
% n                find next occurrence
% q                quit most mode  (usually kills buffer if not modified)
% e                edit buffer
% h                help summary
% t                Top of Buffer
% b                End of Buffer
%#v-
%!%-
define most_mode ()
{
   use_keymap (Most_Map);
   set_readonly (1);
   set_mode (Most_Map, 0);
   set_status_line (" MOST : press 'h' for help.   %b    (%p)", 0);
   Most_N_Windows = nwindows ();
}
  
variable Most_Search_Dir = 1;

     
define most_find_n ()
{
   variable r;
   !if (strlen (LAST_SEARCH))
     error ("Find What?");
   
   if (Most_Search_Dir > 0)
     {
	r = right (1);
	if (fsearch (LAST_SEARCH)) return;
	go_left (r);
     }
   else
     {
	if (bsearch (LAST_SEARCH)) return;
     }
   
   error ("Not Found.");
}

define most_find_b ()
{
   Most_Search_Dir = -1;
   search_backward ();
}

define most_find_f ()
{
   Most_Search_Dir = 1;
   search_forward ();
}

%%% Emulate MOST fileviewer.
variable Most_Min_Arg, Most_Current_Arg;

define run_most (arg)
{
   Most_Min_Arg = arg;
   Most_Current_Arg = arg;

   if (arg != __argc) () = find_file (__argv[arg]);
   most_mode ();
   definekey ("most_next_file", ":N", Most_Map);
   definekey ("exit_jed", "q", Most_Map);
   definekey ("exit_jed", "Q", Most_Map);
   definekey ("most_exit_most", ":Q", Most_Map);
   help_for_help_string = 
   "Most Mode: SPC- next screen, DEL- prev screen, Q Quit, :n next file, :Q edit";
}

define most_next_file ()
{
   variable f, file, dir = 1;

   forever 
     {
	Most_Current_Arg += dir;
	if (Most_Current_Arg == __argc) Most_Current_Arg = Most_Min_Arg;
	if (Most_Current_Arg < Most_Min_Arg) Most_Current_Arg = __argc - 1;
     
	file = __argv[Most_Current_Arg];
	flush (strcat ("Next File (Use Arrow keys to select): ", file));
	(,f) = get_key_binding ();
	if (typeof (f) == String_Type)
	  {
	     if (f == "previous_line_cmd")
	       {
		  dir = 1;
		  continue;
	       }
	     
	     if (f == "next_line_cmd")
	       {
		  dir = -1;
		  continue;
	       }
	     break;
	  }
	else beep ();
     } 
   () = find_file (file);
   message (Null_String);
   most_mode ();
}

provide ("most");
