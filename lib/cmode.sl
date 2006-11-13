% New C-mode indentation routines
%require ("profile");
%profile_calibrate ();
%profile_begin ();

autoload ("c_make_comment", "cmisc");
autoload ("c_format_paragraph", "cmisc");
autoload ("c_paragraph_sep", "cmisc");
%autoload ("c_comment_region", "cmisc");
autoload ("c_top_of_function", "cmisc");
autoload ("c_end_of_function", "cmisc");
autoload ("c_mark_function", "cmisc");
autoload ("c_indent_buffer", "krconv");

%!%+
%\variable{C_Autoinsert_CPP_Comments}
%\synopsis{Control insertion of C++ comments}
%\description
% In c-mode, if a line starts with //, then pressing return will cause the
% next line to also start with //.  This feature is useful for writing
% multiple comment lines using C++ style comments.
%\seealso{c_mode}
%!%-
custom_variable ("C_Autoinsert_CPP_Comments", 1);
custom_variable ("C_Switch_Offset", 0);
%\variable{C_Namespace_Offset}
%\synopsis{C_Namespace_Offset}
%\description
% Integer C_Namespace_Offset = 3;
% This variable may be changed to adjust the indentation of members
% inside of a class declaration block.
%\seealso{c_mode}
%\seealso{C_BRA_NEWLINE, C_BRACE, C_Class_Offset, C_INDENT, C_Namespace_Offset}
%!%-

custom_variable ("C_Namespace_Offset", 3);
%!%+
%\variable{C_Class_Offset}
%\synopsis{C_Class_Offset}
%\description
% Integer C_Class_Offset = 3;
% This variable may be changed to adjust the indentation of members
% inside of a class declaration block.
%\seealso{c_mode}
%\seealso{C_BRA_NEWLINE, C_BRACE, C_INDENT, C_Namespace_Offset}
%!%-
custom_variable ("C_Class_Offset", 3);

%!%+
%\variable{C_Param_Offset_Max}
%\synopsis{Control indentation of continued parameter list}
%\usage{Integer C_Param_Offset_Max = 6}
%\description
% This variable may be changed to adjust the indentation of parameters
% in a funcion call that extends over multiple lines.
% 
% If the value is less than 0, the feature is off, otherwise
% it holds the max number of spaces to indent the parameters on
% the continuation line(s).
%\seealso{c_mode}
%\seealso{C_BRA_NEWLINE, C_BRACE, C_INDENT}
%!%-
custom_variable ("C_Param_Offset_Max", -1);

% If non-zero, align matching parenthesis that appear on different lines
custom_variable ("C_Align_Parenthesis", 1);

% Indent for macros
custom_variable ("C_Macro_Indent", 2);

define cmode_is_slang_mode ()
{
   variable is_slang;
   (, is_slang) = what_mode ();
   is_slang & 8;
}

private define blooking_at_continuation_char ()
{
   return eolp () and
     blooking_at ("\\") and not (blooking_at ("\\\\"));
}

private define bskip_all_whitespace ()
{
   forever
     {
	bskip_chars (" \f\t\n");
	if (blooking_at_continuation_char ())
	  {
	     go_left (1);
	     continue;
	  }
	return;
     }
}

private define skip_all_whitespace ()
{
   skip_chars (" \f\t\n");
}

private define skip_identifier ()
{
   skip_chars ("a-zA-Z0-9_$");
}

private define bskip_identifier ()
{
   bskip_chars ("a-zA-Z0-9_$");
}

private define bextract_identifier ()
{
   push_mark ();
   bskip_identifier ();
   return bufsubstr ();
}

private define skip_over_comment ()
{
   variable is_slang = cmode_is_slang_mode ();

   forever 
     {
	skip_all_whitespace ();
	if (eobp ())
	  return;
	
	if (is_slang)
	  {
	     !if (looking_at ("%"))
	       return;
	     
	     eol ();
	     continue;
	  }

	if (looking_at ("/*"))
	  {
	     !if (fsearch ("*/"))
	       return;
	     go_right(2);
	     continue;
	  }
	
	if (looking_at ("//"))
	  {
	     eol (); 
	     continue;
	  }
	    
	return;
     }
}

private define extract_identifier ()
{
   push_mark ();
   skip_identifier ();
   return bufsubstr ();
}

private define c_search (fun, str)
{
   while ((@fun)(str))
     {
	if (0 != parse_to_point ())
	  {
	     go_right_1 ();
	     continue;
	  }

	return 1;
     }
   return 0;
}

private define c_fsearch (str)
{
   return c_search (&fsearch, str);
}

private define c_bol_fsearch (str)
{
   return c_search (&bol_fsearch, str);
}

