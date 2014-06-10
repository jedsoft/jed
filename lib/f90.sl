% -*- mode: slang; mode: fold -*-
%  Free/Fixed format source F90 mode

custom_variable ("F90_Continue_Char", "&");
custom_variable ("F90_Comment_String", "!");
custom_variable ("F90_Indent_Amount", 2);
custom_variable ("F90_Default_Format", "free");   %  or "fixed"

private define get_format_mode ()
{
   if (blocal_var_exists ("F90_Mode_Format"))
     return get_blocal_var ("F90_Mode_Format");
   return 0;
}

private define set_format_mode (x)
{
   create_blocal_var ("F90_Mode_Format");
   set_blocal_var (strlow(x) == "free", "F90_Mode_Format");
}

private variable Zero_Indent_Words =
  ["PROGRAM"];

private variable Block_Indent_Keywords =
  [
   "ASSOCIATE",
   "CASE",
   "CONTAINS",
   "DO",
   "ELSE",
   "FORALL",
   "IF",
   "SELECT",
   "TYPE",
   "WHERE",
   "ElSEIF", "ELSEWHERE",
   "INTERFACE",
   "FUNCTION", "SUBROUTINE", "MODULE",
   Zero_Indent_Words,
  ];

private define goto_end_of_code_line ()
{
   eol ();
   while (not bolp ())
     {
	bskip_white ();
	if (0 == parse_to_point ())
	  break;
	go_left (1);
     }
}

private define bskip_non_code ()
{
   while (not bobp())
     {
	bskip_white ();
	if (bolp ())
	  {
	     go_up (1);
	     continue;
	  }
	variable p = parse_to_point ();
	if (p)
	  {
	     if ((p != -2) || (0 == bfind ("!")))
	       go_left (1);

	     continue;
	  }

	push_mark ();
	bol ();
	if (looking_at ("#"))
	  {
	     pop_mark_0 ();
	     continue;
	  }
	pop_mark_1 ();
	return;
     }
}

private define line_continues ()
{
   push_spot ();
   goto_end_of_code_line ();
   variable c = blooking_at ("&");
   pop_spot ();
   return c;
}

private define line_was_continued ()
{
   push_spot ();
   bol ();
   bskip_non_code ();
   variable c = blooking_at ("&");
   pop_spot ();
   return c;
}

% If looking at one of the words, return 1 otherwise 0.
% If wordp reference is non-NULL, set it to word.
% The words array is assumed to be uppercase
private define looking_at_word (words, wordp)
{
   push_spot ();
   push_mark ();
   skip_chars ("A-Za-z0-9_");
   variable word = strup (bufsubstr ());
   pop_spot ();
   if (wordp != NULL) @wordp = word;
   return any (words == word);
}

private define blooking_at_words (words, wordp)
{
   push_spot ();
   push_mark ();
   bskip_chars ("A-Za-z0-9_");
   variable word = strup (bufsubstr ());
   pop_spot ();
   if (wordp != NULL) @wordp = word;
   return any (words == word);
}

private define bskip_to_bos ()
{
   bskip_non_code ();
   while (line_was_continued () && up_1())
     ;
   bol_skip_white ();
}

