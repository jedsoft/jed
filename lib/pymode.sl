% Python mode 
% File: pymode.sl v1.3.1
%
% For editing source code written in the Python programming language.
% Provides basic compatibility with Python mode under real Emacs
%
% Authors: Harri Pasanen <hpa@iki.fi>
%          Brien Barton <brien_barton@hotmail.com>
%
% following keys have python specific bindings:
%
% DELETE deletes to previous indent level
% TAB indents line
% ^C#  comments region or current line
% ^C>  shifts line or region right
% ^C<  shifts line or region left
% ^C^C executes the region, or the buffer if region not marked.
% ^C|  executes the region
% ^C\t reindents the region
% :    colon dedents appropriately
%
% See python_mode function for available hooks
%
% Shortcomings: does not really support triple-quoted strings in any way.
%

% Changes from v1.0:
% 
% Major improvements, mostly done by Brien Barton:
%
% - execution of python code from JED
% - DFA syntax support
% - improved indent - dedent.
%  

% Changes from v1.1:
%
% Minor fixes, by Tom Culliton
%
% - corrected a syntax error
% - fixed non-DFA syntax hilighting tables to work better
% - added the new assert keyword

% Changes from v1.2
% - autoindent correction

% Changes from v1.3
% - discard return value from run_shell_cmd
% - avoid use of create_array and explicit loop for initializing it.

$1 = "python";

!if (keymap_p ($1)) make_keymap ($1);

definekey_reserved ("py_comment_region", "#", $1);
definekey_reserved ("py_uncomment_region", "3", $1);
definekey_reserved ("py_shift_region_right", ">", $1);
definekey_reserved ("py_shift_region_left", "<", $1);
definekey_reserved ("py_exec", "^C", $1);    % Execute buffer, or region if defined
definekey_reserved ("py_exec_region", "|", $1);
definekey_reserved ("py_reindent_region", "\t", $1);

definekey ("py_backspace_key", "^?", $1);
definekey ("indent_line", "\t", $1);
definekey ("py_electric_colon", ":", $1);
#ifdef MSWINDOWS
definekey ("py_help_on_word", "^@;", $1);
#endif



% Set the following to your favourite indentation level
custom_variable("Py_Indent_Level", 4);

private define py_whitespace(cnt)
{
   if ( get_blocal_var("py_use_tab") )
     loop (cnt / TAB) insert_char('\t');
   else
     insert_spaces (cnt);
}

private define py_line_ends_with_colon()
{
   eol();
   if (bfind_char(':')) {
      go_right(1);
      skip_white();
      if (eolp() or looking_at_char('#'))
	return 1;
   }
   return 0;
}

private define py_endblock_cmd()
{
   bol_skip_white();
   push_mark();
   skip_chars("a-z");
   return is_list_element("return,raise,break,pass,continue",
                          bufsubstr(), ',') > 0;
}

private define py_line_starts_subblock()
{
   bol_skip_white();
   if (looking_at("else") or
      looking_at("elif")  or
      looking_at("except")  or
      looking_at("finally"))
      return 1;
   return 0;
}

private define py_line_starts_block()
{
   bol_skip_white();
   if (looking_at("if") or
      looking_at("try") or
      py_line_starts_subblock())
      return 1;
   return 0;
}

private define py_indent_calculate()
{  % return the indentation of the previous python line
   variable col = 0;
   variable subblock = 0;
    
   EXIT_BLOCK
     {
	pop_spot ();
	return col;
     }
    
   % check if current line starts a sub-block
   subblock = py_line_starts_subblock();
   
   % go to previous non blank line
   push_spot_bol ();
   !if (re_bsearch ("[^ \t\n]"))
     return;
   bol_skip_white();
    
   col = what_column() - 1;
    
   variable indent;
   if ( get_blocal_var("py_use_tab") )
      indent = TAB;
   else
      indent = Py_Indent_Level;

   if (py_line_ends_with_colon())
      col += indent;
   if (py_endblock_cmd() or (subblock and not py_line_starts_block()))
      col -= indent;
}

define py_indent_line()
{
   variable col;
    
   col = py_indent_calculate();
   bol_trim ();
   py_whitespace( col );
}