private define c_re_fsearch (str)
{
   return c_search (&re_fsearch, str);
}

private define c_find_effective_eol ()
{
   bol ();
   while (ffind_char ('%'))
     {
	go_right_1 ();
	if (parse_to_point () == -2)
	  {
	     return;
	  }
     }
   eol ();
}

private define slmode_bskip_comment (skip_pp)
{
   forever
     {
	while (parse_to_point () == -2)
	  {
	     () = bfind ("%");
	  }

	bskip_white ();

	push_mark ();
	bol ();
	pop_mark (not(looking_at_char ('#')));

	!if (bolp ()) return;
	!if (left (1)) return;
	c_find_effective_eol ();
     }
}
% This function also skips preprocessor lines
define c_bskip_over_comment (skip_pp)
{
   variable num_up;
   if (cmode_is_slang_mode ()) return slmode_bskip_comment (skip_pp);

   forever
     {
	bskip_all_whitespace ();
	if (bobp ())
	  return;

	if (skip_pp)
	  {
	     push_mark ();
	     %num_up = 0;
	     while (up_1 ())
	       {
		  %eol (); -- up(1) leaves point at eol
		  !if (blooking_at_continuation_char ())
		    {
		       go_down_1 ();
		       break;
		    }
		  %num_up++;
	       }

	     bol_skip_white ();
	     if (looking_at_char ('#'))
	       {
		  pop_mark_0 ();
		  continue;
	       }
	     pop_mark_1 ();
	  }

	!if (blooking_at ("*/"))
	  {
	     push_mark ();
	     variable ptp = -2;
	     while (andelse{ptp == -2}{bfind ("//")})
	       ptp = parse_to_point ();

	     if (ptp == 0)
	       {
		  % Not in a comment or string
		  pop_mark_0 ();
		  continue;
	       }
	     bol ();
	     !if (bobp ())
	       {
		  if (skip_pp and looking_at_char ('#'))
		    {
		       pop_mark_0 ();
		       continue;
		    }
	       }
	     pop_mark_1 ();
	     break;
	  }
	!if (bsearch ("/*")) break;
     }
}

private define c_looking_at (token)
{
   variable cse = CASE_SEARCH, ret = 0;
   CASE_SEARCH = 1;

   if (looking_at(token))
     {
	push_spot ();
	go_right(strlen(token));
	_get_point ();
	skip_chars ("\t :({");
	ret = (_get_point () - ()) or eolp();
	pop_spot ();
     }
   CASE_SEARCH = cse;
   ret;
}

private define c_indent_to (n)
{
   bol ();
   % Force a reindent if the line does not contain tabs followed by spaces.
   skip_chars ("\t");
   skip_chars (" ");

   if ((what_column != n)
       or (_get_point () != (skip_white (), _get_point ())))
     {
	bol_trim ();
	n--;
	whitespace (n);
     }
}

private define c_indent_preprocess_line ()
{
   variable col;

   push_spot_bol ();

   trim ();
   !if (up_1 ())
     {
	pop_spot ();
	return;
     }

   !if (bol_bsearch_char ('#'))
     {
	pop_spot ();
	return;
     }
   go_right_1 ();
   skip_white ();
   col = what_column ();
   if (looking_at ("if"))
     col += C_Preprocess_Indent;
   else if (looking_at ("el"))
     col += C_Preprocess_Indent;

   pop_spot ();
   go_right_1 ();
   skip_white ();

   !if (looking_at ("error"))
     {
	if (looking_at_char ('e'))
	  col -= C_Preprocess_Indent;
     }

   if (what_column () == col)
     return;
   bskip_white ();
   trim ();
   whitespace (col - 2);
}

