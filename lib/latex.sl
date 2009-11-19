% latex.sl
%
% AUC-TeX style LaTeX-mode (v0.18) by Kevin Humphreys <kwh@cogsci.ed.ac.uk>
%
% For JED (v0.97.9b) by John E. Davis <davis@space.mit.edu>
%
% Based on AUC-TeX (v9.1i) by Per Abrahamsen <auc_tex_mgr@iesd.auc.dk>
%
% Differences from AUC-TeX 9: - no shell interaction commands
%                             - simplified insert TeX macro (C-c RET)

% ------------------------------------------------------------------------
%
% TO USE THIS MODE: add the line:
%
%    add_mode_for_extension ("latex", "tex");
%
% to your .jedrc file.
%
% To override the default values below, simply declare and initialize
% the approriate variables in your .jedrc file, e.g.,
%
%        variable LaTeX_default_options = "12pt,a4paper";
%        variable LaTeX_default_document_style = "article";
%
% To make LaTeX209 the default set LaTeX_default_documentstyle variable to a
% non-null value (e.g. "article").
%
% Other variables to change in the latex mode hook include:
%
% variable LaTeX_default_documentclass = "article"; % for LaTeX 2e
% variable LaTeX_default_options       = Null_String;
% variable LaTeX_default_section       = "section";
% variable LaTeX_default_environment   = "itemize";
% variable LaTeX_default_figure_label  = "fig:";
% variable LaTeX_default_table_label   = "tab:";
% variable LaTeX_default_section_label = "sec:";
% variable LaTeX_default_documentstyle = Null_String;  % for LaTeX 2.09
% ------------------------------------------------------------------------

% CHANGES HISTORY:
% --- v0.17-1.0
% Fix synchronization of latex_math_mode/latex_mode and made each of the modes
% local to a buffer.  This allows one buffer to be in latex_mode and another
% to be in latex math mode.  --JED
%
% --- v0.17-0.1?
% have indentation recognize \[ \]
%
% PC: changed most of the '\\\\', as they were not necessary
%     added a newline_indent_hook
%     debugged the indent_calculate_last by adding LaTeX_item_indent to
%      the \begin, etc indent level
%     debugged indent_calculate by subtracting LaTeX_item_indent from
%      the \end, etc level
% ---
%     Modified to work with jed0.97-5.
% --- v0.17-0.18
% fixed indent_line to keep the correct cursor position
% --- v0.16-0.17
% added latex_help (C-c i) using latex.texi info file (should be on C-h C-l)
% added completion for documentclass/style, sections, environments
% added tex_complete_symbol (M-TAB) using ltx-comp.dat file
% added simple tex_insert_macro (C-c RET) with completion from ltx-comp.dat
% added tex_current_env for use by tex_mark_environment etc.
% --- v0.15-0.16 (released  2/11/94)
% added insertions for letter class
% used built-in indent_hook for indentation
% added tex_embrace for font specifier insertion using region
% added latex_embrace_env for environment insertion using region
% added latex_change_env for C-c C-e with prefix arg
% made tex_mark_environment more intuitive by doing fsearch first
% --- v0.14-0.15 (released 30/10/94)
% added an attempt to do indentation
% --- v0.13-0.14 (released 27/10/94)
% added option to use latex209 font selections and documentstyle
% added C-c : for uncomment_region and made uncomment_region more robust
% --- v0.12-0.13 (released 25/10/94)
% always pass env to insert_env functions
% fixed insert_figure_env to prompt for center environment
% fixed insert_list_env to insert \item
% --- v0.11-0.12 (first release 20/10/94)
% added math mode
% added all standard environments

%-------------------------------------------------------------------------
% Load the common definitions if not already loaded.  This also defines
% the TeX-Mode syntax table
require ("texcom");