%{{{ Free Format Functions

private define line_contains_word (f)
{
   push_spot ();
   EXIT_BLOCK
     {
	pop_spot();
     }

   return (ffind (f)
	   && (re_looking_at (strcat ("\\C", f,  "[\t ]+[A-Za-z_]")))
	   && (0 == parse_to_point ()));
}

% This function does not preserve the point
private define compute_free_f90_indent ();   %  recursive
private define compute_free_f90_indent ()
{
   variable start_pos = create_user_mark ();

   variable goal = 0;		       %  0 signifies not set

   bol_skip_white ();
   if (looking_at_word (Zero_Indent_Words, NULL))
     return 1;

   if (looking_at ("!"))
     {
	push_spot ();
	if (up_1 ())
	  {
	     bol_skip_white ();
	     if (looking_at ("!"))
	       {
		  goal = what_column ();
		  pop_spot ();
		  return goal;
	       }
	  }
	pop_spot ();
     }

   variable is_paren = looking_at (")") || looking_at ("]")
     || looking_at ("/)");
   if (1 == find_matching_delimiter (')'))
     {
	goal = what_column () + (is_paren == 0);
	% If the match is at the end of the line, reset the indent point
	% This should be made configurable.  This idea is to avoid excess
	% whitespace
	%go_right_1 (); skip_white ();
	skip_chars ("(/ \t");
	ifnot (looking_at ("&"))
	  {
	     goto_user_mark (start_pos);
	     return goal;
	  }
	goal = 0;
     }
   goto_user_mark (start_pos);

   variable bol_word;
   while (up_1 ())
     {
	bskip_non_code ();
	variable eol_word;
	() = blooking_at_words ("", &eol_word);

	variable will_continue = line_continues ();
	variable was_continued = line_was_continued ();

	if (will_continue == 0)
	  {
	     was_continued = 0;
	     bskip_to_bos ();
	  }

	if (will_continue && (goal == 0))
	  {
	     go_left_1();
	     bskip_white ();
	     if (blooking_at (")"))
	       {
		  was_continued = 0;
		  bskip_to_bos ();
	       }
	  }

	bol_skip_white ();

	% Check for the presence of labels
	ifnot (was_continued)
	  {
	     variable p = _get_point ();
	     skip_chars (" \t0-9");
	     if (p != _get_point ())
	       {
		  push_spot ();
		  goal = compute_free_f90_indent ();
		  pop_spot ();
	       }
	  }

	if (goal == 0)
	  goal = what_column ();

	if (eol_word == "THEN")
	  {
	     ifnot (was_continued)
	       goal += F90_Indent_Amount;
	     break;
	  }

	% Check for the presence of labels
%	ifnot (was_continued)
%	  skip_chars (" \t0-9");%  numeric label

	push_mark ();
	skip_chars ("A-Za-z0-9_ \t");
	if (looking_at (":") && not looking_at ("::"))
	  {
	     skip_chars (": \t");
	     pop_mark_0();
	  }
	else pop_mark_1();

	ifnot (looking_at_word (Block_Indent_Keywords, &bol_word))
	  {
	     variable is_func = 0;
	     if (bol_word != "END") foreach (["FUNCTION", "SUBROUTINE"])
	       {
		  variable f = ();
		  if (line_contains_word (f))
		    {
		       is_func = 1;
		       break;
		    }
	       }
	     % pop_spot ();
	     if (is_func == 0)
	       {
		  if (was_continued) goal -= F90_Indent_Amount;
		  if (will_continue) goal += F90_Indent_Amount;
		  break;
	       }
	  }

	goal += F90_Indent_Amount;

	switch (bol_word)
	  {
	   case "IF":
	     if ((will_continue == 0) && (eol_word != "THEN"))
	       goal -= F90_Indent_Amount;
	  }
	  {
	   case "TYPE":
	     if (will_continue == 0)
	       {
		  go_right(4);
		  skip_white ();
		  if (looking_at_char ('('))
		    {
		       ifnot (line_contains_word ("FUNCTION")
			      || (line_contains_word ("SUBROUTINE")))
			 goal -= F90_Indent_Amount;
		    }
	       }
	  }
	  {
	   case "MODULE":
	     if (will_continue == 0)
	       {
		  go_right(6);
		  skip_white ();
		  if (looking_at ("PROCEDURE"))
		    goal -= F90_Indent_Amount;
	       }
	  }
	  {
	   case "WHERE":
	     if (will_continue == 0)
	       {
		  goal -= F90_Indent_Amount;
		  go_right (5);
		  skip_white ();
		  if (looking_at ("(")
		      && (1 == find_matching_delimiter ('(')))
		    {
		       go_right (1);
		       skip_white ();
		       if (looking_at ("!") || eolp ())
			 goal += F90_Indent_Amount;
		    }
	       }
	     else goal += F90_Indent_Amount;
	  }
	  {
	     if (will_continue)
	       goal += F90_Indent_Amount;
	  }
	break;
     }

   % now check current line
   variable curr_word;
   goto_user_mark (start_pos);
   bol ();
   skip_chars ("0-9 \t");
   if (looking_at_word (["CASE", "ELSE", "ELSEWHERE", "ELSEIF", "CONTAINS"], &curr_word))
     {
	goal -= F90_Indent_Amount;
	if ((curr_word == "CASE") && (bol_word == "SELECT"))
	  {
	     goal += F90_Indent_Amount;
	  }
     }
   else if (0 == strncmp (curr_word, "END", 3))
     {
	go_right (3);
	skip_white ();
	if (looking_at_word (Zero_Indent_Words, NULL))
	  return 1;
	if (looking_at_word (Block_Indent_Keywords, &curr_word))
	  {
	     goal -= F90_Indent_Amount;
	     if ((curr_word == "SELECT") && (bol_word != "SELECT"))
	       {
		  goal -= F90_Indent_Amount;
	       }
	  }
     }
   return goal;
}

private define free_f90_indent ()
{
   variable cs = CASE_SEARCH; CASE_SEARCH = 0;
   EXIT_BLOCK
     {
	CASE_SEARCH = cs;
     }

   variable start_pos = create_user_mark ();
   variable goal = compute_free_f90_indent ();

   bol_skip_white ();
   variable col = what_column ();
   skip_chars ("0-9");
   variable label_end_col = 0;
   if (col != what_column())
     {
	ifnot (line_was_continued ())
	  {
	     if (col != 2)
	       {
		  bol_trim ();
		  insert_single_space ();
	       }
	     skip_chars ("0-9");
	     label_end_col = what_column();
	     ifnot (looking_at (" ")) insert_single_space ();
	     skip_white ();
	  }
	else bol_skip_white ();
     }

   col = what_column ();
   if (goal != col)
     {
	bskip_white ();
	if (label_end_col)
	  {
	     goal -= label_end_col;
	     if (goal < 1) goal = 1;
	     goal++;
	  }
	if ((label_end_col == 0) || (goal > 1))
	  {
	     col = what_column ();
	     skip_white ();
	     if (goal - 1 != what_column() - col)
	       {
		  bskip_white ();
		  trim ();
		  insert_spaces (goal-1);
	       }
	  }
     }
   eol ();

   if (_get_point() > 132)
     vmessage ("Line %d contains more than 132 bytes", what_line());

   goto_user_mark (start_pos);
   bskip_white ();
   if (bolp())
     {
	skip_white ();
	return;
     }
   goto_user_mark (start_pos);
}


private define free_f90_is_comment ()
{
   bol ();
   looking_at("!");
}

private define old_free_f90_newline ()
{
   variable p, cont , cont1;

   if (bolp ())
     {
	newline ();
	return;
     }

   free_f90_indent ();
   push_spot ();
   bskip_white (); trim ();

   if (what_column () > 72)
     {
	push_spot ();
	bol_skip_white();
	ifnot (bolp()) message ("Line exceeds 72 columns.");
	pop_spot ();
     }

   p = _get_point ();
   bskip_chars("-+*=/,(&<>");

   cont = (p != _get_point ());
   cont1 = cont;

   if ( cont )
     {
	if ( looking_at( "&" ) )
	  {
	     cont1 = 0;
	  }
     }

   if (free_f90_is_comment ()) cont = 0;

   bol_skip_white ();
   if (looking_at("data ")) cont = 0;

   pop_spot ();

   if (cont1)
     {
	insert( " " );
	insert(F90_Continue_Char);
     }
   newline ();
   if ( cont )
     {
	insert(F90_Continue_Char);
	insert( " " );
     }
   insert_single_space ();
   free_f90_indent ();
}

private define free_f90_newline ()
{
   bskip_white ();
   if (0 == parse_to_point ())
     {
	variable m = create_user_mark();
	bskip_chars("-+*=/,(<>:");
	variable needs_continued = (m != create_user_mark ());
	ifnot (needs_continued)
	  needs_continued = (1 == find_matching_delimiter (')'));
	goto_user_mark (m);
	if (needs_continued && not blooking_at ("&"))
	  insert (" &");
     }
   trim ();

   if (what_column () > 72)
     message ("Line exceeds 72 columns.");

   newline ();
   free_f90_indent ();
}

%}}}