private define c_indent_continued_comment (col)
{
   push_spot ();
   col++;			       %  add 1 so the we indent under * in /*
   c_indent_to (col);

   if (looking_at ("*")
       or not (eolp ()))
     pop_spot ();
   else
     {
	insert ("* ");
	pop_spot ();
	if (what_column () <= col)
	  {
	     goto_column (col + 2);
	  }
     }
}

private define c_mode_if_bol_skip_white ()
{
   push_mark ();
   bskip_white ();
   1;
   if (bolp ())
     {
	pop ();
	skip_white ();
	0;
     }
   pop_mark (());		       %  take argument from stack
}

private define continued_statement_bol ()
{
   while (up_1 ())
     {
	!if (blooking_at_continuation_char ())
	  {
	     go_down_1 ();
	     break;
	  }
     }
   bol ();
}

#iftrue
% Return true if the spot is inside of a class definition
% Takes the opening brace of the enclosing block as an
% argument.
private define inside_class_or_namespace (bra, name)
{
   push_spot ();

   EXIT_BLOCK
     {
	pop_spot ();
     }

   goto_user_mark (bra);

   % Assume that class is at the beginning of a line.  We may want to
   % change this assumption later.
   variable re = sprintf ("\\c\\<%s\\>", name);
   while (re_bsearch (re))
     {
	if (name == "class")
	  {
	     bskip_chars (" \t<");	       %  allow: template <class...
	     if (blooking_at ("template"))
	       go_left (8);
	  }

	bskip_white ();
	if (bolp () and
	    (0 == parse_to_point ()))
	  {
	     if (0 == c_fsearch ("{"))
	       return 0;

	     return bra == create_user_mark ();
	  }

	!if (left(1))
	  break;
     }

   return 0;
}
#endif

% overview of indentation tracking variables
%
% this_char:
%   - 1st character at the begin of the current line
% match_char:
%   - start character of an enclosing '([{}])' construct, or where we gave up
% match_indent:
%   - indentation of the match_char (possibly our reference point)
% match_line:
%   - where the match occurred
% prep_indent:
%   - extra indentation for multi-line preprocessor statements
% skip_pp:
%   - 1: indenting normal statement;
%   - 0: indenting preprocessor statements (use prep_indent)
define c_indent_line ()
{
   variable val, col, extra_indent = 0;
   variable prep_indent = 0, prep_line = 0;
   variable match_char, match_indent, match_line, this_char;
   variable match_mark;
   variable skip_pp = 1;
   variable is_continuation = 0;

   push_spot ();
   bol_skip_white ();
   this_char = what_char ();
   if (-2 == parse_to_point ())
     {
	% In a comment.  Indent it at level of matching /* string
	() = bsearch ("/*");
	col = what_column ();
	pop_spot ();
	c_indent_continued_comment (col);
	c_mode_if_bol_skip_white ();
	return;
     }

   EXIT_BLOCK
     {
	bol_trim ();
	pop_spot ();
     }

   if (looking_at_char ('#'))
     {
	c_indent_preprocess_line ();
	return;
     }

   push_spot ();
   variable indenting_solitary_left_parens = 0;
   variable indenting_iostream_op = 0; %  not implemented
   if (this_char == '(')
     {
	go_right_1 ();
	skip_white ();
	if (looking_at_char ('\\'))
	  go_right_1 ();
	indenting_solitary_left_parens = eolp ();
     }
   else if (looking_at (">>") or looking_at ("<<"))
     {
	indenting_iostream_op = 1;
	% TODO
    }

   if (up_1 ())
     {
	eol ();
	if (blooking_at_continuation_char ())
	  {
	     continued_statement_bol ();
	     !if (looking_at_char ('#'))
	       {
		  pop_spot ();
		  return;
	       }
	     %extra_indent += C_INDENT;
	     prep_indent = C_Macro_Indent;
	     skip_pp = 0;
	     prep_line = what_line ();
	  }
     }
   pop_spot ();

   EXIT_BLOCK
     {
	c_mode_if_bol_skip_white ();
     }

   if (orelse
       { c_looking_at("case") }
	 { c_looking_at("default") }
	 { c_looking_at("protected") }
	 { c_looking_at("private") }
	 { c_looking_at("public") }
       )
     {
	if (ffind_char (':'))
	  {
	     extra_indent -= C_INDENT;
	     extra_indent += C_Colon_Offset;
	  }
	bol ();
     }
   else
     {
	forever
	  {
	     c_bskip_over_comment (skip_pp);
	     if (blooking_at_continuation_char ())
	       {
		  go_left_1 ();
		  continue;
	       }

	     !if (orelse
		  { blooking_at (";") }
		    { blooking_at ("{") }
		    { blooking_at ("}") }
		    { blooking_at ("),") }
		    { blooking_at ("},") }
		    { blooking_at (":") }
		    { bobp () }
		  )
	       {
		  if (skip_pp or is_continuation)
		    {
		       if (0 == indenting_solitary_left_parens)
			 extra_indent = C_CONTINUED_OFFSET;
		    }
		  else
		    {
		       push_spot ();
		       bol_skip_white ();
		       !if (looking_at ("#"))
			 extra_indent = C_CONTINUED_OFFSET;
		       pop_spot ();
		    }
		  is_continuation++;
	       }
	     !if (blooking_at (")")) break;
	     push_mark ();
	     go_left_1 ();
	     if (1 != find_matching_delimiter (')'))
	       {
		  pop_mark_1 ();
		  break;
	       }
	     c_bskip_over_comment (skip_pp);
	     push_spot ();
	     if ((1 == find_matching_delimiter (')')), pop_spot ())
	       {
		  pop_mark_1 ();
		  break;
	       }

	     pop_mark_0 ();
	     bol ();

	     if (skip_pp == 0)
	       {
		  if (looking_at ("#"))
		    break;
	       }
	  } %  end of forever loop
     }

   if ((skip_pp == 0) and (looking_at ("#")))
     val = 0;
   else
     val = find_matching_delimiter (')');

   match_mark = create_user_mark ();
   match_char = what_char();
   match_line = what_line ();

   if ((val < 0) and looking_at ("/*")) val = -2;
   else if (val == 1)
     {
	if ((C_Align_Parenthesis == 0) or (this_char != ')'))
	  {
	     go_right (1);
	     skip_white ();
	  }
     }
   col = what_column ();

   bol_skip_white ();
   match_indent = what_column ();
   if (what_line () < prep_line)
     {
	match_char = 0;
     }

   pop_spot ();

   variable notCcomment;

#iftrue
   % Added 04/06/98 MDJ to facilitate C++ style comments
   if (val == 0)
     {				       %  match not found for ')'
	push_spot();
	bol_skip_white();
	if (eolp())
	  {
	     go_up_1();
	     bol_skip_white();
	     % Added slang checks 04/09/98 MDJ
	     if (cmode_is_slang_mode()) notCcomment = "%% ";
	     else notCcomment = "// ";

	     if (looking_at(notCcomment))
	       {
		  val = -3;
		  col = what_column();
	       }
	  }
	pop_spot();
     }
#endif

   switch (val)
     {
      case 0: %  Could not find matching '(' for ')'
	if (match_char == '{') %  within {...} block
	  {
	     push_spot ();
	     goto_user_mark (match_mark);
	     bskip_all_whitespace ();
	     if (blooking_at (")"))  %  ... (expr) {...} block
	       {
		  variable same_line = (what_line == match_line);

		  go_left_1 ();
		  if (1 == find_matching_delimiter (')'))
		    {
		       bol_skip_white ();
		       if (same_line)
			 match_indent = what_column ();

		       if ((this_char != '}')
			   and looking_at("switch"))
			 match_indent += C_Switch_Offset;
		    }
	       }
	     else if (blooking_at("="))%  ... = {...} list
	       {
		  if (is_continuation)
		    extra_indent -= C_CONTINUED_OFFSET;
		  if (this_char == '{') % ... = {\n{...}...}
		    extra_indent -= C_BRACE;   %  undone below
	       }

	     pop_spot ();
	     col = match_indent;

	     if (this_char == '}')
	       col += C_INDENT;
	     else if (inside_class_or_namespace (match_mark, "class"))
	       col += C_Class_Offset;
	     else if (inside_class_or_namespace (match_mark, "namespace"))
	       col += C_Namespace_Offset;
	     else
	       col += C_INDENT;

	     prep_indent = 0;
	  }
	else if (match_char == '[')
	  {
	     push_spot ();
	     if (this_char != ']')
	       col++;
	     c_indent_to (col);
	     pop_spot ();
	     return;
	  }
	else
	  {
	     push_spot ();
	     bol_skip_white ();
	     %if (skip_pp and looking_at_char ('{'))
	     if (looking_at_char ('{'))
	       extra_indent = 0;
	     extra_indent++;
	     c_indent_to (extra_indent + prep_indent);
	     pop_spot ();
	     return;
	  }
     }
     {
      case 1:
	extra_indent = 0;	       %  match found
	prep_indent = 0;
        if (C_Param_Offset_Max >= 0)
	  if (col - match_indent > C_Param_Offset_Max)
	    extra_indent = match_indent + C_Param_Offset_Max - col;
     }
     {
      case -2:			       %  inside comment
	if (cmode_is_slang_mode ()) return;
	if (this_char != '\\') col++;
	c_indent_continued_comment (col);
	return;
     }
     {
      case 2:
	push_spot_bol ();
	trim ();
	pop_spot ();
	return;
     }
     {
#iftrue
      case -3: 				% Inside C++ comment
	!if ((looking_at(notCcomment)) or not(eolp()))
	  {
	     goto_column(col);
	     if (C_Autoinsert_CPP_Comments) insert(notCcomment);
	  }
	return;
     }
#endif

   switch (this_char)
     {
      case '}':
	col -= C_INDENT;
	!if (skip_pp) col += extra_indent;
     }
     {
      case '{':
	col += C_BRACE;
	if (is_continuation)
	  col -= C_CONTINUED_OFFSET;
	col += extra_indent;
	% !if (skip_pp) col += extra_indent;
     }
     {
	col += extra_indent;
     }

   push_spot ();
   c_indent_to (col + prep_indent);
   pop_spot ();
}

% This function returns zero if the line does not begin with "* @ "
% or returns the indent column if it does.  This combination of characters
% will most certainly appear in a comment.  See the c file jed/src/intrin.c
% for examples of how it is exploited.
private define c_is_comment_example ()
{
   push_spot ();
   bol_skip_white ();
   0;
   if (looking_at ("* @ "))
     {
	pop ();
	what_column ();
     }
   pop_spot ();
}

define c_newline_and_indent ()
{
   variable notCcomment = "//";

   if (bolp ())
     {
	newline ();
	indent_line ();
	return;
     }

   if (cmode_is_slang_mode ())
     {
	variable slcom = "%";
	push_spot_bol ();
	if (looking_at (slcom) and C_Autoinsert_CPP_Comments)
	  {
	     push_mark ();
	     skip_chars ("%!");
	     skip_white ();
	     slcom = bufsubstr ();
	     pop_spot ();
	     newline ();
	     insert (slcom);
	     return;
	  }
	pop_spot ();
	notCcomment = "%%";
     }

   variable col;
   variable notcomment_len = strlen (notCcomment);

   if (C_Autoinsert_CPP_Comments)
     {
	col = what_column ();
	push_spot_bol();
	if (looking_at(notCcomment))
	  {
	     push_mark ();
	     go_right (notcomment_len);
	     skip_white ();
	     notCcomment = bufsubstr ();
	     pop_spot ();
	     newline();
	     if (col > notcomment_len) insert(notCcomment);
	     return;
	  }
	pop_spot();
     }

   col = c_is_comment_example ();
   newline ();
   if (col)
     {
	c_indent_to (col);
	insert ("* @ ");
     }
   else indent_line ();
}

private define c_parse_to_point ()
{
   parse_to_point () or c_is_comment_example ();
}

define c_insert_bra ()
{
   if (c_parse_to_point ())
     insert_char ('{');
   else
     {
	push_spot ();
	c_bskip_over_comment (0);
	if (blooking_at (","), pop_spot ())
	  {
	     insert_char ('{');
	  }
	else
	  {
	     push_spot ();
	     skip_white ();
	     if (eolp ())
	       {
		  bskip_white ();
		  if (not (bolp ()) and C_BRA_NEWLINE, pop_spot ()) newline ();
		  push_spot ();
		  bskip_white ();
		  bolp ();	       %  on stack
		  pop_spot ();
		  insert_char ('{');
		  if ( () ) indent_line ();   %  off stack
		  eol ();
		  if (C_BRA_NEWLINE) c_newline_and_indent ();
	       }
	     else
	       {
		  pop_spot ();
		  insert_char ('{');
	       }
	  }
     }
}

define c_insert_ket ()
{
   variable status = c_parse_to_point ();
   variable line = what_line ();

   push_spot ();
   skip_white ();
   push_spot ();

   if (status
       or not (eolp ())
       or (1 == find_matching_delimiter ('}')) and (line == what_line ()))
     {
	pop_spot ();
	pop_spot ();
	insert_char ('}');
	blink_match ();
	return;
     }
   pop_spot ();
   bskip_white ();
   if (bolp (), pop_spot ())
     {
	insert_char ('}');
	trim ();
     }
   else
     {
	eol ();
	insert ("\n}");
     }
   indent_line ();
   eol ();
   blink_match ();
   if (C_BRA_NEWLINE) c_newline_and_indent ();
}

define c_insert_colon ()
{
   insert_char (':');
   !if (c_parse_to_point ())
     indent_line ();
}

$1 = "C";
!if (keymap_p ($1)) make_keymap ($1);
definekey ("indent_line", "\t", $1);
definekey ("newline_and_indent", "\r", $1);
definekey ("c_insert_bra", "{", $1);
definekey ("c_insert_ket", "}", $1);
definekey ("c_insert_colon", ":", $1);
definekey ("c_make_comment", "\e;", $1);
%definekey ("c_comment_region", "^X;", $1);
% definekey ("c_format_paragraph", "\eq", $1);
definekey ("c_top_of_function", "\e^A", $1);
definekey ("c_end_of_function", "\e^E", $1);
definekey ("c_mark_function", "\e^H", $1);

% Now create and initialize the syntax tables.
create_syntax_table ("C");
define_syntax ("/*", "*/", '%', "C");
define_syntax ("//", "", '%', "C");
define_syntax ("([{", ")]}", '(', "C");
define_syntax ('"', '"', "C");
define_syntax ('\'', '\'', "C");
define_syntax ('\\', '\\', "C");
define_syntax ("0-9a-zA-Z_", 'w', "C");        % words
define_syntax ("-+0-9a-fA-F.xXL", '0', "C");   % Numbers
define_syntax (",;.?:", ',', "C");
define_syntax ('#', '#', "C");
define_syntax ("%-+/&*=<>|!~^", '+', "C");
set_syntax_flags ("C", 0x4|0x40);

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_enable_highlight_cache("cmode.dfa", name);
   dfa_define_highlight_rule("^[ \t]*#", "PQpreprocess", name);
   dfa_define_highlight_rule("//.*", "comment", name);
   dfa_define_highlight_rule("/\\*.*\\*/", "Qcomment", name);
   dfa_define_highlight_rule("^([^/]|/[^\\*])*\\*/", "Qcomment", name);
   dfa_define_highlight_rule("/\\*.*", "comment", name);
   dfa_define_highlight_rule("^[ \t]*\\*+([ \t].*)?$", "comment", name);
   dfa_define_highlight_rule("[A-Za-z_\\$][A-Za-z_0-9\\$]*", "Knormal", name);
   dfa_define_highlight_rule("[0-9]+(\\.[0-9]*)?([Ee][\\+\\-]?[0-9]*)?",
			     "number", name);
   dfa_define_highlight_rule("0[xX][0-9A-Fa-f]*[LlUu]*", "number", name);
   dfa_define_highlight_rule("[0-9]+[LlUu]*", "number", name);
   dfa_define_highlight_rule("\"([^\"\\\\]|\\\\.)*\"", "string", name);
   dfa_define_highlight_rule("\"([^\"\\\\]|\\\\.)*\\\\?$", "string", name);
   dfa_define_highlight_rule("'([^'\\\\]|\\\\.)*'", "string", name);
   dfa_define_highlight_rule("'([^'\\\\]|\\\\.)*\\\\?$", "string", name);
   dfa_define_highlight_rule("[ \t]+", "normal", name);
   dfa_define_highlight_rule("[\\(\\[{}\\]\\),;\\.\\?:]", "delimiter", name);
   dfa_define_highlight_rule("[%\\-\\+/&\\*=<>\\|!~\\^]", "operator", name);
   dfa_build_highlight_table(name);
}
dfa_set_init_callback (&setup_dfa_callback, "C");
%%% DFA_CACHE_END %%%
#endif

