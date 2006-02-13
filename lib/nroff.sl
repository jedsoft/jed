% nroff.sl -*- SLang -*-
% 
% Primitive nroff editing mode -- just enough to define paragraphs
% to prevent <period>CMD from wrapping
% 

define nroff_parsep ()
{
   bol ();
   (looking_at_char('.') or looking_at_char('\\') or (skip_white(), eolp()));
} 

$1 = "nroff";
create_syntax_table ($1);

define_syntax ("'\\\"", "", '%', $1);	% Comment Syntax
define_syntax ('.', '\\', $1);		% Quote character
define_syntax ("({[", ")]}", '(', $1);	% are all these needed?
define_syntax ("{}[]<>()", ',', $1);	% delimiters
define_syntax (".a-zA-Z", 'w', $1);

% A few standard keywords -- uppercase only, nothing fancy
() = define_keywords ($1, ".B.I", 2);
() = define_keywords ($1, ".BR.DT.IP.PP.RB.RI.RE.RS.SB.SH.TH.TP.fi.nf", 3);


%!%+
%\function{nroff_mode}
%\synopsis{nroff_mode}
%\description
% Protoytype: Void nroff_mode ();
% A primitive mode for editing nroff/troff files.
% mostly to define the paragraph separator
%!%-
define nroff_mode ()
{
   variable nroff = "nroff";
   set_mode (nroff, 1);
   use_syntax_table (nroff);
   set_buffer_hook ("par_sep", "nroff_parsep");
   run_mode_hooks ("nroff_mode_hook");
}

