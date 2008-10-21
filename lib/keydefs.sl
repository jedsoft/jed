% -*- mode: slang; mode: fold; -*-
% This file defines symbolic constants for many function and arrow keys.
% It may need some modifications on VMS as well as other systems.
% originally part of ide.sl by Guido Gonzato
% modified by GM <g.milde@physik.tu-dresden.de>
% modified by JED

#ifdef IBMPC_SYSTEM %{{{
variable Key_F1		= "^@;";
variable Key_F2		= "^@<";
variable Key_F3		= "^@=";
variable Key_F4		= "^@>";
variable Key_F5		= "^@?";
variable Key_F6		= "^@@";
variable Key_F7		= "^@A";
variable Key_F8		= "^@B";
variable Key_F9		= "^@C";
variable Key_F10	= "^@D";
variable Key_F11	= "^@\d133";
variable Key_F12	= "^@\d134";

variable Key_Up		= "\xE0H";
variable Key_Down	= "\xE0P";
variable Key_Right	= "\xE0M";
variable Key_Left	= "\xE0K";

variable Key_Ins	= "\xE0R";
variable Key_Del	= "\xE0S";
variable Key_Home	= "\xE0G";
variable Key_End	= "\xE0O";
variable Key_PgUp	= "\xE0I";
variable Key_PgDn	= "\xE0Q";

variable Key_BS		= _Backspace_Key;

% ALT keys

variable Key_Alt_F1	= "^@h";
variable Key_Alt_F2	= "^@i";
variable Key_Alt_F3	= "^@j";
variable Key_Alt_F4	= "^@k";
variable Key_Alt_F5	= "^@l";
variable Key_Alt_F6	= "^@m";
variable Key_Alt_F7	= "^@n";
variable Key_Alt_F8	= "^@o";
variable Key_Alt_F9	= "^@p";
variable Key_Alt_F10	= "^@q";
variable Key_Alt_F11	= "^@\d139";
variable Key_Alt_F12	= "^@\d140";

variable Key_Alt_Up	= "^@\d152";
variable Key_Alt_Down	= "^@\d160";
variable Key_Alt_Right	= "^@\d157";
variable Key_Alt_Left	= "^@\d155";

variable Key_Alt_Ins	= "^@\d162";
variable Key_Alt_Del	= "^@\d163";
variable Key_Alt_Home	= "^@\d151";
variable Key_Alt_End	= "^@\d159";
variable Key_Alt_PgUp	= "^@\d153";
variable Key_Alt_PgDn	= "^@\d161";

variable Key_Alt_BS	= strcat("\e", Key_BS);

% SHIFT keys

variable Key_Shift_F1	= "^@T";
variable Key_Shift_F2	= "^@U";
variable Key_Shift_F3	= "^@V";
variable Key_Shift_F4	= "^@W";
variable Key_Shift_F5	= "^@X";
variable Key_Shift_F6	= "^@Y";
variable Key_Shift_F7	= "^@Z";
variable Key_Shift_F8	= "^@[";
variable Key_Shift_F9	= "^@\\";
variable Key_Shift_F10	= "^@]";
variable Key_Shift_F11  = "^@\d135";
variable Key_Shift_F12  = "^@\d136";

variable Key_Shift_Up	= "\xE01";
variable Key_Shift_Down	= "\xE06";
variable Key_Shift_Right= "\xE04";
variable Key_Shift_Left	= "\xE03";

variable Key_Shift_Ins	= "\xE08";
variable Key_Shift_Del	= "\xE09";
variable Key_Shift_Home	= "\xE00";
variable Key_Shift_End	= "\xE05";
variable Key_Shift_PgUp	= "\xE02";
variable Key_Shift_PgDn	= "\xE07";
variable Key_Shift_Tab  = "^@^O";
variable Key_Shift_BS	= "\x08"; %  ??

% Ctrl keys

variable Key_Ctrl_F1	= "^@^";
variable Key_Ctrl_F2	= "^@_";
variable Key_Ctrl_F3	= "^@`";
variable Key_Ctrl_F4	= "^@a";
variable Key_Ctrl_F5	= "^@b";
variable Key_Ctrl_F6	= "^@c";
variable Key_Ctrl_F7	= "^@d";
variable Key_Ctrl_F8	= "^@e";
variable Key_Ctrl_F9	= "^@f";
variable Key_Ctrl_F10	= "^@g";
variable Key_Ctrl_F11   = "^@\d137";
variable Key_Ctrl_F12   = "^@\d138";

variable Key_Ctrl_Up	= "\xE0\d141";
variable Key_Ctrl_Down	= "\xE0\d145";
variable Key_Ctrl_Right	= "\xE0t";
variable Key_Ctrl_Left	= "\xE0s";

