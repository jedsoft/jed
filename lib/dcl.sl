% dcl mode--  Special mode to facilitate editing of DCL files on VMS systems.
%

. ( [goal] 3 =goal
 
.   push_spot
.   {up_1}
.    {
.       eol_trim
.       bol_skip_white 
.       '$' looking_at_char
.          { "$\t " skip_chars
. 	   '!' looking_at_char {what_column =goal break} !if
. 	 } 
. 	 {
. 	   '!' looking_at_char 
. 	      {
	         % This takes care of running text following something
		 % like type sys$input
. 	         push_spot
. 	         up_1 eol bolp not and
. 		   {trim go_left_1
. 		    '-' looking_at_char 
. 		      { pop_spot what_column =goal break} !if
. 		   } if
. 		 pop_spot
. 	      } !if
. 	 } else
.    }
.   while
  
.   {"then" looking_at {2 +=goal 1} {0} else}
.   {"else" looking_at {2 +=goal 1} {0} else}
.   orelse pop  % orelse puts final value on stack
.   pop_spot
  
.   bol "$\t " skip_chars
.   {"else" looking_at {2 -=goal 1} {0} else}
.   {"endif" looking_at {2 -=goal 1} {0} else}
.   orelse pop
.   goal 1 > {goal}{3} else
. ) dcl_get_ind


. (
.    [goal]  
.    push_spot
.    dcl_get_ind =goal
   
.    push_spot
.    up_1 {eol_trim bolp {1 go_left '-' looking_at_char {2 +=goal} if} !if} if
.    pop_spot
   
.    bol "\t $" skip_chars
.    '!' looking_at_char 
.      { "\t " bskip_chars trim goal what_column - whitespace} 
.    !if 
.    pop_spot
.    skip_white
. )  dcl_indent


. ( [cont p] 0 =cont
.   bobp bolp '$' looking_at_char and or {"$ \n" insert 1 left pop return} if
.   trim
.   push_spot 
  % If line does not start with '$', assume something like 
  % type sys$input going on and do not give dollar
.   push_spot bol
.   '$' looking_at_char not up_1 and
.     {
.        bolp eol_trim
.        1 go_left
.        what_char '-' != =cont
.     } 
.   if
.   pop_spot
  
.   cont {
.     _get_point =p "sys$input" bfind 
.       { 9 right pop 
.         ':' what_char == {1 right pop} if 
. 	_get_point p == =cont
.       } if
.     p _set_point
.   } !if
  
.   bolp {1 left pop 
.         '-' looking_at_char { 1 =cont} if
.        } 
.       !if
.   pop_spot
.   newline
.   cont {'$' insert_char} !if
.   dcl_indent
. ) dcl_newline
	    

create_syntax_table("dcl");

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_define_highlight_rule("!.*$", "comment", name);
   dfa_define_highlight_rule("\"[^\"]*\"", "string", name);
   dfa_define_highlight_rule("/[a-zA-Z][_a-zA-Z0-9\\-]*", "keyword2", name);
   dfa_define_highlight_rule("\\.([gG]|[lL]|[nN])[eE][sS]?\\.", "preprocess", name);
   dfa_define_highlight_rule("\\.([gG]|[lL])[tT][sS]?\\.", "preprocess", name);
   dfa_define_highlight_rule("\\.[eE][qQ][sS]?\\.", "preprocess", name);
   dfa_define_highlight_rule("\\.[nN][oO][tT]\\.", "preprocess", name);
   dfa_define_highlight_rule("\\.[aA][nN][tT]\\.", "preprocess", name);
   dfa_define_highlight_rule("\\.[oO][rR]\\.", "preprocess", name);
   dfa_define_highlight_rule("[a-zA-Z][\\$a-zA-Z0-9_\\-]*", "Knormal", name);
   dfa_define_highlight_rule("@", "keyword", name);
   dfa_define_highlight_rule("[0-9]+", "number", name);
   dfa_build_highlight_table(name);
}
dfa_set_init_callback (&setup_dfa_callback, "dcl");
%%% DFA_CACHE_END %%%
#endif

set_color ("keyword2","blue","black");

() = define_keywords_n ("dcl", "ifon", 2, 0);
() = define_keywords_n ("dcl", "eodmcrrunset", 3, 0);
() = define_keywords_n ("dcl", "callelseexitgotoopenreadshowthenwait", 4, 0);
() = define_keywords_n ("dcl", "closeendifgosubspawnwrite", 5, 0);
() = define_keywords_n ("dcl", "assigndefinereturn", 6, 0);
() = define_keywords_n ("dcl", "deassign", 8, 0);
() = define_keywords_n ("dcl", "subroutine", 10, 0);
() = define_keywords_n ("dcl", "endsubroutine", 13, 0);

() = define_keywords_n ("dcl", "f$faof$pid", 5, 1);
() = define_keywords_n ("dcl", "f$csidf$cvsif$cvuif$editf$filef$modef$timef$typef$user", 6, 1);
() = define_keywords_n ("dcl", "f$parse", 7, 1);
() = define_keywords_n ("dcl", "f$cvtimef$devicef$getdvif$getenvf$getjpif$getquif$getsyif$lengthf$locatef$searchf$setprvf$stringf$trnlnmf$verify", 8, 1);
() = define_keywords_n ("dcl", "f$contextf$elementf$extractf$integerf$messagef$process", 9, 1);
() = define_keywords_n ("dcl", "f$directoryf$privilege", 11, 1);
() = define_keywords_n ("dcl", "f$identifier", 12, 1);
() = define_keywords_n ("dcl", "f$environment", 13, 1);
() = define_keywords_n ("dcl", "f$file_attributes", 17, 1);

!if (keymap_p ("DCL"))
{
   make_keymap ("DCL");
   definekey ("dcl_newline", "^M", "DCL");
   definekey ("newline_and_indent", "\e^M", "DCL");
   definekey ("dcl_indent", "^I" , "DCL");
   definekey ("self_insert_cmd", "\"", "DCL");
   definekey ("self_insert_cmd", "'", "DCL");
}

define dcl_mode ()
{
   set_syntax_flags ("dcl",0x81);
   use_syntax_table("dcl");
   use_dfa_syntax(1);
   
   use_keymap ("DCL");
   set_mode ("dcl", 4);
   run_mode_hooks("dcl_mode_hook");
}

