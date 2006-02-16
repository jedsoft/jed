% This is a text-macro mode designed to edit text using user defined macros.
% Create a syntax table.  Basically \ is a quote and {} are matching delimiters.
$1 = "tm";
create_syntax_table ($1);

define_syntax ("#%+", "#%-", '%', $1);       % Comment Syntax
define_syntax ("#%", "", '%', $1);       % Comment Syntax
define_syntax ('\\', '\\', $1);         % Quote character
define_syntax ("{", "}", '(', $1);    % nothing else matches
define_syntax ("-+a-zA-Z_0-9#", 'w', $1);
%define_syntax ('#', '#', $1);
set_syntax_flags ($1, 8);
() = define_keywords_n ($1, "#d#i#p#v", 2, 1);
() = define_keywords_n ($1, "#p+#p-#s+#s-#v+#v-", 3, 1);

define textmac_paragraph_separator ()
{
   bol ();
   if (looking_at ("#") or looking_at ("\\"))
     return 1;
   skip_white ();
   eolp ();
}

define textmac_wrap_hook ()
{
   push_spot ();
   EXIT_BLOCK
     {
	pop_spot ();
     }
   go_up_1 ();			       %  at eol
   trim ();
   bol ();

   if (looking_at ("#% "))
     {
	go_down_1 ();
	insert ("#% ");
	return;
     }
   if (looking_at_char ('#'))
     {
	eol ();
	!if (blooking_at ("\\"))
	  {
	     insert_single_space ();
	     insert_char ('\\');
	  }
     }
   go_down_1 ();
   indent_line ();
}

private define in_verbatim ()
{
   variable m = create_user_mark ();
   EXIT_BLOCK 
     {
	goto_user_mark (m);
     }
   !if (bol_bsearch ("#v+"))
     return 0;
   !if (bol_fsearch ("#v-"))
     return 1;
   return (create_user_mark () >= m);
}

define tm_insert_quote ()
{
   if (in_verbatim ())
     {
	insert ("\"");
	return;
     }
   call ("text_smart_quote");
}

$1 = "tm";
!if (keymap_p ($1)) make_keymap ($1);
definekey ("tm_insert_quote", "\"", $1);

define tm_mode ()
{
   no_mode ();			       %  reset
   variable mode = "tm";
   use_keymap (mode);
   set_mode (mode, 0x1 | 0x20);
   set_buffer_hook ("par_sep", "textmac_paragraph_separator");
   set_buffer_hook ("wrap_hook", "textmac_wrap_hook");
   use_syntax_table (mode);
   
   mode_set_mode_info (mode, "fold_info", "#%{{{\r#%}}}\r\r");
   TAB = 0;
   run_mode_hooks ("tm_mode_hook");
}