% Type 0 keywords
#iffalse
() = define_keywords_n ("C", "doif", 2, 0);
#else
() = define_keywords_n ("C", "doifor", 2, 0);
#endif
() = define_keywords_n ("C", "andasmforintnewnottryxor", 3, 0);
() = define_keywords_n ("C", "autoboolcasecharelseenumgotolongthistruevoid", 4, 0);
#iffalse
() = define_keywords_n ("C", "breakcatchclassconstfalsefloatshortthrowunionusingwhile", 5, 0);
() = define_keywords_n ("C", "deletedoubleexternfriendinlinepublicreturnsignedsizeofstaticstructswitchtypeid", 6, 0);
#else
() = define_keywords_n ("C", "bitorbreakcatchclasscomplconstfalsefloator_eqshortthrowunionusingwhile", 5, 0);
() = define_keywords_n ("C", "and_eqbitanddeletedoubleexportexternfriendinlinenot_eqpublicreturnsignedsizeofstaticstructswitchtypeidxor_eq", 6, 0);
#endif
() = define_keywords_n ("C", "defaultmutableprivatetypedefvirtualwchar_t", 7, 0);
() = define_keywords_n ("C", "continueexplicitoperatorregistertemplatetypenameunsignedvolatile", 8, 0);
() = define_keywords_n ("C", "namespaceprotected", 9, 0);
() = define_keywords_n ("C", "const_cast", 10, 0);
() = define_keywords_n ("C", "static_cast", 11, 0);
() = define_keywords_n ("C", "dynamic_cast", 12, 0);
() = define_keywords_n ("C", "reinterpret_cast", 16, 0);

