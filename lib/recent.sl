% File:          recent.sl      -*- SLang -*-
%
% Author:        Guido Gonzato, <ggonza@tin.it>
% 
% Version:       1.0.1. This file provides easy access to recently
%                accessed files.
%
% Installation:  to install this feature globally, load recent.sl from site.sl
%                and insert the following lines in defaults.sl:
%
%                  variable RECENT_FILES_LIST = ".jedrecent";    % or other
%                  variable MAX_RECENT_FILES  = 20;              % ditto
%
%                For personal customisation, insert these lines in your .jedrc:
% 
%                  % WANT_RECENT_FILES_LIST = 1 % set this to 0 to disable
%                  % RECENT_FILES_LIST = ".jedrecent"; % uncomment to customise
%                  % MAX_RECENT_FILES  = 10;
%
% Last updated:  17 April 2001

custom_variable ("WANT_RECENT_FILES_LIST", 1);
#ifdef IBMPC_SYSTEM
custom_variable ("RECENT_FILES_LIST", "_jedrcnt");
#else
custom_variable ("RECENT_FILES_LIST", ".jedrecent");
#endif
custom_variable ("MAX_RECENT_FILES", 15);

% -----

private variable Recent_Files_Buf = " *recent files*";
private variable List_Of_Buffers = NULL;

private define get_recent_file_list_name ()
{
   variable file = RECENT_FILES_LIST;
   
% Versions of slang prior to 1.4.3 have a bug in the path_is_absolute function.
   if (_slang_version >= 10403)
     {
	if (path_is_absolute (file))
	  return file;
     }

   variable dir = Jed_Home_Directory;
#ifdef IBMPC_SYSTEM
   if (dir == "")
     {
	dir = getenv ("TEMP");
	if (dir == NULL)
	  dir = "";
     }
#endif
   return dircat (dir, file);
}

% Load the list of recent files in recent_files_buf.
private define load_recent_file_list ()
{
   variable file = get_recent_file_list_name ();

   if (bufferp (Recent_Files_Buf))
     {
	setbuf (Recent_Files_Buf);
	if (file_changed_on_disk (file))
	  delbuf (Recent_Files_Buf);
     }

   !if (bufferp (Recent_Files_Buf))
     {
	() = read_file (file);
	rename_buffer (Recent_Files_Buf);
     }
   bob ();
}


% Build the menu of recent files.
public define recent_files_menu_callback (popup)
{
  variable buf, tmp, i, cmd;
  variable menu;

  buf = whatbuf ();
  load_recent_file_list ();  % load the list of recent files
  bob ();
  
  i = '1'; % use 1-9 first, then a-z, then A-Z, then give up and restart
   
  forever
     {
	tmp = line_as_string ();
	!if (strlen (tmp))
	  break;
	
	cmd = sprintf ("()=find_file (\"%s\")", tmp);
	(cmd, ) = strreplace (cmd, "\\", "\\\\", strlen (cmd)); % fix DOS
	menu_append_item (popup, sprintf ("&%c %s", i, tmp), cmd);
	go_down_1 ();

	% check - what should we use?
	switch (i)
	  { case '9': i = 'a' - 1; }
	  { case 'z': i = 'A' - 1; }
	  { case 'Z': i = '1' - 1; }
	i++;
     }
  
  setbuf (buf);
}


% This function is called by _jed_switch_active_buffer_hooks
public define append_recent_files (buf)
{
   variable file, dir, n;
   variable blist;

   !if (WANT_RECENT_FILES_LIST)
     return;

   % find out the file name with full path
   (file,dir,buf,) = getbuf_info ();
   !if (strlen (file))
     return;

   blist = [buffer_list (), pop ()];

   file = dircat (dir, file);
   load_recent_file_list ();

   EXIT_BLOCK
     {
	sw2buf (buf);
	List_Of_Buffers = blist;
     }
   
   if (is_readonly ())
     return;			       %  no permission to modify buffer/list

   % Check to see if this one is already on the list
   bob ();
   if (file == line_as_string ())
     return;

   % Only add the file to the list if it is not on the list, or it was 
   % not in the previously checked list of buffers.
   
   if (List_Of_Buffers != NULL)
     {
	if (any (List_Of_Buffers == buf))
	  return;
     }

  if (bol_fsearch (file))
    {
       go_right (strlen (file));
       if (eolp ())
	 delete_line ();
    }
   bob ();
   vinsert ("%s\n", file);

   % are there more entries than allowed?
   goto_line (MAX_RECENT_FILES);
   if (down_1 ())
     {
	bol ();
	push_mark ();
	eob ();
	del_region ();
     }
   
   ERROR_BLOCK
     {
	_clear_error ();
     }
   file = get_recent_file_list_name ();
   () = write_buffer (file);
   () = chmod (file, 0600);
}

%add_to_hook ("_jed_find_file_after_hooks", &append_recent_files);
append_to_hook ("_jed_switch_active_buffer_hooks", &append_recent_files);

private define add_recent_files_popup_hook (menubar)
{
   !if (WANT_RECENT_FILES_LIST)
     return;

   variable menu = "Global.&File";
   
   menu_append_separator (menu);
   menu_append_popup (menu, "&Recent Files");
   menu_set_select_popup_callback (strcat (menu, ".&Recent Files"),
				   &recent_files_menu_callback);
}

#ifexists _jed_run_hooks
append_to_hook ("load_popup_hooks", &add_recent_files_popup_hook);
#else
variable Menu_Load_Popups_Hook = &add_recent_files_popup_hook;
#endif

provide ("recent");
% End of file recent.sl
