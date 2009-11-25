%   Borland-like IDE Mode for JED       -*- SLang -*-
%
%   Put the line: () = evalfile ("ide.sl");
%   in your jed.rc startup file.
%
%   Written by Guido Gonzato <guido.gonzato@univr.it>;
%   based on John E. Davis' original wordstar.sl.
%   Contributions by J\o ergen Larsen <jl@dirac.ruc.dk>,
%   and John Fattaruso <johnf@ti.com>
%
%   This file makes jed a jolly good 99% compatible with the true-blue
%   WordStar, jstar, the DOS editor, and the good old Borland IDE.
%   Some Emacs compatibility is maintained: blocks are actually "regions"
%   as in Emacs mode.
%
%   Please send me requests and bug reports, should you find any.
%
%   Version 1.3.4; for jed B0.99.13 upwards.
%   Last modified: 2 April 2003
%   Version 1.3.4a: Changed static declarations to private ones for
%    0.99.19 release (JED)

Help_File = "ide.hlp";

require ("keydefs");
require ("rcs");

_Reserved_Key_Prefix = "\032";         %  ^Z
_Jed_Emulation = "ide";

% users can tailor the way skip_word works
custom_variable ("Ide_Skippable_Chars",
		 "\n\t !\"#$%&'()*+,-./:;<=>?@[\]^`{|}~");

#ifndef IBMPC_SYSTEM
private variable Key_BS_Del      = "^?";
private variable Key_Alt_BS_Del  = strcat("\e", Key_BS_Del);
#endif

private variable Ide_Bookmark_Exist = 1;
set_status_line (" Jed %v: %b (%m%n) (%p %c) %t", 1);

unset_ctrl_keys ();
unsetkey (Key_F1);      % prevent EDT error in xterm
unsetkey (Key_F2);
unsetkey (Key_F3);
unsetkey (Key_F4);
unsetkey (Key_Del);
unsetkey (Key_Up);      % arrows keys are redefined to add the
unsetkey (Key_Down);    % "goto previous position" feature
unsetkey (Key_Left);
unsetkey (Key_Right);
unsetkey (Key_PgUp);
unsetkey (Key_PgDn);

set_abort_char (7); % ^G. Note - wordstar.sl uses ^^.
setkey ("kbd_quit", "^G");