% Type 1 keywords (commonly used libc functions)
() = define_keywords_n("C",
		       "EOFabscosdivexplogpowsintan",
		       3, 1);

() = define_keywords_n("C",
		       "FILENULLacosasinatanatofatoiatolceilcosh"
		       + "exitfabsfeoffmodfreegetcgetslabsldivmodf"
		       + "putcputsrandsinhsqrttanhtime",
		       4, 1);

() = define_keywords_n("C",
		       "abortatan2clockctimediv_terrnofgetcfgets"
		       + "floorfopenfputcfputsfreadfrexpfseekftell"
		       + "ldexplog10qsortraisescanfsrandstdin",
		       5, 1);

() = define_keywords_n("C",
		       "assertatexitcallocfcloseferrorfflushfscanf"
		       + "fwritegetenvgmtimemallocmemchrmemcmpmemcpy"
		       + "memsetmktimeperrorprintfremoverenamerewind"
		       + "setbufsetjmpsignalsize_tsscanfstderrstdout"
		       + "strcatstrchrstrcmpstrcpystrlenstrspnstrstr"
		       + "strtodstrtokstrtolsystemtime_ttmpnamungetc"
		       + "va_argva_end",
		       6, 1);
() = define_keywords_n("C",
		       "asctimebsearchclock_tfgetposfprintffreopen"
		       + "fsetposgetcharisalnumisalphaiscntrlisdigit"
		       + "isgraphislowerisprintispunctisspaceisupper"
		       + "jmp_buflongjmpmemmoveputcharreallocsetvbuf"
		       + "sprintfstrcspnstrncatstrncmpstrncpystrpbrk"
		       + "strrchrstrtoultmpfiletolowertoupperva_list"
		       + "vprintf",
		       7, 1);
