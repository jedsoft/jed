% tcl mode 0.5 (derived from shmode.sl, cmode.sl and latex.sl)
% by David Schweikert (dwschwei@stud.ee.ethz.ch)
%
% ChangeLog
% ---------
% 0.1    18 Aug 97    Initial version
% 0.2    25 Aug 97    The indenting is much better. Small bug fixes.
%                     Escaped braces correctly parsed.
% 0.3    31 Aug 97    Syntax highlighting reworked. Now only Tcl/Tk
%                     commands are highlighted.
%                     Highlighting of command options added.
%                     Syntax Check added.
% 0.4    27 Sep 97    Little bugfix in indenting.
% 0.5    24 Feb 98    Integrated into the JED distribution.
%                     Added syntax highlighting for numbers.
%
% Description
% -----------
% JED-mode for the syntax-highlighting and automatic indentation of
% Tcl/Tk programs.
%
% Notes
% -----
% - This mode only does DFA syntax-highlighting and a very
% rough automatic indentation (assumes that the previous line
% is correctly indented)
% - How to highlight is very personal... Modify the code to match your
%   preference!
% - I am very disponible for comments, suggestions,... (via e-mail)
%
% Keybindings
% -----------
% ^C^Q     tcl_indent_region   
% {        tcl_insert_bra
% }        tcl_insert_ket
% #        tcl_insert_comment
%
% Syntax Check
% ------------
% This mode does recognise evident syntax-errors which are common
% to beginners (I am a beginner :-)):
% 
% - The open brace should be opened on the same line of the command
%   it belongs to and not on a line by itself as many do in C:
%   Wrong:                      Right:
%          if {$a == 1}                  if {$a == 1} {
%          {                                set b 2
%             set b 2                    }
%          }
%   Note that the open-brace at the beginning of a line could be
%   correct but isn't certainly common.
%
% - The comment is also a sort of command in Tcl, so you can't just add
%   comments on the right of some statement. You have to add
%   a command separator if you want to:
%   # this is right
%   set a 2 # this is wrong
%   set b 3 ;# this is right
%
% If tclmode recognises such an error, it displays a "Warning: ...".
% You can disable the Syntax Check by setting 'Tcl_Check_Syntax' to 0.
% 
% Syntax-highlighting
% -------------------
% The syntax-coloring-types of jed aren't very adequate for tcl,
% so, I used them as follows:
%
% -     normal: as expected
% -    comment: as expected
% -  delimiter: as expected
% -     string: as expected (plus "$variable")
% -    keyword: as expected (tcl: type 0, tk: type 1)
% -     number: as expected
% - preprocess: proc definition (first line)
% -   operator: command option (for example '-font')
%
% ToDo
% ----
% - More syntax-checks (ideas?).
% - Interaction with tclsh/wish (difficult).
% - Smarter indenting.
% - More commands from incr Tcl, BLT, ...
% - tcl_comment_region.
% - syntax highlighting without DFA.

variable Tcl_Check_Syntax = 1;
% ---

$1 = "TCL";

create_syntax_table ($1);

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_enable_highlight_cache ("tclmode.dfa", name);
   dfa_define_highlight_rule ("^[ \\t]*#.*$", "comment", name);
   dfa_define_highlight_rule (";[ \\t]*#.*$", "comment", name);
   dfa_define_highlight_rule ("\"([^\\\\\"]|\\\\.)*\"", "string", name);
   dfa_define_highlight_rule ("[{}\\[\\]]", "Qdelimiter", name);
   dfa_define_highlight_rule("[0-9]+(\\.[0-9]*)?([Ee][\\+\\-]?[0-9]*)?","number", name);
   dfa_define_highlight_rule (".", "normal", name);
   dfa_define_highlight_rule("[A-Za-z_\\.:]+", "Knormal", name);
   dfa_define_highlight_rule ("\\$[a-zA-Z0-9_:]+", "string", name);
   dfa_define_highlight_rule ("\\${.*}", "Qstring", name);
   dfa_define_highlight_rule ("^[ \\t]*proc.*$", "Qpreprocess", name);
   dfa_define_highlight_rule (" -[a-z]+ ", "operator", name);
   dfa_build_highlight_table (name);
}
dfa_set_init_callback (&setup_dfa_callback, "TCL");
%%% DFA_CACHE_END %%%
#endif