%
% Basic commands: cursor movement, delete, search & replace, etc.
%
setkey ("begin_macro", "\e(");
setkey ("compile", Key_F9);
setkey ("backward_delete_char", Key_BS);
setkey ("dabbrev", "^V");
setkey ("delete_char_cmd", "\eg");      % ^G is used for break
setkey ("delete_char_cmd", Key_Del);
setkey ("delete_line", "^Y");
setkey ("end_macro", "\e)");
setkey ("execute_macro", "\er");
setkey ("format_paragraph", "^B");
setkey ("ide_better_help", Key_F1);
setkey ("ide_toggle_overwrite", Key_Ins);
setkey ("undo", "^U");
setkey ("kbd_quit", "\e\e\e");
setkey ("ide_bdelete_word", Key_Alt_BS);
setkey ("list_buffers", "\e0");
#ifndef IBMPC_SYSTEM
setkey ("ide_bdelete_word", Key_Alt_BS_Del);
#endif
setkey ("ide_bdelete_word", "\et");     %  ESC-o is used for Mode menu
setkey ("ide_bskip_word", "^A");
setkey ("ide_delete_word", "^T");
setkey ("ide_next_char_cmd", "^D");
setkey ("ide_next_char_cmd", Key_Right);
setkey ("ide_next_line_cmd", "^X");
setkey ("ide_next_line_cmd", Key_Down);
setkey ("ide_page_down", "^C");
setkey ("ide_page_down", Key_PgDn);
setkey ("ide_page_up", "^R");
setkey ("ide_page_up", Key_PgUp);
setkey ("ide_previous_char_cmd", "^S");
setkey ("ide_previous_char_cmd", Key_Left);
setkey ("ide_previous_line_cmd", "^E");
setkey ("ide_previous_line_cmd", Key_Up);
setkey ("ide_repeat_search", "^L");
setkey ("ide_skip_word", "^F");
setkey ("other_window", "^KO");
setkey ("other_window", "^K^O");
setkey ("ide_window_up", "^W");
setkey ("ide_window_down", "\eZ");
setkey ("ide_insert_any_char", "^P");
%
% Control-Q keys  --- hope you figure out how to pass ^Q/^S through system
% In case you *cannot* figure out how, you can use ESC+^key instead of ^Q; 
% for instance, ESC-^Y acts as ^Q-Y. Twisted, but some terminals need it.
% There's an exception: use ESC + ESC + digit to mimick ^Q + digit; this is
% to avoid overriding the ESC + number + operation feature.
%
setkey (".0 ide_goto_mark_n", "^Q0");
setkey (".1 ide_goto_mark_n", "^Q1");
setkey (".2 ide_goto_mark_n", "^Q2");
setkey (".3 ide_goto_mark_n", "^Q3");
setkey (".4 ide_goto_mark_n", "^Q4");
setkey (".5 ide_goto_mark_n", "^Q5");
setkey (".6 ide_goto_mark_n", "^Q6");
setkey (".7 ide_goto_mark_n", "^Q7");
setkey (".8 ide_goto_mark_n", "^Q8");
setkey (".9 ide_goto_mark_n", "^Q9");
#ifndef MSDOS WIN32
setkey (".0 ide_goto_mark_n", "\e\e0");
setkey (".1 ide_goto_mark_n", "\e\e1");
setkey (".2 ide_goto_mark_n", "\e\e2");
setkey (".3 ide_goto_mark_n", "\e\e3");
setkey (".4 ide_goto_mark_n", "\e\e4");
setkey (".5 ide_goto_mark_n", "\e\e5");
setkey (".6 ide_goto_mark_n", "\e\e6");
setkey (".7 ide_goto_mark_n", "\e\e7");
setkey (".8 ide_goto_mark_n", "\e\e8");
setkey (".9 ide_goto_mark_n", "\e\e9");
#endif
setkey ("kill_line", "^QY");
setkey ("kill_line", "^Q^Y");
setkey ("quoted_insert", "^Q^Q");
setkey ("ide_bob", "^QR");
setkey ("ide_bob", "^Q^R");
setkey ("ide_bol", "^QS");
setkey ("ide_bol", "^Q^S");
setkey ("ide_bol", Key_Home);
setkey ("ide_eob", "^QC");
setkey ("ide_eob", "^Q^C");
setkey ("ide_eol", "^QD");
setkey ("ide_eol", "^Q^D");
setkey ("ide_eol", Key_End);
setkey ("ide_goto_begin_block", "^QB");
setkey ("ide_goto_begin_block", "^Q^B");
setkey ("ide_goto_bottom_of_window", "^QX");
setkey ("ide_goto_bottom_of_window", "^Q^X");
setkey ("ide_goto_end_block", "^QK");
setkey ("ide_goto_end_block", "^Q^K");
setkey ("ide_goto_line_cmd", "^QI");
setkey ("ide_goto_line_cmd", "^Q^I");
setkey ("ide_goto_middle_of_window", "^QM");
setkey ("ide_goto_prev", "^QP");
setkey ("ide_goto_top_of_window", "^QE");
setkey ("ide_goto_top_of_window", "^Q^E");
setkey ("ide_replace_cmd", "^QA");
setkey ("ide_replace_cmd", "^Q^A");
setkey ("ide_search_forward", "^QF");
setkey ("ide_search_forward", "^Q^F");
setkey ("ide_toggle_case", "^QT");
setkey ("ide_toggle_case", "^Q^T");
#ifndef MSDOS WIN32
setkey ("kill_line", "\e^Y");
setkey ("ide_bob", "\e^R");
setkey ("ide_bol", "\e^S");
setkey ("ide_eob", "\e^C");
setkey ("ide_eol", "\e^D");
setkey ("ide_goto_begin_block", "\e^B");
setkey ("ide_goto_bottom_of_window", "\e^X");
setkey ("ide_goto_end_block", "\e^K");
setkey ("ide_goto_line_cmd", "\e^I");
setkey ("ide_goto_prev", "\e^P");
setkey ("ide_goto_top_of_window", "\e^E");
setkey ("ide_replace_cmd", "\e^A");
setkey ("ide_search_forward", "\e^F");
setkey ("ide_toggle_case", "\e^T");
#endif
%
% Control-K map
%
setkey (".0 ide_set_mark_n", "^K0");
setkey (".1 ide_set_mark_n", "^K1");
setkey (".2 ide_set_mark_n", "^K2");
setkey (".3 ide_set_mark_n", "^K3");
setkey (".4 ide_set_mark_n", "^K4");
setkey (".5 ide_set_mark_n", "^K5");
setkey (".6 ide_set_mark_n", "^K6");
setkey (".7 ide_set_mark_n", "^K7");
setkey (".8 ide_set_mark_n", "^K8");
setkey (".9 ide_set_mark_n", "^K9");
setkey ("exit_jed", "^KX");
setkey ("exit_jed", "^K^X");
setkey ("find_file", "^KE");
setkey ("find_file", "^K^E");
setkey ("find_file", Key_F3);
setkey ("kill_buffer", "^KQ");
setkey ("kill_buffer", "^K^Q");
setkey ("kill_buffer", Key_Alt_F3);
setkey ("split_window", Key_Alt_F5);
setkey ("one_window", "^KI");
setkey ("one_window", "^K^I");
setkey ("one_window", Key_F5);
setkey ("save_buffer", "^KD");
setkey ("save_buffer", "^K^D");
setkey ("save_buffer", Key_F2);
setkey ("suspend", "^KZ");
setkey ("suspend", "^K^Z");
setkey ("ide_next_buffer (0)", "^KP");  % next
setkey ("ide_next_buffer (0)", "^K^P");
setkey ("ide_next_buffer (1)", "^KN");  % previous
setkey ("ide_next_buffer (1)", "^K^N");
setkey ("ide_next_buffer (1)", Key_F6);
setkey ("ide_begin_block", "^KB");      % set mark
setkey ("ide_begin_block", "^K^B");
setkey ("ide_copy_block", "^KC");       % yank
setkey ("ide_copy_block", "^K^C");
setkey ("ide_delete_block", "^KY");     % cut
setkey ("ide_delete_block", "^K^Y");
setkey ("comment_region", "^K;");
setkey ("uncomment_region", "^K:");
setkey_reserved ("comment_line", ";");
setkey_reserved ("uncomment_line", ":");
setkey ("ide_filter_region", "^K/");
setkey ("ide_open_file_at_cursor", "\e^M");
setkey ("ide_insert_file", "^KR");
setkey ("ide_insert_file", "^K^R");
setkey ("ide_lowercase_region", "^KL");
setkey ("ide_lowercase_region", "^K^L");
setkey ("ide_save_buffer", "^KS");
setkey ("ide_save_buffer", "^K^S");
setkey ("ide_select_word", "^KT");
setkey ("ide_select_word", "^K^T");
setkey ("ide_void_block", "^KK");
setkey ("ide_void_block", "^K^K");
setkey ("ide_clear_block", "^KH");      % copy to the clipboard
setkey ("ide_clear_block", "^K^H");
setkey ("ide_uppercase_region", "^KU");
setkey ("ide_uppercase_region", "^K^U");
setkey ("write_region", "^KW");
setkey ("write_region", "^K^W");
setkey ("reg_insert_register", "^KJ");  % registers are something like
setkey ("reg_insert_register", "^K^J"); % a multiple clipboard
setkey ("reg_copy_to_register", "^KG");
setkey ("reg_copy_to_register", "^K^G");
% compiler/debugger interface
setkey ("compile_parse_errors", Key_Alt_F8);     % was ^X'
setkey_reserved ("compile_parse_errors", "'");
setkey ("compile_previous_error", Key_Alt_F7);   % was ^X,
setkey_reserved ("compile_previous_error", ",");
setkey ("ispell", Key_F7);

