%
%  Wordstar Mode for JED
%
%   Put the line: () = evalfile ("wordstar.sl");
%   in your jed.rc startup file.
%
%   Additions and changes by Guido Gonzato, guido@ibogfs.df.unibo.it
%   Now the wordstar mode is more compatible with the true-blue WordStar,
%   the Turbo IDE editor, and jstar.
%   Some features still missing:
%
%   o  hilighted blocks a la WordStar
%   o  move block across buffers not implemented yet (tip: setbuf())
%
%   Please send me requests and bug reports, should you find any.
%
%   Last modified: 5 June 1997

Help_File = "wordstar.hlp";

_Reserved_Key_Prefix = NULL;
_Jed_Emulation = "wordstar";

CASE_SEARCH = 1;			% I prefer this way
variable Ws_Bookmark_Exist = 1;		% internal use

set_status_line (" Jed %v : %b   (%m%n)   (%p)   %t", 1);

set_abort_char (30);    % ^^ (Control 6 on most keyboards)
% Note, the above command BREAKS ^G emacs abort.
unsetkey("^B");
unsetkey("^F");
unsetkey("^K");
unsetkey("^U");
unsetkey("^W");
unsetkey("^X");
%
% Basic commands: cursor movement, delete, search & replace, etc.
%
setkey ("begin_macro","\e(");
setkey ("delete_char_cmd","^G");
setkey ("delete_line","^Y");
setkey ("end_macro","\e)");
setkey ("execute_macro","\eE");
setkey ("execute_macro","\ee");
setkey ("format_paragraph", "^B");
setkey ("next_char_cmd","^D");
setkey ("previous_char_cmd","^S");
setkey ("ws_previous_line_cmd","^E");
setkey ("undo", "^U");
setkey ("ws_bdelete_word","\eo");
setkey ("ws_bskip_word","^A");
setkey ("ws_delete_word","^T");
setkey ("ws_insert_last_block", "^P");
setkey ("ws_next_line_cmd","^X");
setkey ("ws_page_down","^C");
setkey ("ws_page_up","^R");
setkey ("ws_repeat_search","^L");
setkey ("ws_skip_word","^F");
setkey ("ws_window_down","^Z");
setkey ("ws_window_up","^W");
%
% Control-Q keys  --- hope you figure out how to pass ^Q/^S through system
%
% In case you _cannot_ figure that out, you can use ESC instead of ^Q;
% for instance, ESC-A instead of ^Q-A. For wordstar bookmarks, use
% ESC-ESC-digit.
% ESC-X is not bound, for it's already pretty useful... use ESC-^X instead.
%
setkey (".0 ws_goto_mark_n","\e\e0");
setkey (".0 ws_goto_mark_n","^Q0");
setkey (".1 ws_goto_mark_n","\e\e1");
setkey (".1 ws_goto_mark_n","^Q1");
setkey (".2 ws_goto_mark_n","\e\e2");
setkey (".2 ws_goto_mark_n","^Q2");
setkey (".3 ws_goto_mark_n","\e\e3");			   
setkey (".3 ws_goto_mark_n","^Q3");
setkey (".4 ws_goto_mark_n","\e\e4");
setkey (".4 ws_goto_mark_n","^Q4");
setkey (".5 ws_goto_mark_n","\e\e5");
setkey (".5 ws_goto_mark_n","^Q5");
setkey (".6 ws_goto_mark_n","\e\e6");
setkey (".6 ws_goto_mark_n","^Q6");
setkey (".7 ws_goto_mark_n","\e\e7");
setkey (".7 ws_goto_mark_n","^Q7");
setkey (".8 ws_goto_mark_n","\e\e8");
setkey (".8 ws_goto_mark_n","^Q8");
setkey (".9 ws_goto_mark_n","\e\e9");
setkey (".9 ws_goto_mark_n","^Q9");
setkey ("ws_goto_begin_block", "\eB");
setkey ("ws_goto_begin_block", "^QB");
setkey ("ws_goto_begin_block", "^Q^B");
setkey ("ws_goto_end_block", "\eK");
setkey ("ws_goto_end_block", "^QK");
setkey ("ws_goto_end_block", "^Q^K");
setkey ("kill_line","\eY");
setkey ("kill_line","^QY");
setkey ("kill_line","^Q^Y");
setkey ("ws_bob","\eR");
setkey ("ws_bob","^QR");
setkey ("ws_bob","^Q^R");
setkey ("ws_bol","\eS");
setkey ("ws_bol","^QS");
setkey ("ws_bol","^Q^S");
setkey ("ws_eob","\eC");
setkey ("ws_eob","^QC");
setkey ("ws_eob","^Q^C");
setkey ("ws_eol","\eD");
setkey ("ws_eol","^QD");
setkey ("ws_eol","^Q^D");
setkey ("ws_goto_bottom_of_window","\e^X");  % M-X is already reserved!
setkey ("ws_goto_bottom_of_window","^QX");
setkey ("ws_goto_bottom_of_window","^Q^X");
setkey ("ws_goto_line_cmd","\eI");
setkey ("ws_goto_line_cmd","^QI");
setkey ("ws_goto_line_cmd","^Q^I");
setkey ("ws_goto_prev","^QP");
setkey ("ws_goto_top_of_window","^QE");
setkey ("ws_goto_top_of_window","^Q^E");
setkey ("ws_goto_top_of_window","^\eE");
setkey ("ws_replace_cmd","\eA");
setkey ("ws_replace_cmd","^QA");
setkey ("ws_replace_cmd","^Q^A");
setkey ("ws_search_forward","\eF");
setkey ("ws_search_forward","^QF");
setkey ("ws_search_forward","^Q^F");
setkey ("ws_toggle_case","^QT");
setkey ("ws_toggle_case","^Q^T");
%
% Control-K map
%
setkey (".0 ws_set_mark_n","^K0");
setkey (".1 ws_set_mark_n","^K1");
setkey (".2 ws_set_mark_n","^K2");
setkey (".3 ws_set_mark_n","^K3");
setkey (".4 ws_set_mark_n","^K4");
setkey (".5 ws_set_mark_n","^K5");
setkey (".6 ws_set_mark_n","^K6");
setkey (".7 ws_set_mark_n","^K7");
setkey (".8 ws_set_mark_n","^K8");
setkey (".9 ws_set_mark_n","^K9");
setkey ("exit_jed","^KX");
setkey ("exit_jed","^K^X");
setkey ("find_file", "^KE");
setkey ("find_file", "^K^E");
setkey ("kill_buffer","^KQ");
setkey ("kill_buffer","^K^Q");
setkey ("one_window", "^KI");
setkey ("one_window", "^K^I");
setkey ("save_buffer","^KD");
setkey ("save_buffer","^K^D");
setkey ("suspend", "^KZ");
setkey ("suspend", "^K^Z");
setkey ("switch_to_buffer","^KP");
setkey ("switch_to_buffer","^K^P");
setkey ("ws_begin_block","^KB");
setkey ("ws_begin_block","^K^B");
setkey ("ws_comment_block", "^K;");
setkey ("ws_copy_block","^KC");
setkey ("ws_copy_block","^K^C");
setkey ("ws_delete_block","^KY");
setkey ("ws_delete_block","^K^Y");
setkey ("ws_end_block","^KK");
setkey ("ws_end_block","^K^K");
setkey ("ws_filter_region","^K/");
setkey ("ws_insert_file","^KR");
setkey ("ws_insert_file","^K^R");
setkey ("ws_lowercase_region","^KL");
setkey ("ws_lowercase_region","^K^L");
setkey ("ws_move_block", "^KV");
setkey ("ws_move_block", "^K^V");
setkey ("ws_save_buffer","^KS");
setkey ("ws_save_buffer","^K^S");
setkey ("ws_select_word","^KT");
setkey ("ws_select_word","^K^T");
setkey ("ws_uppercase_region","^KU");
setkey ("ws_uppercase_region","^K^U");
setkey ("ws_write_region","^KW");
setkey ("ws_write_region","^K^W");
%
% Implementation
%
!if (is_defined("_Ws_Bookmarks"))
{
  % user marks are of type 128
   $1 = 13;

  % bookmarks 0..9 are for the user; bookmark 10 is used by some of the
  % following functions; bookmark 11 and 12 mark the beginning and end of
  % the block (used by ^QB and ^QK)

  variable _Ws_Bookmarks = Mark_Type [$1];
  variable _Ws_Bookmarks_Exist = Integer_Type [$1];
  variable i;
  for (i = 0; i < $1; i++)
    _Ws_Bookmarks_Exist [i] = -1; % not initialized
}

