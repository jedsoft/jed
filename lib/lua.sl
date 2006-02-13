% Lua mode 
% File: lua.sl v1.02
%
% For editing source code written in the Lua programming language.
%
% Authors: Reuben Thomas <rrt@sc3d.org>
% 
% Adapted from Python mode (pymode 1.2) by:
%          Harri Pasanen <hpa@iki.fi>
%          Brien Barton <brien_barton@hotmail.com>
%          
%
% following keys have lua specific bindings:
%
% DELETE deletes to previous indent level
% TAB indents line
% ^C#  comments region or current line
% ^C>  shifts line or region right
% ^C<  shifts line or region left
% ^C^C executes the region, or the buffer if region not marked.
% ^C|  executes the region
% ^C\t reindents the region
%
% See lua_mode function for available hooks
%
% Shortcomings: a rough hack at the moment.
%

$1 = "Lua";

!if (keymap_p ($1)) make_keymap ($1);

definekey ("lua_backspace_key", "^?", $1);
!if (is_defined ("Win_Keys")) {  % Ctrl-C conflicts with windows region copy.
   definekey_reserved ("lua_comment_region",	"#", $1);
   definekey_reserved ("lua_uncomment_region",	"3", $1);
   definekey_reserved ("lua_shift_region_right",">", $1);
   definekey_reserved ("lua_shift_region_left", "<", $1);
   definekey_reserved ("lua_exec", 		"^C",$1);    % Execute buffer, or region if defined
   definekey_reserved ("lua_exec_region", 	"|", $1);
   definekey_reserved ("lua_reindent_region",	"\t",$1);
   definekey ("indent_line", "\t", $1);
}
#ifdef MSWINDOWS
definekey ("lua_help_on_word", "^@;", $1);
#endif


% Set the following to your favourite indentation level
custom_variable ("Lua_Indent_Level", 4);

private define lua_endblock_cmd()
{
   if (looking_at("end") or
       looking_at("until"))
     return 1;
   return 0;
}

private define lua_line_starts_block()
{
   if (blooking_at("do") or
      blooking_at("then") or
      blooking_at("else") or
      blooking_at("repeat"))
      return 1;
   return 0;
}

private define lua_indent_calculate()
{  % return the indentation of the previous lua line
   variable col = 0;
   variable end_block = 0;
    
   EXIT_BLOCK
     {
	pop_spot ();
	return col;
     }
   
   % check if current line ends a block
   bol_skip_white();
   if (lua_endblock_cmd()) end_block = 1;
  
   % go to previous non blank line
   push_spot_bol ();
   !if (re_bsearch ("[^ \t\n]"))
     return;
   bol_skip_white();
   col = what_column() - 1;
   if (looking_at("function") or (eol(), lua_line_starts_block()))
     col += Lua_Indent_Level;
   if (end_block) col -= Lua_Indent_Level;
}

define lua_indent_line()
{
   variable col;

   col = lua_indent_calculate();
   bol_trim ();
   whitespace( col );
}

define lua_comment_line() 
{
   bol();
   insert("--");
}

define lua_comment_region()
{
   variable n;
    
   check_region (1);
   n = what_line ();
   pop_mark_1 ();
   loop (n - what_line ())
     {
	lua_comment_line();
	go_down_1 ();
     }
   pop_spot();
}

define lua_comment() 
{
   push_spot();
   if (markp()) {
      lua_comment_region();
   } else {
      lua_comment_line();
   }
   pop_spot();
}

define lua_uncomment_line() 
{
   bol_skip_white();
   while (looking_at_char('-')) del();
}

define lua_uncomment_region()
{
   variable n;
   
   check_region (1);
   n = what_line ();
   pop_mark_1 ();
   loop (n - what_line ())
     {
	lua_uncomment_line();
	go_down_1 ();
     }
   pop_spot();
}

define lua_uncomment() {
   push_spot();
   if (markp()) {
      lua_uncomment_region();
   } else {
      lua_uncomment_line();
   }
   pop_spot();
}

define lua_backspace_key() 
{ 
   variable col;                                                    
                                   
   col = what_column(); 
   push_spot(); 
   bskip_white(); 
   if (bolp() and (col > 1)) { 
      pop_spot();                                                     
      bol_trim (); 
      col--;                                                         
      if (col mod Lua_Indent_Level == 0) 
        col--; 
      whitespace ( (col / Lua_Indent_Level) * Lua_Indent_Level ); 
   } 
   else { 
      pop_spot(); 
      call("backward_delete_char_untabify"); 
   } 
} 

define lua_shift_line_right()
{
   bol_skip_white();
   whitespace(Lua_Indent_Level);
}