%{{{ Fixed Format Functions

define fixed_f90_indent ()
{
   variable goal = 7;		% at top of buffer it should be 7 n'est pas?
   variable cs = CASE_SEARCH;
   variable ch;

   % goto beginning of line and skip past continuation char
   USER_BLOCK0
     {
	bol ();
	skip_chars ("0-9 \t");
	if (looking_at(F90_Continue_Char)) go_right_1 ();
	skip_white ();
     }

   push_spot ();
   push_spot ();
   CASE_SEARCH = 0;	% F90 is not case sensitive
   while (up_1 ())
     {
	bol_skip_white();
	if (eolp() || looking_at(F90_Continue_Char)) continue;
	X_USER_BLOCK0 ();
	goal = what_column ();

	if (goal == 1) continue;

	if (looking_at("do ") || looking_at("else")
	    || looking_at("subroutine")
	    || looking_at("interface")
	    || looking_at("program")
	    )
	  goal += F90_Indent_Amount;
	else if (looking_at("if ") || looking_at("if("))
	  {
	     if (ffind ("then")) goal += F90_Indent_Amount;
	  }
	else if (looking_at("type ") || looking_at("module "))
	  {
	     if (not (ffind ("::"))) goal += F90_Indent_Amount;
	  }
	break;
     }

   % now check current line
   pop_spot ();
   push_spot ();
   X_USER_BLOCK0 ();

   if (looking_at("end") or
       looking_at("continue") or
       looking_at("else")) goal -= F90_Indent_Amount;

   CASE_SEARCH = cs;		% done getting indent
   if (goal < 7) goal = 7;
   pop_spot ();

   bol_skip_white ();

   % after the label or continuation char and indent the rest to goal
   USER_BLOCK1
     {
	skip_chars ("0-9");
	trim ();
	if (looking_at (F90_Continue_Char))
	  {
	     insert_spaces (6 - what_column());
	     go_right_1 (); trim();
	     goal += F90_Indent_Amount;
	  }
	insert_spaces (goal - what_column());
     }

   ch = char(what_char());
   switch (ch)
     {
	isdigit (ch) :		% label

	if (what_column () >= 6)
	  {
	     bol (); trim ();
	     insert_single_space ();
	  }
	X_USER_BLOCK1 ();
     }
     {
	case F90_Continue_Char :	% continuation character
	bol (); trim (); insert ("     ");
	X_USER_BLOCK1 ();
     }
     {
	not (bolp()) || eolp ():	% general case
	bol (); trim ();
	goal--;
	insert_spaces (goal);
     }
   pop_spot ();
   skip_white ();
}

define fixed_f90_is_comment ()
{
   bol ();
   skip_chars (" \t0-9");
   bolp () and not (eolp());
}

define fixed_f90_newline ()
{
   variable p, cont;

   if (bolp ())
     {
	newline ();
	return;
     }

   fixed_f90_indent ();
   push_spot ();
   bskip_white (); trim ();

   if (what_column () > 72)
     {
	push_spot ();
	bol_skip_white();
	ifnot (bolp()) message ("Line exceeds 72 columns.");
	pop_spot ();
     }

   p = _get_point ();
   bskip_chars("-+*=/,(");

   cont = (p != _get_point ());

   if (fixed_f90_is_comment ()) cont = 0;

   bol_skip_white ();
   if (looking_at("data ")) cont = 0;

   pop_spot ();

   newline ();
   insert_single_space ();
   if (cont) insert(F90_Continue_Char);
   fixed_f90_indent ();
}

%}}}

