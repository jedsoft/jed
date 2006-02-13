%  matlab.sl			-*- SLang -*-
%
%  This file provides a mode for editing Matlab/Octave files.
%
%  Written by Guido Gonzato  <guido@ibogeo.df.unibo.it>
%  Very simple, but it works.
%  Last updated: 19 May 1999

$1 = "matlab";
!if (keymap_p ($1)) make_keymap ($1);

% very few facilities at the moment: simple syntax highlighting,
% indent, and comment/uncomment.

definekey ("matlab_comment",	"\e;",	$1);
definekey ("matlab_uncomment",	"\e:",	$1);

% Now create and initialize a simple syntax table.
create_syntax_table ($1);
define_syntax ("%", "", '%', $1);		% comments (doesn't work) 
define_syntax ("#", "", '%', $1);		% comments -- Octave
define_syntax ("([{", ")]}", '(', $1);		% parentheses
define_syntax ('"', '"', $1);			% strings
define_syntax ('\\', '\\', $1);			% escape character
define_syntax ("0-9a-zA-Z_", 'w', $1);		% identifiers
define_syntax ("0-9a-fA-F.xXL", '0', $1);	% numbers
define_syntax (",;", ',', $1);			% delimiters
define_syntax ("!&+-.*^;<>\|~='/:", '+', $1);	% operators
set_syntax_flags ($1, 4);

% Matalb/Octave reserved words. Are there more?
() = define_keywords_n ($1, "cdifls", 2, 0);
() = define_keywords_n ($1, "direndforsettrywho", 3, 0);
() = define_keywords_n ($1, "caseechoelsegsethdirhelpholdloadmoresaveshowtypewhos", 4, 0);
() = define_keywords_n ($1, "breakcatchcleardiaryendifgplotgshowwhichwhile", 5, 0);
() = define_keywords_n ($1, "elseifendforformatglobalgsplotreturnswitch", 6, 0);
() = define_keywords_n ($1, "casesenhistory", 7, 0);
() = define_keywords_n ($1, "continueendwhilefunction", 8, 0);
() = define_keywords_n ($1, "endswitch", 9, 0);
() = define_keywords_n ($1, "all_va_argsendfunctionrun_history", 11, 0);
() = define_keywords_n ($1, "edit_history", 12, 0);
() = define_keywords_n ($1, "end_try_catch", 13, 0);
() = define_keywords_n ($1, "unwind_protect", 14, 0);
() = define_keywords_n ($1, "end_unwind_protect", 18, 0);
() = define_keywords_n ($1, "unwind_protect_cleanup", 22, 0);

variable Matlab_Continue_Char = "\\";
variable Matlab_Indent = 2;

% Matlab indent routine.
define matlab_indent ()
{
  variable goal = 1;
  variable cs = CASE_SEARCH;
  variable ch;

  % goto beginning of line and skip past continuation char
  USER_BLOCK0
    {
      bol ();
      skip_chars (" \t");
      if (looking_at(Matlab_Continue_Char)) go_right_1 ();
      skip_white ();
    }

  push_spot ();
  push_spot ();
  CASE_SEARCH = 1;	% Octave is case sensitive
  while (up_1 ())
    {
      bol_skip_white();
      if (eolp() or looking_at(Matlab_Continue_Char)) continue;
      X_USER_BLOCK0 ();
      goal = what_column ();
      
      if (looking_at("switch"))
	goal += 2 * Matlab_Indent; % to account for 'case'
      
      if (looking_at ("if ") or looking_at ("if(") or
	  looking_at ("case") or 
	  looking_at ("otherwise") or
	  looking_at ("while") or
	  looking_at ("for ") or 
	  looking_at ("else") or looking_at ("elseif") or
	  looking_at ("unwind_protect_cleanup") or
	  looking_at ("unwind_protect") or
	  looking_at ("try") or looking_at ("catch") or
	  looking_at ("function") )
	 goal += Matlab_Indent;
      
      break;
    }

  % now check the current line
  pop_spot ();
  push_spot ();
  X_USER_BLOCK0 ();

  if (looking_at ("endswitch"))
    goal -= 2 * Matlab_Indent;
  
  if (looking_at ("end") or
      looking_at ("endif") or
      looking_at ("case") or
      looking_at ("endwhile") or
      looking_at ("endfor") or
      looking_at ("end_try_catch") or
      looking_at ("unwind_protect_cleanup") or
      looking_at ("end_unwind_protect") or
      looking_at ("else") or 
      looking_at ("endfunction") )
    goal -= Matlab_Indent;
  
  CASE_SEARCH = cs;		% done getting indent
  if (goal < 1) goal = 1;
  pop_spot ();

  bol_skip_white ();
  ch = char(what_char());
  bol_trim ();
   goal--;
   insert_spaces (goal);
  pop_spot ();
  skip_white ();

} % matlab_indent

define matlab_newline ()
{

   if (bolp ())
     {
	newline ();
	return;
    }

  matlab_indent ();
  newline ();
  matlab_indent ();
}

% Matlab comment routines.

define matlab_is_comment ()
{
   bol ();
   skip_chars (" \t0-9");
   bolp () and not (eolp());
}

define matlab_uncomment ()
{
   push_spot ();
   if (matlab_is_comment ())
     {
	bol ();
	if (looking_at ("%") | looking_at("#"))
	  deln (1);
	else del ();
     }

   matlab_indent ();
   pop_spot ();
   go_down_1 ();
}

define matlab_comment ()
{
   !if (matlab_is_comment ())
     {
	push_spot_bol ();
	insert ("%");
     }
   pop_spot ();
   go_down_1 ();
}

%!%+
%\function{matlab_mode}
%\synopsis{matlab_mode}
%\description
% Protoytype: Void matlab_mode ();
% This is a mode that is dedicated to facilitate the editing of 
% Matlab/Octave language files.  
% Functions that affect this mode include:
%#v+
%  function:             default binding:
%  matlab_indent         RETURN
%#v-
% Variables affecting indentation include:
% Hooks: \var{matlab_mode_hook}
%!%-
define matlab_mode ()
{
  variable kmap = "matlab";
  set_mode(kmap, 2);
  use_keymap(kmap);
  use_syntax_table (kmap);
  set_buffer_hook ("indent_hook", "matlab_indent");
  set_buffer_hook ("newline_indent_hook", "matlab_newline");
  run_mode_hooks("matlab_mode_hook");
}


% --- End of file matlab.sl ---
