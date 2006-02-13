% TurboPascal Mode for the jed editor
% jed097b9 and later
%
% Version: minimal
% Update : 06 apr 1995
% Author : Carsten Tinggaard Nielsen, tinggard@iesd.auc.dk
%

variable tpas_objname = "*UnDef ObjectName*";
variable tpas_indent = 2;
variable tpas_tab_save = 0;

define tpas_is_comment() {
  return 0;
}

define tpas_getname(tellstring) {
   variable gname = read_mini(tellstring, Null_String, Null_String);
   return gname;
}

% --------------------------------------------------------------
% Utility routines
define ins_snlp (pos, str) {
   % insert str, newline and indent to pos
   % note that str must be formatted with Sprintf
   insert(str);
   insert("\n");
   if (pos > 0)
     loop(pos) insert_single_space();
}

define tpas_pos() { return what_column() - 1;}

define tpas_paspf(p, name) {
   ins_snlp(p, "(* ");
   ins_snlp(p, " * ");
   ins_snlp(p, " *)");
   ins_snlp(p, "BEGIN");
   insert("END; (* ");
   insert(name);
   ins_snlp(p, " *)");
}

define tpas_delim_string() {
   return "--------------------------------------------------";
}

define tpas_prog_unit_start() {
   variable s = tpas_delim_string();
   vinsert("(* %s\n", s);
   insert(" * Author : Carsten Tinggaard Nielsen\n");
   insert(" * Project: \n");
   insert(" * Module : \n * \n");
   insert(" * $Revision$ $Date$\n");
   insert(" * $Locker$ $Source$\n");
   vinsert(" * %s *)\n", s);
   insert("(*$I-,V-,B+*)\n");
}

define tpas_prog_unit_end() {
   insert("\n  USES\n    DOS;\n\n");
   insert("BEGIN\n");
   insert("END.\n");
   insert("(* History:\n * --------\n * $Log$\n *)\n");
}

define tpas_main() {
   variable progname = tpas_getname("Name of program:");
   bob();
   vinsert ("PROGRAM %s;\n", progname);
   tpas_prog_unit_start();
   insert("(*$M 16000, 128000, 512000 *)\n");
   tpas_prog_unit_end();
   bob();
}

define tpas_unit() {
   variable unitname = tpas_getname("Name of unit:");
   bob();
   vinsert ("UNIT %s;\n", unitname);
   insert("INTERFACE\n\n  USES\n    DOS;\n\n");
   insert("IMPLEMENTATION\n");
   tpas_prog_unit_start();
   tpas_prog_unit_end();
   bob();
}

define tpas_proc() {
   variable p = tpas_pos();
   variable name = tpas_getname("Procedure:");
   ins_snlp(p, sprintf("PROCEDURE %s();", name));
   tpas_paspf(p, name);
   bsearch(");");
}

define tpas_func() {
   variable p = tpas_pos();
   variable name = tpas_getname("Function:");
   ins_snlp(p, sprintf("FUNCTION %s() : ;", name));
   tpas_paspf(p, name);
   bsearch(") :");
}

define tpas_wrap_hook() {
   variable p;
   push_spot();
   go_up_1 ();  bol_skip_white();
   p = _get_point ();
   if (looking_at("BEGIN")) {
      go_down_1 (); skip_white ();
      p = what_column ();
      bol_trim ();
      whitespace (p + tpas_indent);
   }
   pop_spot();
}

% --------------------------------------------------------------
% keymap definiiton
%

!if (keymap_p("TPas")) {
   make_keymap("TPas");
   %undefinekey ("^C");
   definekey_reserved ("tpas_main", "m", "TPas");
   definekey_reserved ("tpas_unit", "u", "TPas");
   definekey_reserved ("tpas_proc", "p", "TPas");
   definekey_reserved ("tpas_func", "f", "TPas");
   definekey ("self_insert_cmd", "\t", "TPas");
}

create_syntax_table("TPas");
define_syntax("(*", "*)", '%', "TPas");
define_syntax ("([", ")]", '(', "TPas");
define_syntax ('\'', '\'', "TPas");
define_syntax ("0-9a-zA-Z_", 'w', "TPas");        % words
define_syntax ("-+0-9a-FA-F.", '0', "TPas");   % Numbers
define_syntax (",;.?:", ',', "TPas");
define_syntax ("@$()[]%-+/*=<>^", '+', "TPas");
set_syntax_flags ("TPas", 5); % case insensitive + C-mode

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_enable_highlight_cache("tpascal.dfa", name);
   dfa_define_highlight_rule("\\(\\*.*\\*\\)", "Qcomment", name);
   dfa_define_highlight_rule("^([^\\(]|\\([^\\*])*\\*\\)", "Qcomment", name);
   dfa_define_highlight_rule("\\(\\*.*", "comment", name);
   dfa_define_highlight_rule("{.*}", "Qcomment", name);
   dfa_define_highlight_rule("^[^{]*}", "Qcomment", name);
   dfa_define_highlight_rule("{.*", "comment", name);
   dfa_define_highlight_rule("^[ \t]*\\*+([ \t].*)?$", "comment", name);
   dfa_define_highlight_rule("[A-Za-z_][A-Za-z_0-9]*", "Knormal", name);
   dfa_define_highlight_rule("[0-9]+(\\.[0-9]+)?([Ee][\\+\\-]?[0-9]*)?",
			 "number", name);
   dfa_define_highlight_rule("\\$[0-9A-Fa-f]*", "number", name);
   dfa_define_highlight_rule("'[^']*'", "string", name);
   dfa_define_highlight_rule("'[^']*$", "string", name);
   dfa_define_highlight_rule("#($[0-9A-Fa-f]+|[0-9]+)", "string", name);
   dfa_define_highlight_rule("[ \t]+", "normal", name);
   dfa_define_highlight_rule("[\\(\\[\\]\\),;\\.\\?:]", "delimiter", name);
   dfa_define_highlight_rule("[@\\-\\+/\\*=<>\\^]", "operator", name);
   dfa_build_highlight_table(name);
}
dfa_set_init_callback (&setup_dfa_callback, "TPas");
%%% DFA_CACHE_END %%%
#endif

() = define_keywords ("TPas", "doifofto", 2);
() = define_keywords ("TPas", "endfornewnilsetvar", 3);
() = define_keywords ("TPas", "bytecasecharelseexitfilehaltrealtextthentypeunituseswithword", 4);
() = define_keywords ("TPas", "arraybeginconstuntilwhile", 5);
() = define_keywords ("TPas", "downtoinlineobjectrecordrepeatstring", 6);
() = define_keywords ("TPas", "booleanintegerlongintpointerprogram", 7);
() = define_keywords ("TPas", "functionshortint", 8);
() = define_keywords ("TPas", "interfaceotherwiseprocedure", 9);
() = define_keywords ("TPas", "implementation", 14);

define tpas_par_sep_hook() {
   variable p;
   push_spot();
   pop_spot();
}

% --------------------------------------------------------------
% Main entry
%
define tpas_mode () {
   set_mode("TPas", 2);
   use_keymap("TPas");
   use_syntax_table("TPas");
   set_buffer_hook("wrap_hook", "tpas_wrap_hook");
   set_buffer_hook("par_sep", "tpas_par_sep_hook");
   %tpas_set_localkeys();
   run_mode_hooks("tpas_mode_hook");
}