define py_comment_line() 
{
   bol();
   insert("##");
}

define py_electric_colon() 
{
   variable i;
   insert(":");
   push_spot();
   if (py_line_starts_subblock())  % Only dedents on colon
     {
	pop_spot();
	i = what_column();
	bol_skip_white();
	i = i - what_column();
	if (py_indent_calculate() < what_column()) % Ensure dedent only
	  py_indent_line();
	bol_skip_white();
	goto_column( i + what_column() );
     }
   else
     pop_spot();
}

define py_comment_region()
{
   variable n;
    
   check_region (1);
   n = what_line ();
   pop_mark_1 ();
   loop (n - what_line ())
     {
	py_comment_line();
	go_down_1 ();
     }
   pop_spot();
}

define py_comment() 
{
   push_spot();
   if (markp()) {
      py_comment_region();
   } else {
      py_comment_line();
   }
   pop_spot();
}

define py_uncomment_line() 
{
   bol_skip_white();
   while (looking_at("#")) del();
}

define py_uncomment_region()
{
   variable n;
   
   check_region (1);
   n = what_line ();
   pop_mark_1 ();
   loop (n - what_line ())
     {
	py_uncomment_line();
	go_down_1 ();
     }
   pop_spot();
}

define py_uncomment() {
   push_spot();
   if (markp()) {
      py_uncomment_region();
   } else {
      py_uncomment_line();
   }
   pop_spot();
}

define py_backspace_key() 
{ 
   variable col;                                                    
                                   
   col = what_column(); 
   push_spot(); 
   bskip_white(); 
   if (bolp() and (col > 1)) 
     { 
	pop_spot();                                                     
	if ( blooking_at("\t") )
	  {
	     go_left (1);
	     del();
	  }
	else
	  {
	     bol_trim (); 
	     col--;                                                         
	     if (col mod Py_Indent_Level == 0) 
	       col--; 
	     py_whitespace ( (col / Py_Indent_Level) * Py_Indent_Level );
	  }
     } 
   else 
     {
	pop_spot(); 
	call("backward_delete_char_untabify"); 
     } 
} 

define py_shift_line_right()
{
   variable times = prefix_argument(1);
   bol_skip_white();
   py_whitespace(Py_Indent_Level*times);
}

define py_shift_region_right()
{
   variable times = prefix_argument(1);
   check_region (1);		       %  spot_pushed, now at end of region
   variable n = what_line ();
   pop_mark_1 ();
   loop (n - what_line ())
     {
	set_prefix_argument(times);
	py_shift_line_right();
	go_down_1 ();
     }
   pop_spot();
}

define py_shift_right()
{
   push_spot();
   if (markp()) {
      py_shift_region_right();
   } else {
      py_shift_line_right();
   }
   pop_spot();
}

define py_shift_line_left()
{
   variable times = prefix_argument(1);
   bol_skip_white();
   if (what_column() > Py_Indent_Level*times) 
     {
	if ( get_blocal_var("py_use_tab") )
	  {
	     go_left (times);
	     deln(times);
	  }
	else
	  {
	     push_mark();
	     goto_column(what_column() - Py_Indent_Level*times);
	     del_region();
	  }
     }
}

define py_shift_region_left()
{
   variable times = prefix_argument(1);
  
   check_region (1);
   variable n = what_line ();
   pop_mark_1 ();
   loop (n - what_line ())
     {
	set_prefix_argument (times);
	py_shift_line_left();
	go_down_1 ();
     }
   pop_spot();
}

define py_shift_left() {
   push_spot();
   if (markp()) {
      py_shift_region_left();
   } else {
      py_shift_line_left();
   }
   pop_spot();
}

define py_newline_and_indent()
{
   newline();
   py_indent_line();
}

define file_path(fullname)
{
   variable filename;
   filename = extract_filename(fullname);
   substr(fullname, 1, strlen(fullname)-strlen(filename));
}

