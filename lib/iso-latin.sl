%%
%%  Initializes upper/lowercase lookup tables for ISO Latin 1
%%  Information here derived from staring at a vt420
%%
if (_slang_utf8_ok == 0)
{
.   0  64 1 { dup define_case } _for
.  65  90 1 { dup 32 + define_case } _for
.  91  96 1 { dup define_case } _for
. 123 191 1 { dup define_case } _for
. 192 222 1 { dup 32 + define_case } _for

. 215 215 define_case
. 223 223 define_case
. 247 247 define_case
. 255 255 define_case

%. "0-9a-zA-Z\d191-\d214\d216-\d222\d224-\d246\d248-\d254" define_word
%. "0-9a-zA-Z\d191-\d214\d216-\d246\d248-\d255" define_word
. "0-9a-zA-Z\d192-\d214\d216-\d246\d248-\d255" define_word
}