% ----------------------------------------------------------------------------
% Exported global variables
%
custom_variable ("LaTeX_default_documentclass", "article");
custom_variable ("LaTeX_default_options", "");
custom_variable ("LaTeX_default_section", "section");
custom_variable ("LaTeX_default_environment", "itemize");
custom_variable ("LaTeX_default_figure_label", "fig:");
custom_variable ("LaTeX_default_table_label", "tab:");
custom_variable ("LaTeX_default_section_label", "sec:");
custom_variable ("LaTeX_default_documentstyle", "");

%-----------------------------------------------------------------------------
% documentclass/style symbols
variable LaTeX_classes = "book,article,letter,slides,report";
% section symbols
variable LaTeX_sections = "part,chapter,section,subsection,subsubsection,paragraph,subparagraph";
% environment symbols
variable LaTeX_environments = "document,enumerate,itemize,list,trivlist,picture,tabular,array,eqnarray,equation,minipage,description,figure,table,thebibliography,theindex,sloppypar,tabbing,verbatim,flushright,flushleft,displaymath,math,quote,quotation,";
% problems with above line over 255 chars.
LaTeX_environments += "abstract,center,titlepage,verse";

% Load LaTeX-math-mode as required
autoload("latex_toggle_math_mode", "ltx-math");
autoload("latex_math_mode", "ltx-math");

$1 = "LaTeX-Mode";
!if (keymap_p($1)) make_keymap ($1);

% from tex.sl
definekey ("tex_insert_quote", "\"", $1);
definekey ("tex_insert_quote", "'",  $1);
definekey ("tex_blink_dollar", "$",  $1);
definekey ("tex_ldots",        ".",  $1);

% AUC-TeX bindings
definekey_reserved ("tex_insert_braces",       "{",  $1);
definekey_reserved ("tex_font",                "^F", $1);
definekey_reserved ("latex_environment",       "^E", $1);
definekey_reserved ("latex_section",           "^S", $1);
definekey_reserved ("latex_close_environment", "]",  $1);
definekey_reserved ("latex_insert_item",       "^J", $1);
definekey_reserved ("tex_comment_region",      ";",  $1);
definekey_reserved ("tex_uncomment_region",    ":",  $1);
definekey_reserved ("tex_comment_paragraph",   "%",  $1);
definekey_reserved ("tex_mark_environment",    ".",  $1);
definekey_reserved ("tex_mark_section",        "*",  $1);
definekey_reserved ("latex_toggle_math_mode",  "~",  $1);
definekey_reserved ("tex_insert_macro",        "^M", $1);
definekey_reserved ("latex_help",              "i",  $1);

definekey ("tex_complete_symbol",     "\e^I", $1);
% indentation
definekey ("latex_indent_next_line",   "^J",     $1);
definekey_reserved ("latex_indent_region",      "^Q^R", $1);
definekey_reserved ("latex_indent_section",     "^Q^S", $1);
definekey_reserved ("latex_indent_environment", "^Q^E", $1);

% Font Selection - LaTeX2e

define tex_embrace (pre, post)
{
   if (markp())
     {  % if region is set
	check_region (1);
	pop_mark_1 (); insert(pre);
	pop_spot(); insert(post);
	return;
     }
   % if not mid (or end of) word just insert normally
   go_left_1 ();
   if (looking_at_char(' ')
       or looking_at_char('\t')
       or looking_at_char('\n'))
     {
	go_right_1 ();
	insert(pre); push_spot();
     }
   else
     {  % surround word, including any initial '\'
	go_right_1 (); push_spot();
	bskip_word();
	go_left_1 (); !if (looking_at_char('\\')) go_right_1 ();
	insert(pre);
	skip_word();
     }
   insert(post); pop_spot();
}

define tex_insert_font (pre, post, arg)
{
   if (arg == -1)
     {
	tex_embrace(pre, post);
	return;
     }

   % if prefix argument
   push_spot();
   () = bsearch("\\");
   if (looking_at ("\\text")
       or looking_at ("\\emph"))
     {
	delete_word(); del ();
	insert (pre);
     }
   pop_spot ();
}

