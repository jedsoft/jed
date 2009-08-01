$1 = "black"; $2 = "white";

set_color("normal", $1, $2);
set_color("status", "yellow", "blue");
set_color("operator", $1, $2);      % +, -, etc..
set_color("number", "brightblue", $2);    % 10, 2.71, etc..
set_color("comment", "black", "brightcyan");% /* comment */
set_color("region", "yellow", "brightmagenta");
set_color("string", "brightblue", $2);    % "string" or 'char'
set_color("keyword", "brightred", $2);    % if, while, unsigned, ...
set_color("keyword1", "red", $2);    % if, while, unsigned, ...
set_color("delimiter", $1, $2);     % {}[](),.;...
set_color("preprocess", "magenta", $2);
set_color("message", "blue", $2);
set_color("error", "brightred", $2);
set_color("dollar", "brightred", $2);
set_color("...", "red", $2);			  % folding indicator

set_color ("menu_char", "yellow", "blue");
set_color ("menu", "lightgray", "blue");
set_color ("menu_popup", "lightgray", "blue");
set_color ("menu_shadow", "blue", "black");
set_color ("menu_selection", "green", "red");
set_color ("menu_selection_char", "yellow", "red");

set_color ("cursor", "black", "red");
set_color ("cursorovr", "black", "red");

%% The following have been automatically generated:
set_color("linenum", "yellow", "blue");
set_color("trailing_whitespace", "black", "brightcyan");
set_color("tab", "black", "brightcyan");
set_color("url", "brightblue", $2);
set_color("italic", $1, $2);
set_color("underline", "red", $2);
set_color("bold", "brightred", $2);
set_color("html", "brightred", $2);
set_color("keyword2", $1, $2);
set_color("keyword3", $1, $2);
set_color("keyword4", $1, $2);
set_color("keyword5", $1, $2);
set_color("keyword6", $1, $2);
set_color("keyword7", $1, $2);
set_color("keyword8", $1, $2);
set_color("keyword9", $1, $2);
