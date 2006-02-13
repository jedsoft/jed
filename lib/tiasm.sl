% A simple TI asm mode

$1 = "tiasm";
create_syntax_table ($1);

define_syntax (";", "", '%', $1);       % Comment Syntax
%define_syntax ('\\', '\\', $1);         % Quote character
define_syntax ("{[", "}]", '(', $1);    %  are all these needed?
define_syntax ('\'', '"', $1);           %  string
define_syntax ("$~^_&#", '+', $1);      %  operators
define_syntax ("|&{}[],", ',', $1);      %  delimiters
define_syntax ("a-zA-Z0-9.", 'w', $1);
set_syntax_flags ($1, 1 | 2);
set_fortran_comment_chars ($1, "*");

% Type 0 keywords
() = define_keywords_n ($1, 
			"andashbuddbdldaldilshmh1noppopsti",
			3, 0);

() = define_keywords_n ($1, 
			"addibeqdbledcmpildizlhu0lhu1pushretsrptbsubi",
			4, 0);

() = define_keywords_n ($1,
			"ldinzrptbd",
			5, 0);

% Type 1 keywords
() = define_keywords_n ($1,
			".if",
			3, 1);
() = define_keywords_n ($1,
			".end",
			4, 1);
() = define_keywords_n ($1,
			".else.text",
			5, 1);
() = define_keywords_n ($1,
			".endif.globl",
			6, 1);

define tiasm_mode ()
{
   variable kmap = "tiasm";
   set_mode(kmap, 4);
   use_syntax_table (kmap);
   mode_set_mode_info (kmap, "fold_info", "*{{{\r*}}}\r\r");
   run_mode_hooks("tiasm_mode_hook");
}