define tex_delete_font ()
{
   push_spot();
   () = bsearch("\\");
   if (looking_at ("\\text")
       or looking_at ("\\emph"))
     {
	delete_word(); del ();
	() = fsearch_char ('}');
	del ();
     }
   pop_spot ();
}

define tex_font ()
{
   variable arg = prefix_argument(-1);
   variable brace = "}";
   variable ch;
   flush ("\\text??: b(bf) c(sc) e(em) f(sf) i(it) m(md) n r(rm) s(sl) t(tt) u(up)");

   ch = getkey ();
   if (ch > ' ') ch = (ch | 0x20) - 96;
   switch (ch)
     { case 2  : "\\textbf{";}	       % C-b
     { case 3  : "\\textsc{";}	       % C-c
     { case 5  : "\\emph{";}	       % C-e
     { case 6  : "\\textsf{";}	       % C-f
     { case 9  : "\\textit{";}	       % C-i
     { case 13 : "\\textmd{";}	       % C-m
     { case 14 : "\\textnormal{";}     % C-n
     { case 18 : "\\textrm{";}	       % C-r
     { case 19 : "\\textsl{";}	       % C-s
     { case 20 : "\\texttt{";}	       % C-t
     { case 21 : "\\textup{";}	       % C-u
     { case 4  : tex_delete_font (); return; } % C-d
     { beep (); return;}

   tex_insert_font ( (), brace, arg);
   message (Null_String);
}

% Command Insertion

% environments:

% find previous unmatched \begin statement
define tex_current_env ()
{
   variable env = Null_String, count;

   push_mark(); bob(); narrow(); eob ();
   while (bsearch("\\begin{"))
     {
	count = 1;
	push_spot();
	go_right(7);
	push_mark();
	() = ffind_char ('}');
	env = bufsubstr();
	push_spot();
	while (fsearch(sprintf("\\begin{%s}", env)) and down_1 ())
	  count++;
	pop_spot();
	while (fsearch(sprintf("\\end{%s}", env)) and down_1 ())
	  count--;
	pop_spot();
	if (count > 0) break;
     }
   widen();
   return env;
}

define latex_embrace_env(pre, post)
{
   if (markp())
     {
	check_region (0);
	exchange_point_and_mark ();
	insert (pre);
	pop_mark_1 ();
	insert (post);
	%% This was:
	% check_region(1);
	% pop_mark_1 (); insert(pre);
	% pop_spot(); insert(post);
	return;
     }

   insert(pre);
   indent_line (); eol(); push_spot(); newline();
   insert(post); pop_spot();
}

define latex_insert_simple_env (env)
{
   latex_embrace_env(sprintf("\\begin{%s}\n", env),
		     sprintf("\\end{%s}\n", env));
}

define latex_insert_item_env (env)
{
   variable braces = " ";
   !if (strcmp(env, "description")) braces = "[]";
   latex_embrace_env(sprintf("\\begin{%s}\n\\item%s", env, braces),
		     sprintf("\\end{%s}\n", env));
   () = bfind_char (']');
}

define latex_insert_array_env (env)
{
   variable position = read_mini ("Enter Position:", Null_String, "[htbp]");
   variable format = read_mini ("Enter Format:", Null_String, Null_String);
   latex_embrace_env(sprintf("\\begin{%s}%s{%s}\n", env, position, format),
		     sprintf("\\end{%s}\n", env));
}

define latex_insert_label_env (env)
{
   variable label = read_mini ("Enter Label:", Null_String, Null_String);
   if (strcmp(label, Null_String)) label = sprintf("\\label{%s}\n", label);
   latex_embrace_env(sprintf("\\begin{%s}\n%s", env, label),
		     sprintf("\\end{%s}\n", env));
}

