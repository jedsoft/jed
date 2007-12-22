% The functions here are common to both TeX and LaTeX modes.

$1 = "TeX-Mode";
create_syntax_table ($1);

define_syntax ("%", "", '%', $1);       % Comment Syntax
define_syntax ('\\', '\\', $1);         % Quote character
define_syntax ("{[", "}]", '(', $1);    %  are all these needed?
define_syntax ('$', '"', $1);           %  string
define_syntax ("~^_&#", '+', $1);      %  operators
define_syntax ("|&{}[]", ',', $1);      %  delimiters
define_syntax ("a-zA-Z@", 'w', $1);
set_syntax_flags ($1, 8);

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_enable_highlight_cache ("texcom.dfa", name);
   dfa_define_highlight_rule ("%.*$", "comment", name);
   dfa_define_highlight_rule ("\\\\([A-Za-z]+|.)", "keyword0", name);
   dfa_define_highlight_rule ("[\\|&{}\\[\\]]", "delimiter", name);
   dfa_define_highlight_rule ("[~\\^_&]", "operator", name);
   dfa_define_highlight_rule ("#[1-9]", "operator", name);
   dfa_define_highlight_rule ("\\$\\$?", "string", name);
   dfa_define_highlight_rule (".", "normal", name);
   dfa_build_highlight_table (name);
}
dfa_set_init_callback (&setup_dfa_callback, "TeX-Mode");
%%% DFA_CACHE_END %%%
#endif

%  This hook identifies lines containing TeX comments as paragraph separator
define tex_is_comment ()
{
   bol ();
   while (ffind ("\\%")) go_right (2);
   return ffind_char ('%');
}

variable Tex_Ignore_Comment = 0;       % if true, line containing a comment
                                       % does not delimit a paragraph

define tex_paragraph_separator ()
{
   bol_skip_white ();

   if (eolp () or looking_at("$$")) 
     return 1;

   if (looking_at ("\\"))
     {
	if (orelse 
	    {looking_at ("\\begin")}
	    {looking_at ("\\item")}
	    {looking_at ("\\end")}
	    {re_looking_at ("\\\\[sub]+section{")})
	  return 1;
     }

   %
   %  look for comment
   %
   return not (Tex_Ignore_Comment) and tex_is_comment ();
} 


define tex_wrap_hook ()
{
   variable yep;
   push_spot ();
   yep = up_1 () and tex_is_comment ();
   pop_spot ();
   if (yep)
     {
	push_spot ();
	bol_skip_white ();
	insert ("% ");
	pop_spot ();
     }
   indent_line ();
}

define tex_isolate_paragraph ()
{
   variable ic = Tex_Ignore_Comment;
   Tex_Ignore_Comment = 1;
   push_spot (); push_mark ();
   backward_paragraph ();
   narrow ();
   Tex_Ignore_Comment = ic;
}

define tex_blink_dollar ()
{
   variable pdollar = create_user_mark ();

   insert_char ('$');
   if (blooking_at ("\\$")) return;

   push_spot ();   
   EXIT_BLOCK
     {
	pop_spot ();
     }

   Tex_Ignore_Comment++;
   backward_paragraph ();
   Tex_Ignore_Comment--;
   
   variable pmatch = NULL;
   
   while (fsearch_char ('$'))
     {
	variable tmp_match = create_user_mark ();
	if (tmp_match >= pdollar)
	  break;

	if (tex_is_comment ())
	  {
	     % point left at comment start
	     if (create_user_mark () < tmp_match)
	       {
		  % match in comment
		  eol ();
		  continue;
	       }
	  }
	goto_user_mark (tmp_match);

	if (blooking_at("\\")) 
	  {
	     go_right_1 ();
	     continue;
	  }
	
	if (pmatch == NULL)
	  pmatch = tmp_match;
	else
	  pmatch = NULL;

	go_right_1 ();
     }

   if (pmatch == NULL)
     return;

   variable line = what_line ();
   goto_user_mark (pmatch);
   variable match_line = what_line ();
   if (line - match_line >= window_info ('r'))
     {
	message ("Matches " + line_as_string ());
     }
   else
     {
	update_sans_update_hook(0);
	() = input_pending(10);
     }
}

define tex_insert_quote ()
{
   variable c;
   

   if ((LAST_CHAR != '\'') and (LAST_CHAR != '"'))
     {
	insert_char(LAST_CHAR);
	return;
     }
   
   c = '[';
   !if (bolp())
     {
	go_left_1 ();
	c = what_char();
	go_right_1 ();
     }
   
   if (c == '\\')
     {
	insert_char (LAST_CHAR);
	return;
     }
   
   if (is_substr("[({\t ", char(c)))
     {
	insert_char ('`');
	if (LAST_CHAR == '"') insert_char ('`');
     }
   else
     {
	insert_char ('\'');
	if (LAST_CHAR == '"') insert_char ('\'');
     }
}



define latex_do_environment ()
{
   variable env = Null_String;
   
   push_spot ();
   if (bsearch ("\\begin{"))
     { 
	go_right (7); push_mark ();
	() = ffind_char ('}');
	env = bufsubstr ();
     }
   pop_spot ();
   
   eol (); newline ();
   env = read_mini ("Enter Environment Name:", env, Null_String);
   vinsert ("\\\\begin{%s}\n\n\\\\end{%s}\n", env, env);
   go_up(2);
}

define tex_is_verbatum_environment ()
{
   variable m;

   EXIT_BLOCK
     {
	goto_user_mark (m);
     }
   m = create_user_mark ();
   !if (bsearch ("\\begin{verbatum}"))
     return 0;
   !if (fsearch ("\\end{verbatum}"))
     return 1;
   return m <= create_user_mark ();
}

   
define tex_ldots ()
{
   if (blooking_at (".."))
     {
	!if (tex_is_verbatum_environment ())
	  {
	     go_left (2);
	     deln (2);
	     insert ("{\\ldots} ");
	     return;
	  }
     }
   insert_char ('.');
}

provide ("texcom");