() = define_keywords_n("C",
		       "clearerrdifftimeisxdigitstrerror"
		       + "strftimeva_startvfprintfvsprintf",
		       8, 1);
() = define_keywords_n("C", "localtime",
		       9, 1);

_debug_info = 1;

private define get_function_names (names)
{
   for (;c_bol_fsearch ("{");pop_spot (), eol ())
     {
	push_spot ();

	go_left_1 ();
	if (blooking_at ("\\"))
	  {
	     % probably a macro --- skip it
	     continue;
	  }
	% get the function name
	c_bskip_over_comment (1);

	if (blooking_at (")"))
	  {
	     go_left_1 ();
	     if (1 != find_matching_delimiter (')'))
	       continue;

	     c_bskip_over_comment (1);
	     names[bextract_identifier ()] = what_line ();
	  }
     }
}

private define get_macro_names (names)
{
   while (c_re_fsearch ("^[ \t]*#[ \t]*define[ \t]+"))
     {
	() = ffind ("define");
	go_right (6);
	skip_chars (" \t\\\\");
	names [extract_identifier ()] = what_line ();
     }
}

private define get_typedef_names (names)
{
   while (c_re_fsearch ("\\<typedef\\>"))
     {
	go_right (7);

	skip_all_whitespace ();
	if (looking_at ("struct"))
	  {
	     go_right (6);
	     skip_over_comment ();
	     skip_identifier ();       %  struct tag
	     skip_over_comment ();
	     if (looking_at ("{"))
	       {
		  !if (find_matching_delimiter ('{'))
		    continue;
		  go_right (1);
	       }
	  }

	() = c_fsearch (";");

	c_bskip_over_comment (1);
	names [bextract_identifier ()] = what_line();
     }
}

