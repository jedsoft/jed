% idl mode

$1 = "IDL";
create_syntax_table ($1);

define_syntax (";", "", '%', $1);
define_syntax ("([{", ")]}", '(', $1);
%define_syntax ('"', '"', $1);
define_syntax ('\'', '"', $1);
define_syntax ('\\', '\\', $1);
define_syntax ("$0-9a-zA-Z_", 'w', $1);        % words
define_syntax ("-+0-9a-fA-F.xXL", '0', $1);   % Numbers
define_syntax (",.?:", ',', $1);
define_syntax ("%-+/&*=<>|!~^", '+', $1);
define_syntax ('@', '#', $1);

set_syntax_flags ($1, 1);	       %  case insensitive

% These are IDL reserved words

() = define_keywords ($1, "$", 1);
() = define_keywords ($1, "doeqgegtifleltneofor", 2);
() = define_keywords ($1, "andendformodnotproxor", 3);
() = define_keywords ($1, "caseelsegotothen", 4);
() = define_keywords ($1, "beginendifuntilwhile", 5);
() = define_keywords ($1, "commonendforendrependwhirepeatreturn", 6);
() = define_keywords ($1, "endcaseendelse", 7);
() = define_keywords ($1, "endwhilefunctionon_error", 8);
() = define_keywords ($1, "endrepeat", 9);
() = define_keywords ($1, "on_ioerror", 10);

define_keywords_n ($1,"abscoseofexpfixhdrmaxminsintan", 3, 1);
define_keywords_n ($1,"alogplotsavesizesortsqrtstopuniqwset", 4, 1);
define_keywords_n ($1,"closefloatflooropenropenwplotsprintranksreadfreadsreadurebinspawntotalwherewshow", 5, 1);
define_keywords_n ($1,"alog10dblarrdoublefinitefltarrindgenintarrlonarrprintfstrarrstringstrlenstrmidstrposstrputwriteuwindow", 6, 1);
define_keywords_n ($1,"defsysvdindgenexecutefindgenget_lunintegerrandomnrandomustr_sepstretchstrtrim", 7, 1);
define_keywords_n ($1,"findfilefree_lunn_params", 8, 1);
define_keywords_n ($1,"histogramimaginaryreplicatestrupcasetranspose", 9, 1);
define_keywords_n ($1,"n_elementsstrlowcasestrmessage", 10, 1);
define_keywords_n ($1,"keyword_setstrcompress", 11, 1);

% idl indentation routines

define idl_find_effective_eol ()
{
   bol ();
   while (ffind_char (';'))
     {
	go_right_1 ();
	if (parse_to_point () == -2)
	  {
	     go_left_1 ();
	     return;
	  }
     }
   eol ();
}

define idl_beginning_of_statement ()
{
   variable n = 0;
   bol_skip_white ();
   
   if (looking_at ("pro ") or looking_at ("function "))
     {
	return 0;
     }
   
   while (up_1 ())
     {
	idl_find_effective_eol (); bskip_white ();
	!if (blooking_at ("$"))
	  {
#iffalse
	     if (bolp ())
	       {
		  skip_white ();
		  if (looking_at_char (';')) continue;
	       }
#endif     
	     go_down_1 ();
	     break;
	  }
	n++;
     }
   bol_skip_white ();
   return n;
}

define idl_indent_to (col)
{
   push_spot ();
   bol_skip_white ();
   if (col != what_column ())
     {
	col--;
	bol_trim ();
	whitespace (col);
     }
   pop_spot ();
}

define idl_looking_at_block (word, begin)
{
   !if (looking_at (word)) return 0;
   EXIT_BLOCK
     {
	pop_spot ();
     }
   
   push_spot ();
   go_right (strlen (word));
   if (ffind (begin)) return 1;
   do 
     {
	idl_find_effective_eol ();
	bskip_white ();
	!if (blooking_at ("$")) break;
     }
   while (down (1));
   bol ();
   return ffind(begin);
}


define idl_is_block_beginnning ()
{
   push_spot ();
   bol_skip_white ();
   orelse
     {idl_looking_at_block ("if ", " begin")}
     {idl_looking_at_block ("while", " begin")}
     {idl_looking_at_block ("else ", " begin")}
     {idl_looking_at_block ("for ", " begin")}
     {idl_looking_at_block ("case ", " of")}
     {idl_looking_at_block ("endif else", " begin")}
     {idl_looking_at_block ("repeat", " begin")}
     {looking_at ("function ")}
     {looking_at ("pro ")}
     {
	idl_find_effective_eol (),
	bskip_chars ("$ \t"),
	blooking_at ("BEGIN") and bfind(":")
     };

   pop_spot ();
}

custom_variable ("Idl_Indent_Amount", 2);
define idl_indent_line ()
{
   variable len = 0, extra_indent = 0;
   variable word;
   
   push_spot ();
   EXIT_BLOCK
     {
	pop_spot ();
	idl_indent_to (len + extra_indent);
	push_mark ();
	bskip_white ();
	pop_mark (not(bolp ()), skip_white ());
     }
   

   if (idl_beginning_of_statement ())
     {
	push_mark ();
	pop_spot ();
	push_spot ();
	bol ();
	if (1 == find_matching_delimiter (')'))
	  {
	     len = what_column ();
	     pop_mark_0 ();
	     len++;
	     return;
	  }
	pop_mark_1 ();
	if (looking_at ("pro ") or looking_at ("function "))
	  {
	     () = ffind_char (' ');
	     skip_white ();
	  }
	skip_chars ("A-Za-z$_!");
	len = what_column () + Idl_Indent_Amount;
	return;
     }
   
   if (looking_at_char ('@')) return;
   
   if (looking_at ("end")) extra_indent = -Idl_Indent_Amount;
   else if (looking_at ("else"))
     {
	go_right (4); skip_white ();
	if (not(looking_at_char (':')))
	  {
	     extra_indent = -Idl_Indent_Amount;
	  }
     }
   
   
   !if (up_1 ())
     {
	len = 0;
	return;
     }
   
   do
     {
	bol_skip_white ();
	!if (eolp () or looking_at_char (';') or looking_at_char ('@'))
	  break;
     }
   while (up_1 ());
   () = idl_beginning_of_statement ();
   len = what_column ();
   
   if (idl_is_block_beginnning ())
     {
	len += Idl_Indent_Amount;
     }
}

define idl_newline_and_indent ()
{
   if (idl_is_block_beginnning ()) idl_indent_line ();
   bskip_white ();
   if (blooking_at (",")) insert (" $");
   newline ();
   idl_indent_line ();
}

$1 = "IDL";
!if (keymap_p ($1))
{
   make_keymap ($1);
   definekey ("newline_and_indent", "\r", $1);
   definekey ("indent_line", "\t", $1);
}

define idl_mode ()
{
   variable idl = "IDL";
   set_mode (idl, 0);
   use_syntax_table (idl);
   use_keymap (idl);
   set_buffer_hook ("indent_hook", "idl_indent_line");
   set_buffer_hook ("newline_indent_hook", "idl_newline_and_indent");
   run_mode_hooks ("idl_mode_hook");
}
