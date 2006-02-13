%
%   Emacs like bindings for Jed.
%
%      A subset of the emacs global map is implemented here.  If you
%      are lacking a useful keybinding, contact davis@space.mit.edu

_Jed_Emulation = "emacs";

set_status_line("(Jed %v) Emacs: %b    (%m%a%n%o)  %p   %t", 1);

Help_File = "emacs.hlp";
KILL_LINE_FEATURE = 0;
#ifdef UNIX
enable_flow_control(0);  %turns off ^S/^Q processing (Unix only)
#endif
%
%  The default keybindings for Jed use ^W, ^F, and ^K keymaps.  Emacs
%  does not bind these so turn them off

unset_ctrl_keys ();

set_abort_char (7);		       %  ^G

_Reserved_Key_Prefix = "\003";         %  ^C

%  Jed default binding of the tab char ^I is to indent line.  Here just insert
%  the tab

%  setkey("self_insert_cmd",	"^I");

%% The default binding for the quote keys (", ') is 'text_smart_quote'.
%% Most users do not seem to like this so it is unset here.
%%
setkey("self_insert_cmd",	"\"");
setkey("self_insert_cmd",	"'");

%  setkey("backward_delete_char",	"^?");
setkey("backward_delete_char_untabify",	"^?");

#ifdef IBMPC_SYSTEM
setkey("smart_set_mark_cmd",		"^@^C");
#else
setkey("smart_set_mark_cmd",		"^@");
#endif

setkey("beg_of_line",		"^A");
setkey("previous_char_cmd",	"^B");
setkey("delete_char_cmd",	"^D");
setkey("eol_cmd",		"^E");
setkey("next_char_cmd",		"^F");
setkey("kbd_quit",		"^G");
if (_Backspace_Key != "\x08")
  setkey("help_prefix",		"^H");
setkey("newline",		"^J");
setkey("emacs_yp_kill_line",	"^K");
setkey("emacs_recenter",	"^L");
setkey("newline_and_indent",	"^M");
setkey("next_line_cmd",		"^N");
setkey("emacs_open_line",	"^O");
setkey("previous_line_cmd",	"^P");
setkey("quoted_insert",		"^Q");
setkey("isearch_backward",	"^R");
setkey("isearch_forward",	"^S");
setkey("transpose_chars",	"^T");
setkey("universal_argument",	"^U");
setkey("page_down",		"^V");
setkey("yp_kill_region",	"^W");

%
%    ^X map
%

setkey("list_buffers",		"^X^B");
setkey("exit_jed",		"^X^C");
setkey("list_directory",	"^X^D");
setkey("find_file",		"^X^F");
setkey(".'d'xform_region",	"^X^L");
setkey("delete_blank_lines",	"^X^O");
setkey("toggle_readonly",	"^X^Q");
setkey("find_file_read_only",	"^X^R");
setkey("save_buffer",		"^X^S");
setkey("transpose_lines",	"^X^T");
setkey(".'u'xform_region",	"^X^U");
setkey("find_alternate_file",	"^X^V");
setkey("write_buffer",		"^X^W");
setkey("exchange",		"^X^X");
setkey("evaluate_cmd",		"^X\e");
setkey("set_selective_display",	"^X$");
setkey("compile_parse_errors",	"^X'");
setkey("begin_macro",		"^X(");
setkey("end_macro",		"^X)");
setkey("mark_spot",		"^X/");
setkey("delete_window",		"^X0");
setkey("one_window",		"^X1");
setkey("split_window",		"^X2");
setkey("changelog_add_change",	"^X4A");
setkey("find_buffer_other_window","^X4B");
setkey("find_file_other_window","^X4F");
setkey("digraph_cmd",		"^X8");
setkey("comment_region_or_line","^X;");
setkey("scroll_left",		"^X<");
setkey("whatpos",		"^X=");
setkey("scroll_right",		"^X>");
setkey("whatpos",		"^X?");
setkey("switch_to_buffer",	"^XB");
setkey("dired",			"^XD");
setkey("execute_macro",		"^XE");
setkey("set_fill_column",	"^XF");
setkey("reg_insert_register",	"^XG");
setkey("emacs_mark_buffer",	"^XH");
setkey("insert_file",		"^XI");
setkey("pop_spot",		"^XJ");
setkey("kill_buffer",		"^XK");
#ifdef UNIX
setkey("mail",			"^XM");
#endif
setkey("narrow_to_region",	"^XN");
setkey("other_window",		"^XO");
setkey("macro_query",		"^XQ");
setkey("kill_rect",		"^XRK");
setkey("open_rect",		"^XRO");
setkey("copy_rect",		"^XRR");
setkey("insert_rect",		"^XRY");
setkey("string_rectangle",	"^XRT");
setkey("save_some_buffers",	"^XS");
setkey("undo",			"^XU");
setkey("widen_region",		"^XW");
setkey("reg_copy_to_register",	"^XX");
setkey("enlarge_window",	"^X^");

setkey("yp_yank",		"^Y");
setkey("sys_spawn_cmd",		"^Z");

setkey("goto_match",		"^\\");
setkey("undo",			"^_");

%
%                   The escape map
%
setkey("forward_sexp",	"\e^F");
setkey("backward_sexp",	"\e^B");
setkey("kill_sexp",		"\e^K");
setkey(".bskip_white trim insert_single_space", "\e ");
setkey("bskip_word",		"\eb");
setkey("delete_word",		"\eD");
setkey("bdelete_word",	"\e^?");
setkey("skip_word",		"\ef");
setkey("replace_cmd",		"\e%");
setkey("beg_of_buffer",	"\e<");
setkey("end_of_buffer",	"\e>");
setkey("narrow_paragraph",	"\eN");
setkey ("goto_line_cmd",	"\eg");
setkey("scroll_down_in_place", "\ep");
setkey("scroll_up_in_place",  "\en");
setkey("format_paragraph",	"\eQ");
setkey("page_up",		"\eV");	   %  see emacs_pageup below
setkey("copy_region",		"\ew");
setkey("trim_whitespace",	"\e\\");
setkey ("forward_paragraph",	"\e}");
setkey ("backward_paragraph",	"\e{");
setkey ("yp_yank_pop", "\ey");
setkey ("yp_kill_word", "\eD");
setkey ("yp_bkill_word", "\e^?");
setkey ("yp_copy_region_as_kill", "\ew");


autoload ("find_file_other_window",	"emacsmsc");
autoload ("find_buffer_other_window",	"emacsmsc");
autoload ("find_alternate_file",	"emacsmsc");
autoload ("delete_blank_lines",		"emacsmsc");
autoload ("forward_sexp",		"emacsmsc");
autoload ("backward_sexp",		"emacsmsc");
autoload ("kill_sexp",			"emacsmsc");
autoload ("scroll_up_in_place",		"emacsmsc");
autoload ("scroll_down_in_place",	"emacsmsc");
autoload ("string_rectangle",		"emacsmsc");

%% misc functions
%%
#iffalse
% Is this more emacs-like???  Anyone?
define emacs_pageup ()
{
   variable n = prefix_argument (-1);
   variable m;
   if (n == -1)
     {
	call ("page_up");
	return;
     }
   m = window_line ();
   go_up (n);
   recenter (m);
}
#endif

define emacs_mark_buffer()
{
   mark_buffer ();
   exchange_point_and_mark ();
}

define emacs_open_line()
{
   newline();  go_left_1 ();
}

define emacs_recenter() { recenter(0); }

public define emacs_yp_kill_line ()
{
   variable n = prefix_argument (-1);
   if (n == -1)
     {
	yp_kill_line ();
	return;
     }
   push_mark ();
   if (n != down (n))
     eol ();
   yp_kill_region ();
}

     
define transpose_chars ()
{
   variable c, err;
   err = "Top of Buffer";

   if (eolp()) go_left_1 ();
   !if (left(1)) error(err);
   c = what_char();
   del();
   go_right_1 ();
   insert_char(c);
}

%%0 9 1 { "^U" exch string strcat "digit_arg" exch setkey } _for
%
%  Emacs Universal argument--- bound to ^U
%
define universal_argument ()
{
   variable n, key, count, msg, cu, force;
   n = 4; count = 0; cu = "C-u"; msg = cu; force = 0;

   forever
     {
	!if (force) !if(input_pending(10)) force = 1;

	if (force)
	  {
	     message(msg + "-");
	     update_sans_update_hook (0);
	  }

	msg += " ";
	key = getkey();

	switch(key)
	  {
	     isdigit(char(key)) :
	     key = key - '0';
	     count = 10 * count + key;
	     msg += string(key);
	  }
	  {
	   case 21 :		       %  ^U
	     !if (count) n = 4 * n;
	     count = 0;
	     msg += cu;
	  }
	  {
	     ungetkey(key);
	     !if (count) count = n;
	     count = string(count);
	     n = strlen(count);
	     _for (n, 1, -1)
	       {
		  count; exch();
		  ungetkey(int (substr((), (), 1)));
	       }
	     ungetkey(27);
	     return;
	  }
     }
}

runhooks ("keybindings_hook", _Jed_Emulation);