variable Key_Ctrl_Ins	= "\xE0\d146";
variable Key_Ctrl_Del	= "\xE0\d147";
variable Key_Ctrl_Home	= "\xE0w";
variable Key_Ctrl_End	= "\xE0u";
variable Key_Ctrl_PgUp	= "\xE0\d132";
variable Key_Ctrl_PgDn	= "\xE0v";

variable Key_Ctrl_BS	= "\e@"; %  ??

%}}}
#else		%  UNIX, VMS %{{{

private variable Is_Xjed = is_defined ("x_server_vendor");

private define setkey_via_terminfo (tc, def)
{
   if (Is_Xjed)
     return def;

#ifexists get_termcap_string
   variable s = get_termcap_string (tc);
   if (s != "")
     return s;
#endif
   return def;
}

variable Key_F1		= setkey_via_terminfo ("k1",	"\e[11~");
variable Key_F2		= setkey_via_terminfo ("k2",	"\e[12~");
variable Key_F3		= setkey_via_terminfo ("k3",	"\e[13~");
variable Key_F4		= setkey_via_terminfo ("k4",	"\e[14~");
variable Key_F5		= setkey_via_terminfo ("k5",	"\e[15~");
variable Key_F6		= setkey_via_terminfo ("k6",	"\e[17~");
variable Key_F7		= setkey_via_terminfo ("k7",	"\e[18~");
variable Key_F8		= setkey_via_terminfo ("k8",	"\e[19~");
variable Key_F9		= setkey_via_terminfo ("k9",	"\e[20~");
variable Key_F10	= setkey_via_terminfo ("k;",	"\e[21~");
variable Key_F11	= setkey_via_terminfo ("F1",	"\e[23~");
variable Key_F12	= setkey_via_terminfo ("F2",	"\e[24~");

variable Key_Up		= setkey_via_terminfo ("ku", "\e[A");
variable Key_Down	= setkey_via_terminfo ("kd", "\e[B");
variable Key_Right	= setkey_via_terminfo ("kr", "\e[C");
variable Key_Left	= setkey_via_terminfo ("kl", "\e[D");

variable Key_Ins	= setkey_via_terminfo ("kI", "\e[2~");
variable Key_Del	= setkey_via_terminfo ("kD", "\e[3~");
variable Key_Home	= setkey_via_terminfo ("kh", "\e[1~");
variable Key_End	= setkey_via_terminfo ("@7", "\e[4~");
variable Key_PgUp	= setkey_via_terminfo ("kP", "\e[5~");
variable Key_PgDn	= setkey_via_terminfo ("kN", "\e[6~");

variable Key_BS		= _Backspace_Key;

% Assume that ALT keys are prefixed with ESC

variable Key_Alt_F1	= strcat("\e", Key_F1);
variable Key_Alt_F2	= strcat("\e", Key_F2);
variable Key_Alt_F3	= strcat("\e", Key_F3);
variable Key_Alt_F4	= strcat("\e", Key_F4);
variable Key_Alt_F5	= strcat("\e", Key_F5);
variable Key_Alt_F6	= strcat("\e", Key_F6);
variable Key_Alt_F7	= strcat("\e", Key_F7);
variable Key_Alt_F8	= strcat("\e", Key_F8);
variable Key_Alt_F9	= strcat("\e", Key_F9);
variable Key_Alt_F10	= strcat("\e", Key_F10);
variable Key_Alt_F11	= strcat("\e", Key_F11);
variable Key_Alt_F12	= strcat("\e", Key_F12);

variable Key_Alt_Up	= strcat("\e", Key_Up);
variable Key_Alt_Down	= strcat("\e", Key_Down);
variable Key_Alt_Right	= strcat("\e", Key_Right);
variable Key_Alt_Left	= strcat("\e", Key_Left);

variable Key_Alt_Ins	= strcat("\e", Key_Ins);
variable Key_Alt_Del	= strcat("\e", Key_Del);
variable Key_Alt_Home	= strcat("\e", Key_Home);
variable Key_Alt_End	= strcat("\e", Key_End);
variable Key_Alt_PgUp	= strcat("\e", Key_PgUp);
variable Key_Alt_PgDn	= strcat("\e", Key_PgDn);

variable Key_Alt_BS	= strcat("\e", Key_BS);

