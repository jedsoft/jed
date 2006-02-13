%  Initializes upper/lowercase lookup tables for Windows 1250 code page.
%  Derived from dos852.sl by the code conversion utility. 
%  852 is the default code page for Czech in DOS, 1250 is the equivalent
%  for Windows (ask Microsoft why the encoding is different).
%  not complete! - Czech only (Petr Peringer <peringer@dcse.fee.vutbr.cz>)
%
.   0  64 1 { dup define_case } _for
.  65  90 1 { dup 32 + define_case } _for
.  91  96 1 { dup define_case } _for
. 123 255 1 { dup define_case } _for
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
. 0xC1 0xE1 define_case  % \'A   --> \'a
. 0xC8 0xE8 define_case  % \v{C} --> etc. in TeX sequences
. 0xCF 0xEF define_case  % \v{D}
. 0xCC 0xEC define_case  % \v{E}
. 0xC9 0xE9 define_case  % \'E
. 0xCD 0xED define_case  % \'I  
. 0xD2 0xF2 define_case  % \v{N}
. 0xD3 0xF3 define_case  % \'O
. 0xD8 0xF8 define_case  % \v{R}
. 0x8A 0x9A define_case  % \v{S}
. 0x8D 0x9D define_case  % \v{T}
. 0xD9 0xF9 define_case  % \r{U}
. 0xDA 0xFA define_case  % \'U
. 0xDD 0xFD define_case  % \'Y
. 0x8E 0x9E define_case  % \v{Z}
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
. "0-9a-zA-Z_\xC1\xE1\xC8\xE8\xCF\xEF\xCC\xEC\xC9\xE9\xCD\xED\xD2\xF2\xD3\xF3\xD8\xF8\x8A\x9A\x8D\x9D\xD9\xF9\xDA\xFA\xDD\xFD\x8E\x9E"
. define_word
