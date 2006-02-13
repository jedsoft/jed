%%
%%  Initializes upper/lowercase lookup tables for ISO Latin 2
%%
.   0  64 1 { dup define_case } _for
.  65  90 1 { dup 32 + define_case } _for
.  91  96 1 { dup define_case } _for
. 123 160 1 { dup define_case } _for
. 161 177 define_case
. 162 162 define_case
. 163 179 define_case
. 164 164 define_case
. 165 166 1 { dup 16 + define_case } _for
. 167 168 1 { dup define_case } _for
. 169 172 1 { dup 16 + define_case } _for
. 173 173 define_case
. 174 175 1 { dup 16 + define_case } _for
. 176 176 define_case
. 178 178 define_case
. 180 180 define_case
. 183 183 define_case
. 184 184 define_case
. 189 189 define_case
. 192 222 1 { dup 32 + define_case } _for
. 215 215 define_case
. 223 223 define_case
. 247 247 define_case
. 255 255 define_case

define_word (strcat ("0-9a-zA-Z\d161\d163\d165-\d166\d169-\d172",
		      "\d174\d175\d177\d179\d181\d182\d185-\d188",
		      "\d190-\d214\d216-\d222\d224-\d246\d248-\d254"));