%
% Now let's implement the damn thing
%

$1 = 13;

% bookmarks 0..9 are for the user; bookmark 10 is used by some of the
% following functions; bookmark 11 and 12 mark the beginning and end of
% the block (used by ^QB and ^QK)

private variable _Ide_Bookmarks = Mark_Type[$1];

%  ide_set_bookmark () and ide_goto_bookmark () are implemented to provide
%  a more Borland-ish way of copying and moving blocks, and of moving 
%  around generally.

define ide_set_bookmark ()
{
  _Ide_Bookmarks[10] = create_user_mark ();
  Ide_Bookmark_Exist = 1;
}
   
define ide_goto_bookmark ()
{
  variable mrk = _Ide_Bookmarks[10];
  ide_set_bookmark ();
  sw2buf (mrk.buffer_name);
  goto_user_mark (mrk);
}

%
% Basic commands: cursor movement, delete, search & replace, etc.
%

define ide_execute_macro ()             % ESC-R
{
  ide_set_bookmark ();
  call ("execute_macro");
}

define ide_previous_char_cmd ()         % Key_Left
{
  ide_set_bookmark ();
  call ("previous_char_cmd");
}

define ide_next_char_cmd ()             % Key_Right
{
  ide_set_bookmark ();
  call ("next_char_cmd");
}

