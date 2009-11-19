% C-mode indentation routines
% For the full list of changes, see the changes.txt file.
%
% 2009-02-06
%   recognize ternary operators within parenthesis outdent
% 2007-03-21
%   Fixed off-by-one on parenthesis indent.
%   Promote "foam" to an indentation style
% 2006-12-02
%   Merged in Mark Olesen's patches dated 2006-11-21
%
autoload ("c_make_comment", "cmisc");
autoload ("c_format_paragraph", "cmisc");
autoload ("c_paragraph_sep", "cmisc");
% autoload ("c_comment_region", "cmisc");
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

%!%+
%\variable{C_Switch_Offset}
%\synopsis{Additional indentation to switch blocks}
%\usage{C_Switch_Offset = 0}
%\description
% This function may be used to increase the indentation of code
% within a \exmp{switch} block.  Since this also affects the
% indentation of \exmp{case} statements, \svar{C_Colon_Offset} may
% need to be adjusted.
%\seealso{C_Colon_Offset}
%!%-
custom_variable ("C_Switch_Offset", 0);


%!%+
%\variable{C_Outer_Block_Offset}
%\synopsis{Indentation offset for code in an outer block}
%\usage{C_Outer_Block_Offset = 0}
%\description
% The value of this variable may be used to adjust the indentation of
% code in an outer block.  An outer block is one that has its opening
% brace at the start of a line.  The values of this variable does not
% affect the indentation of C++ classes and namespaces.
%\example
%\notes
%\seealso{}
%!%-
custom_variable ("C_Outer_Block_Offset", 0);

%!%+
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
%\usage{Integer C_Param_Offset_Max = -1}
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

%!%+
%\variable{C_Macro_Indent}
%\synopsis{Control indentation of pre-processor macros}
%\usage{Integer C_Macro_Indent = 3}
%\description
% This variable may be changed to adjust the indentation of
% pre-processor macros.
%
%\seealso{c_mode}
%\seealso{C_INDENT}
%!%-
custom_variable ("C_Macro_Indent", 3);

%!%+
%\variable{C_Bracket_Indent}
%\synopsis{Control indentation within lone brackets}
%\usage{Integer C_Bracket_Indent = 4}
%\description
% Control the alignment of within parenthetic content that start with a lone
% left parenthesis. A value greater than zero uses C_INDENT to determine the
% indentation level. Additionally, common operators are outdented.
%
% eg,
%    while
%    (
%        expr1
%      + expr2
%     == expr3
%    ) ...
%
% A value less than 1 turns this feature off.
%
%\seealso{c_mode}
%\seealso{C_INDENT}
%!%-
custom_variable ("C_Bracket_Indent", 4);


%!%+
%\variable{C_Label_Indents_Relative}
%\synopsis{Set labels to indent relative to block}
%\usage{C_Label_Indents_Relative = 0;}
%\description
% If the value of this variable is non-zero, then label statements
% (goto targets) will get indented by the value of the
% \svar{C_Label_Offset} variable relative to the enclosing block.
% Otherwise, \svar{C_Label_Offset} will be interpreted as an absolute
% offset from the beginning of the line.
%\seealso{C_Label_Offset, C_Colon_Offset}
%!%-
custom_variable ("C_Label_Indents_Relative", 0);

%!%+
%\variable{C_Label_Offset}
%\synopsis{Controls the indentation of label statements}
%\usage{C_Label_Offset = 0;}
%\description
% The value of this variable controls the indentation of (goto) label
% statements.  It is interpreted as an absolute or relative offset
% according to the \svar{C_Label_Indents_Relative} variable.  It does
% not affect the indentation of \exmp{case} statements.
%\seealso{C_Label_Indents_Relative, C_Colon_Offset}
%!%-
custom_variable ("C_Label_Offset", 0);

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
	     !if (looking_at_char ('%'))
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

