%%
%%  Initializes upper/lowercase lookup tables for ISO Latin 3,
%%  defined in the ISO 8859-3 standard.
%%  Made by Byrial Jensen <byrial@post3.tele.dk>
%%

.   0  64 1 { dup define_case } _for
.  65  90 1 { dup 32 + define_case } _for
.  91  96 1 { dup define_case } _for
. 123 160 1 { dup define_case } _for

. 161 177 define_case
. 162 165 1 { dup dup define_case 16 + dup define_case } _for
. 166 182 define_case
. 167 168 1 { dup dup define_case 16 + dup define_case } _for
. 169 172 1 { dup 16 + define_case } _for
. 173 174 1 { dup dup define_case 16 + dup define_case } _for
. 175 191 define_case

. 192 222 1 { dup 32 + define_case } _for
% That was too much - redefine 3 non-letter holes in the range 192-222
. 195 195 define_case 227 227 define_case
. 208 208 define_case 240 240 define_case
. 215 215 define_case 247 247 define_case

% character 223 is german sharp s: it cannot be treated correctly
. 223 223 define_case 255 255 define_case

% And now the definition of a word.
% I have included all ciphers, letters, hyphen, and soft hyphen (char. 173).
define_word (strcat ("-0-9A-Za-z", "\d161\d177", "\d166\d182",
		      "\d169-\d173\d185-\d188", "\d175\d191",
		      "-\d194", "\d196-\d207", "\d209-\d214",
		      "\d216-\d226","\d228-\d239","\d241-\d246",
		      "\d248-\d254"));