%  ws_set_bookmark () and ws_goto_bookmark () are implemented to provide
%  a more Wordstar-ish way of copying and moving blocks, and of moving 
%  around generally.

define ws_set_bookmark ()
{
   _Ws_Bookmarks[10] = create_user_mark ();
   Ws_Bookmark_Exist = 1;
}
   
define ws_goto_bookmark ()
{
   variable mrk = _Ws_Bookmarks[10];
   
   sw2buf (user_mark_buffer (mrk));
   goto_user_mark (mrk);
}
%
% Basic commands: cursor movement, delete, f&r, etc.
%
define ws_execute_macro ()
{
  ws_set_bookmark ();
 call ("execute_macro");
}

define ws_next_line_cmd ()
{
  ws_set_bookmark ();
  call ("next_line_cmd");
}

define ws_previous_line_cmd ()
{
  ws_set_bookmark ();
  call ("previous_line_cmd");
}

define ws_page_down ()
{
  ws_set_bookmark ();
  call ("page_down");
}

define ws_page_up ()
{
  ws_set_bookmark ();
  call ("page_up");
}

define ws_bob ()
{
  ws_set_bookmark ();
  bob ();
}

define ws_eob ()
{
  ws_set_bookmark ();
  eob ();
}

define ws_bol ()
{
  ws_set_bookmark ();
  bol ();
}