define latex_insert_figure_env (env)
{
   variable label_prefix = Null_String;
   switch (env)
     { case "figure"  : label_prefix = LaTeX_default_figure_label; }
     { case "figure*" : label_prefix = LaTeX_default_figure_label; }
     { case "table"   : label_prefix = LaTeX_default_table_label; }
     { case "table*"  : label_prefix = LaTeX_default_table_label; }

   variable position = read_mini ("Enter Position:", Null_String, "[htbp]");

   variable caption = read_mini ("Enter Caption:", Null_String, Null_String);
   if (strcmp(caption, Null_String))
     caption = sprintf("\\caption{%s}\n", caption);

   variable label = read_mini ("Enter Label:", Null_String, label_prefix);
   if (strcmp(label, label_prefix))
     label = sprintf("\\label{%s}\n", label);
   else
     label = Null_String;

   latex_embrace_env(sprintf("\\begin{%s}%s\n", env, position),
		     sprintf("%s%s\\end{%s}\n", caption, label, env));
   push_spot();

   !if (strcmp(env, "figure") and strcmp(env, "figure*"))
     {
	if (get_y_or_n ("Center Figure"))
	  {
	     newline();
	     latex_insert_simple_env ("center");
	     pop_spot(); go_down(2); push_spot();
	  }
     }
   pop_spot();
}

define latex_insert_list_env (env)
{
   variable label = read_mini ("Enter Default Label:", Null_String, Null_String);
   latex_embrace_env(sprintf("\\begin{%s}{%s}{}\n\\item ", env, label),
		     sprintf("\\end{%s}\n", env));
}

define latex_insert_minipage_env (env)
{
   variable position = read_mini ("Enter Position:", Null_String, "[htbp]");
   variable width = read_mini ("Enter Width:", "4cm", Null_String);
   latex_embrace_env(sprintf("\\begin{%s}%s{%s}\n", env, position, width),
		     sprintf("\\end{%s}\n", env));
}

define latex_insert_picture_env (env)
{
   variable width = read_mini ("Enter Width:", Null_String, Null_String);
   variable height = read_mini ("Enter Height:", Null_String, Null_String);
   variable x_offset = read_mini ("Enter X Offset:", "0", Null_String);
   variable y_offset = read_mini ("Enter Y Offset:", "0", Null_String);
   variable offset = Null_String;

   if (strcmp(x_offset, "0") or strcmp(y_offset, "0"))
     offset = sprintf("(%s,%s)", x_offset, y_offset);

   latex_embrace_env(sprintf("\\begin{%s}(%s,%s)%s\n", env, width, height, offset),
		     sprintf("\\end{%s}\n", env));
}

define latex_insert_bib_env (env)
{
   variable label = read_mini ("Enter Label for Bibitem:", Null_String, "99");
   variable blabel = read_mini ("Enter (Optional) Bibitem Label:", Null_String, Null_String);

   if (strlen(blabel)) blabel = sprintf("[%s]", blabel);
   variable key = read_mini ("Enter Key:", Null_String, Null_String);
   latex_embrace_env(sprintf("\\begin{%s}{%s}\n\\bibitem%s{%s}", env, label, blabel, key),
		     sprintf("\\end{%s}\n", env));
}

define latex_insert_letter_args ()
{
   push_spot ();
   insert ("\\address{}\n\\signature{}\n\\begin{letter}{}\n\\opening{}\n\n\n\n\\closing{}\n\\end{letter}");
   pop_spot (); () = ffind_char ('}');
}

define latex_insert_document_env ()
{
   variable class = read_with_completion(LaTeX_classes, "Enter Document Class:", LaTeX_default_documentclass, Null_String, 's');
   variable options = read_mini ("Enter Class Options:", LaTeX_default_options, Null_String);
   if (strlen (options)) options = sprintf("[%s]", options);

   bob ();
   vinsert ("\\documentclass%s{%s}\n\n", options, class);
   insert ("\\begin{document}\n\n\n\n\\end{document}\n"); go_up(3);
   !if (strcmp(class, "letter")) latex_insert_letter_args();
}

