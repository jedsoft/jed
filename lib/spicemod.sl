% This is a simple spice mode.  It does not defined any form of indentation
% style.  Rather, it simply implements a highlighting scheme.

$1 = "SPICE";

create_syntax_table ($1);
define_syntax ("*","",'%', $1);
define_syntax ("#","",'%', $1);
define_syntax ("([{", ")]}", '(', $1);
define_syntax ('"', '"', $1);
define_syntax ('\'', '\'', $1);
define_syntax ('\\', '\\', $1);
define_syntax ("0-9a-zA-Z._", 'w', $1);        % words
define_syntax ("-+0-9F.xupXLPU", '0', $1);   % Numbers
define_syntax (",;?:=<>", ',', $1);
define_syntax ('X', '#', $1);
define_syntax ("%-+/&*<>|!~^", '+', $1);
set_syntax_flags ($1, 4);

() = define_keywords ($1, "LWlw", 1);
() = define_keywords ($1, "ADASCDPDPSacadascdpdps", 2);
() = define_keywords ($1, "ENDSDCONNMOSPMOSPOSTendsdconnmospmospost", 4);
() = define_keywords ($1, "BEGINbriefnomod", 5);
() = define_keywords ($1, "SUBCKTsubckt", 6);
() = define_keywords ($1, "INCLUDEOPTIONSincludeoptions", 7);

define spice_mode ()
{
   variable sp = "SPICE";
   set_mode(sp, 0);
   use_syntax_table (sp);
   run_mode_hooks("spice_mode_hook");
}
