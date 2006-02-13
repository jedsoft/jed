% Lisp mode

$1 = "LISP";
create_syntax_table ($1);
define_syntax ("([", ")]", '(', $1);
define_syntax (";", "", '%', $1);
define_syntax ('"', '"', $1);
define_syntax ('\\', '\\', $1);

define_syntax ("0-9a-zA-Z_", 'w', $1);        % words
define_syntax ("-+0-9", '0', $1);   % Numbers
% define_syntax ("", ',', $1);      % Delimiters
define_syntax ('#', '#', $1);	       %  preprocessor
% define_syntax ("%-+/&*=<>|!~^", '+', $1);  % binary operators

() = define_keywords ($1, "eqifor", 2);
() = define_keywords ($1, "letnot", 3);
() = define_keywords ($1, "setq", 4);
() = define_keywords ($1, "defunprognwhile", 5);

define lisp_indent_line ()
{
   variable val, col;
   push_spot ();
   bol ();
   val = find_matching_delimiter (')');
   col = what_column ();
   if (val == 1) col += 3;
   pop_spot ();
   push_spot ();
   bol_skip_white ();
   if (col != what_column ())
     {
	bol_trim ();
	col--; whitespace (col);
     }
   pop_spot ();
   push_mark ();
   bskip_white ();
   if (bolp ()) 
     {
	skip_white ();
	pop_mark_0 ();
     }
   else pop_mark_1 ();
}

define lisp_mode ()
{
   set_mode("lisp", 2);
   set_buffer_hook ("indent_hook", "lisp_indent_line");
   use_syntax_table ("LISP");
   run_mode_hooks ("lisp_mode_hook");
}
