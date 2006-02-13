%
%  simple TeX mode for JED
%
%  When tex mode is loaded, 'tex_mode_hook' is called.  This hook will allow
%  users to customize the mode.  In particular, certain functions here have
%  no keybindings, e.g., 'latex_do_environment'.  So, in your jed.rc file,
%  add something like:
%    define tex_mode_hook () {
%       local_setkey ("latex_do_environment", "^C^E");
%    }
%  which binds the function to Ctrl-C Ctrl-E.


% Load the common definitions if not already loaded.
require ("texcom");


$1 = "TeX-Mode";

!if (keymap_p($1))
{
   make_keymap ($1);
   definekey ("tex_insert_quote", "\"", $1);
   definekey ("tex_insert_quote", "'", $1);
   definekey ("tex_blink_dollar", "$", $1);
   definekey ("tex_ldots", ".", $1);
}


%!%+
%\function{tex_mode}
%\synopsis{tex_mode}
%\description
% Mode useful for editing TeX and LaTeX modes.  
% Useful bindings:
%#v+
%  '"'  :  tex_insert_quote
%  '\'' :  tex_insert_quote
%  '$'  :  tex_blink_dollar
%  '.'  :  tex_ldots.  Inserts a '.' except if preceeded by two dots.  In 
%           this case, the dots are converted to \ldots.
%#v- 
%  When tex mode is loaded, 'tex_mode_hook' is called.  This hook will allow
%  users to customize the mode.  In particular, certain functions here have
%  no keybindings, e.g., 'latex_do_environment'.  So, in your jed.rc file,
%  add something like:
%#v+
%    define tex_mode_hook () {
%       local_setkey ("latex_do_environment", "^C^E");
%    }
%#v-
%  which binds the function to Ctrl-C Ctrl-E.
%!%-
define tex_mode ()
{
   variable mode = "TeX";
   variable texmode = "TeX-Mode";
   use_keymap (texmode);
   set_mode (mode, 0x1 | 0x20);
   set_buffer_hook ("par_sep", "tex_paragraph_separator");
   set_buffer_hook ("wrap_hook", "tex_wrap_hook");
   unset_buffer_hook ("indent_hook");
   unset_buffer_hook ("newline_indent_hook");

   use_syntax_table (texmode);

   mode_set_mode_info (mode, "fold_info", "%{{{\r%}}}\r\r");

   run_mode_hooks ("tex_mode_hook");
   % This is called after the hook to give the hook a chance to load the
   % abbrev table.
   %if (abbrev_table_p (mode)) use_abbrev_table (mode); 
   %(Now handled in run_mode_hooks)
}

%-----------------------------------------------------------%
#ifdef 0
define tex_info_find_node ()
{
   variable node;
   
   node = read_mini ("Node:", Null_String, Null_String);
   !if (strlen (node)) return;
   info_mode ();
   info_find_node ("(latex)top");
   info_find_node ("(latex)" + node);
}
#endif