define ide_next_line_cmd ()             % Key_Down
{
  ide_set_bookmark ();
  call ("next_line_cmd");
}

define ide_previous_line_cmd ()         % Key_Up
{
  ide_set_bookmark ();
  call ("previous_line_cmd");
}

define ide_page_down ()                 % Key_PgDn
{
  ide_set_bookmark ();
  call ("page_down");
}

define ide_page_up ()                   % Key_PgUp
{
  ide_set_bookmark ();
  call ("page_up");
}

define ide_bob ()                       % ^QR
{
  ide_set_bookmark ();
  bob ();
}

define ide_eob ()                       % ^QC
{
  ide_set_bookmark ();
  eob ();
}

define ide_bol ()                       % Key_Home
{
  ide_set_bookmark ();
  bol ();
}

define ide_eol ()                       % Key_End
{
  ide_set_bookmark ();
  eol ();
}

define ide_goto_bottom_of_window ()     % ^QX
{
  ide_set_bookmark ();
  goto_bottom_of_window ();
}

define ide_goto_top_of_window ()        % ^QE
{
  ide_set_bookmark ();
  goto_top_of_window ();
}

define ide_goto_middle_of_window ()     % ^QM
{
  % incorrect if the buffer has fewer lines
  % than the windows that contains it. Duh.
  goto_top_of_window ();
  go_down (window_info ('r') / 2);
}

define ide_window_up ()                 % ^W - J.F.
{
  variable current,bottom;
  current = window_line ();
  bottom = window_info ('r');
  ide_set_bookmark ();
  if (current == bottom) {
    ide_previous_line_cmd ();
    recenter (bottom);
  }
  else
    recenter (current + 1);
}

define ide_window_down ()               % \eZ J.F.
{
  variable current = window_line ();
  ide_set_bookmark ();
  if (current == 1) {
    ide_next_line_cmd ();
    recenter (1);
  }
  else
    recenter (current - 1);
}

define ide_goto_line_cmd ()             % ^QI
{
  ide_set_bookmark ();
  goto_line_cmd ();
}

define ide_replace_cmd ()               % ^QA
{
  ide_set_bookmark ();
  replace_cmd ();
}

define ide_search_forward ()            % ^QF
{
  ide_set_bookmark ();
  search_forward ();
}

define ide_toggle_case ()               % ^QT
{
  variable on_off;
  CASE_SEARCH = not(CASE_SEARCH);
  if (CASE_SEARCH == 1)
    on_off = "On";
  else
    on_off = "Off";
  vmessage ("Case search is %s ", on_off);
}

define ide_toggle_overwrite ()
{
   toggle_overwrite ();
   if (is_overwrite_mode ())
     message ("Overwrite");
   else
     message ("Insert");
}
  
define ide_repeat_search ()             % ^L
{
  ide_set_bookmark ();
  go_right_1 ();
   !if (fsearch(LAST_SEARCH)) error ("Not found.");
}

define ide_bdelete_word ()              % M-O
{
  variable p = _get_point ();
  ide_set_bookmark ();
  push_mark ();
  bskip_chars ("a-zA-Z0-9");
  if (_get_point () == p) bskip_chars (" \n\t"); 
  if (_get_point () == p) go_left (1);
  del_region ();
}

define ide_bskip_word ()                % ^A
{
  variable p = _get_point ();
  ide_set_bookmark ();
  push_mark ();
  bskip_chars (Ide_Skippable_Chars);
  bskip_chars ("^" + Ide_Skippable_Chars);
  pop_mark_0 ();
}