private define process_menu_popup (popup, func)
{
   variable names = Assoc_Type[Int_Type];

   push_spot_bob ();
   (@func) (names);
   pop_spot ();

   variable keys = assoc_get_keys (names);
   keys = keys[array_sort (keys)];
   foreach (keys)
     {
	variable key = ();
	variable line = names[key];
	menu_append_item (popup, key, &goto_line, line);
     }
}

private define macros_popup_callback (popup)
{
   process_menu_popup (popup, &get_macro_names);
}

private define functions_popup_callback (popup)
{
   process_menu_popup (popup, &get_function_names);
}

private define typedefs_popup_callback (popup)
{
   process_menu_popup (popup, &get_typedef_names);
}

public define c_init_menu (menu)
{
   menu_append_popup (menu, "&Functions");
   menu_set_select_popup_callback (strcat (menu, ".&Functions"), &functions_popup_callback);

   if (cmode_is_slang_mode () == 0)
     {
	menu_append_popup (menu, "M&acros");
	menu_append_popup (menu, "&Typedefs");
	menu_set_select_popup_callback (strcat (menu, ".M&acros"), &macros_popup_callback);
	menu_set_select_popup_callback (strcat (menu, ".&Typedefs"), &typedefs_popup_callback);
     }
   menu_append_separator (menu);
   menu_append_item (menu, "&Comment Region", "comment_region");
   menu_append_item (menu, "&Top of Function", "c_top_of_function");
   menu_append_item (menu, "&End of Function", "c_end_of_function");
   menu_append_item (menu, "&Mark Function", "c_mark_function");
   menu_append_item (menu, "&Format Buffer", "c_indent_buffer");
}

