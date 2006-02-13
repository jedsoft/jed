#ifdef IBMPC_SYSTEM
%  Initializes upper/lowercase lookup tables for DOS code page 437.
%  437 is the original DOS character table. So if your DOS version
%  does not support code pages, use this table. 437 is also the 
%  default code page for USA, United Kingdom, and International English.
%  It is the alternate code page for Belgium, Brazil, Finland, France,
%  Germany, Italy, Latin America, Netherlands, Spain, Sweden, and 
%  Switzerland. 
%  Information from MSDOS 5.0 manual
%
.   0  64 1 { dup define_case } _for
.  65  90 1 { dup 32 + define_case } _for
.  91  96 1 { dup define_case } _for
. 123 255 1 { dup define_case } _for
. 128 135 define_case
. 142 132 define_case
. 143 134 define_case
. 144 130 define_case
. 146 145 define_case
. 153 148 define_case
. 154 129 define_case
. 165 164 define_case
%
% Now define a word.  This IS NOT based on above
%
. "0-9a-zA-Z\d128-\d167\d224-\d235" define_word
% Marko thinks:
. "0-9a-zA-Z\d128-\d154\d160-\d165\d224-\d235" define_word

#endif


