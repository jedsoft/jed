% This file contains jed's native keybindings.

_Jed_Emulation = "jed";
_Reserved_Key_Prefix = "\003";         %  ^C

set_status_line(" : Jed %v : %b     =(%m%a%n%o)=    (%p)   %t", 1);

define left_justify_line ()
{
   push_spot(); bol_trim(); pop_spot();
}  

define insert_other_window()
{
  if (2 != nwindows) return;
  otherwindow();
  whatbuf();
  otherwindow();
  insbuf(());
}
set_abort_char (7);		       %  ^G
unset_ctrl_keys ();

setkey("query_replace_match",	"\e%");
setkey("forward_paragraph",	"\e]");
setkey("backward_paragraph",	"\e{");
setkey("forward_paragraph",	"\e}");

setkey("dabbrev", 		"^A");
setkey("beg_of_line",		"^B");
setkey("page_down",		"^D");
setkey("eol_cmd",		"^E");

setkey("occur",			"^FA");
setkey("search_backward",	"^FB");
setkey("search_forward",	"^FF");
setkey("isearch_backward",	"^FIB");
setkey("isearch_forward",	"^FIF");
setkey("re_search_backward",	"^FWB");
setkey("re_search_forward",	"^FWF");

setkey("kbd_quit",		"^G");
setkey("indent_line",		"\t");
setkey("yp_kill_word",		"^J");


setkey("set_mark_cmd",		"^K^B");
setkey("insert_file",		"^K^I");
setkey("double_line",		"^K^L");
setkey("bkmrk_goto_mark", 	"^K\r");
setkey("yp_yank",		"^K^P");
setkey("replace_cmd",		"^K^R");
setkey("yp_kill_region",	"^K^V");
setkey("write_buffer",		"^K^W");
setkey("begin_macro",		"^K(");
setkey("end_macro",		"^K)");
setkey("goto_column_cmd",	"^KC");
setkey("evaluate_cmd",		"^KD");
setkey("exit_jed",		"^KE");
setkey("find_file",		"^KG");
setkey("yp_copy_region_as_kill","^KK");
setkey("goto_line_cmd",		"^KL");
setkey("bkmrk_set_mark", 	"^KM");
setkey("execute_macro",		"^KX");

setkey("left_justify_line",	"^K^?");

setkey("yp_kill_line",		"^L");
setkey("newline_and_indent",	"^M");
setkey("redraw",		"^R");
setkey("page_up",		"^U");
setkey("delete_char_cmd",	"^V");

setkey("insert_other_window",	"^W\t");
setkey("one_window",		"^W1");
setkey("split_window",		"^W2");
setkey("other_window",		"^Wo");
setkey("delete_window",		"^W0");

setkey("list_buffers",		"^X^B");
setkey("list_directory",	"^X^D");
setkey("transpose_lines",	"^X^T");
setkey("exchange",		"^X^X");
setkey("evaluate_cmd",		"^X\e");
setkey("set_selective_display",	"^X$");
setkey("compile_parse_errors",	"^X'");
setkey("changelog_add_change",	"^X4a");
setkey("digraph_cmd",		"^X8");
setkey("uncomment_region_or_line","^X:");
setkey("comment_region_or_line","^X;");
setkey("whatpos",		"^X?");
setkey("switch_to_buffer",	"^XB");
setkey("set_fill_column",	"^XF");
setkey("reg_insert_register",	"^XG");
setkey("kill_buffer",		"^XK");
setkey("narrow_to_region",	"^XN");
setkey("macro_query",		"^XQ");
setkey("save_some_buffers",	"^XS");
setkey("toggle_readonly",	"^XT");
setkey("undo",			"^XU");
setkey("widen_region",		"^XW");
setkey("reg_copy_to_register",	"^XX");
setkey("enlarge_window",	"^X^");

#ifdef UNIX VMS
setkey("mail",			"^XM");
#endif

setkey ("sys_spawn_cmd",	"^Z");
setkey ("goto_match",		"^\\");
setkey ("undo",			"^_");

#ifndef IBMPC_SYSTEM
setkey("bob",			"^K\eOA");
setkey("bob",			"^K\e[A");
setkey("eob",			"^K\eOB");
setkey("eob",			"^K\e[B");
setkey("scroll_left",		"^K\eOC");
setkey("scroll_left",		"^K\e[C");
setkey("scroll_right",		"^K\eOD");
setkey("scroll_right",		"^K\e[D");
setkey("next_wind_up",		"^W\e[A");
setkey("next_wind_dn",		"^W\e[B");
#else
setkey("bob",			"^K^@H");
setkey("bob",			"^K\eOx");
setkey("bob",			"^K‡H");
setkey("eob",			"^K^@P");
setkey("eob",			"^K\eOr");
setkey("eob",			"^K‡P");
setkey("next_wind_up",		"^W^@H");
setkey("next_wind_up",		"^W\eOx");
setkey("next_wind_up",		"^W‡H");
setkey("next_wind_dn",		"^W^@P");
setkey("next_wind_dn",		"^W\eOr");
setkey("next_wind_dn",		"^W‡P");
#endif

#ifdef UNIX
setkey("compile_parse_errors",	"^(k1)");
setkey("compile_previous_error","^(k2)");
#endif

setkey("yp_yank_pop",		"\ey");
setkey("yp_bkill_word",		"\e^?");


runhooks ("keybindings_hook", _Jed_Emulation);
