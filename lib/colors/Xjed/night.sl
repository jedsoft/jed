% night.sl
% By Guido Gonzato <guido.gonzato@univr.it>

$1 = "#EDFF00"; % flash yellow
$2 = "#000050"; % very dark blue
$3 = "#FF4242"; % bright red
$4 = "#90FF90"; % bright green
$5 = "#4FFF4F"; % green
$6 = "#D1D1D1"; % whitish
$7 = "#040FDD"; % blue
$8 = "#400070"; % dark purple
$9 = "#20b0f0"; % mid cyan

set_color ("normal", $1, $2);                 % default fg/bg
set_color ("status", "black", "white");       % status or mode line
set_color ("region", "black", "white");       % for marking regions
set_color ("operator", "white", $2);          % +, -, etc..
set_color ("number", "red", $2);              % 10, 2.71,... TeX formulas
set_color ("comment", $4, $2);                % /* comment */
set_color ("string", "cyan", $7);             % "string" or 'char'
set_color ("keyword", "white", $8);           % if, while, unsigned, ...
set_color ("keyword1", "cyan", $8);           % malloc, exit, etc...
set_color ("keyword2", $3, $8);               % other keywords
set_color ("delimiter", $3, $2);              % {}[](),.;...
set_color ("preprocess", "magenta", $2);      % #ifdef ....
set_color ("message", "white", $2);           % color for messages
set_color ("error", $2, $3);                  % color for errors
set_color ("dollar", "white", $2);            % color dollar sign continuation
set_color ("...", "red", $2);                 % folding indicator

set_color ("menu_char", "red", $6);
set_color ("menu", "black", $6);
set_color ("menu_popup", "black", $6);
set_color ("menu_shadow", $2, "black");
set_color ("menu_selection", "black", $9);
set_color ("menu_selection_char", $3, $6);

set_color("cursor", "red", $5);
set_color("cursorovr", "red", "green");

% End of file night.sl

%% The following have been automatically generated:
set_color("linenum", "black", "white");
set_color("trailing_whitespace", $4, $2);
set_color("tab", $4, $2);
set_color("url", "cyan", $7);
set_color("italic", $3, $8);
set_color("underline", "cyan", $8);
set_color("bold", "white", $8);
set_color("html", "white", $8);
set_color("keyword3", $1, $2);
set_color("keyword4", $1, $2);
set_color("keyword5", $1, $2);
set_color("keyword6", $1, $2);
set_color("keyword7", $1, $2);
set_color("keyword8", $1, $2);
set_color("keyword9", $1, $2);
