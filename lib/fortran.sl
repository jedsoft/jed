%  Fortran mode		-* SLang -*-
%
% Loading this file, then executing 'fortran_mode' will start fortran mode
% on the current buffer.

custom_variable ("Fortran_Continue_Char", "&");
custom_variable ("Fortran_Comment_String", "C");
custom_variable ("Fortran_Indent_Amount", 2);

% Skip past labels and continuation char
private define bol_skip_to_code_start ()
{
   bol ();
   skip_chars ("0-9 \t");
   if (looking_at(Fortran_Continue_Char)) go_right_1 ();
   skip_white ();
}

private define indent_line_to_col (col)
{
   bol_skip_white ();
   skip_chars ("0-9");
   trim ();
   if (looking_at (Fortran_Continue_Char))
     {
	insert_spaces (6 - what_column());
	go_right_1 (); trim();
	col += Fortran_Indent_Amount;
     }
   insert_spaces (col - what_column());
}


% fortran indent routine
define fortran_indent ()
{
   variable goal = 7;		% at top of buffer it should be 7 n'est pas?
   variable cs = CASE_SEARCH;
   variable ch;

   push_spot ();
   push_spot ();
   CASE_SEARCH = 0;	% Fortran is not case sensitive
   while (up_1 ())
     {
	bol_skip_white();
	if (eolp() or looking_at(Fortran_Continue_Char)) continue;
	bol_skip_to_code_start ();
	goal = what_column ();

	if (goal == 1) continue;

	if (looking_at("do ") or looking_at("else"))
	  goal += Fortran_Indent_Amount;
	else if (looking_at("if ") or looking_at("if("))
	  {
	     % We want to check for 'then' so take care of continuations 
	     push_spot ();
	     while (down_1 ())
	       {
		  bol_skip_white ();
		  !if (looking_at (Fortran_Continue_Char))
		    {
		       go_up_1 ();
		       bol ();
		       break;
		    }
	       }
	     if (ffind ("then")) goal += Fortran_Indent_Amount;
	     pop_spot ();
	  }
	break;
     }

   % now check current line
   pop_spot ();
   push_spot ();
   bol_skip_to_code_start ();

   push_mark ();
   skip_chars ("a-zA-Z");
   variable word = strlow (bufsubstr ());
   if ((word == "end") || (word == "endif") || (word == "enddo")
       || (word == "continue") || (word == "else"))
     goal -= Fortran_Indent_Amount;

   CASE_SEARCH = cs;		% done getting indent
   if (goal < 7) goal = 7;
   pop_spot ();

   bol_skip_white ();

   % after the label or continuation char and indent the rest to goal

   ch = char(what_char());
   switch (ch)
     {
	isdigit (ch) :		% label

	if (what_column () >= 6)
	  {
	     bol_trim ();
	     insert_single_space ();
	  }
	indent_line_to_col (goal);
     }
     {
	case Fortran_Continue_Char :	% continuation character
	bol_trim (); insert_spaces (5);
	indent_line_to_col (goal);
     }
     {
	not (bolp()) or eolp ():	% general case
	bol_trim ();
	goal--;
	insert_spaces (goal);
     }
   pop_spot ();
   skip_white ();
}

define fortran_is_comment ()
{
   bol ();
   skip_chars (" \t0-9");
   bolp () and not (eolp());
}

define fortran_newline ()
{
   variable p, cont;

   if (bolp ())
     {
	newline ();
	return;
     }

   fortran_indent ();
   push_spot ();
   bskip_white (); trim ();

   if (what_column () > 72)
     {
	push_spot ();
	bol_skip_white();
	!if (bolp()) message ("Line exceeds 72 columns.");
	pop_spot ();
     }

   p = _get_point ();
   bskip_chars("-+*=/,(");

   cont = (p != _get_point ());

   if (fortran_is_comment ()) cont = 0;

   bol_skip_white ();
   if (looking_at("data ")) cont = 0;

   pop_spot ();

   newline ();
   insert_single_space ();
   if (cont) insert(Fortran_Continue_Char);
   fortran_indent ();
}

