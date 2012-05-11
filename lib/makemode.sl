% -------------------------------------------- -*- mode:SLang; mode:folding -*-
%
% MAKE MODE FOR JED
%
%  Copyright (c) 2000  Gerell, Francesc Rocher
%  Released under the terms of the GNU General Public License (ver. 2 or later)
%
%  2005-11-11 define_highlight_rule -> dfa_define_highlight_rule  
%             (implemented dpatch by Rafael Laboissiere)
%  2009-02-16 documentation update.
%  2010-11-19 Modifications for integration into jed
%
% $Id: make.sl,v 1.14 2000/12/30 01:04:50 rocher Exp $
%
% --------------------------------------------------------------------  %{{{
%
% DESCRIPTION
%	A very simple mode to write 'Makefile' files.
%
% USAGE
%       If you put a line like
%
%		# -*- make -*-
%
%	near the beginning of your Makefile, make_mode will be used
%	for the mode of the buffer.  Alternatively, you may put
%	something like the following in your jedrc configuration file:
%       		
%       % set modes based on filename or more complicated patterns
%       private define set_modes_hook(base, ext)
%       {
%          if ((base == "Makefile") || (base == "GNUmakefile")
%              || (ext == ".mak"))
%            {
%               make_mode ();
%               return 1;
%            }
%          % Other tests here
%          %  ...
%          return0;
%       }
%       list_append (Mode_Hook_Pointer_List, &set_modes_hook);
%
% AUTHOR
%	Francesc Rocher (f.rocher@computer.org)
%       Feel free to send comments, suggestions or improvements.
%
% ------------------------------------------------------------------------ %}}}

private define is_comment_line      ()  %{{{
{
   push_spot_bol ();
   skip_white ();
   variable col = 0;
   if (what_char () == '#')
      col = what_column ();
   pop_spot ();
   return col;
}

%}}}
private define in_comment           ()  %{{{
{
   push_spot ();
   variable col = 0;
   if (bfind_char ('#'))
      col = what_column ();
   pop_spot ();
   return col;
}

%}}}
private define is_continuation_line ()  %{{{
{
   push_spot ();
   variable col = 0;
   if (up (1))
     {
        eol ();
        bskip_white ();
        ifnot (bolp ())
          {
             go_left (1);
             if (what_char () == '\\')
               {
                  bol_skip_white ();
                  col = what_column ();
               }
          }
     }
   pop_spot ();
   return col;
}

%}}}
private define is_rule_head         ()  %{{{
{
   push_spot_bol ();
   variable r = 0;
   while (ffind_char (':'))
     {
        r = 1;
        go_right (1);
     }
   go_left (1);
   r = (r && (in_comment () == 0) && (looking_at (":=") == 0));
   pop_spot ();
   return r;
}

%}}}
private define is_rule_body         ();
private define is_rule_body         ()  %{{{
{
   if (is_comment_line ())
      return 0;

   if (is_rule_head ())
     return 1;

   push_spot ();
   variable r = up(1) && not bolp() && is_rule_body ();
   pop_spot ();

   return r;
}

%}}}
public  define make_indent_line     ()  %{{{
{
   variable col = is_continuation_line ();
   if (col)
     {
        push_spot ();
        bol_skip_white ();
        if (what_column () < col)
          {
             bol_trim ();
             insert_char ('\t');
             while (what_column () < col-TAB+1)
	       insert_char ('\t');
             whitespace (col - what_column ());
          }
        else
          {
             while (what_column () > col)
	       call ("backward_delete_char_untabify");
          }
        pop_spot ();
        if (what_column () < col)
           skip_white ();
        return;
     }

   if (in_comment ())
     {
        % insert_char ('\t');   % This is a possibility ...
        return;
     }

   if (is_rule_head ())
     {
        push_spot_bol ();
        trim ();
        pop_spot ();
        return;
     }

   if (is_rule_body ())
     {
        push_spot_bol ();
        if (what_char () != '\t')
          {
             trim ();
             insert_char ('\t');
          }
        pop_spot ();
        if (bolp ())
           go_right (1);

        return;
     }
}

%}}}
public  define make_newline         ()  %{{{
{
   variable col = is_comment_line ();
   if (col)
     {
        insert_char ('\n');
        whitespace (col-1);
        insert ("# ");
        return;
     }

   newline ();
   make_indent_line ();
}

%}}}