define ws_eol ()
{
  ws_set_bookmark ();
  eol ();
}

define ws_goto_bottom_of_window ()
{
  ws_set_bookmark ();
  goto_bottom_of_window ();
}

define ws_goto_top_of_window ()
{
  ws_set_bookmark ();
  goto_top_of_window ();
}

define ws_window_up ()		% ^W
{
   recenter (window_line() + 1);
}

define ws_window_down ()	% ^Z
{
   recenter (window_line() + 1);
}

define ws_goto_line_cmd ()	% ^QI
{
  ws_set_bookmark ();
  goto_line_cmd ();
}

define ws_replace_cmd ()
{
  ws_set_bookmark ();
  replace_cmd ();
}

define ws_search_forward ()
{
  ws_set_bookmark ();
  search_forward ();
}

define ws_toggle_case ()
{
  CASE_SEARCH = not(CASE_SEARCH);
  vmessage ("Case search is %d ", CASE_SEARCH);
}

define ws_repeat_search ()	% ^L
{
  ws_set_bookmark ();
  go_right_1 ();
   !if (fsearch(LAST_SEARCH)) error ("Not found.");
}

% !"#$%&'()*+,-./:;<=>?@[\]^`{|}~ , but not _

%  define ws_delete_word ()	% ^T
%  {
% variable p = _get_point ();
% push_mark ();
% skip_chars ("\n\t !\"#$%&'()*+,-./:;<=>?@[\]^`{|}~"); 
% if (_get_point () == p) {
%   skip_chars ("^\n\t !\"#$%&'()*+,-./:;<=>?@[\]^`{|}~"); 
% }
% del_region ();
%  }

define ws_delete_word ()	% ^T
{
  variable p = _get_point ();
  push_mark ();
  skip_chars ("a-zA-Z0-9");
  if (_get_point() == p) skip_chars (" \n\t"); 
  if (_get_point() == p) go_right_1 ();
  del_region ();
}

define ws_bdelete_word ()	% ESC-O
{
  variable p = _get_point ();
  push_mark ();
  bskip_chars ("a-zA-Z0-9");
  if (_get_point () == p) bskip_chars (" \n\t"); 
  if (_get_point () == p) go_left_1 ();
  del_region ();
}

define ws_skip_word ()		% ^F
{
  variable p = _get_point ();
  push_mark ();
  skip_chars ("^\n\t !\"#$%&'()*+,-./:;<=>?@[\]^`{|}~"); 
  if (_get_point () == p) {
    skip_chars ("\n\t !\"#$%&'()*+,-./:;<=>?@[\]^`{|}~");
    skip_chars ("^\n\t !\"#$%&'()*+,-./:;<=>?@[\]^`{|}~"); 
  }
  pop_mark_0 ();
}

define ws_bskip_word ()		% ^A
{
  variable p = _get_point ();
  push_mark ();
  bskip_chars ("\n\t !\"#$%&'()*+,-./:;<=>?@[\]^`{|}~");
  bskip_chars ("^\n\t !\"#$%&'()*+,-./:;<=>?@[\]^`{|}~");
  pop_mark_0 ();
}