define lua_shift_region_right()
{
   variable n;
   check_region (1);		       %  spot_pushed, now at end of region
   n = what_line ();
   pop_mark_1 ();
   loop (n - what_line ())
     {
	lua_shift_line_right();
	go_down_1 ();
     }
   pop_spot();
}

define lua_shift_right() 
{
   push_spot();
   if (markp()) {
      lua_shift_region_right();
   } else {
      lua_shift_line_right();
   }
   pop_spot();
}

define lua_shift_line_left()
{
   bol_skip_white();
   if (what_column() > Lua_Indent_Level) {
      push_mark();
      goto_column(what_column() - Lua_Indent_Level);
      del_region();
   }
}

define lua_shift_region_left()
{
   variable n;
   
   check_region (1);
   n = what_line ();
   pop_mark_1 ();
   loop (n - what_line ())
     {
	lua_shift_line_left();
	go_down_1 ();
     }
   pop_spot();
}

define lua_shift_left() {
   push_spot();
   if (markp()) {
      lua_shift_region_left();
   } else {
      lua_shift_line_left();
   }
   pop_spot();
}

define lua_newline_and_indent()
{
   push_spot();
   lua_indent_line();
   pop_spot();
   newline();
   lua_indent_line();
}

define file_path(fullname)
{
   variable filename;
   filename = extract_filename(fullname);
   substr(fullname, 1, strlen(fullname)-strlen(filename));
}

define lua_exec_region() 
{ 
   % Run lua interpreter on current region. 
   % Display output in *shell-output* buffer window. 
   variable oldbuf, thisbuf, file, line, start_line; 
   variable tmpfile = "_lua.tmp"; 
   variable error_regexp = "^  File \"\\([^\"]+\\)\", line \\(\\d+\\).*"; 
   variable lua_source = buffer_filename(); 
   change_default_dir(file_path(lua_source));
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
   run_shell_cmd(sprintf("lua %s 2>&1", tmpfile));
#else 
   run_shell_cmd(sprintf("lua %s", tmpfile));
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
} 

define lua_exec() 
{
   % Run lua interpreter on current region if one is defined, otherwise
   % on the whole buffer.
   % Display output in *shell-output* buffer window.
   !if (markp()) {		% create region containing entire buffer
      push_spot_bob ();
      push_mark_eob ();
   }
   lua_exec_region();
}

define lua_reindent() {
   % Reindents a (correctly) indented buffer using the current
   % value of Lua_Indent_Level.
   % Warning: Current version can be fooled by implicit or explicit
   %   continuation lines.
   variable indent_level = Int_Type[64]-1;
   variable level = -1;
   variable current_indent = -1;
   variable errmsg, i, col, ignore, oldlevel;
   
   bob();
   do {
      bol_skip_white();
      ignore = looking_at_char('-') or eolp();
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
	 whitespace(level * Lua_Indent_Level + (col - current_indent));
      } else {
	 current_indent = col;
	 indent_level[level] = col;
	 bol_trim();
	 whitespace(level * Lua_Indent_Level);
      }
   } while (down(1));
}

define lua_reindent_region()
{
   narrow();
   lua_reindent();
   widen();
}

create_syntax_table ($1);
define_syntax ("--", "", '%', $1);		% comments
define_syntax ("([{", ")]}", '(', $1);		% delimiters
define_syntax ('"', '"', $1);			% quoted strings
define_syntax ('\'', '\'', $1);			% quoted characters
define_syntax ("0-9a-zA-Z_", 'w', $1);		% words
define_syntax ("-+0-9.eE", '0', $1);	        % Numbers
define_syntax (",;.", ',', $1);		        % punctuation
define_syntax ("%-+/*=<>!^", '+', $1);	        % operators
set_syntax_flags ($1, 0);			% keywords ARE case-sensitive

() = define_keywords ($1, "doifinor", 2); % all keywords of length 2
() = define_keywords ($1, "andendfornilnot", 3); % of length 3 ...
() = define_keywords ($1, "elsethen", 4);
() = define_keywords ($1, "breaklocaluntilwhile", 5);
() = define_keywords ($1, "elseifrepeatreturn", 6);
() = define_keywords ($1, "function", 8);

% Type 1 keywords (basic library functions)

() = define_keywords_n ($1, "tag", 3, 1);
() = define_keywords_n ($1, "callgetnnextsorttype", 4, 1);
() = define_keywords_n ($1, "errorprint", 5, 1);
() = define_keywords_n ($1, "_ALERTassertdofilenewtagrawgetrawsetsettag", 6, 1);
() = define_keywords_n ($1, "foreachglobalstinserttremove", 7, 1);
() = define_keywords_n ($1, "dostringforeachitonumbertostring", 8, 1);
() = define_keywords_n ($1, "getglobalsetglobal", 9, 1);
() = define_keywords_n ($1, "gettagmethodsettagmethod", 12, 1);
() = define_keywords_n ($1, "collectgarbagecopytagmethods", 14, 1);