% SHIFT keys.  Do not depend upon these being available.  I cannot find
% any relevant terminfo entries for most of them.  The default values are
% appropriate for Xjed.
variable Key_Shift_F1	= setkey_via_terminfo ("", "\e[11$");
variable Key_Shift_F2	= setkey_via_terminfo ("", "\e[12$");
variable Key_Shift_F3	= setkey_via_terminfo ("", "\e[13$");
variable Key_Shift_F4	= setkey_via_terminfo ("", "\e[14$");
variable Key_Shift_F5	= setkey_via_terminfo ("", "\e[15$");
variable Key_Shift_F6	= setkey_via_terminfo ("", "\e[17$");
variable Key_Shift_F7	= setkey_via_terminfo ("", "\e[18$");
variable Key_Shift_F8	= setkey_via_terminfo ("", "\e[19$");
variable Key_Shift_F9	= setkey_via_terminfo ("", "\e[20$");
variable Key_Shift_F10	= setkey_via_terminfo ("", "\e[21$");
variable Key_Shift_F11  = setkey_via_terminfo ("", "\e[23$");
variable Key_Shift_F12  = setkey_via_terminfo ("", "\e[24$");

variable Key_Shift_Up	= setkey_via_terminfo ("", "\e[a");
variable Key_Shift_Down	= setkey_via_terminfo ("", "\e[b");
variable Key_Shift_Right= setkey_via_terminfo ("%i", "\e[c");
variable Key_Shift_Left	= setkey_via_terminfo ("#4", "\e[d");

variable Key_Shift_Ins	= setkey_via_terminfo ("#3", "\e[2$");
if (Key_Shift_Ins == "\e2$")
{
   % Work-around rxvt-terminfo bug
   $1 = getenv ("TERM");
   if ($1 != NULL)
     {
	if (is_substr ($1, "rxvt") || is_substr ($1, "screen"))
	  Key_Shift_Ins = "\e[2$";
     }
}

variable Key_Shift_Del	= setkey_via_terminfo ("*4", "\e[3$");
variable Key_Shift_Home	= setkey_via_terminfo ("#2", "\e[1$");
variable Key_Shift_End	= setkey_via_terminfo ("*7", "\e[4$");
variable Key_Shift_PgUp	= setkey_via_terminfo ("", "\e[5$");
variable Key_Shift_PgDn	= setkey_via_terminfo ("", "\e[6$");

variable Key_Shift_Tab  = setkey_via_terminfo ("bt", "\e[Z");  % reverse-tab
variable Key_Shift_BS	= setkey_via_terminfo ("", "\e[16$");

% Ctrl keys

variable Key_Ctrl_F1	= setkey_via_terminfo ("", "\e[11^");
variable Key_Ctrl_F2	= setkey_via_terminfo ("", "\e[12^");
variable Key_Ctrl_F3	= setkey_via_terminfo ("", "\e[13^");
variable Key_Ctrl_F4	= setkey_via_terminfo ("", "\e[14^");
variable Key_Ctrl_F5	= setkey_via_terminfo ("", "\e[15^");
variable Key_Ctrl_F6	= setkey_via_terminfo ("", "\e[17^");
variable Key_Ctrl_F7	= setkey_via_terminfo ("", "\e[18^");
variable Key_Ctrl_F8	= setkey_via_terminfo ("", "\e[19^");
variable Key_Ctrl_F9	= setkey_via_terminfo ("", "\e[20^");
variable Key_Ctrl_F10	= setkey_via_terminfo ("", "\e[21^");
variable Key_Ctrl_F11	= setkey_via_terminfo ("", "\e[23^");
variable Key_Ctrl_F12	= setkey_via_terminfo ("", "\e[24^");

variable Key_Ctrl_Up	= setkey_via_terminfo ("", "\e[^A");
variable Key_Ctrl_Down	= setkey_via_terminfo ("", "\e[^B");
variable Key_Ctrl_Right	= setkey_via_terminfo ("", "\e[^C");
variable Key_Ctrl_Left	= setkey_via_terminfo ("", "\e[^D");

variable Key_Ctrl_Ins	= setkey_via_terminfo ("", "\e[2^");
variable Key_Ctrl_Del	= setkey_via_terminfo ("", "\e[3^");
variable Key_Ctrl_Home	= setkey_via_terminfo ("", "\e[1^");
variable Key_Ctrl_End	= setkey_via_terminfo ("", "\e[4^");
variable Key_Ctrl_PgUp	= setkey_via_terminfo ("", "\e[5^");
variable Key_Ctrl_PgDn	= setkey_via_terminfo ("", "\e[6^");

%variable Key_Ctrl_Tab	= setkey_via_terminfo ("", "\e[\t^");
variable Key_Ctrl_BS	= setkey_via_terminfo ("", "\e[16^" );

% We no longer need this
private define setkey_via_terminfo ();

%}}}
#endif % not IBMPC_SYSTEM

provide ("keydefs");
