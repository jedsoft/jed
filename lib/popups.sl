% Main menu function autoloaded by menus.sl 

#ifndef VMS
private define add_files_popup_with_callback (parent, popup, dir, pattern, fun)
{
   variable files;
   variable i;

   files = listdir (dir);
   if (files == NULL)
     return;

   i = where (array_map (Int_Type, &string_match, files, pattern, 1));
   if (length (i) == 0)
     return;
   
   files = files[i];
   files = files[array_sort (files)];

   menu_append_popup (parent, popup);
   popup = parent + "." + popup;
   
   foreach (files)
     {
	variable file = ();
	file = path_sans_extname(path_sans_extname (file));   %  could have foo.txt.gz
	menu_append_item (popup, file, fun, file);
     }
}

private define browse_docs_callback (file)
{
   file = dircat (JED_ROOT, "doc/txt/" + file + ".txt");
   variable is_compressed = 0;

   if (file_status (file) != 1)
     {
	foreach ([".gz", ".bz2", ".Z"])
	  {
	     variable ext = ();
	     if (file_status (file + ext) != 1)
	       continue;

	     file = file + ext;
	     is_compressed = 1;
	     break;
	  }
     }
   variable state;
#ifdef UNIX
   if (is_compressed)
     auto_compression_mode (1, &state);
#endif
   () = read_file (file);
#ifdef UNIX
   if (is_compressed) 
     auto_compression_mode (state);
#endif
   pop2buf (whatbuf ());
   most_mode ();
}
#endif

private define close_file (clientdata)
{
   delbuf (whatbuf ());
}


$1 = "Global.&File";
menu_append_item ($1, "&Open", "find_file");
menu_append_item ($1, "&Close", &close_file, NULL);
menu_append_item ($1, "&Save", "save_buffer");
menu_append_item ($1, "Save &As", "write_buffer");
menu_append_item ($1, "Save &Buffers", "save_some_buffers");
menu_append_item ($1, "&Insert File", "insert_file");
#iffalse
menu_append_separator ($1);
menu_append_popup ($1, "&Version Control");
#endif
menu_append_separator ($1);
menu_append_item ($1, "Canc&el Operation", "kbd_quit");
menu_append_item ($1, "E&xit", "exit_jed");

#iffalse
$1 = "Global.&File.&Version Control";
menu_append_item ($1, "RCS &Open", "rcs_open_file"); % rcs.sl required
menu_append_item ($1, "RCS &Check In/Out", "rcs_check_in_and_out");
menu_append_item ($1, "Numbered Bac&kups On", "backups_on");
menu_append_item ($1, "Numbered Back&ups Off", "backups_off");
#endif

$1 = "Global.&Edit";
menu_append_item ($1, "&Begin Region/Rect", "smart_set_mark_cmd");
menu_append_item ($1, "&Cut Region", "yp_kill_region");
menu_append_item ($1, "C&opy Region", "yp_copy_region_as_kill");
menu_append_item ($1, "&Paste", "yp_yank");
menu_append_popup ($1, "Re&gion Ops");
menu_append_popup ($1, "&Rectangles");
menu_append_separator ($1);
menu_append_popup ($1, "&Key Macros");
menu_append_separator ($1);
menu_append_item ($1, "&Undo", "undo");

$1 = "Global.&Edit.&Key Macros";
menu_append_item ($1, "&Start Macro", "begin_macro");
menu_append_item ($1, "S&top Macro", "end_macro");
menu_append_item ($1, "Replay &Last Macro", "execute_macro");

$1 = "Global.&Edit.&Rectangles";
menu_append_item ($1, "&Cut Rectangle", "kill_rect");
menu_append_item ($1, "C&opy Rectangle", "copy_rect");
menu_append_item ($1, "&Paste Rectangle", "insert_rect");
menu_append_item ($1, "Op&en Rectangle", "open_rect");
menu_append_item ($1, "&Blank Rectangle", "blank_rect");

$1 = "Global.&Edit.Re&gion Ops";
menu_append_item ($1, "&Upper Case", ".'u' xform_region");
menu_append_item ($1, "&Lower Case", ".'d' xform_region");
menu_append_item ($1, "&Comment", "comment_region");
menu_append_item ($1, "U&ncomment", "uncomment_region");
menu_append_separator ($1);
menu_append_item ($1, "&Save to File", "write_region");
menu_append_item ($1, "&Append to File", "append_region");
menu_append_separator ($1);
menu_append_item ($1, "Copy To &Register", "reg_copy_to_register");
menu_append_item ($1, "&Paste From Register", "reg_insert_register");
menu_append_item ($1, "&View Registers", "register_mode");

$1 = "Global.&Search";
menu_append_item ($1, "Search &Forward", "search_forward");
menu_append_item ($1, "Search &Backward", "search_backward");
menu_append_item ($1, "R&egexp Search Forward", "re_search_forward");
menu_append_item ($1, "Re&gexp Search Backward", "re_search_backward");
menu_append_separator ($1);
menu_append_item ($1, "&Replace", "replace_cmd");
menu_append_item ($1, "Regexp Re&place", "query_replace_match");
menu_append_separator ($1);
menu_append_item ($1, "Se&t Bookmark", "bkmrk_set_mark");
menu_append_item ($1, "Got&o Bookmark", "bkmrk_goto_mark");
menu_append_item ($1, "&Goto Line", "goto_line_cmd");


private define change_buffer_callback (popup)
{
   loop (buffer_list ())
     {
	variable b = ();
	if (b[0] == ' ')
	  continue;

	menu_append_item (popup, b, &sw2buf, b);
     }
}