define py_exec_region() 
{ 
   % Run python interpreter on current region. 
   % Display output in *shell-output* buffer window. 
   variable oldbuf, thisbuf, file, line, start_line; 
   variable tmpfile = "_python.tmp"; 
   variable error_regexp = "^  File \"\\([^\"]+\\)\", line \\(\\d+\\).*"; 
   variable py_source = buffer_filename(); 
   change_default_dir(file_path(py_source));
   thisbuf = whatbuf(); 
   % Check if 1st line starts in column 1 
   exchange_point_and_mark(); 
   bol_skip_white(); 
   start_line = what_line(); 
   if (what_column() > 1) { 
      % Workaround in case block is indented 
      write_string_to_file("if 1:\n", tmpfile); bol(); 
      start_line--;   % offset for this extra line 
   } 
   exchange_point_and_mark(); 
   append_region_to_file(tmpfile); 
   oldbuf = pop2buf_whatbuf("*shell-output*"); erase_buffer (); 
#ifdef UNIX 
   ()=run_shell_cmd(sprintf("python %s 2>&1", tmpfile));
#else 
   ()=run_shell_cmd(sprintf("python %s", tmpfile));
#endif 
   () = delete_file(tmpfile); 
 
   % try to restore any window that got replaced by the shell-output 
   if (strlen(oldbuf) and (strcmp(oldbuf, "*shell-output*") != 0) 
       and (strcmp(thisbuf, oldbuf) != 0)) { 
      splitwindow(); sw2buf(oldbuf); pop2buf("*shell-output*"); 
   } 
   eob(); 
   %  Check for error message 
   while (re_bsearch(error_regexp) != 0) { 
      %  Make sure error occurred in the file we were executing 
      file = regexp_nth_match(1); 
      line = integer(regexp_nth_match(2)); 
      if (strcmp(file, tmpfile) == 0) { 
	 %  Move to line in source that generated the error 
	 pop2buf(thisbuf); 
	 goto_line(line + start_line - 1); 
	 break; 
      } else { 
	 %  Error is in another file, try previous error message 
	 continue; 
      } 
   } 
   % if there is no output, then close the shell-window and
   % put a message up. This is how emacs works. <jimbag>
   if( bobp() and eobp() ) {
      pop2buf( thisbuf );
      onewindow();
      message( "No output." );
   }
} 


define py_exec() 
{
   % Run python interpreter on current region if one is defined, otherwise
   % on the whole buffer.
   % Display output in *shell-output* buffer window.
   !if (markp()) {		% create region containing entire buffer
      push_spot_bob ();
      push_mark_eob ();
   }
   py_exec_region();
}

define py_reindent() {
   % Reindents a (correctly) indented buffer using the current
   % value of Py_Indent_Level.
   % Warning: Current version can be fooled by implicit or explicit
   %   continuation lines.
   variable indent_level = Int_Type[64];
   %variable indent_level = create_array('i', 64, 1);
   variable level = -1;
   variable current_indent = -1;
   variable errmsg, i, col, ignore, oldlevel;
   
   indent_level[*] = -1;
   bob();
   do {
      bol_skip_white();
      ignore = looking_at_char('#') or eolp();
      if (ignore) continue;	% skip comments and blank lines
      col = what_column() - 1;
      oldlevel = level;		% save current level
      if (col > current_indent) {	% indenting
	 level++;
      } else if (col < current_indent) {	% dedent
	 while ((level > 0) and (indent_level[level] > col)) {
	    indent_level[level] = -1;  % clear current level setting
	    level--;
	 }
      } 
      if ((indent_level[level] != -1) and (indent_level[level] != col)) {
	 % Indent is wrong.  Hopefully it's a continuation line.
	 level = oldlevel;	% reset level
	 bol_trim();
	 py_whitespace(level * Py_Indent_Level + (col - current_indent));
      } else {
	 current_indent = col;
	 indent_level[level] = col;
	 bol_trim();
	 py_whitespace(level * Py_Indent_Level);
      }
   } while (down(1) == 1);
}

define py_reindent_region()
{
   narrow();
   py_reindent();
   widen();
}

#ifdef MSWINDOWS
define py_help_on_word()
{
   variable tag = "0-9A-Z_a-z";

   push_spot ();
   skip_white ();
   bskip_chars (tag);
   push_mark ();
   skip_chars (tag);
   tag = bufsubstr ();		% leave on the stack
   pop_spot ();
   message( strcat("Help on ", tag) );
   msw_help( getenv("PYLIBREF"), tag, 0);
}