private define dispatch_f90_function (free, fixed)
{
   if (get_format_mode ())
     {
	(@free) ();
	return;
     }

   (@fixed) ();
}

define f90_indent ()
{
   dispatch_f90_function (&free_f90_indent, &fixed_f90_indent);
}

define f90_is_comment ()
{
   dispatch_f90_function (&free_f90_is_comment, &fixed_f90_is_comment);
}

define f90_newline ()
{
   dispatch_f90_function (&free_f90_newline, &fixed_f90_newline);
}

define f90_continue_newline ()
{
   f90_newline ();

   push_spot ();
   bol_skip_white ();
   if (looking_at(F90_Continue_Char)) pop_spot ();
   else
     {
	insert (F90_Continue_Char);
	pop_spot ();
	f90_indent ();
	go_right_1 ();
	skip_white ();
     }
}

%
%   electric labels
%
define f90_electric_label ()
{
   insert_char (LAST_CHAR);

   if (get_format_mode ())
     return;			       %  free-format

   push_spot ();

   if (f90_is_comment ()) pop_spot ();
   else
     {
	bol_skip_white ();
	skip_chars ("0-9"); trim ();
	pop_spot ();
	f90_indent ();
     }
}

% f90 comment/uncomment functions

define f90_uncomment ()
{
   push_spot ();
   if (f90_is_comment ())
     {
	bol ();
	if (looking_at (F90_Comment_String))
	  deln (strlen (F90_Comment_String));
	else del ();
     }

   f90_indent ();
   pop_spot ();
   go_down_1 ();
}

define f90_comment ()
{
   ifnot (f90_is_comment ())
     {
	push_spot ();
	bol ();
	insert (F90_Comment_String);
     }
   pop_spot ();
   go_down_1 ();
}

%
% Look for beginning of current subroutine/function
%
define f90_beg_of_subprogram ()
{
   variable cs = CASE_SEARCH;

   CASE_SEARCH = 0;
   do
     {
	bol_skip_white ();
	if (_get_point ())
	  {
	     if (looking_at ("program")
		 || looking_at ("function")
		 || looking_at ("subroutine")) break;
	  }
     }
   while (up_1 ());
   CASE_SEARCH = cs;
}