define ide_delete_word ()               % ^T
{
  variable p = _get_point ();
  push_mark ();
  skip_chars ("a-zA-Z0-9");
  if (_get_point () == p) skip_chars (" \n\t"); 
  if (_get_point () == p) go_right (1);
  del_region ();
}

define ide_skip_word ()                 % ^F
{
  variable p = _get_point ();
  ide_set_bookmark ();
  push_mark ();
  skip_chars ("^" + Ide_Skippable_Chars);
  if (_get_point () == p) {
     skip_chars (Ide_Skippable_Chars);
     skip_chars ("^" + Ide_Skippable_Chars);
  }
  pop_mark_0 ();
}

define ide_insert_any_char ()           % ^P
{
  variable i, ch = 0, c, msg = "";
  message ("ASCII-");
  update_sans_update_hook (0);
  for (i = 100; i > 0; i = i / 10) {
    vmessage ("ASCII-%s", msg);
    update_sans_update_hook (0);
    do {
      c = getkey ();
    } while ( (c < '0') or (c > '9'));
    ch = ch + i * (c - '0');
    msg += sprintf ("%c", c);
    vmessage ("ASCII-%s", msg);
    update_sans_update_hook (0);
  }
  insert_char (ch);
  vmessage ("ASCII-%s", msg);
}

define ide_next_buffer (previous)       % Key_F6 | ^KN | Key_Alt_F6 | ^KP
{
  variable n, buf;
  n = buffer_list (); % get the buffers on the stack
  if (previous)
    _stk_reverse (n-1);
  loop (n) {
    buf = ();
    n--;
    if (buf[0] == ' ') continue;
    sw2buf (buf);
    _pop_n (n);
    return;
  }
}

% Blocks: ^K-something
%
% blocks are no longer as in wordstar.sl. We don't cheat anymore. Rather,
% blocks are implemented a la Emacs to maintain compatibility with most .sl
% files (e.g., latex.sl, cmode.sl, etc)

private variable IDE_Block_Buffer = "*ide-clipboard*";
private variable IDE_Block_Buffer_Empty = 1;

define ide_copy_block_to_buffer ()
% paste the new region to the clipboard, then delete the old stuff
{
  setbuf(IDE_Block_Buffer);
  erase_buffer ();
  call ("yank");
}

define ide_begin_block ()               % ^KB
{
  _Ide_Bookmarks[11] = create_user_mark ();
  call ("set_mark_cmd");
}

define ide_end_block ()
{
  _Ide_Bookmarks[12] = create_user_mark ();
  exchange_point_and_mark ();
}

define ide_copy_block ()                % ^KC
{
  ide_set_bookmark ();
  insbuf (IDE_Block_Buffer);
  ide_goto_bookmark ();
}

define ide_goto_begin_block ()          % ^QB
{
  variable mrk = _Ide_Bookmarks[11];
  ide_set_bookmark ();
  sw2buf (mrk.buffer_name);
  ide_set_bookmark ();
  goto_user_mark (mrk);
}

define ide_goto_end_block ()            % ^QK
{
  variable mrk = _Ide_Bookmarks[12];
  ide_set_bookmark ();
  sw2buf (mrk.buffer_name);
  ide_set_bookmark ();
  goto_user_mark (mrk);
}

define ide_void_block ()                % ^KK
{
  pop_mark_0 ();
  error ("Obsolete - use ^KH instead.");
}

define ide_clear_block ()               % ^KH
{
  ide_end_block ();
  call ("copy_region");                 %  copy region to internal buffer  
  ide_copy_block_to_buffer ();
  IDE_Block_Buffer_Empty = 0;
  ide_goto_end_block ();
}

define ide_delete_block ()              % ^KY
{
  ide_clear_block ();        % copy the region to the clipboard, then
  ide_goto_begin_block ();   % redefine the region and delete it. Rather
  ide_begin_block ();        % twisted, but that's the way I like it!
  ide_goto_end_block ();
  ide_end_block ();
  call ("kill_region");
}

define ide_goto_prev ()                 % ^QP
{
  if (Ide_Bookmark_Exist != 1) 
    error ("No previous location!");
  ide_goto_bookmark ();
}