#endif

create_syntax_table ($1);
define_syntax ("#", "", '%', $1);		% comments
define_syntax ("([{", ")]}", '(', $1);		% delimiters
define_syntax ('"', '"', $1);			% quoted strings
define_syntax ('\'', '"', $1);			% quoted strings
%define_syntax ('\'', '\'', $1);			% quoted characters
define_syntax ('\\', '\\', $1);			% continuations
define_syntax ("0-9a-zA-Z_", 'w', $1);		% words
define_syntax ("-+0-9a-fA-FjJlLxX.", '0', $1);	% Numbers
define_syntax (",;.:", ',', $1);		% punctuation
define_syntax ("%-+/&*=<>|!~^`", '+', $1);	% operators
set_syntax_flags ($1, 0);			% keywords ARE case-sensitive

() = define_keywords ($1, "ifinisor", 2); % all keywords of length 2
() = define_keywords ($1, "anddefdelfornottry", 3); % of length 3 ....
() = define_keywords ($1, "elifelseexecfrompass", 4);
() = define_keywords ($1, "breakclassprintraisewhileyield", 5);
() = define_keywords ($1, "assertexceptglobalimportlambdareturn", 6);
() = define_keywords ($1, "finally", 7);
() = define_keywords ($1, "continue", 8);

% Type 1 keywords (actually these are what's in __builtins__)
() = define_keywords_n ($1, "id", 2, 1);
() = define_keywords_n ($1, "setsumzipabschrcmpdirhexintlenmapmaxminoctordpowstr", 3, 1);
() = define_keywords_n ($1, "TruebooldictexitfilehelpiterlistquitNoneevalhashlongopenreprtypevars", 4, 1);
() = define_keywords_n ($1, "Falseslicesuperapplyfloatinputrangeroundtuple", 5, 1);
() = define_keywords_n ($1, "bufferinternobjectsortedunichrcoercedivmodfilterlocalsreducereloadxrange", 6, 1);
() = define_keywords_n ($1, "OSErrorWarningcomplexcreditslicenseunicodeIOError__doc__compiledelattrgetattrglobalshasattrsetattr", 7, 1);
() = define_keywords_n ($1, "EllipsisTabErrorpropertyreversedEOFErrorKeyError__name__callableexecfile__call__", 8, 1);
() = define_keywords_n ($1, "Exception__debug__copyrightenumeratefrozensetNameErrorTypeErrorraw_input", 9, 1);
() = define_keywords_n ($1, "basestringisinstanceissubclassIndexErrorSystemExitValueError__import__", 10, 1);
() = define_keywords_n ($1, "LookupErrorUserWarningclassmethodAccessErrorImportErrorMemoryErrorSyntaxErrorSystemError", 11, 1);
() = define_keywords_n ($1, "UnicodeErrorstaticmethodRuntimeError", 12, 1);
() = define_keywords_n ($1, "FutureWarningStandardErrorStopIterationSyntaxWarningConflictErrorOverflowError", 13, 1);
() = define_keywords_n ($1, "AssertionErrorNotImplementedReferenceErrorRuntimeWarningAttributeErrorAssertionError", 14, 1);
() = define_keywords_n ($1, "ArithmeticErrorOverflowWarning", 15, 1);
() = define_keywords_n ($1, "EnvironmentErrorIndentationError", 16, 1);
() = define_keywords_n ($1, "UnboundLocalErrorKeyboardInterruptZeroDivisionError", 17, 1);
() = define_keywords_n ($1, "DeprecationWarningFloatingPointErrorUnicodeDecodeErrorUnicodeEncodeError", 18, 1);
() = define_keywords_n ($1, "NotImplementedError", 19, 1);
() = define_keywords_n ($1, "UnicodeTranslateError", 21, 1);
() = define_keywords_n ($1, "PendingDeprecationWarning", 25, 1);

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_enable_highlight_cache("python.dfa", name);
   dfa_define_highlight_rule("\"\"\".+\"\"\"", "string", name);	% long string (""")
   dfa_define_highlight_rule("'''.+'''", "string", name);	% long string (''')
   dfa_define_highlight_rule("\"[^\"]*\"", "string", name);	% normal string
   dfa_define_highlight_rule("'[^']*'", "string", name);		% normal string
   dfa_define_highlight_rule("#.*", "comment", name);		% comment
   dfa_define_highlight_rule("[A-Za-z_][A-Za-z_0-9]*", "Knormal", name); % identifier
   dfa_define_highlight_rule("[1-9][0-9]*[lL]?", "number", name);	% decimal int
   dfa_define_highlight_rule("0[0-7]*[lL]?", "number", name);		% octal int
   dfa_define_highlight_rule("0[xX][0-9a-fA-F]+[lL]?", "number", name);	% hex int
   dfa_define_highlight_rule("[1-9][0-9]*\\.[0-9]*([Ee][\\+\\-]?[0-9]+)?",
			 "number", name);				% float n.[n]
   dfa_define_highlight_rule("0?\\.[0-9]+([Ee][\\+\\-]?[0-9]+)?",
			 "number", name);				% float [n].n
   dfa_define_highlight_rule("[ \t]+", "normal", name);
   dfa_define_highlight_rule("[\\(\\[{}\\]\\),:\\.\"`'=;]", "delimiter", name);
   dfa_define_highlight_rule("[\\+\\-\\*/%<>&\\|\\^~]", "operator", name); % 1 char
   dfa_define_highlight_rule("<<|>>|==|<=|>=|<>|!=", "operator", name);	  % 2 char
   
   % Flag badly formed numeric literals or identifiers.  This is more effective
   % if you change the error colors so they stand out.
   dfa_define_highlight_rule("[1-9][0-9]*[lL]?[0-9A-Za-z\\.]+", "error", name);	% bad decimal
   dfa_define_highlight_rule("0[0-7]+[lL]?[0-9A-Za-z\\.]+", "error", name); % bad octal
   dfa_define_highlight_rule("0[xX][0-9a-fA-F]+[lL]?[0-9A-Za-z\\.]+", "error", name); % bad hex
   dfa_define_highlight_rule("\\.[0-9]+([Ee][\\+\\-]?[0-9]+)?[A-Za-z]+", "error", name);	% bad float
   dfa_define_highlight_rule("[A-Za-z_][A-Za-z_0-9]*\\.[0-9]+[A-Za-z]*", "error", name); % bad identifier

   dfa_build_highlight_table(name);
}
dfa_set_init_callback (&setup_dfa_callback, "python");
%%% DFA_CACHE_END %%%
#endif

%!%+
%\function{python_mode}
%\synopsis{python_mode}
%\usage{python_mode ()}
%\description
% A major mode for editing python files.
% 
% The following keys have python specific bindings:
%#v+
% DELETE deletes to previous indent level
% TAB indents line
% ^C# comments region or current line
% ^C> shifts line or region right
% ^C< shifts line or region left
% ^C^C executes the region, or the buffer if region not marked.
% ^C|  executes the region
% ^C\t reindents the region
% :    colon dedents appropriately
%#v- 
% Hooks: \var{python_mode_hook}
% 
%\seealso{Py_Indent_Level}
%\seealso{set_mode, c_mode}
%!%-
define python_mode ()
{
   variable python = "python";
   
   create_blocal_var("py_use_tab");
   set_blocal_var(Py_Indent_Level == TAB, "py_use_tab");
   push_spot();
   bob();
   do
     {
	skip_white();
	if ( looking_at("\"\"\"") )
	  {
	     go_right (1);
	     () = fsearch("\"\"\"");
	  }
	else !if (looking_at_char('#') or eolp() or what_column() == 1)
	  {
             bol();
             set_blocal_var(looking_at_char('\t'), "py_use_tab");
             break;
	  }
     }
   while (down(1));
   pop_spot();

   set_mode (python, 0x4); % flag value of 4 is generic language mode
   use_keymap(python);
   set_buffer_hook ("indent_hook", "py_indent_line");
   set_buffer_hook ("newline_indent_hook", "py_newline_and_indent");
   use_syntax_table (python);
   run_mode_hooks ("python_mode_hook");
}