%
% Look for end of current subroutine/function
%
define f90_end_of_subprogram ()
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

define f90_mark_subprogram ()
{
   f90_end_of_subprogram ();
   go_down_1 ();
   push_mark (); call ("set_mark_cmd");
   f90_beg_of_subprogram ();
   bol ();
}

%
% shows a ruler for F90 source. Press any key to get rid of
%
define f90_ruler ()
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

private define f90_prev_next_statement (dirfun)
{
   while (@dirfun ())
     {
	bol ();
	skip_chars ("^0-9 \t\n");
	ifnot (_get_point ()) break;
     }

   variable col = 7;
   if (get_format_mode ())
     col = 1;

     () = goto_column_best_try (col);
}
%
% moves cursor to the next statement, skipping comment lines
%
define f90_next_statement ()
{
   f90_prev_next_statement (&down_1);
}

%
% moves cursor to the previous f90 statement, skipping comments
%
define f90_previous_statement ()
{
   f90_prev_next_statement (&up_1);
}

%
% main entry point into the f90 mode
%

$1 = "F90";
ifnot (keymap_p ($1)) make_keymap ($1);

definekey ("f90_comment",		"\e;",	$1);
definekey ("f90_uncomment",		"\e:",	$1);
definekey ("f90_continue_newline",	"\e\r",	$1);
definekey ("self_insert_cmd",		"'",	$1);
definekey ("self_insert_cmd",		"\"",	$1);
definekey ("f90_beg_of_subprogram",	"\e^A",	$1);
definekey ("f90_end_of_subprogram",	"\e^E",	$1);
definekey ("f90_mark_function",		"\e^H", $1);

definekey_reserved ("f90_next_statement",	"^N",	$1);
definekey_reserved ("f90_previous_statement",	"^P",	$1);
definekey_reserved ("f90_ruler",		"^R", $1);

_for (0, 9, 1)
{
   $2 = ();
   definekey ("f90_electric_label", string($2), $1);
}

% Set up syntax table
foreach (["F90_free", "F90_fixed"])
{
   $1 = ();
   create_syntax_table ($1);
   define_syntax ("!", "", '%', $1);
   define_syntax ("([", ")]", '(', $1);
   define_syntax ('"', '"', $1);
   define_syntax ('\'', '"', $1);      %  quoted strings
   %define_syntax ('\'', '"', $1);      % quoted characters
   % define_syntax ('\\', '\\', $1);
   define_syntax ("0-9a-zA-Z_", 'w', $1);        % words
   define_syntax ("-+0-9eEdD", '0', $1);   % Numbers
   define_syntax (",.", ',', $1);
   define_syntax ('#', '#', $1);
   define_syntax ("-+/*=", '+', $1);

% F77 keywords + include, record, structure, while:
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
%
% Extensions for Fortran 90:
% allocatable
% allocate
% case
% contains
% cycle
% deallocate
% elsewhere
% endblockdata
% endfunction
% endinterface
% endmodule
% endprogram
% endselect
% endsubroutine
% endtype
% endwhere
% intent
% interface
% kind
% module
% moduleprocedure
% namelist
% nullify
% optional
% pointer
% private
% public
% recursive
% select
% selectcase
% sequence
% target
% type
% use
% where
%
() = define_keywords ($1, "dogoifto", 2);
() = define_keywords ($1, "enduse", 3);
() = define_keywords ($1, "callcasedataelseexitgotokindopenreadrealsavestopthentype", 4);
() = define_keywords ($1, "blockclosecycleenddoendifentrypauseprintwherewhilewrite", 5);
() = define_keywords ($1, "commondoubleformatintentmodulepublicrecordreturnrewindselecttarget", 6);
() = define_keywords ($1, "complexendfileendtypeincludeinquireintegerlogicalnullifypointerprivateprogram", 7);
() = define_keywords ($1, "allocatecontainscontinueendwhereexternalfunctionimplicitnamelistoptionalsequence", 8);
() = define_keywords ($1, "associatebackspacecharacterdimensionelsewhereendmoduleendselectinterfaceintrinsicparameterprecisionrecursivestructure", 9);
() = define_keywords ($1, "deallocateendprogramselectcasesubroutine", 10);
() = define_keywords ($1, "allocatableendfunctionequivalence", 11);
() = define_keywords ($1, "endblockdataendinterface", 12);
() = define_keywords ($1, "endsubroutine", 13);
() = define_keywords ($1, "moduleprocedure", 15);

() = define_keywords_n ($1, "eqgegtleltneor", 2, 1);
() = define_keywords_n ($1, "absallandanycosdimexpintiorlenlgelgtllelltlogmaxminmodnotsinsumtan", 3, 1);
() = define_keywords_n ($1, "acosaintasinatancharcoshdblehugeiandieorkindnintpackrealscansignsinhsizesqrttanhtinytrimtrue", 4, 1);
() = define_keywords_n ($1, "aimaganintatan2btestcmplxconjgcountdprodfalseflooribclribitsibseticharindexishftlog10mergeradixrangescaleshape", 5, 1);
() = define_keywords_n ($1, "cshiftdigitsiacharishftclboundmatmulmaxlocmaxvalminlocminvalmodulomvbitsrepeatspreaduboundunpackverify", 6, 1);
() = define_keywords_n ($1, "adjustladjustrceilingeoshiftepsilonlogicalnearestpresentproductreshapespacing", 7, 1);
() = define_keywords_n ($1, "bit_sizeexponentfractionlen_trimtransfer", 8, 1);
() = define_keywords_n ($1, "allocatedprecisionrrspacingtranspose", 9, 1);
() = define_keywords_n ($1, "associated", 10, 1);
() = define_keywords_n ($1, "dot_productmaxexponentminexponentrandom_seed", 11, 1);
() = define_keywords_n ($1, "set_exponentsystem_clock", 12, 1);
() = define_keywords_n ($1, "date_and_timerandom_number", 13, 1);
() = define_keywords_n ($1, "selected_int_kind", 17, 1);
() = define_keywords_n ($1, "selected_real_kind", 18, 1);
}
set_syntax_flags ("F90_free", 1);
set_syntax_flags ("F90_fixed", 1|2);
set_fortran_comment_chars ("F90_fixed", "^0-9 \t\n");