define latex_change_env (env)
{
   push_spot();
   variable oldenv = tex_current_env();
   if (strlen(oldenv))
     {
	() = ffind_char ('{');
	go_right_1 (); push_mark();
	() = ffind_char ('}');
	del_region();
	insert(env);
	goto_spot ();
	if (fsearch(sprintf("\\end{%s}", oldenv)))
	  {
	     () = ffind_char ('{');
	     go_right_1 (); push_mark();
	     () = ffind_char ('}');
	     del_region();
	     insert(env);
	  }
     }
   pop_spot();
}

% This function takes a single argument which will ride on the stack
define latex_insert_env ()
{
   dup ();    %  2 copies, 1 for switch and for function

   switch ()
     { case "document"        : pop (); latex_insert_document_env ();}
     { case "enumerate"       : latex_insert_item_env ( () );}
     { case "itemize"         : latex_insert_item_env ( () );}
     { case "list"            : latex_insert_list_env ( () );}
     { case "trivlist"        : latex_insert_item_env ( () );}
     { case "picture"         : latex_insert_picture_env ( () );}
     { case "tabular"         : latex_insert_array_env ( () );}
     { case "tabular*"        : latex_insert_array_env ( () );}
     { case "array"           : latex_insert_array_env ( () );}
     { case "eqnarray"        : latex_insert_label_env ( () );}
     { case "eqnarray*"       : latex_insert_label_env ( () );}
     { case "equation"        : latex_insert_label_env ( () );}
     { case "minipage"        : latex_insert_minipage_env ( () );}
     { case "description"     : latex_insert_item_env ( () );}
     { case "figure"          : latex_insert_figure_env ( () );}
     { case "figure*"         : latex_insert_figure_env ( () );}
     { case "table"           : latex_insert_figure_env ( () );}
     { case "table*"          : latex_insert_figure_env ( () );}
     { case "thebibliography" : latex_insert_bib_env ( () );}
     { case "theindex"        : latex_insert_item_env ( () );}
     {                          latex_insert_simple_env ( () );}
}

define latex_environment ()
{
   variable arg = prefix_argument(-1);
   variable env = LaTeX_default_environment;

   if (bobp) env = "document";
   env = read_with_completion(LaTeX_environments, "Enter Environment Name:", env, Null_String, 's');
   if (strcmp(env, "document")) LaTeX_default_environment = env;

   if (arg == -1)
     latex_insert_env (env);
   else
     latex_change_env (env);
}

define latex_close_environment ()
{
   push_spot ();
   variable env = tex_current_env();
   pop_spot();
   if (strlen (env))
     {
	vinsert ("\\end{%s}\n", env);
     }
}

% sectioning

define latex_section ()
{
   LaTeX_default_section = read_with_completion (LaTeX_sections, "Enter Section Level:", LaTeX_default_section, Null_String, 's');
   variable name = read_mini ("Enter Section Name:", Null_String, Null_String);
   variable label = read_mini ("Enter Label Name:", Null_String, LaTeX_default_section_label);

   eol (); newline ();
   vinsert ("\\%s{%s}\n", LaTeX_default_section, name);
   if (strcmp(label, LaTeX_default_section_label))
     vinsert ("\\label{%s}\n", label);
   newline ();
}

% commenting

define tex_insert_comment ()
{
   variable c = "% ";

   insert(c);
   % Although this appears to be a bizarre way of coding this, I cannot
   % think of a more compact method.
   while (2 == down(2))
     {
	go_up_1 ();
	bol();
	insert(c);
     }
}

define tex_uncomment_region ()
{
   narrow ();
   push_spot_bob ();
   go_down_1 ();
   do
     {
	go_up_1 (); bol();
	if (looking_at("% ")) deln(2);
     }
   while (down(2) == 2);
   pop_spot ();
   widen ();
}

define tex_comment_region ()
{
   if (-1 == prefix_argument (-1))
     {
	narrow ();
	push_spot_bob ();
	tex_insert_comment ();
	pop_spot ();
	widen ();
	return;
     }

   tex_uncomment_region();
}

