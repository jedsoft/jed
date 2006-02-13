% Brief editor emulation
% I do not think that this will work very well at all on non-IBMPC systems
% because of the heavy dependence on keys that are meaningless.
_Jed_Emulation = "brief";

% Since alt keys are used, make sure that they are enabled.
ALT_CHAR = 0;

set_status_line("(Jed %v) Brief: %b    (%m%a%n%o)  %p   %t", 1);
Help_File = Null_String;

define brief_home ()
{
   if (bolp ())
     {
	if (window_line () == 1) bob ();
	else goto_top_of_window ();
     }
   bol ();
}


define brief_end ()
{
   if (eolp ())
     {
	if (window_line () == window_info ('r')) eob ();
	else goto_bottom_of_window ();
     }
   eol ();
}

define brief_line_to_eow ()
{
   recenter (window_info ('r'));
}

define brief_line_to_bow ()
{
   recenter (1);
}

define brief_line_to_mow ()
{
   recenter (window_info ('r') / 2);
}

define brief_set_bkmrk_cmd (n)
{
   ungetkey (n + '0');
   bkmrk_set_mark ();
}

define brief_delete_to_bol ()
{
   push_mark ();
   bol();
   del_region ();
}

define brief_toggle_case_search ()
{
   CASE_SEARCH = not (CASE_SEARCH);
}

variable Brief_Regexp_Search = 0;
define brief_toggle_regexp ()
{
   Brief_Regexp_Search = not (Brief_Regexp_Search);
}

variable Brief_Search_Forward = 1;
define brief_search_cmd ()
{
   if (Brief_Search_Forward)
     {
	if (Brief_Regexp_Search) re_search_forward ();
	else search_forward ();
     }
   else
     {
	if (Brief_Regexp_Search) re_search_backward ();
	else search_backward ();
     }
}

define brief_reverse_search ()
{
   Brief_Search_Forward = not (Brief_Search_Forward);
   brief_search_cmd ();
}

define brief_line_mark ()
{
   bol ();
   set_mark_cmd ();
   eol ();
}

variable Brief_Use_Rectangle = 0;
define brief_yank ()
{
   if (Brief_Use_Rectangle)
     {
	insert_rect ();
     }
   else call ("yank");
}

define brief_copy_region ()
{
   if (Brief_Use_Rectangle)
     {
	copy_rect ();
     }
   else call ("copy_region");
}

define brief_kill_region ()
{
   if (Brief_Use_Rectangle)
     {
	kill_rect ();
     }
   else call ("kill_region");
}

define brief_delete ()
{
   if (markp ())
     {
        if (Brief_Use_Rectangle)
          {
	     kill_rect ();
          } 
	else 
	  {
	     del_region ();
          }
	return;
     }
   del ();
}


define brief_set_mark_cmd ()
{
   Brief_Use_Rectangle = 0;
   smart_set_mark_cmd ();
}

define brief_set_column_mark ()
{
   Brief_Use_Rectangle = 1;
   set_mark_cmd ();
   message ("Column mark set.");
}

unsetkey ("^K");
unsetkey ("^X");
unsetkey ("^W");
unsetkey ("^F");
	
