% S-Lang mode is just a superset of C mode so make sure it is loaded.
require ("cmode");

$1 = "SLANG";

create_syntax_table ($1);
define_syntax ("%", "", '%', $1);
define_syntax ("([{", ")]}", '(', $1);
define_syntax ('"', '"', $1);
define_syntax ('`', '"', $1);
define_syntax ('\'', '\'', $1);
define_syntax ('\\', '\\', $1);
define_syntax ("0-9a-zA-Z_$", 'w', $1);        % words
define_syntax ("-+0-9a-fA-F.xX", '0', $1);   % Numbers
define_syntax (",;:.", ',', $1);
define_syntax ('#', '#', $1);
define_syntax ("%-+/&*=<>|!~^", '+', $1);

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_enable_highlight_cache ("slmode.dfa", name);
   dfa_define_highlight_rule("^[ \t]*#", "PQpreprocess", name);
   dfa_define_highlight_rule("%.*$", "comment", name);
   dfa_define_highlight_rule("[A-Za-z_\\$][A-Za-z_0-9\\$]*", "Knormal", name);
   dfa_define_highlight_rule("[0-9]+(\\.[0-9]*)?([Ee][\\+\\-]?[0-9]*)?",
			 "number", name);
   dfa_define_highlight_rule("0[xX][0-9A-Fa-f]*", "number", name);
   dfa_define_highlight_rule("\"([^\"\\\\]|\\\\.)*\"", "string", name);
   dfa_define_highlight_rule("\"([^\"\\\\]|\\\\.)*\\\\?$", "string", name);
   dfa_define_highlight_rule("'([^'\\\\]|\\\\.)*'", "string", name);
   dfa_define_highlight_rule("'([^'\\\\]|\\\\.)*\\\\?$", "string", name);
   dfa_define_highlight_rule("[ \t]+", "normal", name);
   dfa_define_highlight_rule("[\\(\\[{}\\]\\),;\\.\\?:]", "delimiter", name);
   dfa_define_highlight_rule("[%\\-\\+/&\\*=<>\\|!~\\^]", "operator", name);
   dfa_define_highlight_rule("!if", "keyword0", name);
   dfa_build_highlight_table(name);
}
dfa_set_init_callback (&setup_dfa_callback, "SLANG");
%%% DFA_CACHE_END %%%
#endif

() = define_keywords ($1, "doifor", 2);
() = define_keywords ($1, "andchsformodnotpopshlshrsqrtryxor", 3);
() = define_keywords ($1, "NULL_forcaseelseexchloopmul2signthen", 4);
() = define_keywords ($1, "__tmpbreakcatchifnotthrowusingwhile", 5);
() = define_keywords ($1, "defineorelsepublicreturnstaticstructswitch", 6);
() = define_keywords ($1, "andelsefinallyforeachforeverprivatetypedef", 7);
() = define_keywords ($1, "continuevariable", 8);
() = define_keywords ($1, "EXIT_BLOCK", 10);
() = define_keywords ($1, "ERROR_BLOCK", 11);
() = define_keywords ($1, "EXECUTE_ERROR_BLOCK", 19);

% Exceptions -- keyword1
() = define_keywords_n ($1, "IOErrorOSError", 7, 1);
() = define_keywords_n ($1, "AnyError", 8, 1);
() = define_keywords_n ($1, "DataErrorMathErrorOpenErrorReadErrorUTF8Error", 9, 1);
() = define_keywords_n ($1, "IndexErrorParseErrorStackErrorUsageErrorWriteError", 10, 1);
() = define_keywords_n ($1, "DomainErrorImportErrorMallocErrorSyntaxError", 11, 1);
() = define_keywords_n ($1, "NumArgsErrorRunTimeErrorUnicodeErrorUnknownError", 12, 1);
() = define_keywords_n ($1, "InternalErrorReadOnlyError", 13, 1);
() = define_keywords_n ($1, "NamespaceErrorUserBreakError", 14, 1);
() = define_keywords_n ($1, "ApplicationErrorInvalidParmError", 16, 1);
() = define_keywords_n ($1, "DivideByZeroErrorTypeMismatchError", 17, 1);
() = define_keywords_n ($1, "ArithOverflowErrorLimitExceededErrorStackOverflowErrorUndefinedNameError", 18, 1);
() = define_keywords_n ($1, "ArithUnderflowErrorNotImplementedErrorStackUnderflowError", 19, 1);
() = define_keywords_n ($1, "DuplicateDefinitionError", 24, 1);
() = define_keywords_n ($1, "VariableUninitializedError", 26, 1);