% Keywords (machine generated)
% Tcl commands
$2 = 0;
() = define_keywords_n ($1, "cdif", 2, $2);
() = define_keywords_n ($1, "eofforpidpwdset", 3, $2);
() = define_keywords_n ($1, "caseelseevalexecexitexprfilegetsglobincrinfojoinlistloadopenprocputsreadscanseektelltime", 4, $2);
() = define_keywords_n ($1, "afterarraybreakcatchclockcloseerrorfcopyflushlsortsplitsubsttraceunsetupvarvwaitwhile", 5, $2);
() = define_keywords_n ($1, "appendbinaryconcatformatglobalinterplindexlrangeregexpregsubrenamereturnsocketsourcestringswitch", 6, $2);
() = define_keywords_n ($1, "consoleforeachhistorylappendlinsertllengthlsearchunknownuplevel", 7, $2);
() = define_keywords_n ($1, "continuefblockedlreplace", 8, $2);
() = define_keywords_n ($1, "fileevent", 9, $2);

% Tk commands
$2 = 1;
() = define_keywords_n ($1, "tkwm", 2, $2);
() = define_keywords_n ($1, "bellbindfontgrabgridmenupacksendtext", 4, $2);
() = define_keywords_n ($1, "entryeventfocusframeimagelabellowerplaceraisescalewinfo", 5, $2);
() = define_keywords_n ($1, "buttoncanvasoptiontkwaitupdate", 6, $2);
() = define_keywords_n ($1, "destroylistboxmessagetkerror", 7, $2);
() = define_keywords_n ($1, "bindtagstk_popuptoplevel", 8, $2);
() = define_keywords_n ($1, "clipboardscrollbarselectiontk_bisquetk_dialog", 9, $2);
() = define_keywords_n ($1, "menubutton", 10, $2);
() = define_keywords_n ($1, "checkbuttonradiobutton", 11, $2);
() = define_keywords_n ($1, "tk_focusNexttk_focusPrev", 12, $2);
() = define_keywords_n ($1, "tk_messageBoxtk_setPalette", 13, $2);
() = define_keywords_n ($1, "tk_chooseColortk_getOpenFiletk_getSaveFiletk_optionsMenu", 14, $2);


define tcl_indent_line ();	       %  forward declaration

% Indentation and Syntax Check
define tcl_newline_and_indent ()
{
   newline ();
   tcl_indent_line ();
}

define tcl_indent_to (n)
{
   bol_skip_white ();
   if (what_column != n)
     {
	bol_trim ();
	n--;
	whitespace (n);
     }
}

% Counts unmatched braces
define tcl_count_braces ()
{
   variable c, open_count = 0, close_count = 0;
   variable escaped = 0;
   
   push_spot();
   
   for(bol (); not(eolp()); go_right_1()) {
      c = what_char ();
      switch(c) {
       case '\\': !if(escaped) escaped = 2;
      }{
       case '{': !if(escaped) open_count++;
      }{
       case '}': !if(escaped) {
	  if (open_count) open_count--;
	  else close_count++;
       }
      }
      if(escaped) escaped--;
   }
   
   pop_spot();
   return(open_count, close_count);
}

% Search for a valid previous line and go to it.
define tcl_go_up ()
{
   forever {
      if (up_1 ()) {
	 bol_skip_white ();
	 if (eolp ()) continue;
	 if (what_char () != '#') return(1);
      }
      else return(0);
   }
}

% Is the next line the continuation of this one?
define tcl_is_continued_line ()
{
   eol();
   bskip_white ();
   if (blooking_at ("\\")) 1;
   else 0;
   return;
}