% Blocks: ^K-something
%
% Not implemented:
%    ^K^H  - Hide or unhide the currently selected block
%
% the blocks are very different.  Here we cheat.  Two marks are pushed--
% One at beginning of block and one at end.  Assumption is that the spots
% we see are the ones we put.

variable WS_Mark_Pushed = 0;

define ws_begin_block ()
{
   loop (WS_Mark_Pushed) pop_mark_0 ();
   WS_Mark_Pushed = 1;
   push_mark(); push_mark ();
   call ("set_mark_cmd");
   _Ws_Bookmarks[11] = create_user_mark ();
   message ("Begin Block.");
}

% copies block to internal buffer-- preserves block
variable WS_Block_Buffer = " *ws-internal*";

define ws_copy_block_to_buffer ()
{
   if (WS_Mark_Pushed < 2) error ("Block Not defined.");
   push_spot ();
   pop_mark_1 (); % end of block
   dupmark(); pop();  % dup beginning because we want to keep it
   whatbuf ();
   setbuf (WS_Block_Buffer); erase_buffer ();
   setbuf(());
   copy_region (WS_Block_Buffer);
   push_mark ();
   pop_spot ();
}

define ws_end_block ()
{
   if (WS_Mark_Pushed != 1)
     {
	loop (WS_Mark_Pushed) pop_mark_0 (); 
	WS_Mark_Pushed = 0;
	error ("Begin Block First!");
     }
   
   !if (markp()) 
     {
	WS_Mark_Pushed = 0;
	error ("Wordstar Error.");
     }
   
   WS_Mark_Pushed = 2;
   pop_mark_0 ();           % pops visible mark from begin block
   push_mark ();
   ws_copy_block_to_buffer ();
   _Ws_Bookmarks[12] = create_user_mark ();
   message ("Block Defined.");
}

define ws_delete_block ()
{
   ws_copy_block_to_buffer ();
   pop_mark_1 (); del_region ();
}

define ws_write_region ()
{
  ws_copy_block_to_buffer ();
  pop_mark_1 (); write_region ();
}
  

define ws_copy_block ()
{
   ws_set_bookmark (); % we will return to this location afterwards
   insbuf (WS_Block_Buffer);
   ws_goto_bookmark ();
   message ("Block Copied.");
}

define ws_comment_region ()
{
  variable cbeg, cmid, cend;
  variable c, c1, celm, extra, smode, mode;
  
  if (WS_Mark_Pushed != 2) {
    error ("Block Undefined!");
  }
  
  (smode, mode) = what_mode ();

  !if (strcmp(smode,"Text")) {	% Text mode
    return;
  }
  
  !if (strcmp(smode,"TeX")) {	% TeX mode
    cbeg = "%  ";
    cmid = "%  ";
    cend = Null_String;
  }

  !if (strcmp(smode,"html")) {	% html mode
    cbeg = "<!--";
    cmid = " -*-"; % Null_String;
    cend = " -->";
  }
  
  !if (strcmp(smode,"C")) {	% C mode
    cbeg = "/* ";
    cmid = " * ";
    cend = " */";
  }
  
  !if (strcmp(smode,"SL")) {	% Slang mode
    cbeg = "%  ";
    cmid = "%  ";
    cend = Null_String;
  }

  !if (strcmp(smode,"Fortran")) {	% Fortran mode
    cbeg = "C  ";
    cmid = "C  ";
    cend = Null_String;
  }
  
  % the remaining is almost the same as c_comment_region ()
  
  check_region (1);
  exchange_point_and_mark ();
  c = what_column ();
  
  narrow ();
  bob (); 

  USER_BLOCK0 
    {
      extra = ();
      celm = ();
      bol_skip_white ();
      c1 = what_column ();
      if (c1 > c)
        {
	  goto_column (c);
	  insert (celm);
	  trim ();
	  whitespace (c1 - what_column () + extra);
	}
      else 
	{
	  if (eolp ()) goto_column (c);
	  insert (celm);
	}
    }
   
  X_USER_BLOCK0 (cbeg, 0);
   
  while (down_1 ())
    {
      if (down_1 ()) {  % check for last but one
	up_1 ();
        X_USER_BLOCK0 (cmid, 0); % 1
      }
    }
  widen ();
  
  if (looking_at(cmid))
    {
      deln (3);
    }

  X_USER_BLOCK0 (cend, 0);

  pop_spot ();
}