define ide_open_file_at_cursor ()       % Alt-Return, J.L.
{
   push_spot ();
   % Find the substring which could be a file name. -
   % The following method assumes reasonably standard file names.
#ifdef UNIX
   bskip_chars ("-0-9a-zA-Z_!%+~./"); % left limit
   push_mark ();
   skip_chars  ("-0-9a-zA-Z_!%+~./"); % right limit
#else % DOS is supposed here:
   % DOS path names have backslashes and may contain a drive spec.
   bskip_chars ("-0-9a-zA-Z_!%+~./\\:"); % left limit
   push_mark ();
   skip_chars  ("-0-9a-zA-Z_!%+~./\\:"); % right limit
#endif
   variable fn = bufsubstr (); % the file name
   pop_mark_0 ();
   pop_spot ();
   !if (1 == file_status (fn)) error(strcat("File ",fn," not found"));
   () = find_file (fn);
}

define ide_insert_file ()               % ^KR
{
  variable file = 
    read_with_completion ("File:", Null_String, Null_String, 'f');
  push_spot ();
  () = insert_file (file);
  pop_spot ();
}

define ide_select_word ()               % ^KT, Borland IDE facility
{
  ide_skip_word ();
  ide_bskip_word ();
  ide_begin_block ();
  ide_skip_word ();
  ide_end_block ();
  message ("Word selected.");
}

#ifndef IBMPC_SYSTEM

private variable Last_Process_Command = Null_String;

define ide_filter_region ()             % ^K/, Joe extension
{
  variable cmd, tmp_file;
  ide_end_block ();
  cmd = read_mini ("Pipe to command:", Last_Process_Command, Null_String);
  !if (strlen (cmd)) 
     return;
   
  Last_Process_Command = cmd;
  ide_set_bookmark ();
  tmp_file = make_tmp_file ("/tmp/jedpipe");
  cmd = strcat (cmd, " > ", tmp_file, " 2>&1");

  if (pipe_region (cmd)) {
    error ("Process returned a non-zero exit status.");
  }

  () = insert_file (tmp_file);
  ide_begin_block ();
  ide_goto_end_block ();
  ide_end_block ();
  call ("kill_region");
  () = delete_file (tmp_file);
}

#endif

define ide_uppercase_region()           % ^KU
{
  ide_end_block ();
  xform_region('u');
  ide_goto_end_block ();
}
   
define ide_lowercase_region()           % ^KL
{
  ide_end_block ();
  xform_region('d');
  ide_goto_end_block ();
}

%
% These are the predefined bookmarks 0..9, a la Wordstar.
%

define ide_goto_mark_n (n)
{
   variable mrk = _Ide_Bookmarks[n];
   if (mrk == NULL)
     error ("Bookmark not set!");

   ide_set_bookmark ();
   sw2buf (mrk.buffer_name);
   goto_user_mark (mrk);
   message ("done.");
}

define ide_set_mark_n (n)               % ^K0..9
{
  _Ide_Bookmarks[n] = create_user_mark ();
  vmessage ("Bookmark %d set.", n);
}
   
define ide_save_buffer ()               % ^KS
{
  variable file = read_file_from_mini ("Save to file:");
  if (file == "")
    return;
  () = write_buffer (file);
}

define ide_better_help ()
{
  variable file = expand_jedlib_file (Help_File);
  () = read_file (file);
  pop2buf (whatbuf ());
  most_mode ();
  call ("one_window");
  set_readonly (1);
}

