% This is a simple shell mode.  It does not defined any form of indentation
% style.  Rather, it simply implements a highlighting scheme.

$1 = "SH";

create_syntax_table ($1);
define_syntax ("#", "", '%', $1);
define_syntax ("([{", ")]}", '(', $1);

% Unfortunately, the editor cannot currently correctly deal with multiple 
% string characters.  So, inorder to handle something like:
%    echo "I'd rather be home"
% make the '"' character the actual string character but also give '\'' 
% a string syntax.  However, this will cause '"' to give problems but 
% usually, '"' characters will be paired.
define_syntax ('\'', '"', $1);
define_syntax ('"', '"', $1);

define_syntax ('\\', '\\', $1);
define_syntax ("-0-9a-zA-Z_", 'w', $1);        % words
define_syntax ("-+0-9", '0', $1);   % Numbers
define_syntax (",;:", ',', $1);
define_syntax ("%-+/&*=<>|!~^", '+', $1);

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_enable_highlight_cache ("shmode.dfa", name);
   dfa_define_highlight_rule ("\\\\.", "normal", name);
   dfa_define_highlight_rule ("#.*$", "comment", name);
   dfa_define_highlight_rule ("\"([^\\\\\"]|\\\\.)*\"", "string", name);
   dfa_define_highlight_rule ("\"([^\\\\\"]|\\\\.)*$", "string", name);
   dfa_define_highlight_rule ("'[^']*'", "string", name);
   dfa_define_highlight_rule ("'[^']*$", "string", name);
   dfa_define_highlight_rule ("[\\|&;\\(\\)<>]", "Qdelimiter", name);
   dfa_define_highlight_rule ("[\\[\\]\\*\\?]", "Qoperator", name);
   dfa_define_highlight_rule ("[^ \t\"'\\\\\\|&;\\(\\)<>\\[\\]\\*\\?]+",
			  "Knormal", name);
   dfa_define_highlight_rule (".", "normal", name);
   dfa_build_highlight_table (name);
}
dfa_set_init_callback (&setup_dfa_callback, "SH");
%%% DFA_CACHE_END %%%
#endif

() = define_keywords ($1, "cddofiifin", 2);
() = define_keywords ($1, "forletpwdset", 3);
() = define_keywords ($1, "casedoneechoelifelseesacevalexitifeqreadtestthentype", 4);
() = define_keywords ($1, "aliasbreakendifendswifdefifneqlocalshiftumaskunsetuntilwhile", 5);
() = define_keywords ($1, "exportifndefreturnsetenvsourceswitch", 6);
() = define_keywords ($1, "breaksw", 7);
() = define_keywords ($1, "continuefunction", 8);

define sh_mode ()
{
   set_mode("SH", 0);
   use_syntax_table ("SH");
   mode_set_mode_info ("SH", "fold_info", "#{{{\r#}}}\r\r");
   run_mode_hooks("sh_mode_hook");
}