define tex_comment_paragraph ()
{
   push_spot ();
   if (-1 == prefix_argument (-1))
     {
	forward_paragraph ();
	tex_isolate_paragraph ();   % pushes spot
	pop_spot ();
	bob (); go_down_1 ();
	tex_insert_comment ();
	widen ();
     }
   else
     {
	push_spot (); go_up_1 (); bol();
	while (looking_at("% ")) { deln(2); go_up_1 (); bol(); }
	pop_spot ();
	while (looking_at("% ")) { deln(2); go_down_1 (); }
     }
   pop_spot ();
}

% marking

define tex_mark_environment ()
{
   push_spot();
   variable env = tex_current_env();
   if (strlen (env))
     {
	set_mark_cmd ();
	pop_spot();
	if (fsearch(sprintf("\\end{%s}", env))) go_down_1 ();
	else eob();
     }
   else pop_spot();
}

define tex_mark_section ()
{
   if (re_bsearch ("\\\\[sub]*section"))
     {
	bol(); set_mark_cmd (); go_down_1 ();
	if (re_fsearch ("\\\\[sub]*section")) { go_up_1 ();}
	else eob();
     }
   else if (bsearch ("\\chapter"))
     {
	bol(); set_mark_cmd (); go_down_1 ();
	if (fsearch ("\\chapter")) { go_up_1 ();}
	else eob();
     }
}

% indentation

variable LaTeX_indent_level     = 2;
variable LaTeX_item_indent      = 2;
variable TeX_brace_indent_level = 2;

% This is a useful function that should be made either an intrinsic or
% more available to other packages.
define current_indentation ()
{  % return column of first non-whitespace character
   push_spot();
   bol_skip_white();
   what_column();		       %  left on stack
   pop_spot();
   () - 1;
}

define tex_brace_count_line ()
{  % this will count braces even inside comments
   variable count = 0;
   push_spot_bol ();
   while (ffind_char ('{')) { count += TeX_brace_indent_level; go_right_1 (); }
   bol();
   while (ffind_char ('}')) { count -= TeX_brace_indent_level; go_right_1 (); }
   pop_spot();
   count;
}

define latex_indent_calculate_last () {} % need dummy to allow recursive defn.
define latex_indent_calculate_last ()
{  % return the indentation of the previous normal text

   bol(); if (bobp()) return 0;

   % ignore comments
   do
     {
	go_up_1 (); bol_skip_white();
     }
   while (looking_at_char ('%') and not(bobp()));

   if (looking_at("\\begin{document}")) return current_indentation();

   if (looking_at("\\end{verbatim"))
     {  % skip to before start of verbatim environment
	if (bsearch("\\begin{verbatim"))
	  return latex_indent_calculate_last();
	else
	  return 0;
     }

   if (looking_at("\\begin{") or looking_at("\\left")
       or looking_at ("\\["))
     LaTeX_indent_level + LaTeX_item_indent;
   else if (looking_at("\\item") or looking_at("\\bibitem"))
     LaTeX_item_indent;
   else
     0;

   () + tex_brace_count_line() + current_indentation();
}

define latex_indent_calculate ()
{  % return the indentation of the current text

   bol_skip_white();

   % keep verbatim environments flush left
   if (looking_at("\\begin{verbatim") or looking_at("\\end{verbatim"))
     return 0;

   if (looking_at("\\end{") or looking_at("\\right")
       or looking_at ("\\]"))
     return latex_indent_calculate_last() - LaTeX_item_indent - LaTeX_indent_level;

   if (looking_at("\\item") or looking_at("\\bibitem"))
     return latex_indent_calculate_last() - LaTeX_item_indent;

   latex_indent_calculate_last();
}

define latex_indent_line ()
{
   push_spot();
   latex_indent_calculate();  % on stack
   goto_spot ();
   bol_trim();
   whitespace( () );
   pop_spot();
}

define latex_newline_indent_line ()
{
   if (bolp ())
     {
	newline ();
	return;
     }
   newline();
   latex_indent_line();
   bol_skip_white();
   %eol();
}