define fortran_continue_newline ()
{
   fortran_newline ();

   push_spot ();
   bol_skip_white ();
   if (looking_at(Fortran_Continue_Char)) pop_spot ();
   else
     {
	insert (Fortran_Continue_Char);
	pop_spot ();
	fortran_indent ();
	go_right_1 ();
	skip_white ();
     }
}

%
%   electric labels
%
define fortran_electric_label ()
{
   insert_char (LAST_CHAR);
   push_spot ();

   if (fortran_is_comment ()) pop_spot ();
   else
     {
	bol_skip_white ();
	skip_chars ("0-9"); trim ();
	pop_spot ();
	fortran_indent ();
     }
}

% fortran comment/uncomment functions

define fortran_uncomment ()
{
   push_spot ();
   if (fortran_is_comment ())
     {
	bol ();
	if (looking_at (Fortran_Comment_String))
	  deln (strlen (Fortran_Comment_String));
	else del ();
     }

   fortran_indent ();
   pop_spot ();
   go_down_1 ();
}

define fortran_comment ()
{
   !if (fortran_is_comment ())
     {
	push_spot_bol ();
	insert (Fortran_Comment_String);
     }
   pop_spot ();
   go_down_1 ();
}

%
% Look for beginning of current subroutine/function
%
define fortran_beg_of_subprogram ()
{
   variable cs = CASE_SEARCH;

   CASE_SEARCH = 0;
   do
     {
	bol_skip_white ();
	if (_get_point ())
	  {
	     if (looking_at ("program")
		 or looking_at ("function")
		 or looking_at ("subroutine")) break;
	  }
     }
   while (up_1 ());
   CASE_SEARCH = cs;
}

%
% Look for end of current subroutine/function
%
define fortran_end_of_subprogram ()
{
   variable cs = CASE_SEARCH;
   CASE_SEARCH = 0;

   do
     {
	bol_skip_white ();
	if (looking_at ("end"))
	  {
	     go_right (3);
	     skip_white ();
	     if (eolp ()) break;
	  }
     }
   while (down_1 ());
   CASE_SEARCH = cs;
}

define fortran_mark_subprogram ()
{
   fortran_end_of_subprogram ();
   go_down_1 ();
   set_mark_cmd ();
   fortran_beg_of_subprogram ();
   bol ();
}

%
% shows a ruler for FORTRAN source. Press any key to get rid of
%
define fortran_ruler ()
{
   variable c = what_column ();
   variable r = window_line ();

   bol ();
   push_mark ();
   insert ("    5 7 10   15   20   25   30   35   40   45   50   55   60   65   70\n");
   insert ("{    }|{ |    |    |    |    |    |    |    |    |    |    |    |    | }\n");

   goto_column (c);
   if (r <= 2) r = 3;
   recenter (r);
   message ("Press SPACE to get rid of the ruler.");
   update_sans_update_hook (1);
   () = getkey ();
   bol ();
   del_region ();
   goto_column (c);
   flush_input ();
   recenter (r);
}

define fortran_prev_next_statement (dirfun)
{
   while (@dirfun ())
     {
	bol ();
	skip_chars ("^0-9 \t\n");
	!if (_get_point ()) break;
     }
   () = goto_column_best_try (7);
}
%
% moves cursor to the next statement, skipping comment lines
%
define fortran_next_statement ()
{
   fortran_prev_next_statement (&down_1);
}

%
% moves cursor to the previous fortran statement, skipping comments
%
define fortran_previous_statement ()
{
   fortran_prev_next_statement (&up_1);
}

%
% main entry point into the fortran mode
%

$1 = "Fortran";
!if (keymap_p ($1)) make_keymap ($1);

definekey ("fortran_comment",		"\e;",	$1);
definekey ("fortran_uncomment",		"\e:",	$1);
definekey ("fortran_continue_newline",	"\e\r",	$1);
% next two really needed?  not if using EDT or Emacs
definekey ("self_insert_cmd",		char('\''),	$1);
definekey ("self_insert_cmd",		char('"'),	$1);
definekey ("fortran_beg_of_subprogram",	"\e^A",	$1);
definekey ("fortran_end_of_subprogram",	"\e^E",	$1);
definekey ("fortran_mark_function",		"\e^H", $1);
definekey_reserved ("fortran_next_statement",		"^N",	$1);
definekey_reserved ("fortran_previous_statement",	"^P",	$1);
definekey_reserved ("fortran_ruler",			"^R", $1);
_for (0, 9, 1)
{
   $2 = ();
   definekey ("fortran_electric_label", string($2), $1);
}


