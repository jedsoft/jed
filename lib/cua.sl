%%  CUA (Windows/Mac/CDE/KDE-like) bindings for Jed.
%% 
%%  Reuben Thomas (rrt@sc3d.org)
%%  modified by Guenter Milde <g.milde at web.de>
%% 
%%  Versions:
%%  1   first version by Guenter Milde <g.milde at web.de>
%%  1.1 05/2003 triple (optional single) ESC-keypress aborts functions
%%              fixed missing definition of Key_Ins 
%%              Key_Ctrl_Del calls cua_delete_word (was delete_word)
%%              F3 bound to repeat_search (tip by Guido Gonzato)
%%              removed definitions for F4...F10 (cua-compatible suggestions?)
%%              ^Q exits without asking for confirmation
%% 
%%  USAGE:
%% 
%%  put somewhere in your path and uncomment the line
%%  %  () = evalfile ("cua");            % CUA-like key bindings
%%  in your .jedrc/jed.rc file
%% 
%%  ESC-problem: unfortunately, some function keys return "\e\e<something>"
%%  as keystring. To have a single ESC-press aborting, insert
%%     autoload("cua_one_press_escape", "cuamisc");
%%     cua_one_press_escape();
%%  into your .jedrc. !! Attention, except for xjed, this is an experimental
%%  feature that can cause problems with functions that use getkey(),
%%  (e.g. showkey(), wmark.sl (before jed 99.16), ...)
%% 
%%  Enhancements (optional helper modes from http://jedmodes.sf.net/):
%%   cuamouse.sl: cua-like mouse bindings
%%   cuamark.sl:  cua-like marking/copy/paste using yp_yank.sl (a ring of
%%                kill-buffers)
%%   numbuf.sl:   fast switch between buffers via ALT + Number
%%   print.sl:    printing
%%   ch_table.sl: popup_buffer with character table (special chars)

% --- Requirements ------------------------------------------------------

require("cuamisc");   % "Outsourced" helper functions
require("keydefs");   % Key definitions for Unix and DOS/Windos
if(strlen(expand_jedlib_file("cuamark.sl"))) % non standard mode
  require("cuamark");
else
  require("wmark");   % cua-like marking, standard version
require("recent");    % save a list of recent files

% --- Variables --------------------------------------------------------
set_status_line(" %b  mode: %m %n  (%p)   %t ", 1);
menu_set_menu_bar_prefix ("Global", " ");

Help_File = "cua.hlp";

%--- Keybindings --------------------------------------------------------

% This key will be used by the extension modes (e.g. c_mode.sl) to bind
% additional functions to
_Reserved_Key_Prefix = "^E";  % Extended functionality :-)

% ESC (unfortunately, some special keys return "\e\e<something>")
% see USAGE at top for workaround
setkey ("cua_escape_cmd", "\e\e\e");              % Triple-Esc -> abort
definekey ("exit_menubar", "\e\e\e", "menu"); % close menus

% Function keys
setkey("menu_select_menu(\"Global.&Help\")",   Key_F1);
%setkey("context_help",                        Key_Shift_F1); % with hyperhelp mode
setkey("cua_save_buffer",                      Key_F2);
setkey("write_buffer",                         Key_Shift_F2); % save as
setkey("repeat_search",                        Key_F3);
% setkey("menu_select_menu(\"Global.&Search\")", Key_F3); % open Search menu

% The "named" keys
setkey("backward_delete_char_untabify",    Key_BS);
setkey("delete_char_cmd",                  Key_Del);
setkey("toggle_overwrite",                 Key_Ins);
setkey("beg_of_line",                      Key_Home);
setkey("eol_cmd",                          Key_End);
setkey("page_up",                          Key_PgUp);
setkey("page_down",                        Key_PgDn);
setkey("cua_bdelete_word",                 Key_Ctrl_BS);
setkey("cua_delete_word",                  Key_Ctrl_Del);
setkey("beg_of_buffer",                    Key_Ctrl_Home);
setkey("eob; recenter(window_info('r'));", Key_Ctrl_End);
setkey("bskip_word",                       Key_Ctrl_Left);
setkey("skip_word",                        Key_Ctrl_Right);
setkey("forward_paragraph",                Key_Ctrl_Up);
setkey("backward_paragraph",               Key_Ctrl_Down);
%setkey("pop_mark(0)",                     Key_Ctrl_Up);
%setkey("push_mark",                       Key_Ctrl_Down);  % define region