% How much indenting based on the previous line?
define tcl_prev_line_rule ()
{
   variable indent = 0, open_count;
   push_spot ();
   if(tcl_go_up ()) {
      % It is assumed that this line is correctly aligned.
      indent = what_column ();
      % If we find first an unmatched '{', then the next line should be indented.
      (open_count, ) = tcl_count_braces ();
      indent += open_count * C_INDENT;
      
      % Is the next a continuation line?
      if (tcl_is_continued_line ()) indent += C_CONTINUED_OFFSET;
      if (tcl_go_up() and tcl_is_continued_line()) indent -= C_CONTINUED_OFFSET;
   }
   
   pop_spot ();
   return (indent);
}

% How much indenting based on the current line?
define tcl_cur_line_rule ()
{
   variable indent = 0, close_count = 0;
   % If we find an unmatched '}', then this line should be unindented.
   (, close_count) = tcl_count_braces ();
   indent -= close_count * C_INDENT;
   return (indent);
}

% Indent the current line.
define tcl_indent_line ()
{
   variable cursor, oldindent;
   variable indent;
   
   % ---- Could be skipped in tcl_indent_region!
   cursor = what_column ();
   bol_skip_white ();
   oldindent = what_column ();
   % ----
   
   indent  = tcl_prev_line_rule ();
   indent += tcl_cur_line_rule ();
   
   % message(Sprintf("%d : %d", indent_prev, indent_cur, 2));
   
   tcl_indent_to (indent);
   goto_column (cursor + indent - oldindent); % Could be skipped in tcl_indent_region!
}

% Indent the selected region (bound to ^C^Q)
define tcl_indent_region ()
{
   check_region(1);
   pop_mark_1 (); 
   push_mark();
   tcl_indent_line(); % set initial line indentation before narrowing
   pop_spot();
   
   push_spot();
   go_up_1 ();
   narrow();
   bob();
   
   flush("Indenting region...");
   while (down_1 ()) {  % indent line by line (ie slowly)
      tcl_indent_line();
      % flush(Sprintf("Indenting line %d", what_line(), 1));
   }
   flush("Indenting region... Done.");
   
   widen();
   pop_spot();
   
}

define tcl_syntax_warning (msg)
{
   flush ("Warning: " + msg);
}

define tcl_insert_ket ()
{
   insert("}");
   tcl_indent_line();
   blink_match ();
}

define tcl_insert_bra ()
{
   if(Tcl_Check_Syntax) {
      push_spot();
      bskip_white();
      if(bolp()) {
	 tcl_syntax_warning("'{' shouldn't be on a separate line");
      }
      pop_spot();
   }
   insert("{");
}

define tcl_insert_comment ()
{
   if(Tcl_Check_Syntax) {
      push_spot();
      bskip_white();
      !if (bolp() or blooking_at (";")) {
	 tcl_syntax_warning("'#' should be on a separate line or after a ';'");
      }
      pop_spot();
   }	
   insert("#");
}

$1 = "TCL";
!if (keymap_p ($1))
  {
     make_keymap ($1);
     definekey("tcl_insert_bra", "{", $1);
     definekey("tcl_insert_ket", "}", $1);
     definekey("tcl_insert_comment", "#", $1);
     definekey_reserved("tcl_indent_region", "^Q", $1);
     definekey("indent_line", "\t", $1);
  }


%!%+
%\function{tcl_mode}
%\synopsis{tcl_mode}
%\description
% Protoytype: Void tcl_mode ();
% This is a mode that is dedicated to facilitate the editing of Tcl language files.
% See the source (tclmode.sl) for more info.
% Functions that affect this mode include:
%#v+
%  function:             default binding:
%  tcl_insert_bra             {
%  tcl_insert_ket             }
%  tcl_insert_comment         #
%  newline_and_indent         RETURN
%  indent_line                TAB
%  tcl_indent_region          Ctrl-C Ctrl-Q
%#v-
% Variables affecting indentation include:
%#v+
%  C_INDENT
%  C_CONTINUED_OFFSET
%#v-
% Hooks: \var{tcl_mode_hook}
%!%-
define tcl_mode ()
{
   variable mode = "TCL";
   set_mode(mode, 4);
   use_keymap(mode);
   use_syntax_table (mode);
   set_buffer_hook ("indent_hook", "tcl_indent_line");
   set_buffer_hook ("newline_indent_hook", "tcl_newline_and_indent");
   run_mode_hooks("tcl_mode_hook");
}