% search within non-comment or non-string regions
private define c_search (fun, str)
{
   while ((@fun)(str))
     {
	if (parse_to_point ())
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
   if (cmode_is_slang_mode ()) return slmode_bskip_comment (skip_pp);

   forever
     {
	bskip_all_whitespace ();
	if (bobp ())
	  return;

	if (skip_pp)
	  {
	     push_mark ();
	     while (up_1 ())
	       {
		  !if (blooking_at_continuation_char ())
		    {
		       go_down_1 ();
		       break;
		    }
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

   % what does all this do - looking at 'endif' perhaps ?
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
   col++;                       %  add 1 so that we indent under * in /*
   c_indent_to (col);

   if (looking_at_char ('*')
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

%
% detect 1 or 2 character tokens at the beginning of the line
% that would appear to be operators.
% this is useful for nice alignment on (space-delimited) operators
% within parentheses
% eg,
%    while
%    (
%        expr1
%      + expr2
%     == expr3
%    ) ...
%
% returns the outdent value including the space (0, -2 or -3)
%
private define c_outdent_operator()
{
   variable len;

   push_spot();
   bol_skip_white();

   EXIT_BLOCK
     {
	pop_spot();
     }

   % avoid false positives on comments and iostream
   if (looking_at ("/*") or looking_at ("//"))
     return 0;

   % leave iostream alone
   if (looking_at ("<<") or looking_at (">>"))
     return 0;

   len = _get_point ();
   skip_chars("-+*/!&<=>|?:");	% assignment, comparison, logicals, ternary
   len -= _get_point();

   _get_point ();
   skip_white ();

   if ((_get_point () - ()) or eolp())  { % isolated
      len--;	                           % include space-separator
      if ((len == -2) or (len == -3))
	{
	   return len;
	}
   }

   return 0;
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

   % Assume that class/namespace is at the beginning of a line.
   % We may want to change this assumption later.

   % This function can be expensive if the indent_line function is
   % called in a loop.  Get out quickly if the buffer does not have
   % a class or namespace block.
   ifnot (bol_bsearch (name))
     {
	if ((name != "class") || (0 == bol_bsearch (name)))
	  return 0;
     }

   goto_user_mark (bra);

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
	     !if (c_fsearch ("{"))
	       return 0;

	     return bra == create_user_mark ();
	  }

	!if (left(1))
	  break;
     }

   return 0;
}
#endif

% Returns 0 if the point is in the middle of a statement, otherwise
% returns non-zero.
private define between_statements ()
{
   push_spot ();
   c_bskip_over_comment (1);
   variable ret = bobp() or blooking_at (";") or blooking_at ("}")
     or blooking_at ("{");
   pop_spot ();
   return ret;
}

private define is_label_statement ()
{
   push_spot ();
   EXIT_BLOCK
     {
	pop_spot ();
     }

   bol_skip_white ();
   if (0 == between_statements ())
     return 0;

   variable label = extract_identifier ();

   if ((label == "")
       or ((label == "finally") and cmode_is_slang_mode ()))
     return 0;

   skip_all_whitespace ();
   if (0 == looking_at_char (':'))
     return 0;
   if (looking_at ("::"))
     return 0;
   return 1;
}

% This function is called with the point at the end of a line that may
% be continued onto the next (the one we want to indent).  It returns 1
% if the next line ought to be indented as a continued one.
private define is_continuation_line ()
{
   if (orelse
       { blooking_at (";") }	       %  end of statement
	 { blooking_at ("{") }	       %  start of block
	 { blooking_at ("}") }	       %  end of block
	 { bobp () })
     return 0;

   if (blooking_at (":"))
     {
	push_spot ();
	bol_skip_white ();
	if (looking_at ("case") || is_label_statement ())
	  {
	     pop_spot ();
	     return 0;
	  }
	pop_spot ();
     }

   if (0 == blooking_at (","))
     return 1;

   % This part is too naive and needs to be expanded -- for now deal
   % with the common usages of ")," and "},"
   if (blooking_at ("),")) return 1;
   if (blooking_at ("},")) return 0;
   
   return 1;
}

private define blooking_at_one_of (set)
{
   if (bobp ())
     return 0;

   go_left_1 ();
   variable ch = char (what_char ());
   go_right_1 ();

   return is_substr (set, ch);
}

% overview of indentation tracking variables
%
% this_char:
%   - 1st character at the begin of the current line
% match_char:
%   - start character of an enclosing '([{}])' construct, or where we gave up
% match_indent_column:
%   - indentation of the match_char (possibly our reference point)
% match_line:
%   - where the match occurred
% prep_indent:
%   - extra indentation for multi-line preprocessor statements
% not_indenting_pp:
%   - 1: indenting normal statement
%   - 0: indenting preprocessor statements (use prep_indent)
define c_indent_line ()
{
   variable val, col, extra_indent = 0;
   variable prep_indent = 0, prep_line = 0;
   variable match_char, match_indent_column, match_line, line_start_char;
   variable match_mark;
   variable not_indenting_pp = 1;      %  pp: pre-processor statement
   variable is_continuation = 0;
   variable context;

   push_spot ();
   bol_skip_white ();
   line_start_char = what_char ();
   context = parse_to_point ();
   if (-2 == context)
     {
	% In a comment.  Indent it at level of matching /* string
	col = 0;
#iftrue
	if (looking_at ("\\*"))   % "\*" ... "*/" corners
	  col = -1;
#endif
	!if (bsearch ("/*"))
	  col = 0;
	col += what_column ();
	pop_spot ();
	c_indent_continued_comment (col);
	c_mode_if_bol_skip_white ();
	return;
     }

   if (context == -1)
     {
	% First non-whitespace on the line is in the middle of a string.
	% Do nothing.
	pop_spot ();
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
   if (line_start_char == '(')
     {
	go_right_1 ();
	skip_white ();
	if (looking_at_char ('\\'))
	  go_right_1 ();
	indenting_solitary_left_parens = eolp ();
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
	     prep_indent = C_Macro_Indent;
	     not_indenting_pp = 0;
	     prep_line = what_line ();
	  }
     }

   pop_spot ();
   % Now at the start of the first non-whitespace character on the line

   EXIT_BLOCK
     {
	c_mode_if_bol_skip_white ();
     }

   % colon statement
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
   else if (is_label_statement ())
     {
	if (C_Label_Indents_Relative)
	  {
	     extra_indent -= C_INDENT;
	     extra_indent += C_Label_Offset;
	  }
	else 
	  {
	     c_indent_to (C_Label_Offset);
	     pop_spot ();
	     return;
	  }
     }
   else
     {
	forever
	  {
	     c_bskip_over_comment (not_indenting_pp);
	     if (blooking_at_continuation_char ())
	       {
		  go_left_1 ();
		  continue;
	       }

	     % Check to see if the line is a continuation
	     if (is_continuation_line ())
	       {
		  if (not_indenting_pp or is_continuation)
		    {
		       !if (indenting_solitary_left_parens)
			 extra_indent += C_CONTINUED_OFFSET;
		    }
		  else
		    {
		       push_spot ();
		       bol_skip_white ();
		       !if (looking_at_char ('#'))
			 extra_indent += C_CONTINUED_OFFSET;
		       pop_spot ();
		    }
		  is_continuation++;
	       }

	     !if (blooking_at (")")) break;
	     % If the line is continued, then we are at the end of a line
	     % with a closing ')',
	     push_mark ();
	     go_left_1 ();
	     if (1 != find_matching_delimiter (')'))
	       {
		  pop_mark_1 ();
		  break;
	       }
	     c_bskip_over_comment (not_indenting_pp);
	     % If we are at the end of a statement, stop here
	     if (blooking_at (";"))
	       {
		  pop_mark_1 ();
		  break;
	       }
	     push_spot ();
	     if ((1 == find_matching_delimiter (')')), pop_spot ())
	       {
		  pop_mark_1 ();
		  break;
	       }

	     pop_mark_0 ();
	     bol ();

	     !if (not_indenting_pp)
	       {
		  if (looking_at_char ('#'))
		    break;
	       }
	  } %  end of forever loop
     }

   if (not(not_indenting_pp) and (looking_at_char ('#')))
     val = 0;
   else
     val = find_matching_delimiter (')');

   match_mark = create_user_mark ();
   match_char = what_char();
   match_line = what_line ();
   col = what_column ();

   if ((val < 0) and looking_at ("/*")) val = -2;
   else if (val == 1)	   % within (...) grouping
     {
	if (line_start_char != ')')
	  {
	     go_right_1 ();
	     skip_white ();
	     if (eolp())   % ignore (invisible) trailing whitespace
	       col++;
	     else
	       col = what_column ();
	  }
     }

   bol_skip_white ();
   match_indent_column = what_column ();
   if (what_line () < prep_line)
     {
	match_char = 0;
     }

   pop_spot ();	%  Return to the original position

   variable notCcomment;

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

   switch (val)
     {
      case 0: %  Could not find matching '(' for ')'
	if (match_char == '{') %  within {...} block
	  {
	     push_spot ();
	     goto_user_mark (match_mark);

	     %bskip_all_whitespace ();
	     c_bskip_over_comment (not_indenting_pp);
	     if (blooking_at (")"))  %  ... (expr) {...} block
	       {
		  variable same_line = (what_line() == match_line);

		  go_left_1 ();
		  if (1 == find_matching_delimiter (')'))
		    {
		       bol_skip_white ();
		       if (same_line)
			 match_indent_column = what_column ();

		       if ((line_start_char != '}')
			   and looking_at("switch"))
			 match_indent_column += C_Switch_Offset;
		    }
	       }
	     else if (blooking_at("=")) % ... = {...} list
	       {
		  if (is_continuation)
		    extra_indent -= C_CONTINUED_OFFSET;
		  if (line_start_char == '{') % ... = {\n{...}...}
		    extra_indent -= C_BRACE;   %  undone below
	       }
	     else if ((blooking_at ("struct")
		       or blooking_at("(")
		       or blooking_at("[")
		       or blooking_at ("enum"))
		      and (line_start_char != '{'))
	       {
		  if (is_continuation)
		    extra_indent -= C_CONTINUED_OFFSET;
	       }

	     pop_spot ();
	     col = match_indent_column;

	     if (line_start_char == '}')
	       col += C_INDENT;    % undone below
	     else if (inside_class_or_namespace (match_mark, "class"))
	       {
		  col += C_Class_Offset;
	       }
	     else if (inside_class_or_namespace (match_mark, "namespace"))
	       {
		  col += C_Namespace_Offset;
	       }
	     else
	       {
		  if (match_indent_column == 1)
		    col += C_Outer_Block_Offset;
		  col += C_INDENT;
	       }
	     prep_indent = 0;
	  }
	else if (match_char == '[')  %  within [...] construct
	  {
	     push_spot ();
	     if (line_start_char != ']')
	       col++;
	     c_indent_to (col);
	     pop_spot ();
	     return;
	  }
	else			       %  outside all blocks
	  {
	     push_spot ();
	     bol_skip_white ();
	     % Set extra_indent=0 at function declarations:
	     %    |static char *
	     %    |foo
	     % Currently, this results:
	     %    |static char *
	     %    |   foo
	     % FIXME: This is a quick-n-dirty fix that does not work 
	     % in all cases.
	     if (looking_at_char ('{'))
	       extra_indent = 0;

	     if (is_continuation
		 && extra_indent && (prep_indent == 0))
	       {
		  variable ops = "+-/,=&|%^<>!~?.!";
		  ifnot (is_substr (ops + "*", char(what_char())))
		    {
		       push_spot ();
		       c_bskip_over_comment (1);
		       ifnot (blooking_at_one_of ("+-/,=&|%^<>!~?.!"))
			 extra_indent = 0;
		       pop_spot();
		    }
	       }

	     c_indent_to (1 + extra_indent + prep_indent);
	     pop_spot ();
	     return;
	  }
     }
     {
      case 1:			       %  within a (...) grouping
	extra_indent = 0;	       %  match found
	prep_indent = 0;
#iftrue
	% starting brace was alone on its line -
	% indent contents like a {...} block
	if (C_Bracket_Indent > 0)
	  {
	     if (col == match_indent_column + 1)
	       {
		  if (C_Bracket_Indent > C_INDENT)
		    extra_indent = C_Bracket_Indent;
		  else
		    extra_indent = C_INDENT;
                  extra_indent--;  % col already incremented

		  % outdent operators, logicals and comparisons
		  if (extra_indent >= 3)
		    extra_indent += c_outdent_operator();
	       }
	  }
#endif
	if (C_Param_Offset_Max >= 0)
	  if (col - match_indent_column > C_Param_Offset_Max)
	    extra_indent = match_indent_column + C_Param_Offset_Max - col;
     }
     {
      case -2:			       %  inside comment
	if (cmode_is_slang_mode ()) return;
	if (line_start_char != '\\') col++;
	c_indent_continued_comment (col);
	return;
     }
     {
      case 2 or case -1:	       %  inside string or no info
	push_spot_bol ();
	trim ();
	pop_spot ();
	return;
     }
     {
      case -3: 				%  inside C++ comment
	!if ((looking_at(notCcomment)) or not(eolp()))
	  {
	     goto_column(col);
	     if (C_Autoinsert_CPP_Comments) insert(notCcomment);
	  }
	return;
     }

   switch (line_start_char)
     {
      case '}':
	col -= C_INDENT;
	if (not_indenting_pp) col -= extra_indent;  % counteract default addition
     }
     {
      case '{':
	col += C_BRACE;
	if (is_continuation)
	  col -= C_CONTINUED_OFFSET;
     }
   col += extra_indent;

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

% Type 0 keywords (include C++ trigraphs)
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
             % In SLang you can write statements at the same level like
             % functions they look like functions but aren't it.
             variable id = bextract_identifier ();
	     if (0 == any (id == ["for", "_for", "loop", "foreach",
				  "while", "if", "ifnot"]))
	       names[id] = what_line ();
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
	     if (looking_at_char ('{'))
	       {
		  !if (find_matching_delimiter ('{'))
		    continue;
		  go_right_1 ();
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
   if (not(bolp ()) and ffind("="))
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
%    "bsd"      Berkeley style
%    "foam"     Derivate bsd-style used in OpenFOAM
%    "linux"    Linux kernel indentation style
%    "jed"      Style used by the author
%    "kw"       The Kitware style used in ITK, VTK, ParaView, ...
%#v-
%\seealso{c_mode}
%!%-
define c_set_style (name)
{
   switch (strlow(name))
     {
      case "gnu":
	(2,2,1,2,0,2,2,2,0,0,0);
     }
     {
      case "k&r":
	(5,0,0,5,0,5,5,5,0,0,0);
     }
     {
      case "bsd":
	(4,0,0,4,0,4,4,4,0,0,0);
     }
     {
      case "foam":
	(C_Switch_Offset, C_Param_Offset_Max) = (4, -1);
	(4,0,0,4,0,4,0,4,0,0,0);
     }
     {
      case "linux":
	(8,0,0,8,0,8,8,8,0,0,0);
     }
     {
      case "jed":
	(3,2,1,2,1,3,3,3,0,0,0);
     }
     {
      case "kw":
	(0,2,1,2,1,2,2,2,0,0,2);
     }
     {
	if (is_defined ("c_set_style_hook") > 0)
	  return eval(sprintf ("c_set_style_hook(\"%s\");", name));
     }

   (C_INDENT, C_BRACE, C_BRA_NEWLINE, C_CONTINUED_OFFSET,
       C_Colon_Offset, C_Class_Offset, C_Namespace_Offset,
       C_Macro_Indent, C_Label_Offset, C_Label_Indents_Relative,
       C_Outer_Block_Offset
   ) = ();

   _C_Indentation_Style = name;
}

if (_C_Indentation_Style != NULL)
  c_set_style (_C_Indentation_Style);

provide ("cmode");

