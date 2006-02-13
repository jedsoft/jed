% cuamisc.sl: helper functions for the cua suite
%
% "Outsourced" from cua.sl, so they can be used by other emulations as well.
% Author: Guenter Milde <g.milde@web.de>
% Version 1      first public version
%         1.1    repeat search opens the Search meanu if LAST_SEARCH is empty
%         1.1.1  tweaked by jed for inclusion into 0.99-17

%!%+
%\function{cua_delete_word}
%\synopsis{Delete the current word (or a defined region)}
%\usage{ Void cua_delete_word ()}
%\description
%   cua_delete_word is somewhat context sensitive:
%    * Delete from the current position to the end of a word.
%    * If there is just whitespace following the editing point, delete it.
%    * If there is any other non-word char, delete just one char.
%    * If a region is defined, delete it (instead of the above actions).
%   This way, you can do a "piecewise" deletion by repeatedly pressing
%   the same key-combination.
%\notes
%   This is actually the ide_delete_word function form Guido Gonzatos
%   ide.sl mode, put here to be usable also with other emulations.
%\seealso{delete_word, delete_cmd, cua_kill_region}
%!%-
public define cua_delete_word ()		% ^T, Key_Ctrl_Del
{
   !if (markp)
     {
	variable m = create_user_mark ();
	push_mark ();
	skip_chars (get_word_chars());
	if (create_user_mark () == m) skip_chars (" \n\t");
	if (create_user_mark () == m) go_right (1);
     }
  del_region ();
}

% Context sensitive backwards deleting, again taken from ide.sl
define cua_bdelete_word ()              % Key_Ctrl_BS
{
   push_mark ();
   variable m = create_user_mark ();
   bskip_chars ("a-zA-Z0-9");
   if (create_user_mark () == m) bskip_chars (" \n\t");
   if (create_user_mark () == m) go_left (1);
   del_region ();
}


%!%+
%\function{repeat_search}
%\synopsis{continue searching with last searchstring}
%\usage{define repeat_search ()}
%\seealso{LAST_SEARCH, search_forward, search_backward}
%!%-
public define cua_repeat_search ()
{
%%#ifeval (_jed_version >= 9916)
   !if (strlen(LAST_SEARCH))
     return menu_select_menu("Global.&Search");
%%#endif   
   go_right (1);
   !if (fsearch(LAST_SEARCH)) error ("Not found.");
}

%!%+
%\function{cua_indent_region_or_line}
%\synopsis{Indent the current line or (if defined) the region}
%\usage{Void cua_indent_region_or_line ()}
%\description
%   Call the indent_line_hook for every line in a region.
%   If no region is defined, call it for the current line.
%\seealso{indent_line, set_buffer_hook, is_visible_mark}
%!%-
public define cua_indent_region_or_line ()
{
   !if(is_visible_mark ())
     {
	indent_line ();
	return;
     }

   check_region (1);                  % make sure the mark comes first
   variable end_line = what_line ();
   exchange_point_and_mark();         % now point is at start of region
   while (what_line() <= end_line)
     {indent_line (); go_down_1 ();}
   pop_mark (0);
   pop_spot ();
}

% --- Use the ESC key as abort character (still experimental)

%!%+
%\function{cua_escape_cmd}
%\synopsis{Escape from a command/aktion}
%\usage{cua_escape_cmd()}
%\description
%   Undo/Stop an action. If a region is defined, undefine it. Else 
%   call kbd_quit.
%\seealso{kbd_quit}
%!%-
define cua_escape_cmd()
{
  if (is_visible_mark)
    pop_mark(0);
  else
    call ("kbd_quit");
}

%!%+
%\function{cua_escape_cmd}
%\synopsis{Distinguish the ESC key from other keys starting with "\\e"}
%\usage{Void cua_escape_cmd()}
%\description
%   If there is input pending (i.e. if the keycode is multi-character),
%   "\\e" will be put back to the input stream. Otherwise (if the
%   ESC key is pressed, "\\e\\e\\e" is pushed back. With ALT_CHAR = 27, the Alt 
%   key can be used as Meta-key as usual (i.e. press both ALT + <some-key> 
%   to get the equivalent of the ESC <some-key> key sequence.
%\seealso{escape_cmd, one_press_escape, kbd_quit, map_input, setkey}
%!%-
define cua_meta_escape_cmd ()
{
   if (input_pending(0))
     ungetkey (27);
   else
     buffer_keystring("\e\e\e");
}

%!%+
%\function{cua_one_press_escape}
%\synopsis{Redefine the ESC key to issue "\\e\\e\\e"}
%\usage{cua_one_press_escape()}
%\description
%   Dependend on the jed-version, either x_set_keysym or 
%   meta_escape_cmd is used to map the ESC key to "\\e\\e\\e"
%\example
% To let the ESC key abort functions but retain bindings for
% keystrings that start with "\\e" do
%#v+
%    cua_one_press_escape();
%    setkey ("cua_escape_cmd", "\e\e\e");     % Triple-Esc -> abort
%#v-
%\notes
%   The function is experimental and has sideeffects if not using xjed.
%   For not-x-jed:
% 
%   It uses the "^^" character for temporarily remapping, i.e. Ctrl-^ will
%   call cua_escape_cmd().
%   
%   In order to work, it must be loaded before any mode-specific keymaps are
%   defined -- otherwise this modes will be widely unusable due to not 
%   working cursor keys...!
%   
%   It breaks functions that rely on getkey() (e.g. isearch, showkey, old
%   wmark(pre 99.16), ...)
%   
%   It will not work in keybord macros and might fail on slow terminal links.
%     
%\seealso{cua_escape_cmd, cua_escape_cmd, getkey, setkey, x_set_keysym}
%!%-
define cua_one_press_escape()
{
   if (is_defined("x_set_keysym"))
     call_function ("x_set_keysym", 0xFF1B, 0, "\e\e\e");   % one-press-escape
   else
     {
	map_input(27, 30);  % "\e" -> "^^" ("^6" on most keybords, undo in wordstar)
	setkey ("cua_escape_cmd", "^^");
     }
}

%{{{ cua_save_buffer()
%!%+
%\function{cua_save_buffer}
%\synopsis{cua_save_buffer}
%\usage{Void cua_save_buffer();}
%\description
% Save current buffer.
%!%-
define cua_save_buffer()
{
   variable file;
   
   !if (buffer_modified())
     {
	message("Buffer not modified.");
	return;
     }
   
   file = buffer_filename();
   !if (strlen(file))
       save_buffer_as();

   () = write_buffer(file);

} add_completion("cua_save_buffer");
%}}}


provide ("cuamisc");

