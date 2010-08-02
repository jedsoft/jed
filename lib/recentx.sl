% This file is like recent.sl, except that it logs files according to 
% filename extension.  It draws upon the ideas in recent.sl, as well
% as upon those of the version at
% <http://jedmodes.sourceforge.net/mode/recent>.
%
% To make use of this file, put the following in your .jedrc file:
% 
%    require ("recentx");
%    Recentx_Cache_Filename = ".jedrecent";
%    Recentx_Max_Files = 15;     % The number of files per extension
%    Recentx_Cache_Exclude_Patterns = {"^/tmp/", "\.tmp$"R};
%----------------------------------------------------------------------

#ifdef IBMPC_SYSTEM
custom_variable ("Recentx_Cache_Filename", "_jedrcnt");
#else
custom_variable ("Recentx_Cache_Filename", ".jedrecent");
#endif
%!%+
%\variable{Recentx_Cache_Filename}
%\synopsis{The name of the file used for the recent file cache}
%\usage{Recentx_Cache_Filename = ".jedrecent";}
%\description
% This value of this variable specifies the name of the cache file
% used for recently accessed files.  If the filename is given
% as a relative path, it will be taken as relative to value of the
% Jed_Home_Directory variable, which typically coincides with the
% users's HOME directory.
%\notes
% This variable is defined in \file{recentx.sl}.
%\seealso{Recentx_Cache_Exclude_Patterns, Recentx_Max_Files, Recentx_Use_Cache}
%!%-

custom_variable ("Recentx_Use_Cache", 1);
%!%+
%\variable{Recentx_Use_Cache}
%\synopsis{Turn on/off caching of recent filenames}
%\usage{Recentx_Use_Cache=1;}
%\description
% If non-zero, the recent-filename-cache will be enabled.  Otherwise,
% caching will be turned off.
%\notes
% This variable is defined in \file{recentx.sl}. 
%\seealso{Recentx_Cache_Exclude_Patterns, Recentx_Max_Files, Recentx_Cache_Filename}
%!%-

custom_variable ("Recentx_Max_Files", 15);
%!%+
%\variable{Recentx_Max_Files}
%\synopsis{Set the maximum number of recent-files to cache per extension}
%\usage{Recentx_Max_Files=15;}
%\description
% The value of this variable specifies the maximum number of number of
% files per extension to store in the recent-files-cache.  The maximum number
% of filenames in the cache will be the product of the number of
% extensions and the value of this variable.
%\notes
% This variable is defined in \file{recentx.sl}.
%\seealso{Recentx_Cache_Exclude_Patterns, Recentx_Cache_Filename, Recentx_Use_Cache}
%!%-

custom_variable ("Recentx_Cache_Exclude_Patterns", {});
%!%+
%\variable{Recentx_Cache_Exclude_Patterns}
%\synopsis{List of patterns used to exclude filenames from the recent-files-cache}
%\usage{Recentx_Cache_Exclude_Patterns = \{...\};}
%\description
%  The value of this variable is a list of regular expressions such that
%  if a pathname matches any of the patterns, the file will be
%  excluded from the recent-files-cache.
%\example
%#v+
%   variable Recentx_Cache_Exclude_Patterns = {"^/tmp/", "\\.tmp$"};
%#v-
%  This example excludes any file in the /tmp directory, or any file
%  name with the extension ".tmp".
%\notes
% This variable is defined in \file{recentx.sl}.
%\seealso{Recentx_Cache_Ext_Exclude_Patterns, Recentx_Max_Files, Recentx_Cache_Filename, Recentx_Use_Cache}
%!%-

