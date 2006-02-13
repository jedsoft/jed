$1 = "VERILOG";

create_syntax_table ($1);
define_syntax ("//","",'%', $1);
define_syntax ("([{", ")]}", '(', $1);
define_syntax ('"', '"', $1);
%define_syntax ('\'', '\'', $1);
define_syntax ('\$', '\\', $1);
define_syntax ('\`', '\\', $1);
define_syntax ("0-9a-zA-Z_", 'w', $1);        % words
define_syntax ("-+0-9a-FA-F.xXL", '0', $1);   % Numbers
define_syntax (",;.?:=<>", ',', $1);
define_syntax ('#', '#', $1);
define_syntax ("%-+/&*<>|!~^", '+', $1);
set_syntax_flags ($1, 8);

% Kikutani Makoto <kikutani@sprintmail.com> suggests:
() = define_keywords ($1, "IFINISOFTOifinisofto", 2);
() = define_keywords ($1, "ANDENDFORMAXMINOUTUSEandendformaxminoutreguse", 3);
() = define_keywords ($1, "CASEELSELOOPPORTTASKTHENWAITWIREcaseelseloopporttaskthenwaitwire", 4);
() = define_keywords ($1, "BEGINEVENTINOUTINPUTINOUTWHILEbegineventinoutinputwhile", 5);
() = define_keywords ($1, "ASSIGNBUFFERDOWNTOENTITYMODULEREPEATRETURNSIGNALASSIGNalwaysassignbufferdowntoentitymoduleoutputrepeatreturnsignal", 6);
() = define_keywords ($1, "DEFAULTENDCASEENDTASKFOREVERINITIALINTEGERNEGEDGEPOSEDGESPECIFYdefaultendcaseendtaskforeverinitialintegernegedgeposedgespecify", 7);
() = define_keywords ($1, "CONSTANTFUNCTIONconstantfunction", 8);
() = define_keywords ($1, "ENDMODULEPARAMETERSPECPARAMendmoduleparameterspecparam", 9);
() = define_keywords ($1, "ENDSPECIFYendspecify", 10);

define verilog_mode ()
{
   variable kmap = "VERILOG";

   set_mode(kmap, 0x28);
   use_syntax_table (kmap);
   run_mode_hooks("verilog_mode_hook");
}
