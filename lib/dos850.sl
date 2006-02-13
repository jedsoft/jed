%  Initializes upper/lowercase lookup tables for DOS code page 850.
%  850 is the default code page for Belgium, Brazil, Denmark,
%  Finland, France, Germany, Italy, Latin America, Netherlands,
%  Norway, Portugal, Spain, Sweden, and Switzerland.
%  It is the alternate code page for Canadian-French,
%  Czechoslovakia, Hungary, International English, Poland, United
%  Kingdom, USA, and (ex-)Yugoslavia
%  Information from MSDOS 5.0 manual
%  Thanks marko@cs.umd.edu
.   0  64 1 { dup define_case } _for
.  65  90 1 { dup 32 + define_case } _for
.  91  96 1 { dup define_case } _for
. 123 255 1 { dup define_case } _for
%
. 128 135 define_case
. 154 129 define_case
. 144 130 define_case
. 142 132 define_case
. 183 133 define_case
. 143 134 define_case
. 211 137 define_case
. 212 138 define_case
. 216 139 define_case
. 215 140 define_case
. 222 141 define_case
. 146 145 define_case
. 153 148 define_case
. 227 149 define_case
. 234 150 define_case
. 235 151 define_case
. 157 155 define_case
. 181 160 define_case
. 214 161 define_case
. 224 162 define_case
. 233 163 define_case
. 165 164 define_case
. 199 198 define_case
. 209 208 define_case
. 229 228 define_case
. 232 231 define_case
. 237 236 define_case

. "0-9a-zA-Z\d128-\d155\d157\d160-\d165\d181-\d183\d198\d199\d208\d212\d214-\d216\d222\d224-\d237"
. define_word
