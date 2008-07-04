% This file should not be byte-compiled.
% It is loaded from site.sl and permits various flavors of jed to share the
% same set of S-Lang files. Much of it is written in RPN for efficiency.

#ifdef SUBPROCESSES
autoload ("aprocess_stringify_status", "aprocess");
#endif

#ifdef XWINDOWS
autoload ("_jed_run_program_hook", "runpgm");
#endif

#ifdef WIN32
autoload ("_win32_get_helper_app_name", "runpgm");
# ifexists set_import_module_path
$1 = path_concat (JED_ROOT, "slsh");
set_slang_load_path (strcat (get_slang_load_path (), char(path_get_delimiter()), $1));
% set the library path for modules.
set_import_module_path(path_concat($1, "modules"));
# endif
#endif

#ifndef OS2 UNIX WIN32
autoload ("run_shell_cmd", "shell");
#endif

#ifdef MSWINDOWS XWINDOWS MOUSE
#ifnexists x_insert_selection
define x_insert_selection () {x_insert_cutbuffer();}
define x_copy_region_to_selection () {x_copy_region_to_cutbuffer();}
#endif
. "mouse" evalfile pop
#endif

#ifdef WINGUI
. 1 =Simulate_Graphic_Chars
. 4 2 mouse_map_buttons	       %  map Right to Middle
. 2 4 mouse_map_buttons	       %  map Middle to Right
%. "menus" evalfile pop         %  Uncomment to enable text menus
. "wmenu.sl" evalfile pop     %  Uncomment to enable GUI menus
#else
# ifexists menu_create_menu_bar
. "menus" evalfile pop
# endif
#endif

#ifnexists menu_create_menu_bar
define menu_create_menu_bar () { _pop_n (_NARGS); }
define menu_append_item () { _pop_n (_NARGS); }
define menu_append_separator () { _pop_n (_NARGS); }
define menu_append_popup () { _pop_n (_NARGS); }
define menu_use_menu_bar () { _pop_n (_NARGS); }
#endif

#ifnexists enable_menu_keys
define enable_menu_keys ();
#endif

#ifdef XWINDOWS
. "HOST" getenv =$1
% . $1 NULL != { "XJed@" $1 strcat x_set_window_name } if
. "skip_word" 		"\e[c" setkey      %/* shift-right */
. "bskip_word"		"\e[d" setkey      %/* shift-left */
. "goto_top_of_window"	"\e[a" setkey      %/* shift-up */
. "goto_bottom_of_window"	"\e[b" setkey      %/* shift-down */
. "beg_of_line"		"\e[1~" setkey		% Home
. "eol_cmd"		"\e[4~" setkey		% End

. 0xFFFF '$' "\e[3$" x_set_keysym     % Key_Shift_Del
. 0xFFFF '^' "\e[3^" x_set_keysym     % Key_Ctrl_Del
. 0xFF08 '$' "\e[16$" x_set_keysym    % Key_Shift_BS
. 0xFF08 '^' "\e[16^" x_set_keysym    % Key_Ctrl_BS
. 0xFF09 '$' "\e[Z" x_set_keysym      % Key_Shift_Tab (reverse tab)
#endif

% For compatability
define shell_cmd ()
{
   () = run_shell_cmd ();
}

define goto_visible_eol ()
{
#ifdef HAS_LINE_ATTR
   if (down_1 ())
     {
	if (is_line_hidden ())
	  skip_hidden_lines_forward (1);
	go_left_1 ();
     }
#endif
   eol ();
}

define mark_to_visible_eol ()
{
   push_mark ();
   goto_visible_eol ();
}

define transpose_lines ()
{
   bol (); push_mark ();
#ifdef HAS_LINE_ATTR
   mark_to_visible_eol ();
   bufsubstr ();		       %  on stack
   go_right_1 ();
   del_region();
   skip_hidden_lines_backward (1);
   bol();
   insert(());
   newline();
   skip_hidden_lines_forward (1);      %  goes to bol
#else
   line_as_string ();                  %  on stack
   go_right_1 ();
   del_region();
   go_up_1 (); bol();
   insert(());
   newline();
   go_down_1 ();                               %  goes to bol
#endif
}

#ifdef HAS_LINE_ATTR
autoload ("folding_mode", "folding");
add_completion ("folding_mode");

variable Fold_Mode_Ok = 0;
define fold_mode ()
{
   if (Fold_Mode_Ok) folding_mode ();
}

#endif

#ifdef HAS_DFA_SYNTAX
define dfa_enable_highlight_cache (file, name)
{
   variable dirfile = search_path_for_file (Jed_Highlight_Cache_Path, file, ',');
   if (dirfile == NULL)
     dirfile = dircat (Jed_Highlight_Cache_Dir, file);
   _dfa_enable_highlight_cache (dirfile, name);
}
define use_syntax_table_hook (t)
{
   variable x = mode_get_mode_info ("use_dfa_syntax");
   if (x == NULL) x = 0;
   use_dfa_syntax (x);
}
#else
% dummy functions that enable jed to work in mixed environments
define dfa_enable_highlight_cache (x, y);
define dfa_define_highlight_rule (x,y,z);
define dfa_build_highlight_table (x);
define dfa_set_init_callback (x,y);
#endif

#ifdef WIN32 
MSDOS_Has_Long_File_Names = 1;
variable W32shell_Perform_Globbing;
#else
# ifdef MSDOS
#  ifdef 16_BIT_SYSTEM
MSDOS_Has_Long_File_Names = 0;
#  else
$1 = getenv ("LFN");
if ($1 == NULL) $1 = "N";
MSDOS_Has_Long_File_Names = ("Y" == strup ($1));
#  endif
# endif
#endif