% Format paragraph hook
private define skip_comment_whitespace ()
{
   skip_white ();
   skip_chars ("%");
   skip_white ();
}

private define is_empty_comment_line ()
{
   bol ();
   skip_comment_whitespace ();
   return eolp ();
}

private define mark_comment_whitespace ()
{
   bol ();
   push_mark ();
   skip_comment_whitespace ();
}

private define is_para_sep ()
{
   bol_skip_white ();
   !if (looking_at ("%"))
     return 1;
   
   if (is_empty_comment_line ())
     return 1;
   
   % Now look for special documentation marks.  The embedded tm docs begin
   % with %!%+ and end with %!%-
   bol ();
   if (looking_at ("%!%"))
     return 1;

   % Something like %\function{foo}
   if (looking_at ("%\\"))
     return 1;

   return 0;
}

private define wrapok_hook ()
{
   push_spot ();
   EXIT_BLOCK 
     {
	pop_spot ();
     }

   bol_skip_white ();
   return (what_char () == '%');
}

private define wrap_hook ()
{
   push_spot ();
   go_up_1 ();
   mark_comment_whitespace ();
   variable prefix = bufsubstr ();
   go_down_1 ();
   insert (prefix);
   pop_spot ();
}

private define format_paragraph_hook ();
private define format_paragraph_hook ()
{
   push_spot ();
   EXIT_BLOCK
     {
	pop_spot ();
     }
   eol ();

   if (parse_to_point () != -2)
     return;
   bol_skip_white ();
   !if (looking_at ("%"))
     return;
   
   bol();
   push_mark ();
   skip_white ();
   skip_chars ("%");
   variable prefix = bufsubstr ();
   
   skip_white ();
   if (eolp ())
     return;
   variable indent_column = what_column ();
   
   % Find start
   while (up_1 ())
     {
	if (is_para_sep ())
	  {
	     go_down_1 ();
	     break;
	  }
     }
   bol ();
   push_mark ();

   % Find comment end
   while (down_1 ())
     {
	if (is_para_sep ())
	  {
	     up_1 ();
	     break;
	  }
     }
   narrow ();
   
   bob ();
   do
     {
	mark_comment_whitespace ();
	del_region ();
	whitespace (indent_column-1);
     }
   while (down_1 ());

   bob ();

   unset_buffer_hook ("format_paragraph_hook");
   call ("format_paragraph");
   set_buffer_hook ("format_paragraph_hook", &format_paragraph_hook);

   bob ();
   variable prefix_len = strlen (prefix);
   do
     {
	insert (prefix);
	deln (prefix_len);
     }
   while (down_1 ());
   widen ();
   return;
}

% This function attempts to implement some form of wrapping mode in the
% presence of comments.  However, the embedded documentation comment style
% (using text-macro) means that this will need to be modified to be more
% sophisticated.  Basically the modifications would require analysing the
% text-macro context.  For example, wrapping is not appropriate in verbatim 
% sections.
#ifntrue
define slmode_insert_space ()
{
   variable cstr;
   
   if (is_overwrite_mode ())
     {
	call ("self_insert_cmd");
	return;
     }

   EXIT_BLOCK
     {
	insert_single_space ();
     }


   % The following code attempts a wrapping mode in the presence of comments
   !if (cmode_is_slang_mode ()) return;
   if (not (eolp ()) or (what_column () <= WRAP)) return;
   
   % we are at the end of line.
   cstr = "%!% ";
   bol ();
   !if (looking_at (cstr), eol ()) return;
   !if (bfind_char (' ')) return;
   trim ();
   newline ();
   insert (cstr);
   eol ();
}
#endif
   
define slang_mode ()
{
   set_mode("SLang", 2 | 8);
   c_mode_common ();
   use_syntax_table ("SLANG");
   %local_setkey ("slmode_insert_space", " ");
   mode_set_mode_info ("SLang", "fold_info", "%{{{\r%}}}\r\r");
   mode_set_mode_info ("SLang", "dabbrev_case_search", 1);
   set_buffer_hook ("format_paragraph_hook", &format_paragraph_hook);
   set_buffer_hook ("wrap_hook", &wrap_hook);
   set_buffer_hook ("wrapok_hook", &wrapok_hook);
   unset_buffer_hook ("par_sep");
   run_mode_hooks("slang_mode_hook");
}

