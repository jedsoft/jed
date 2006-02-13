%  VHDL mode		-* SLang -*-

% This is a simple vhdl mode. First it implements a highlighting scheme.
%
% modified by Thei Wijnen 22-nov-2001: added: xor xnor with after alias select
% modified by Thei Wijnen 21-mar-2002: added: generate transport rising_edge falling_edge
% Modified by Thei Wijnen 10-Aug-2003: implemented indentation style and folding.
% 


% Loading this file, then executing 'vhdl_mode' will start
% VHDL mode on the current buffer.

custom_variable ("VHDL_Comment_String", "--");
custom_variable ("VHDL_Indent_Amount", 2);

%
% VHDL indent routine

define vhdl_indent ()
{
  variable goal = 1;	        % start in column 1.
  variable cs = CASE_SEARCH;
  variable ch;

  push_spot ();
  push_spot ();
  CASE_SEARCH = 0;	        % VHDL is not case sensitive

  while (up_1 ())
  {
    bol_skip_white();
    if (eolp() ) continue;

    goal = what_column ();
    if (goal == 1) continue;

    if (looking_at("do ")
        or looking_at("elsif ") or looking_at("elsif(")
        or looking_at("else") )
    {
      goal += VHDL_Indent_Amount;
    }
    else if (looking_at("if ") or looking_at("if("))
    {
      if (ffind("then")) { goal += VHDL_Indent_Amount; }
    }
    else if (looking_at("for") or looking_at("for("))
    {
      if (ffind("loop")) goal += VHDL_Indent_Amount;
    }
    else if (looking_at("when"))
    {
      if (ffind("=>")) goal += VHDL_Indent_Amount;
    }
    else if (looking_at("case"))
    {
      if (ffind("is")) goal += VHDL_Indent_Amount;
    }
    else if (looking_at("begin"))
    {
      goal += VHDL_Indent_Amount;
    }

%    else if (looking_at("begin"))
%    {
%      push_spot ();
%      while (up_1 ())
%      {
%        bol_skip_white();
%        if (eolp() ) continue;
%        if (looking_at("variable ") or looking_at("process") )
%        {
%          goal -= VHDL_Indent_Amount;
%        }
%        break;
%      }
%      pop_spot ();
%    }

    break;
  }
  pop_spot ();

  % now check current line

  bol_skip_white();
%  if (looking_at("begin")) 
%    goal += VHDL_Indent_Amount;

  if (looking_at("elsif")
    or looking_at("else")
    or looking_at("end")
    or looking_at("when")
%    or looking_at("end if")
%    or looking_at("end process")
%    or looking_at("end loop")
      )
  {
    goal -= VHDL_Indent_Amount;
  }

  CASE_SEARCH = cs;		% done getting indent

  bol_skip_white ();

  ch = char(what_char());
  switch (ch)
%*     {
%*	isdigit (ch) :		% label
%*
%*	if (what_column () >= 6)
%*	  {
%*	     bol_trim ();
%*	     insert_single_space ();
%*	  }
%*	X_USER_BLOCK1 ();
%*     }
  {
    not (bolp()) or eolp ():	% general case
    bol_trim ();
     goal--;
    insert_spaces (goal);
  }
  pop_spot ();
  skip_white ();
}


define vhdl_newline ()
{
  variable p, cont;

  if (bolp ())
  {
    newline ();
    return;
  }

  vhdl_indent ();
  push_spot ();
  bskip_white (); trim ();
  pop_spot ();

  newline ();
  insert_single_space ();
  vhdl_indent ();
}


%
% Look for beginning of current subroutine/function

define vhdl_beg_of_subprogram ()
{
  variable cs = CASE_SEARCH;

  CASE_SEARCH = 0;
  do
  {
    bol_skip_white ();
    if (_get_point ())
    {
      if (looking_at ("process")
        or looking_at ("switch")) break;
    }
  }
  while (up_1 ());
  CASE_SEARCH = cs;
}

%
% Look for end of current subroutine/function

define vhdl_end_of_subprogram ()
{
  variable cs = CASE_SEARCH;
  CASE_SEARCH = 0;

  do
  {
    bol_skip_white ();
    if (looking_at ("end;")
      or looking_at ("end process")) break;
%	  {
%	     go_right (3);
%	     skip_white ();
%	     if (eolp ()) break;
%	  }
  }
  while (down_1 ());
  CASE_SEARCH = cs;
}

