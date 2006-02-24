% -*- SLang -*-
%
% Display the key codes returned for a particular keystroke, including
% most Alt keys and Control-keypad keys.
%
% The appended key codes were determined from an IBM XT and are useful
% for defining keys.
%
% example:  define Alt-1 to goto top of buffer (bob):
%
% setkey ("bob",  "^@x");
% NB: the NUL character "^@" should be entered as '^' and '@' separately.

define keycode ()
{
   variable ch, key = "", timeout;
#ifdef XWINDOWS
   variable fmt = "Press any key %s: `%s' (keysym 0x%X)";
#else
   variable fmt = "Press any key %s: `%s'";
#endif
   
   variable to_exit="";
   variable last_was_return = 0;
   forever
     {
	timeout = 100;		%  5 second delay
#ifdef XWINDOWS
	flush (sprintf (fmt, key, to_exit, X_LAST_KEYSYM));
#else
	flush (sprintf (fmt, to_exit, key));
#endif

	key = "";
	while (input_pending (timeout))
	  {
	     timeout = 2;	       %  1/5 second
	     ch = getkey ();
	     switch (ch)
	       { case 0:	"^@"; }
	       { case 27:	"\\e"; }
#ifdef IBMPC_SYSTEM
	       { case 224:	"\\xE0"; }
#endif
	       {
		case '\r':
		  if (last_was_return)
		    {
		       message ("");
		       return;
		    }
		  char(ch);
	       }
	       {
		  % Eventually I should use \u{XXXX} for unicode
		  if (ch > 127) "\\d" + string (ch); else char (ch);
	       }
	     ch = ();
	     key += ch;
	  }
	last_was_return = (key == "\r");
	if (last_was_return) 
	  to_exit = "(RETURN to exit)";
	else
	  to_exit = "";
     }
}

%% Key codes returned for most Alt keys and Control-keypad keys
%% The ALT key definitions assume that the global variable ALT_CHAR is 0.

% Alt-1	^@x
% Alt-2	^@y
% Alt-3	^@z
% Alt-4	^@{
% Alt-5	^@|
% Alt-6	^@}
% Alt-7	^@~
% Alt-8	^@
% Alt-A	^@^^
% Alt-B	^@0
% Alt-C	^@.
% Alt-D	^@ SPACE
% Alt-E	^@^R
% Alt-F	^@!
% Alt-G	^@\"
% Alt-H	^@#
% Alt-I	^@^W
% Alt-J	^@$
% Alt-K	^@%
% Alt-L	^@&
% Alt-M	^@2
% Alt-N	^@1
% Alt-O	^@^X
% Alt-P	^@^Y
% Alt-Q	^@^P
% Alt-R	^@^S
% Alt-S	^@^_
% Alt-T	^@^T
% Alt-U	^@^V
% Alt-V	^@/
% Alt-W	^@^Q
% Alt-X	^@-
% Alt-Y	^@^U
% Alt-Z	^@,
%
% These are the 6 keys on the small keypad + 4 Arrow keys below
%   Key         No-modifier     Ctrl        Shift        Alt
% -------------------------------------------------------------
%  End            \xE0O        \xE0u        \xE0O     ^@\d159
%  Insert         \xE0R        \xE0\d146    \xE0R     ^@\d162
%  Home           \xE0G        \xE0w        \xE0G     ^@\d151
%  Delete         \xE0S        \xE0\d147    \xE0S     ^@\d163
%  PgUp           \xE0I        \xE0\d132    \xE0I     ^@\d153
%  PgDn           \xE0Q        \xE0v        \xE0Q     ^@\d161
%  Left           \xE0K        \xE0s        \xE0K     ^@\d155
%  Right          \xE0M        \xE0t        \xE0M     ^@\d157
%  Up             \xE0H        \xE0\d141    \xE0H     ^@\d152
%  Down           \xE0P        \xE0\d145    \xE0P     ^@\d160

% These keys are located on the numeric keypad
%   Key         No-modifier     Ctrl        Shift        Alt
% -------------------------------------------------------------
%  End           Oq          ^@u
%  Insert        Op          ^@\d146
%  Home          Ow          ^@w
%  Delete        On          ^@\d147
%  PgUP          Oy          ^@\d132
%  PgDn          Os          ^@v
%  Left          Ot          ^@s
%  Right         Ov          ^@t
%  Up            Ox          ^@\d141
%  Down          Or          ^@\d145
%  Five          Ou          ^@\d143
%  Slash         OQ          ^@\d149       OQ      ^@\d164
%  Star          OR          ^@\d150       OR      ^@7
%  Minus         OS          ^@\d142       OS      ^@J
%  Plus          Om          ^@\d144       Om      ^@N
%  Enter         OM                        OM      ^@\d166
%
%
%   Key         No-modifier     Ctrl        Shift        Alt
% -------------------------------------------------------------
%   F1:           ^@;           ^@^          ^@T         ^@h
%   F2:           ^@<           ^@_          ^@U         ^@i
%   F3:           ^@=           ^@`          ^@V         ^@j
%   F4:           ^@>           ^@a          ^@W         ^@k
%   F5:           ^@?           ^@b          ^@X         ^@l
%   F6:           ^@@           ^@c          ^@Y         ^@m
%   F7:           ^@A           ^@d          ^@Z         ^@n
%   F8:           ^@B           ^@e          ^@[         ^@o
%   F9:           ^@C           ^@f          ^@\         ^@p
%   F10:          ^@D           ^@g          ^@]         ^@q
%   F11:          ^@\d133       ^@\d137      ^@\d135     ^@\d139
%   F12:          ^@\d134       ^@\d136      ^@\d136     ^@\d140