% Set up syntax table
$1 = "Fortran";
create_syntax_table ($1);
define_syntax ("!", "", '%', $1);
define_syntax ("([", ")]", '(', $1);
define_syntax ('"', '"', $1);
define_syntax ('\'', '"', $1);
% define_syntax ('\\', '\\', $1);
define_syntax ("0-9a-zA-Z_", 'w', $1);        % words
define_syntax ("-+0-9eEdD", '0', $1);   % Numbers
define_syntax (",.", ',', $1);
define_syntax ('D', '#', $1);
define_syntax ("-+/*=", '+', $1);
set_syntax_flags ($1, 1 | 2);
set_fortran_comment_chars ($1, "^0-9 \t\n");

% Fortran 77 keywords + include, record, structure, while:
% backspace block
% call character common complex continue
% data dimension do double
% else end enddo endfile endif entry equivalence exit external
% format function
% goto
% if implicit include inquire integer intrinsic
% logical
% parameter pause precision program
% real return rewind
% save stop subroutine
% then
% while

() = define_keywords ($1, "dogoifto", 2);
() = define_keywords ($1, "end", 3);
() = define_keywords ($1, "calldataelseexitgotoopenreadrealsavestopthen", 4);
() = define_keywords ($1, "blockcloseenddoendifentrypauseprintwhilewrite", 5);
() = define_keywords ($1, "commondoubleformatrecordreturnrewind", 6);
() = define_keywords ($1, "complexendfileincludeinquireintegerlogicalprogram", 7);
() = define_keywords ($1, "continueexternalfunctionimplicit", 8);
() = define_keywords ($1, "backspacecharacterdimensionintrinsicparameterprecisionstructure", 9);
() = define_keywords ($1, "subroutine", 10);
() = define_keywords ($1, "equivalence", 11);

() = define_keywords_n ($1, "eqgegtleltneor", 2, 1);
() = define_keywords_n ($1, "andnot", 3, 1);
() = define_keywords_n ($1, "true", 4, 1);
() = define_keywords_n ($1, "false", 5, 1);

%!%+
%\function{fortran_mode}
%\synopsis{fortran_mode}
%\description
% Mode designed for the purpose of editing FORTRAN files.
% After the mode is loaded, the hook 'fortran_hook' is called.
% Useful functions include
% 
%  Function:                    Default Binding:
%   fortran_continue_newline          ESC RETURN
%     indents current line, and creates a continuation line on next line.
%   fortran_comment                   ESC ;
%     comments out current line
%   fortran_uncomment                 ESC :
%     uncomments current line
%   fortran_electric_label            0-9
%     Generates a label for current line or simply inserts a digit.
%   fortran_next_statement            ^C^N
%     moves to next fortran statementm skips comment lines
%   fortran_previous_statement        ^C^P
%     moves to previous fortran statement, skips comment lines
%   fortran_ruler                     ^C^R
%     inserts a ruler above the current line. Press any key to continue
%   fortran_beg_of_subprogram         ESC ^A
%     moves cursor to beginning of current subroutine/function
%   fortran_end_of_subprogram         ESC ^E
%     moves cursor to end of current subroutine/function
%   fortran_mark_subprogram           ESC ^H
%     mark the current subroutine/function
% 
% Variables include:
%   Fortran_Continue_Char   --- character used as a continuation character.
%     By default, its value is "&"
%   Fortran_Comment_String  --- string used by 'fortran_comment' to
%     comment out a line.  The default string is "C ";
%   Fortran_Indent_Amount   --- number of spaces to indent statements in
%                               a block.  The default is 2.
%!%-
define fortran_mode ()
{
   variable mode = "Fortran";
   set_mode (mode, 0x4 | 0x10);
   use_keymap (mode);
   use_syntax_table (mode);
   set_buffer_hook ("indent_hook", "fortran_indent");
   set_buffer_hook ("newline_indent_hook", "fortran_newline");
   mode_set_mode_info (mode, "fold_info", "C{{{\rC}}}\r\r");
   run_mode_hooks ("fortran_hook");
}