% Syntax highlighting                   %{{{

$0 = "make";
create_syntax_table ($0);
define_syntax ("#", "", '%', $0);
define_syntax ('"', '"', $0);
define_syntax ('\'', '\'', $0);
define_syntax ("(", ")", '(', $0);
define_syntax ("0-9a-zA-Z_", 'w', $0);

#ifdef HAS_DFA_SYNTAX
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_define_highlight_rule("\"[^\"]*\"", "string", name);
   dfa_define_highlight_rule("'[^']*'", "string", name);
   %dfa_define_highlight_rule("\"([^\"\\\\]|\\\\.)*\"", "string", name);
   %dfa_define_highlight_rule("\"([^\"\\\\]|\\\\.)*\\\\?$", "string", name);
   %dfa_define_highlight_rule("'([^'\\\\]|\\\\.)*'", "Qstring", name);
   %dfa_define_highlight_rule("'([^'\\\\]|\\\\.)*\\\\?$", "string", name);
   dfa_define_highlight_rule ("^[ \t]*@", "string", name);
   dfa_define_highlight_rule ("[ \t]*\\\\[ \t]*$", "string", name);
   dfa_define_highlight_rule ("[ \t]*#.*$", "comment", name);
   dfa_define_highlight_rule ("[A-Za-z_][A-Za-z_0-9]*", "Knormal", name);
   %dfa_define_highlight_rule ("[ \t]*[A-Za-z_][A-Za-z_0-9]*", "Knormal", name);
   %dfa_define_highlight_rule ("^[ \t]*[A-Za-z_][A-Za-z_0-9]*", "Knormal", name);
   dfa_define_highlight_rule ("^[^\"']*\\:$", "keyword1", name);
   dfa_define_highlight_rule ("^[^\"']*\\:[ \t]+", "keyword1", name);
   %dfa_define_highlight_rule ("[ \t]*\.PHONY.*", "keyword1", name);
   dfa_define_highlight_rule ("/include", "normal", name);
   dfa_build_highlight_table (name);
}
dfa_set_init_callback (&setup_dfa_callback, $0);
%%% DFA_CACHE_END %%%
#endif

() = define_keywords_n ($0, "ARASCCCOCPFCPCRMfiif", 2, 0);
() = define_keywords_n ($0, "CPPCXXGETLEXTEX", 3, 0);
() = define_keywords_n ($0, "YACCelseifeq", 4, 0);
() = define_keywords_n ($0, "PHONYWEAVEYACCRendefendififdefifneqvpath", 5, 0);
() = define_keywords_n ($0, "CFLAGSCWEAVEFFLAGSGFLAGSIGNORELFLAGSPFLAGSRFLAGSSILENTTANGLEYFLAGSdefineexportifndef", 6, 0);
() = define_keywords_n ($0, "ARFLAGSASFLAGSCOFLAGSCTANGLEDEFAULTLDFLAGSinclude", 7, 0);
() = define_keywords_n ($0, "CPPFLAGSCXXFLAGSMAKEINFOPRECIOUSSUFFIXESTEXI2DVIoverrideunexport", 8, 0);
() = define_keywords_n ($0, "SECONDARY", 9, 0);
() = define_keywords_n ($0, "INTERMEDIATE", 12, 0);
() = define_keywords_n ($0, "EXPORT_ALL_VARIABLES", 20, 0);

set_syntax_flags ($0, 0x10|0x80);

%}}}

public define make_mode            ()  %{{{
{
   variable mode = "make";
   ifnot (keymap_p (mode))
     {
        make_keymap (mode);
        definekey ("make_indent_line", "\t", mode);
        definekey ("make_newline",     "\r", mode);
     }
   set_mode (mode, 4);
   use_keymap (mode);
   use_syntax_table (mode);
   run_mode_hooks ("make_mode_hook");
}

%}}}
