%  Initializes upper/lowercase lookup tables for DOS code page 852.
%  852 is the default code page for Czech, ...
%  not complete! - Czech only (Petr Peringer <peringer@dcse.fee.vutbr.cz>)
%
.   0  64 1 { dup define_case } _for
.  65  90 1 { dup 32 + define_case } _for
.  91  96 1 { dup define_case } _for
. 123 255 1 { dup define_case } _for
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
. 0xB5 0xA0 define_case  % \'A   --> \'a
. 0xAC 0x9F define_case  % \v{C} --> etc. in TeX sequences
. 0xD2 0xD4 define_case  % \v{D}
. 0xB7 0xD8 define_case  % \v{E}
. 0x90 0x82 define_case  % \'E
. 0xD6 0xA1 define_case  % \'I  
. 0xD5 0xE5 define_case  % \v{N}
. 0xE0 0xA2 define_case  % \'O
. 0xFC 0xFD define_case  % \v{R}
. 0xE6 0xE7 define_case  % \v{S}
. 0x9B 0x9C define_case  % \v{T}
. 0xDE 0x85 define_case  % \r{U}
. 0xE9 0xA3 define_case  % \'U
. 0xED 0xEC define_case  % \'Y
. 0xA6 0xA7 define_case  % \v{Z}
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
. "0-9a-zA-Z_\xB5\xA0\xAC\x9F\xD2\xD4\xB7\xD8\x90\x82\xD6\xA1\xD5\xE5\xE0\xA2\xFC\xFD\xE6\xE7\x9B\x9C\xDE\x85\xE9\xA3\xED\xEC\xA6\xA7"
. define_word