define latex_indent_next_line ()
{
   push_spot();
   go_down_1 ();
   latex_indent_line();
   pop_spot();
}

define latex_indent_region ()
{
   check_region(1);
   pop_mark_1 ();
   push_mark();
   latex_indent_line(); % set initial line indentation before narrowing
   pop_spot();

   push_spot();
   go_up_1 ();
   narrow();
   bob();
   while (down_1 ())   % indent line by line (ie slowly)
     latex_indent_line(); % a good latex_format_paragraph would be nice...
   widen();
   pop_spot();
}

define latex_indent_section ()
{
   tex_mark_section();
   latex_indent_region();
}

define latex_indent_environment ()
{
   tex_mark_environment();
   go_down_1 ();
   latex_indent_region();
}

% misc

define tex_insert_braces ()
{
   insert ("{}");
   go_left_1 ();
}

define latex_insert_item ()
{
   eol ();
   insert ("\n\\item ");
   latex_indent_line();
}

% symbol completion

define tex_complete_symbol ()
{
   variable symbol, completion;
   variable insertbuf = whatbuf(), searchbuf = "*ltx-comp*";

   !if (bufferp(searchbuf))
     {
	sw2buf(searchbuf);
	insert_file( expand_jedlib_file("ltx-comp.dat") ); bob();
	set_buffer_modified_flag(0);
	sw2buf(insertbuf);
	bury_buffer(searchbuf);
     }

   push_spot();
   push_mark();
   bskip_word();
   symbol = bufsubstr();

   setbuf(searchbuf);

   !if (bol_fsearch(sprintf("\\%s", symbol))) bob(); % wrap to start

   if (bol_fsearch(sprintf("\\%s", symbol)))
     {
	go_right_1 ();
	go_right(strlen(symbol));
	push_mark_eol();
	completion = bufsubstr();
     }
   else
     {
	setbuf(insertbuf);
	pop_mark_0 ();
	pop_spot();
	error("No completion found");
     }

   setbuf(insertbuf);

   goto_spot ();
   push_mark();
   !if (ffind_char (' ')) eol();
   del_region();
   insert(completion);
   pop_spot();
}

variable LaTeX_macros = Null_String;

define tex_insert_macro ()
{
   variable insertbuf = whatbuf(), searchbuf = "*ltx-comp*";

   !if (strcmp(LaTeX_macros, Null_String))
     {
	!if (bufferp(searchbuf))
	  {
	     sw2buf(searchbuf);
	     insert_file( expand_jedlib_file("ltx-comp.dat") );
	     set_buffer_modified_flag(0);
	     sw2buf(insertbuf);
	     bury_buffer(searchbuf);
	  }
	sw2buf(searchbuf); bob();
	while (bol_fsearch("\\"))
	  {
	     go_right_1 ();
	     push_mark_eol ();
	     LaTeX_macros += bufsubstr() + ",";
	  }
	sw2buf(insertbuf);
     }

   variable macro = read_with_completion (LaTeX_macros, "Enter Macro Name:", Null_String, Null_String, 's');
   vinsert ("\\%s", macro);
   go_left_1 (); !if (looking_at_char('}')) go_right_1 ();
}

% info file interface

variable LaTeX_help = Null_String;