$1 = "Global.&Buffers";
menu_append_popup ($1, "&Toggle");
menu_append_popup ($1, "&Change Buffer");
menu_set_select_popup_callback (strcat ($1, ".&Change Buffer"), &change_buffer_callback);
menu_append_item ($1, "&Kill Buffer", "kill_buffer");
menu_append_item ($1, "&List Buffers", "bufed");
menu_append_popup ($1, "&Select Mode");
menu_append_item ($1, "Enable &Folding", "folding_mode");

$1 = "Global.&Buffers.&Select Mode";
menu_append_item ($1, "&C Mode", "c_mode");
menu_append_item ($1, "&S-Lang Mode", "slang_mode");
menu_append_item ($1, "&Text Mode", "text_mode");
menu_append_item ($1, "&LaTeX Mode", "latex_mode");
menu_append_item ($1, "Te&X Mode", "tex_mode");
menu_append_item ($1, "&No Mode", "no_mode");
menu_append_item ($1, "&Fortran Mode", "fortran_mode");
menu_append_item ($1, "F&90 Mode", "f90_mode");
menu_append_item ($1, "&Python Mode", "python_mode");
menu_append_item ($1, "s&h mode", "sh_mode");

$1 = "Global.&Buffers.&Toggle";
menu_append_item ($1, "&Line Numbers", "toggle_line_number_mode");
menu_append_item ($1, "&Overwrite", "toggle_overwrite");
menu_append_item ($1, "&Read Only", "toggle_readonly");
menu_append_item ($1, "&CR/NL mode", "toggle_crmode");

$1 = "Global.W&indows";
menu_append_item ($1, "&One Window", "one_window");
menu_append_item ($1, "&Split Window", "split_window");
menu_append_item ($1, "O&ther Window", "other_window");
menu_append_item ($1, "&Delete Window", "delete_window");

#ifndef VMS
menu_append_separator ($1);
private variable Gui_Jed;
private define set_gui_color_scheme (file)
{
   set_color_scheme (Gui_Jed + file);
}

menu_append_popup ($1, "&Color Schemes");
if (is_defined ("x_server_vendor"))
{
   Gui_Jed = "Xjed/";
   $2 = $1 + ".&Color Schemes";

   foreach (strtok (Color_Scheme_Path, ","))
     {
	$3 = ();

	add_files_popup_with_callback ($2, "&Xjed",
				       dircat ($3, "Xjed"),
				       "\\C^.*\\.sl$",
				       &set_gui_color_scheme);
     }
   menu_append_separator ($2);
}

foreach (strtok (Color_Scheme_Path, ","))
{
   $2 = ();
   add_files_popup_with_callback ($1, "&Color Schemes", 
				  $2, "\\C^.*\\.sl$",
				  &set_color_scheme);
}

#endif
menu_append_separator ($1);
menu_append_item ($1, "&Redraw", "redraw");


!if (_jed_secure_mode)
{
$1 = "Global.S&ystem";
menu_append_item ($1, "&S-Lang Command", "evaluate_cmd");
menu_append_item ($1, "S&hell Command", "do_shell_cmd");
menu_append_item ($1, "&Compile", "compile");
#ifdef UNIX
#ifexists open_process
autoload ("ashell", "ashell");
menu_append_item ($1, "Shell &Window", "ashell");
#else
menu_append_item ($1, "Shell &Window", "shell");
#endif
menu_append_item ($1, "&Ispell", "ispell");
menu_append_item ($1, "&Mail", "mail");
#else
menu_append_item ($1, "Shell &Window", "shell");
#endif
}


menu_append_item ($1, "C&alendar", "calendar");
menu_append_item ($1, "&Function", "emacs_escape_x");

private define about_jed (unused)
{
   variable about_doc = expand_jedlib_file ("aboutjed.hlp");

   pop2buf ("*about jed*");
   set_readonly (0);
   erase_buffer ();
   vinsert ("Jed Version: %s\nS-Lang Version: %s\n\n",
	    _jed_version_string, _slang_version_string);

   if (about_doc != "")
     () = insert_file (about_doc);
   else
     insert ("aboutjed.hlp not found");

   set_buffer_modified_flag (0);
   bob ();
   most_mode ();
}

$1 = "Global.&Help";
menu_append_item ($1, "About &Jed", &about_jed, NULL);
#ifndef VMS
add_files_popup_with_callback ($1, "&Browse Docs",
			       dircat (JED_ROOT, "doc/txt"),
			       "\\C^.*\\.txt\\.?",
			       &browse_docs_callback);
#endif
menu_append_separator ($1);
menu_append_item ($1, "&Describe Key Bindings", "describe_bindings");
menu_append_item ($1, "Describe &Mode", "describe_mode");
menu_append_item ($1, "Describe &Function", "describe_function");
menu_append_item ($1, "Describe &Variable", "describe_variable");
menu_append_item ($1, "&Apropos", "apropos");
menu_append_separator ($1);
menu_append_item ($1, "Show &Key", "showkey");
menu_append_item ($1, "&Where Is Command", "where_is");
menu_append_separator ($1);
menu_append_item ($1, "&Info Reader", "info_reader");
#ifdef UNIX
menu_append_item ($1, "&Unix Man Page", "unix_man");
#endif

#ifndef VMS
% Delete this function since we nolonger need it
private define add_files_popup_with_callback();
#endif

% This function gets called by menus.sl as the select_menubar_callback.
% Its purpose is to load this file.
% Note: ide.sl calls this file.
define menu_load_popups (menubar)
{
   _jed_run_hooks ("load_popup_hooks", 1, [menubar]);
}
menu_set_select_menubar_callback ("Global", NULL);

Menu_Popups_Loaded = 1;