define ws_comment_block ()	% ^K;
{
   ws_set_bookmark ();
   pop_mark_1 ();
   ws_comment_region ();
   ws_goto_bookmark ();
   message ("Block Commented.");
}

define ws_move_block ()		% ^KV
% Warning - doesn't work across buffers
{ 
   ws_copy_block_to_buffer ();
   ws_set_bookmark ();
   insbuf (WS_Block_Buffer);
   ws_delete_block ();
   ws_goto_bookmark ();
   message ("Block Moved.");
}

define ws_insert_file ()	% ^KR
{
  variable file = 
    read_with_completion ("File:", Null_String, Null_String, 'f');
  ws_set_bookmark ();
  insert_file (file);
  ws_goto_bookmark ();
  message ("File inserted.");
}

define ws_select_word ()	% ^KT, Borland IDE facility
{
  ws_set_bookmark (); % afterwards, we return to this location
  loop (WS_Mark_Pushed) pop_mark_0 ();
  WS_Mark_Pushed = 1;
  ws_skip_word ();
  ws_bskip_word ();
  push_mark(); push_mark ();
  call ("set_mark_cmd");
  ws_skip_word ();
  WS_Mark_Pushed = 2;
  pop_mark_0 ();           % pops visible mark from begin block
  push_mark ();
  ws_copy_block_to_buffer ();
  ws_goto_bookmark ();
  message ("Word Selected.");
}

define ws_insert_last_block ()
{
   if (bufferp(WS_Block_Buffer)) insbuf (WS_Block_Buffer);
}

variable Last_Process_Command = Null_String;

define ws_filter_region ()	% ^K/, Joe extension
{
   variable cmd, tmp_file;
   cmd = read_mini ("Pipe to command:", Last_Process_Command, Null_String);
   !if (strlen (cmd)) return;
   
   Last_Process_Command = cmd;

   ws_copy_block_to_buffer ();
   pop_mark_1 ();
   tmp_file = make_tmp_file ("/tmp/jedpipe");
   cmd = strcat (cmd, " > ", tmp_file, " 2>&1");
   
   !if (dupmark ()) error ("Mark not set.");
   
   if (pipe_region (cmd)) 
     {
	error ("Process returned a non-zero exit status.");
     }
   del_region ();
   () = insert_file (tmp_file);
   () = delete_file (tmp_file);
}

define ws_uppercase_region()	% ^KU
{
  ws_copy_block_to_buffer ();
  pop_mark_1 ();
  xform_region('u');
}
   
define ws_lowercase_region()	% ^KL
{
  ws_copy_block_to_buffer ();
  pop_mark_1 ();
  xform_region('d');
}
%
% These are predefined bookmarks 0..9, a la Wordstar.
%
define ws_goto_mark_n (n)
{
   variable mrk;

   if (_Ws_Bookmarks_Exist[n] != 1)
     error ("Bookmark not set!");

   ws_set_bookmark ();
   mrk = _Ws_Bookmarks[n];
   sw2buf (user_mark_buffer (mrk));
   goto_user_mark (mrk);
   message ("done.");
}


define ws_set_mark_n (n)	% ^K0
{
  _Ws_Bookmarks[n] = create_user_mark ();
  _Ws_Bookmarks_Exist[n] = 1;
  vmessage ("Bookmark %d set.", n);
}
   

define ws_save_buffer ()  % !!! buggy !!!
{
   variable file, dir, flags;
   (file, dir, , flags) = getbuf_info ();
   file = read_file_from_mini ("Save to file:");
   () = write_buffer (file);
}
%
% These are ^QB and ^QK
%
define ws_goto_begin_block ()	% ^QB
{
  variable mrk = _Ws_Bookmarks[11];
  sw2buf (user_mark_buffer (mrk));
  ws_set_bookmark ();
  goto_user_mark (mrk);
}

define ws_goto_end_block ()	% ^QK
{
  variable mrk = _Ws_Bookmarks[12];
  sw2buf (user_mark_buffer (mrk));
  ws_set_bookmark ();
  goto_user_mark (mrk);
}

define ws_goto_prev ()		% ^QP
{
   if (Ws_Bookmark_Exist != 1) 
     error ("No previous location!");
   
    ws_goto_bookmark ();
}

runhooks ("keybindings_hook", _Jed_Emulation);

% --- End of file wordstar.sl ---