%setkey ("bdelete_word", "^@");       %  Ctrl Bksp  
setkey ("brief_home", "\xE0G");	       %  Home
setkey ("brief_home", "Ow");	       %  Home
setkey ("brief_end", "\xE0O");	       %  End
setkey ("brief_end", "Oq");	       %  End
setkey ("brief_line_to_bow", "^T");
setkey ("brief_line_to_mow", "^C");
setkey ("brief_line_to_eow", "^B");
setkey ("brief_delete","\xE0S");       %  Delete
setkey ("brief_delete","\xOn");        %  Delete
setkey ("undo", "\eOR");	       %  Keypad Star  
setkey ("bskip_word", "\xE0s");	       %  Ctrl Left  
setkey ("bskip_word", "^@s");	       %  Ctrl Left  
setkey ("skip_word", "\xE0t");	       %  Ctrl Right  
setkey ("skip_word", "^@t");	       %  Ctrl Right  
setkey ("bob", "\xE0\d132");	       %  Ctrl Pgup  
setkey ("bob", "^@\d132");	       %  Ctrl Pgup  
setkey ("eob", "\xE0v");	       %  Ctrl Pgdn  
setkey ("eob", "^@v");	       %  Ctrl Pgdn  
setkey ("goto_top_of_window", "\xE0w");%  Ctrl Home  
setkey ("goto_top_of_window", "^@w");%  Ctrl Home  
setkey ("goto_bottom_of_window", "\xE0u");%  Ctrl End  
setkey ("goto_bottom_of_window", "^@u");%  Ctrl End  
setkey ("brief_yank", "\xE0R");	       %  Insert  
setkey ("brief_yank", "Op");	       %  Insert  
setkey ("brief_copy_region", "\eOm");	       % Keypad Plus    
setkey ("brief_kill_region", "\eOS");	       % Keypad Minus    
setkey ("find_file", "^@");	       %  Alt E    
setkey ("help_prefix", "^@#");	       %  Alt H  
setkey ("write_buffer", "^@");       %  Alt O  
setkey ("save_buffer", "^@");	       %  Alt W  
setkey ("exit_jed", "^@-");	       %  Alt X  
setkey ("replace_cmd", "^@@");	       %  Key F6  
setkey ("brief_search_cmd", "^@?");     %  Key F5  
setkey ("brief_reverse_search", "^@l");    %  Alt F5  
setkey ("brief_search_cmd", "^@X"); %  Shift F5  
setkey ("brief_toggle_case_search", "^@b");%  Ctrl F5  
setkey ("brief_toggle_regexp", "^@c"); %  Ctrl F6

setkey ("page_down", "^D");
setkey ("page_up", "^E");
setkey ("brief_delete_to_bol", "^K");
setkey ("isearch_forward", "^S");

setkey ("brief_set_mark_cmd", "^@");       %  Alt A  
setkey ("list_buffers", "^@0");	       %  Alt B  
setkey ("brief_set_column_mark", "^@.");%  Alt C    
setkey ("delete_line", "^@ ");	       %  Alt D  
setkey ("goto_line_cmd", "^@\"");      %  Alt G  
setkey ("toggle_overwrite", "^@");   %  Alt I  
setkey ("bkmrk_goto_mark", "^@$");     %  Alt J  
setkey ("kill_line", "^@%");	       %  Alt K  
setkey ("set_mark_cmd", "^@2");	       %  Alt M  
setkey ("switch_to_buffer", "^@1");    %  Alt N  
setkey ("insert_file", "^@");	       %  Alt R  
setkey ("brief_search_cmd", "^@");  %  Alt S  
setkey ("replace_cmd", "^@");	       %  Alt T  
setkey ("undo", "^@");	       %  Alt U

setkey ("brief_line_mark", "^@&");    %  Alt L  


setkey (".0 brief_set_bkmrk_cmd", "^@\d129");%  Alt 0  
setkey (".1 brief_set_bkmrk_cmd", "^@x");%  Alt 1  
setkey (".2 brief_set_bkmrk_cmd", "^@y");%  Alt 2  
setkey (".3 brief_set_bkmrk_cmd", "^@z");%  Alt 3  
setkey (".4 brief_set_bkmrk_cmd", "^@{");%  Alt 4  
setkey (".5 brief_set_bkmrk_cmd", "^@|");%  Alt 5  
setkey (".6 brief_set_bkmrk_cmd", "^@}");%  Alt 6  
setkey (".7 brief_set_bkmrk_cmd", "^@~");%  Alt 7  
setkey (".8 brief_set_bkmrk_cmd", "^@");%  Alt 8  
setkey (".9 brief_set_bkmrk_cmd", "^@\d128");%  Alt 9  
setkey ("delete_word", "^@");	       %  Alt Bksp  
setkey ("goto_match", "^Q[");
setkey ("goto_match", "^Q\e");
setkey ("goto_match", "^Q]");
setkey ("goto_match", "^Q^]");

%  Not sure about this!!!
unsetkey ("\e\e");
setkey ("end_macro", "\e\e");

% These two cannot be bound.
%setkey ("scroll_left", "");             %  Shift End
%setkey ("scroll_right", "");             %  Shift Home

runhooks ("keybindings_hook", _Jed_Emulation);
