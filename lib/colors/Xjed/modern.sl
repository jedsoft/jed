% modern.sl
% By Guido Gonzato <guido.gonzato@univr.it>

$1 = "black";
$2 = "white";
$3 = "#E8E8E8"; % light gray
$4 = "#B0B0B0"; % mid gray
$5 = "#007000"; % dark green
$6 = "#1010A0"; % dark blue
$7 = "#B40A78"; % dark purple
$8 = "#D717E7"; % purple
$9 = "#606060"; % dark gray

set_color ("normal", $1, $2);            % default fg/bg
set_color ("status", $1, $3);            % status line
set_color ("operator", $1, $2);          % +, -, etc...
set_color ("number", "brightred", $2);   % numbers
set_color ("comment", $9, $2);           % /* comment */
set_color ("region", $1, $4);            % region
set_color ("string", $6, $3);            % "string" or 'char'
set_color ("keyword", $7, $2);           % if, while, unsigned, ...
set_color ("keyword1", $6, $2);          % malloc, exit, etc...
set_color ("keyword2", $5, $2);          % other keywords
set_color ("delimiter", "red", $2);      % {}[](),.;...
set_color ("preprocess", "magenta", $2); % #ifdef etc...
set_color ("message", $6, $2);
set_color ("error", $8, $2);
set_color ("dollar", $8, $2);
set_color ("...", $8, $2);               % folding indicator

set_color ("menu_char", "red", $3);
set_color ("menu", $1, $3);
set_color ("menu_popup", $1, $3);
set_color ("menu_shadow", $1, $4);
set_color ("menu_selection", $1, "cyan");
set_color ("menu_selection_char", "red", $3);

set_color ("mouse", "blue", "blue");
set_color ("cursor", $1, $8);
set_color ("cursorovr", $1, $8);

%% The following have been automatically generated:
set_color("linenum", $1, $3);
set_color("trailing_whitespace", $9, $2);
set_color("tab", $9, $2);
set_color("url", $6, $3);
set_color("italic", $5, $2);
set_color("underline", $6, $2);
set_color("bold", $7, $2);
set_color("html", $7, $2);
set_color("keyword3", $1, $2);
set_color("keyword4", $1, $2);
set_color("keyword5", $1, $2);
set_color("keyword6", $1, $2);
set_color("keyword7", $1, $2);
set_color("keyword8", $1, $2);
set_color("keyword9", $1, $2);