% Type 2 keywords (string, math, I/O, debug library & system functions
% and variables)

() = define_keywords_n ($1, "PI", 2, 2);
() = define_keywords_n ($1, "abscosdegexplogmaxminmodradsintan", 3, 2);
() = define_keywords_n ($1, "acosasinatanceildateexitgsubreadseeksqrt", 4, 2);
() = define_keywords_n ($1, "atan2clockfloorflushfrexpldexplog10write", 5, 2);
() = define_keywords_n ($1, "_INPUT_STDINformatgetenvrandomremoverenamestrlenstrrepstrsub", 6, 2);
() = define_keywords_n ($1, "_OUTPUT_STDERR_STDOUTexecutegetinfostrbytestrcharstrfindtmpnamewriteto", 7, 2);
() = define_keywords_n ($1, "appendtogetlocalopenfilereadfromsetlocalstrlowerstrupper", 8, 2);
() = define_keywords_n ($1, "closefilesetlocale", 9, 2);
() = define_keywords_n ($1, "randomseed", 10, 2);
() = define_keywords_n ($1, "setcallhooksetlinehook", 11, 2);

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_enable_highlight_cache("lua.dfa", name);
   dfa_define_highlight_rule("\\[\\[.+\\]\\]", "string", name);	% long string ([[ ]])
   dfa_define_highlight_rule("\"([^\"\\\\]|\\\\.)*\"", "string", name);	% normal string
   dfa_define_highlight_rule("'([^\'\\\\]|\\\\.)*'", "string", name);	% normal string
   dfa_define_highlight_rule("^#.*", "comment", name);           % #! comment
   dfa_define_highlight_rule("--.*", "comment", name);		% comment
   dfa_define_highlight_rule("[A-Za-z_][A-Za-z_0-9]*", "Knormal", name); % identifier
   dfa_define_highlight_rule("[0-9]+", "number", name);	% decimal int
   dfa_define_highlight_rule("[0-9]+\\.[0-9]*([Ee][\\+\\-]?[0-9]+)?",
   		      "number", name);				% float n.[n]
   dfa_define_highlight_rule("0?\\.[0-9]+([Ee][\\+\\-]?[0-9]+)?",
   		      "number", name);				% float [n].n
   dfa_define_highlight_rule("[ \t]+", "normal", name);
   dfa_define_highlight_rule("[\\(\\[{}\\]\\),\\.\\;]", "delimiter", name);
   dfa_define_highlight_rule("\\.\\.\\.", "delimiter", name);
   dfa_define_highlight_rule("[\\+\\-\\*/%<>=]", "operator", name); % 1 char
   dfa_define_highlight_rule("==|<=|>=|~=|\\.\\.", "operator", name);	  % >1 char
   
   % Flag badly formed numeric literals or identifiers.  This is more effective
   % if you change the error colors so they stand out.
   dfa_define_highlight_rule("[0-9]+[0-9A-Za-z\\.]+", "error", name);	% bad decimal
   dfa_define_highlight_rule("\\.[0-9]+([Ee][\\+\\-]?[0-9]+)?[A-Za-z]+", "error", name);	% bad float
   
   dfa_build_highlight_table(name);
}
dfa_set_init_callback (&setup_dfa_callback, "Lua");
%%% DFA_CACHE_END %%%
#endif

%!%+
%\function{lua_mode}
%\synopsis{lua_mode}
%\usage{lua_mode ()}
%\description
% A major mode for editing lua files.
% 
% The following keys have lua specific bindings:
%#v+
% DELETE deletes to previous indent level
% TAB indents line
% ^C# comments region or current line
% ^C> shifts line or region right
% ^C< shifts line or region left
% ^C^C executes the region, or the buffer if region not marked.
% ^C|  executes the region
% ^C\t reindents the region
%#v- 
% Hooks: \var{lua_mode_hook}
% 
%\seealso{Lua_Indent_Level}
%\seealso{set_mode, c_mode}
%!%-
define lua_mode ()
{
   variable lua = "Lua";
   
   TAB = 8;
   set_mode (lua, 0x4); % flag value of 4 is generic language mode
   use_keymap(lua);
   set_buffer_hook ("indent_hook", "lua_indent_line");
   set_buffer_hook ("newline_indent_hook", "lua_newline_and_indent");
   use_syntax_table (lua);
   run_mode_hooks ("lua_mode_hook");
}