private define c_chglog_get_item ()
{
   variable m = create_user_mark ();

   EXIT_BLOCK
     {
	goto_user_mark (m);
     }

   ERROR_BLOCK
     {
	_clear_error ();
	goto_user_mark (m);
	return NULL;
     }

   % First check for a preprocessor macro.
   bol ();
   while (blooking_at ("\\\n"))
     {
	go_left_1 ();
	bol ();
     }
   skip_white ();
   if (looking_at_char ('#'))
     {
	go_right_1 ();
	skip_white ();
	if (looking_at ("define"))
	  {
	     go_right (6);
	     skip_white ();
	     return extract_identifier ();
	  }
     }
   goto_user_mark (m);

   % check for variable
   bol ();
   skip_identifier ();
   if ((0 == bolp ()) and ffind("="))
     {
	bskip_white ();
	return bextract_identifier ();
     }

   % Now try function
   goto_user_mark (m);
   c_end_of_function ();
   variable m_end = create_user_mark ();
   if (m > m_end)
     return NULL;
   c_top_of_function ();
   c_bskip_over_comment (1);

   if (blooking_at (")"))
     {
	go_left_1 ();
	if (1 != find_matching_delimiter (')'))
	  return NULL;
     }

   c_bskip_over_comment (1);
   !if (blooking_at ("typedef struct"))
     return bextract_identifier ();

   goto_user_mark (m_end);
   skip_chars ("} \t\n");
   return extract_identifier ();
}

% This function is called by slang_mode to share the keymap and some hooks
define c_mode_common ()
{
   use_keymap("C");
   set_buffer_hook ("indent_hook", "c_indent_line");
   set_buffer_hook ("newline_indent_hook", "c_newline_and_indent");

   foreach (["C", "SLang"])
     {
	variable mode = ();
	mode_set_mode_info (mode, "init_mode_menu", &c_init_menu);
	mode_set_mode_info (mode, "chglog_get_item", &c_chglog_get_item);
     }
}

%!%+
%\function{c_mode}
%\synopsis{c_mode}
%\usage{Void cmode ();}
%\description
% This is a mode that is dedicated to facilitate the editing of C language files.
% Functions that affect this mode include:
%#v+
%  function:             default binding:
%  c_insert_bra               {
%  c_insert_ket               }
%  newline_and_indent         RETURN
%  indent_line                TAB
%  goto_match                 Ctrl-\
%  c_make_comment             ESC ;
%  c_top_of_function          ESC Ctrl-A
%  c_end_of_function          ESC Ctrl-E
%  c_mark_function            ESC Ctrl-H
%#v-
% Variables affecting indentation include:
%#v+
%  C_INDENT
%  C_BRACE
%  C_BRA_NEWLINE
%  C_CONTINUED_OFFSET
%  C_Comment_Column  (used by c_make_comment)
%  C_Class_Offset
%  C_Switch_Offset
%  C_Colon_Offset
%  C_Namespace_Offset
%#v-
%
% Hooks: \var{c_mode_hook}
%\seealso{c_set_style}
%!%-
define c_mode ()
{
   set_mode("C", 2);

   c_mode_common ();
   set_buffer_hook ("par_sep", "c_paragraph_sep");
   set_buffer_hook ("format_paragraph_hook", &c_format_paragraph);
   mode_set_mode_info ("C", "fold_info", "/*{{{\r/*}}}\r*/\r*/");
   mode_set_mode_info ("C", "dabbrev_case_search", 1);
   use_syntax_table ("C");
   run_mode_hooks("c_mode_hook");
}

%!%+
%\function{c_set_style}
%\synopsis{Set the indentation style for C mode}
%\usage{Void c_set_style (style)}
%\description
% This function sets the C mode indentation variables appropriate for
% a common indentation style.  Currently supported styles include:
%#v+
%    "gnu"      Style advocated by GNU
%    "k&r"      Style popularized by Kernighan and Ritchie
%    "linux"    Linux kernel indentation style
%    "bsd"      Berkeley style
%    "jed"      Style used by the the author
%#v-
%\seealso{c_mode}
%!%-
define c_set_style (name)
{
   switch (strlow(name))
     {
      case "gnu":
	(2,2,1,2,0,2,2,2);
     }
     {
      case "k&r":
	(5,0,0,5,0,5,5,5);
     }
     {
      case "bsd":
	(4,0,0,4,0,4,4,4);
     }
     {
      case "linux":
	(8,0,0,8,0,8,8,8);
     }
     {
      case "jed":
	(3,2,1,2,1,3,3,3);
     }
     {
	if (is_defined ("c_set_style_hook") > 0)
	  return eval(sprintf ("c_set_style_hook(\"%s\");", name));
     }

   (C_INDENT, C_BRACE, C_BRA_NEWLINE, C_CONTINUED_OFFSET,
    C_Colon_Offset, C_Class_Offset, C_Namespace_Offset,
    C_Macro_Indent)
	  =();
}

if (_C_Indentation_Style != NULL)
  c_set_style (_C_Indentation_Style);

provide ("cmode");