define vhdl_mark_subprogram ()
{
  vhdl_end_of_subprogram ();
  go_down_1 ();
  set_mark_cmd ();
  vhdl_beg_of_subprogram ();
  bol ();
}


%
% main entry point into the VHDL mode
%
% Set up syntax table

$1 = "VHDL";

create_syntax_table ($1);
define_syntax ("--","",'%', $1);
define_syntax ("([{", ")]}", '(', $1);
define_syntax ('"', '"', $1);
%define_syntax ('\'', '\'', $1);
define_syntax ('\'', '\\', $1);
define_syntax ("0-9a-zA-Z_", 'w', $1);        % words
define_syntax ("-+0-9a-FA-F.xXL", '0', $1);   % Numbers
define_syntax (",;.?:=<>", ',', $1);
define_syntax ('#', '#', $1);
define_syntax ("%-+/&*<>|!~^", '+', $1);
set_syntax_flags ($1, 8);

% Type 0 keywords

() = define_keywords ($1, "IFINISOFORTOifinisoforto", 2);
() = define_keywords ($1, "ANDENDFORMAXMINNOTOUTRUNUSEXORandendformaxminnotoutrunusexor", 3);
() = define_keywords ($1, "CASEELSELOOPPORTTHENTYPEWAITWHENWITHXNORcaseelseloopportthentypewaitwhenwithxnor", 4);
() = define_keywords ($1, "AFTERALIASBEGINELSIFRANGETRACEUNTILafteraliasbeginelsifrangetraceuntil", 5);
() = define_keywords ($1, "ASSIGNBUFFERDOWNTOENTITYOTHERSRETURNSELECTSIGNALassignbufferdowntoentityothersreturnselectsignal", 6);
() = define_keywords ($1, "GENERICINTEGERLIBRARYPACKAGEPROCESSSUBTYPEgenericintegerlibrarypackageprocesssubtype", 7);
() = define_keywords ($1, "CONSTANTFUNCTIONGENERATEVARIABLEconstantfunctiongeneratevariable", 8);
() = define_keywords ($1, "COMPONENTTRANSPORTcomponenttransport", 9);
%() = define_keywords ($1, "RISING_EDGErising_edge", 11);
%() = define_keywords ($1, "ARCHITECTUREFALLING_EDGEarchitecturefalling_edge", 12);
() = define_keywords ($1, "ARCHITECTUREarchitecture", 12);
() = define_keywords ($1, "CONFIGURATIONconfiguration", 13);

% Type 1 keywords - use for operator like keywords
() = define_keywords_n ($1, "eqgegtleltneor", 2, 1);
() = define_keywords_n ($1, "andnot", 3, 1);
%() = define_keywords_n ($1, "true", 4, 1);
%() = define_keywords_n ($1, "false", 5, 1);
() = define_keywords_n ($1, "RISING_EDGErising_edge", 11, 1);
() = define_keywords_n ($1, "FALLING_EDGEfalling_edge", 12, 1);

% Set up syntax table

$1 = "VHDL";
!if (keymap_p ($1)) make_keymap ($1);

definekey ("vhdl_beg_of_subprogram",	"\e^A",	$1);
definekey ("vhdl_end_of_subprogram",	"\e^E",	$1);
definekey ("vhdl_mark_subprogram",	"\e^H", $1);

%!%+
%\function{vhdl_mode}
%\synopsis{vhdl_mode}
%\description
% Mode designed for the purpose of editing VHDL files.
% After the mode is loaded, the hook 'vhdl_hook' is called.
% Useful functions include
% 
%  Function:                    Default Binding:
%   vhdl_beg_of_subprogram         ESC ^A
%        moves cursor to beginning of current function/process
%   vhdl_end_of_subprogram         ESC ^E
%        moves cursor to end of current function/process
%   vhdl_mark_subprogram           ESC ^H
%        mark the current function/process
% 
% Variables include:
%   VHDL_Comment_String : string used by 'vhdl_comment' to
%                         comment out a line.  Default is "--".
%   VHDL_Indent_Amount  : number of spaces to indent statements in
%                         a block.  The default is 2.
%!%-

define vhdl_mode ()
{
  variable mode = "VHDL";
  set_mode (mode, 0x4 | 0x10);
  use_keymap (mode);
  use_syntax_table (mode);
  set_buffer_hook ("indent_hook", "vhdl_indent");
  set_buffer_hook ("newline_indent_hook", "vhdl_newline");
  mode_set_mode_info (mode, "fold_info", "--{{{\r--}}}\r\r");
  run_mode_hooks ("vhdl_mode_hook");
}