custom_variable ("Recentx_Cache_Ext_Exclude_Patterns", {});
%!%+
%\variable{Recentx_Cache_Ext_Exclude_Patterns}
%\synopsis{List of patterns used to exclude filename extensions from the recent-files-cache}
%\usage{Recentx_Cache_Ext_Exclude_Patterns = \{...\};}
%\description
%  The value of this variable is a list of regular expressions such that
%  if a filename extension matches any of the patterns, the file will be
%  excluded from the recent-files-cache.
%\example
%#v+
%   variable Recentx_Cache_Ext_Exclude_Patterns = {"~$", "^tmp$", "^[0-9]+"};
%#v-
%  This example excludes any file whose extension ends in \exmp{~}, or
%  is \exmp{"tmp"}, or consists entirely of digits.
%\notes
% This variable is defined in \file{recentx.sl}.
%\seealso{Recentx_Cache_Exclude_Patterns, Recentx_Cache_Filename, Recentx_Use_Cache}
%!%-

private variable Last_Sync_Time = 0;
private define new_recent_files_database ()
{
   return Assoc_Type[Assoc_Type];
}
private variable Recentx_Cache = new_recent_files_database ();

private define get_recent_file_list_name ()
{
   variable file = Recentx_Cache_Filename;

   if (path_is_absolute (file))
     return file;

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


private define add_file_to_database (name, time)
{
   variable ext = path_extname (name), pat;
   ext = strtrim_beg (ext, ".");
   if (strlen (ext)) foreach pat (Recentx_Cache_Ext_Exclude_Patterns)
     {
	try
	  {
	     if (string_match (ext, pat, 1))
	       return;
	  }
	catch AnyError:
	  vmessage ("Exclude pattern may be bad: %S", pat);
     }

   foreach pat (Recentx_Cache_Exclude_Patterns)
     {
	try
	  {
	     if (string_match (name, pat, 1))
	       return;
	  }
	catch AnyError:
	  vmessage ("Exclude pattern may be bad: %S", pat);
     }

   if (ext == "") ext = "(none)";

   ifnot (assoc_key_exists (Recentx_Cache, ext))
     Recentx_Cache[ext] = Assoc_Type[Long_Type];

   variable s = Recentx_Cache[ext];
   s[name] = time;
}

private define update_last_sync_time (file)
{
   variable st = stat_file (file);
   if (st != NULL)
     Last_Sync_Time = st.st_mtime;
   else
     Last_Sync_Time = _time ();
}

private define read_recent_file_list (file)
{
   Recentx_Cache = new_recent_files_database ();
   variable fp = fopen (file, "r");
   if (fp == NULL)
     return;

   variable line;
   variable i = 0;
   foreach line (fp)
     {
	line = strtrim_end (line, "\n");
	ifnot (strlen(line))
	  continue;
	variable fields = strchop (line, '|', 0);

	if (length (fields) == 1)
	  {
	     % old format --- filename only
	     add_file_to_database (fields[0], i);
	     i++;
	  }
	else
	  {
	     add_file_to_database (fields[1], atol(fields[0]));
	  }
     }
   () = fclose (fp);
   update_last_sync_time (file);
}

private define sort_files_by_time (files, times)
{
   variable i = array_sort (times);
   array_reverse (i);
   if (length(i) > Recentx_Max_Files)
     i = i[[0:Recentx_Max_Files-1]];
   return (files[i], times[i]);
}

private define save_recent_file_list ()
{
   variable file = get_recent_file_list_name ();
   variable fp = fopen (file, "wb");
   if (fp == NULL)
     return;

   foreach (assoc_get_values (Recentx_Cache))
     {
	variable item = ();
	variable files, times;
	(files, times) = sort_files_by_time (assoc_get_keys(item), assoc_get_values(item));
	() = array_map (Int_Type, &fprintf, fp, "%ld|%s\n", times, files);
     }
   () = fclose (fp);
   () = chmod (file, 0600);
   update_last_sync_time (file);
}

private define load_recent_file_list ()
{
   variable file = get_recent_file_list_name ();
   variable st = stat_file (file);
   if ((st != NULL) && (st.st_mtime > Last_Sync_Time))
     Recentx_Cache = NULL;

   if (Recentx_Cache == NULL)
     read_recent_file_list (file);
}

private define get_most_recent_files ()
{
   variable times, files, num = 0, item;
   foreach item (assoc_get_values (Recentx_Cache))
     num += length(item);

   times = Long_Type[num];
   files = String_Type[num];

   variable i = 0, ii;
   foreach item (assoc_get_values (Recentx_Cache))
     {
	num = length(item);
	ii = i + [0:num-1];
	times[ii] = assoc_get_values (item);
	files[ii] = assoc_get_keys (item);
	i += num;
     }
   (files, ) = sort_files_by_time (files, times);
   return files;
}

private define get_most_recent_files_by_ext (ext)
{
   variable item = Recentx_Cache[ext];
   variable files;
   (files, ) = sort_files_by_time (assoc_get_keys(item), assoc_get_values(item));
   return files;
}

private define menu_select_file_callback (file)
{
   () = find_file (file);
}

% Build the menu of recent files.
private define display_recent_files_menu (popup, files)
{
   variable i = '1'; % use 1-9 first, then a-z, then A-Z, then give up and restart
   foreach (files)
     {
	variable file = ();
	menu_append_item (popup, sprintf ("&%c %s", i, file), 
			  &menu_select_file_callback, file);
	% check - what should we use?
	switch (i)
	  { case '9': i = 'a' - 1; }
	  { case 'z': i = 'A' - 1; }
	  { case 'Z': i = '1' - 1; }
	i++;
     }
}

private variable Recent_Files_Menu_Name = "&Recent Files";
private variable Recent_Files_Ext_Menu_Name = "Recent Files by Ex&t";

private define recent_files_menu_callback (popup)
{
   load_recent_file_list ();
   variable files = get_most_recent_files ();
   display_recent_files_menu (popup, files);
}

private define recent_files_specific_ext_menu_callback (popup)
{
   variable ext = strchop (popup, '.', 0)[-1];
   variable files = get_most_recent_files_by_ext (ext);
   display_recent_files_menu (popup, files);
}

private define recent_files_ext_menu_callback (popup)
{
   load_recent_file_list ();
   variable exts = assoc_get_keys (Recentx_Cache);
   exts = exts [array_sort (exts)];

   variable menu = "Global.&File." + Recent_Files_Ext_Menu_Name;
   foreach (exts)
     {
	variable ext = ();
	menu_append_popup (menu, ext);
	menu_set_select_popup_callback (strcat (menu, ".", ext),
					&recent_files_specific_ext_menu_callback);
     }
}


% This function is called by _jed_switch_active_buffer_hooks
private define switch_active_buffer_hook (buf)
{
   ifnot (Recentx_Use_Cache)
     return;

   variable file, dir, n;
   variable blist;

   % find out the file name with full path
   (file,dir,buf,) = getbuf_info ();
   ifnot (strlen (file))
     return;

   file = dircat (dir, file);
   load_recent_file_list ();
   add_file_to_database (file, _time ());

   save_recent_file_list ();
}

append_to_hook ("_jed_switch_active_buffer_hooks", &switch_active_buffer_hook);

private define add_recent_files_popup_hooks (menubar)
{
   ifnot (Recentx_Use_Cache)
     return;

   variable menu = "Global.&File";

   menu_append_separator (menu);
   variable dmenu = Recent_Files_Menu_Name;
   menu_append_popup (menu, dmenu);
   menu_set_select_popup_callback (strcat (menu, ".", dmenu),
				   &recent_files_menu_callback);
   dmenu = Recent_Files_Ext_Menu_Name;
   menu_append_popup (menu, dmenu);
   menu_set_select_popup_callback (strcat (menu, ".", dmenu),
				   &recent_files_ext_menu_callback);
}
append_to_hook ("load_popup_hooks", &add_recent_files_popup_hooks);

% End of file recent.sl