% The Control Chars
unset_ctrl_keys();                         % unset to get a clear start
#ifdef UNIX
enable_flow_control(0);  %turns off ^S/^Q processing (Unix only)
#endif

setkey("mark_buffer",		"^A");   % mark All
%setkey("dabbrev",              "^A");	 % abbreviation expansion
%setkey("format_paragraph",	"^B");   % (ide default)
setkey("smart_set_mark_cmd",	"^B");   % Begin region
setkey("yp_copy_region_as_kill","^C");   % Copy (cua default)
set_abort_char(0x04);                    % "logout"
% ^E ==  _Reserved_Key_Prefix              Extra functionality
% ^F map: 				   Find
setkey("search_backward", 	"^FB");
setkey("isearch_backward",	"^F^B");
setkey("toggle_case_search", 	"^FC");
setkey("re_search_forward", 	"^FE");  % rEgexp search
setkey("search_forward",	"^FF");
setkey("isearch_forward",	"^F^F");
setkey("re_search_backward",	"^FG");
setkey("isearch_forward",	"^FI");  % Incremental search
setkey("occur", 		"^FO");  % find all Occurences
setkey("query_replace_match", 	"^FP");  % regexp rePlace
setkey("replace_cmd", 		"^FR");

setkey("goto_line_cmd", 	"^G");   % Goto line
% set_abort_char('');                  % Jed Default, now on ^D
% ^H map: 				   Help ...
setkey("apropos", 		"^HA");
setkey("describe_function", 	"^HF");
setkey("help",   		"^HH");
setkey("info_reader", 		"^HI");
setkey("showkey", 		"^HK");
setkey("describe_mode", 	"^HM");
setkey ("unix_man",	      	"^HU");
setkey("describe_variable", 	"^HV");
setkey("where_is", 		"^HW");
setkey("select_menubar", 	"^H?");

setkey("indent_region_or_line", "^I");   % Key_Tab: indent_line
% setkey("self_insert_cmd", 	"^I");
% setkey("",		   	"^J");   % Free!
setkey("del_eol",		"^K");   % Kill line
setkey("cua_repeat_search",	"^L");
%  ^M = Key_Return
setkey("next_buffer",      	"^N");   % Next buffer
setkey("find_file",		"^O");   % Open file (cua default)
%setkey ("print_buffer", 	"^P");   % Print (with print.sl)
%setkey("exit_with_query",  	"^Q");   % Quit (ask for confirmation)
setkey("exit_jed",  		"^Q");   % Quit (without asking)
% ^R: 					   Rectangles
setkey("copy_rect",		"^RC");
setkey("insert_rect",		"^RV");
setkey("kill_rect",		"^RX");  % delete and copy to rect-buffer
setkey("open_rect",		"^R ");  % ^R Space: insert whitespace
setkey("blank_rect",		"^RY");  % delete (replace with spaces)
setkey("blank_rect",		"^R" + Key_Del);
setkey("cua_save_buffer",	"^S");   % Save 
%setkey("transpose_chars",  "^T");
% 				 ^T      % still free
setkey("yp_yank",              	"^V");   % insert/paste
setkey("delbuf(whatbuf)",     	"^W");
setkey("yp_kill_region",        "^X");   % cut
setkey("redo",		        "^Y");
setkey("undo",		        "^Z");

runhooks ("keybindings_hook", "cua");    % eventual modifications

% --- menu additions --------------------------------------------------

private define cua_load_popup_hook (menubar)
{
   menu_delete_item ("Global.&File.&Close");
   menu_insert_item("&Save", "Global.&File", "&Close", "delbuf(whatbuf)");
   if(strlen(expand_jedlib_file("print.sl"))) % non standard mode
     {
     menu_insert_item("Canc&el Operation", "Global.&File", "&Print", 
		      "print_buffer");
     menu_insert_separator("Canc&el Operation", "Global.&File");
     }
   menu_insert_item (3, "Global.&Search",
		     "&Incremental Search Forward", "isearch_forward");
   menu_insert_item (4, "Global.&Search",
		     "I&ncremental Search Backward", "isearch_backward");
   menu_insert_item ("&Replace", "Global.&Search",
		     "Toggle &Case Search", "toggle_case_search");
   menu_insert_separator ("&Replace", "Global.&Search");
}
append_to_hook ("load_popup_hooks", &cua_load_popup_hook);

% signal the success in loading the cua emulation:
_Jed_Emulation = "cua";