% Menu Interface.  Use existing menu definitions in most cases, but
% change definitions in others. 
private define ide_load_popups_hook ()
{
   variable m;

   m = "Global.&File";
   menu_delete_items (m);
   menu_append_item (m, "&Open", "find_file");
   menu_append_item (m, "&Close", ".whatbuf delbuf");
   menu_append_item (m, "&Save", "save_buffer");
   menu_append_item (m, "Save &As", "ide_save_buffer");
   menu_append_item (m, "Save &Buffers", "save_some_buffers");
   menu_append_item (m, "&Insert File", "ide_insert_file");
   menu_append_popup (m, "&Versions");
   menu_append_separator (m);
   menu_append_item (m, "Cance&l Operation", "kbd_quit");
   menu_append_item (m, "S&hell", "shell");
   menu_append_item (m, "E&xit", "exit_jed");
   
   m = "Global.&File.&Versions";
   menu_append_item (m, "RCS &Open File", "rcs_open_file");
   menu_append_item (m, "&Check In/Out", "rcs_check_in_and_out");
   menu_append_item (m, "RCS Read &Log", "rcs_read_log");
   menu_append_item (m, "Backups &On", "backups_on");
   menu_append_item (m, "Backups O&ff", "backups_off");
   
   m = "Global.&Edit";
   menu_delete_items (m);
   menu_append_item (m, "&Begin Region/Rect", "ide_begin_block");
   menu_append_item (m, "&Cut", "ide_delete_block");
   menu_append_item (m, "C&opy", "ide_clear_block");
   menu_append_item (m, "&Paste", "ide_copy_block");
   menu_append_separator (m);
   menu_append_popup (m, "&Rectangles");
   menu_append_popup (m, "Bloc&ks");
   menu_append_popup (m, "&Advanced");
   menu_append_separator (m);
   menu_append_item (m, "Re&format", "format_paragraph");
   menu_append_item (m, "&Undo", "undo");
   
   m = "Global.&Edit.&Advanced";
   menu_append_item (m, "&Compose Character", "ide_insert_any_char");
   if (is_defined ("digraph_cmd"))
     menu_append_item (m, "S&pecial Character", "digraph_cmd");
   menu_append_item (m, "Toggle &Abbrev Mode", "abbrev_mode");
   if (is_defined ("toggle_auto_ispell"))
     menu_append_item (m, "Toggle Auto &Ispell", "toggle_auto_ispell");
   menu_append_item (m, "&Ispell", "ispell");
   menu_append_item (m, "Co&mpletion", "dabbrev");
   menu_append_item (m, "C&enter Line", "center_line");
   menu_append_separator (m);
   menu_append_item (m, "&Start Macro", "begin_macro");
   menu_append_item (m, "S&top Macro", "end_macro");
   menu_append_item (m, "&Replay Last Macro", "execute_macro");
   
   m = "Global.&Edit.&Rectangles";
   menu_append_item (m, "&Cut Rectangle", "kill_rect");
   menu_append_item (m, "C&opy Rectangle", "copy_rect");
   menu_append_item (m, "&Paste Rectangle", "insert_rect");
   menu_append_item (m, "Op&en Rectangle", "open_rect");
   menu_append_item (m, "&Blank Rectangle", "blank_rect");
   
   m = "Global.&Edit.Bloc&ks";
   menu_append_item (m, "&Write to File", "write_region");
   menu_append_item (m, "&Filter", "ide_filter_region");
   menu_append_item (m, "&Sort", "sort");
   menu_append_item (m, "&Upper Case", "ide_uppercase_region");
   menu_append_item (m, "&Lower Case", "ide_lowercase_region");
   menu_append_item (m, "Copy To &Register", "reg_copy_to_register");
   menu_append_item (m, "&Paste From Register", "reg_insert_register");
   menu_append_item (m, "&Comment", "comment_region");
   menu_append_item (m, "U&ncomment", "uncomment_region");
   
   m = "Global.&Search";
   menu_append_item (m, "Search &Forward", "ide_search_forward");
   menu_append_item (m, "Repeat &Last Search", "ide_repeat_search");
   menu_append_item (m, "&Replace", "ide_replace_cmd");
   menu_append_item (m, "Search &Match", "goto_match");
   
   m = "Global.&Buffers";
   % menu_append_separator (m);
   menu_append_item (m, "C&ompile", "compile");
   menu_append_item (m, "&Next Error", "compile_parse_errors");
   menu_append_item (m, "&Previous Error", "compile_previous_error");
   if (is_defined ("gdb_mode"))
     menu_append_item (m, "Debug with &gdb", "gdb_mode");
   
   m = "Global.&Help";
   % menu_append_separator (m);
   menu_append_item (m, "Describe ID&E Mode", "ide_better_help");
}

add_to_hook ("load_popup_hooks", &ide_load_popups_hook);

runhooks ("keybindings_hook", _Jed_Emulation);

% --- End of file ide.sl ---
