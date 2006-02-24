%
%  Special file for Linux systems.
%

% This file sets up the console keys pgup/dn, etc.  
% To use it, simply rename it to defaults.sl.

% The directory where info files are kept.  Modify this appropriately.  See
% 'install.all' for a discussion of this.

%Info_Directory = ???


$1 = getenv ("TERM");
if ($1 == NULL) $1 = "";
if (is_list_element ("linux,console,con80x25,con80x28", $1, ','))
{
   USE_ANSI_COLORS = 1;   % uncomment if your console is a color one!
   OUTPUT_RATE = 0;
   TERM_CANNOT_SCROLL = -1;   % Truth is, linux console does not scroll well.
   setkey("bol",      		"\e[1~");	% home
   setkey("toggle_overwrite",	"\e[2~");       % insert
   setkey("delete_char_cmd",	"\e[3~");       % delete
   setkey("eol",		"\e[4~");       % end
   setkey("page_up",		"\e[5~");	
   setkey("page_down",		"\e[6~");
}
