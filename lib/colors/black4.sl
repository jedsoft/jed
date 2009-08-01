% Contributed by Mark Olesen
$1 = "white";
$2 = "black";
$3 = "brightcyan";
$4 = "brightred";
$5 = "brightmagenta";
$6 = "lightgray";

set_color("normal",	$1, $2);
% use white/gray status for Xjed color scheme
set_color("status",	$1, "blue");
set_color("region",	"yellow", "blue");
set_color("operator",	$3, $2);
set_color("delimiter",	$3, $2);
set_color("number",	$4, $2);
set_color("string",	$6, $2);
set_color("keyword",	"yellow",  $2);
set_color("keyword1",	"green",   $2);
set_color("comment",	$3, "gray");
set_color("preprocess", $5, $2);
set_color("dollar",	"yellow", "gray");
set_color("...",	$5, $2);
set_color("message",	$3, $2);
set_color("error",	$4, $2);

#iftrue
% blue menu
set_color("menu", $6, "blue");
set_color("menu_char", "yellow", "blue");
set_color("menu_popup", $6, "blue");
set_color("menu_shadow", $4, "gray");
set_color("menu_selection", $1, $2);
set_color("menu_selection_char", "yellow", $2);
#else
% gray menu - ok except the shadow looks a bit odd
% set_color("menu", "lightgray", "gray");
% set_color("menu_char", "yellow", "gray");
% set_color("menu_popup", $1, "gray");
% set_color("menu_shadow", $4, "black");
% set_color("menu_selection", $1, $2);
% set_color("menu_selection_char", "yellow", $2);
#endif

set_color("cursor",    "black", "yellow");
set_color("cursorovr", "black", "red");
set_color("mouse",     "green", $2);

%% some of the following have been automatically generated:
set_color("linenum", "yellow", "blue");
set_color("trailing_whitespace", $3, "gray");
set_color("tab", $3, $2);
set_color("url", $6, $2);
set_color("italic", $1, $2);
set_color("underline", "green", $2);
set_color("bold", $1, $2);
set_color("keyword2", $1, $2);
set_color("keyword3", $1, $2);
set_color("keyword4", $1, $2);
set_color("keyword5", $1, $2);
set_color("keyword6", $1, $2);
set_color("keyword7", $1, $2);
set_color("keyword8", $1, $2);
set_color("keyword9", $1, $2);

%% The following have been automatically generated:

set_color("html", "yellow", $2);