private define setup_f90_mode (format)
{
   variable mode = "F90";
   set_mode (sprintf ("%s-%s", mode, format), 0x4 | 0x10);
   use_keymap (mode);
   set_buffer_hook ("indent_hook", "f90_indent");
   set_buffer_hook ("newline_indent_hook", "f90_newline");

   set_format_mode (format);

   use_syntax_table (strcat ("F90_", format));
}

public define f90_free_format_mode ()
{
   setup_f90_mode ("free");
   run_mode_hooks ("f90_free_format_mode_hook");
}

public define f90_fixed_format_mode ()
{
   setup_f90_mode ("fixed");
   run_mode_hooks ("f90_fixed_format_mode_hook");
}

%!%+
%\function{f90_mode}
%\synopsis{f90_mode}
%\description
% Mode designed for the purpose of editing F90 files.
% After the mode is loaded, the hook 'f90_hook' is called.
% Useful functions include:
%#v+
%  Function:                    Default Binding:
%   f90_continue_newline          ESC RETURN
%     indents current line, and creates a continuation line on next line.
%   f90_comment                   ESC ;
%     comments out current line
%   f90_uncomment                 ESC :
%     uncomments current line
%   f90_electric_label            0-9
%     Generates a label for current line or simply inserts a digit.
%   f90_next_statement            ^C^N
%     moves to next f90 statementm skips comment lines
%   f90_previous_statement        ^C^P
%     moves to previous f90 statement, skips comment lines
%   f90_ruler                     ^C^R
%     inserts a ruler above the current line. Press any key to continue
%   f90_beg_of_subprogram         ESC ^A
%     moves cursor to beginning of current subroutine/function
%   f90_end_of_subprogram         ESC ^E
%     moves cursor to end of current subroutine/function
%   f90_mark_subprogram           ESC ^H
%     mark the current subroutine/function
%#v-
% Variables include:
%#v+
%   F90_Continue_Char   --- character used as a continuation character.
%     By default, its value is "&"
%   F90_Comment_String  --- string used by 'f90_comment' to
%     comment out a line.  The default string is "C ";
%   F90_Indent_Amount   --- number of spaces to indent statements in
%                               a block.  The default is 2.
%   F90_Default_Format --- Either "fixed" or "free".
%#v-
%!%-
public define f90_mode ()
{
   setup_f90_mode (strlow (F90_Default_Format));
   run_mode_hooks ("f90_mode_hook");

   set_comment_info (F90_Comment_String, "", 0x4);
}

provide ("f90");