define latex_help ()
{
   variable info_file = "(latex)";
   variable latex_buf = whatbuf(), info_buf = "*Info*";
   push_spot(); variable guess = tex_current_env(); pop_spot();
   !if (strcmp(guess, "document")) guess = Null_String;

   ERROR_BLOCK
     {
	sw2buf(latex_buf);
     }

   !if (strcmp(LaTeX_help, Null_String))
     {
#iffalse
	info_find_node(sprintf("%sList of Commands", info_file));
#else
	info_find_node(sprintf("%sCommands", info_file));
#endif

	sw2buf(info_buf);
	if (fsearch("* Menu:")) go_down_1 ();
	while (bol_fsearch("* "))
	  {
	     go_right(2);
	     push_mark_eol ();
	     go_left(2);
	     LaTeX_help += bufsubstr() + ",";
	  }
     }

   sw2buf(latex_buf);
   push_spot();
   !if (looking_at_char('\\'))
     {
	go_left_1 ();
	!if (looking_at_char('\\'))
	  {
	     goto_spot ();
	     bskip_word(); go_left_1 ();
	  }
     }
   if (looking_at_char('\\'))
     {
	push_mark();
	skip_word();
	guess = bufsubstr();
     }
   pop_spot();

   guess = read_with_completion(LaTeX_help, "Describe LaTeX command:", guess, Null_String, 's');
   info_find_node(sprintf("%s%s", info_file, guess));
   pop2buf(info_buf);
}

private define init_menu (menu)
{
   menu_append_item (menu, "Font", "tex_font");
   menu_append_item (menu, "Environment", "latex_environment");
   menu_append_item (menu, "Section", "latex_section");
   menu_append_item (menu, "Comment Region", "tex_comment_region");
   menu_append_item (menu, "Uncomment Region", "tex_uncomment_region");
   menu_append_item (menu, "Comment Paragraph", "tex_comment_paragraph");
   menu_append_item (menu, "Mark Environment", "tex_mark_environment");
   menu_append_item (menu, "Mark Section", "tex_mark_section");
   menu_append_item (menu, "Indent Region", "latex_indent_region");
   menu_append_item (menu, "Indent Section", "latex_indent_section");
   menu_append_item (menu, "Indent Environment", "latex_indent_environment");
}

%!%+
%\function{latex_mode}
%\synopsis{latex_mode}
%\usage{Void latex_mode ();}
%\description
% This mode is designed to facilitate the task of editing latex files.
% It calls the function \var{latex_mode_hook} if it is defined.  In addition,
% if the abbreviation table \var{"TeX"} is defined, that table is used.
%
% The default key-bindings for this mode include:
%#v+
%    "tex_insert_braces"       "^C{"
%    "tex_font"                "^C^F"
%    "latex_environment"       "^C^E"
%    "latex_section"           "^C^S"
%    "latex_close_environment" "^C]"
%    "latex_insert_item"       "^C^J"
%    "tex_comment_region"      "^C;"
%    "tex_uncomment_region"    "^C:"
%    "tex_comment_paragraph"   "^C%"
%    "tex_mark_environment"    "^C."
%    "tex_mark_section"        "^C*"
%    "latex_toggle_math_mode"  "^C~"
%    "tex_insert_macro"        "^C^M"
%    "tex_complete_symbol"     "\e^I"
%    "latex_help"              "^Ci"
%    "latex_indent_next_line"   "^J"
%    "latex_indent_region"      "^C^Q^R"
%    "latex_indent_section"     "^C^Q^S"
%    "latex_indent_environment" "^C^Q^E"
%#v-
%!%-
define latex_mode ()
{
   variable tex = "TeX";
   variable quote = "`";

   use_keymap ("LaTeX-Mode");
   set_mode ("LaTeX", 0x1 | 0x20);

   set_buffer_hook ("par_sep", "tex_paragraph_separator");
   set_buffer_hook ("wrap_hook", "tex_wrap_hook");
   set_buffer_hook ("indent_hook", "latex_indent_line");
   set_buffer_hook ("newline_indent_hook", "latex_newline_indent_line");

   % latex math mode will map this to something else.
   %local_unsetkey (quote);
   %local_setkey ("quoted_insert", quote);

   mode_set_mode_info ("LaTeX", "init_mode_menu", &init_menu);
   mode_set_mode_info ("LaTeX", "fold_info", "%{{{\r%}}}\r\r");

   use_syntax_table ("TeX-Mode");
   run_mode_hooks("latex_mode_hook");
}

% overload alternative function definitions for LaTeX 2.09 if required
if (strcmp(LaTeX_default_documentstyle, ""))
  () = evalfile("latex209");

provide ("latex");
