% Contributed by Mark Olesen
$1 = "white";
$2 = "black";
$3 = "cyan";
$4 = "red";
$5 = "magenta";
$6 = "gray75";

set_color("normal",	$1, $2);
% use white/blue status for console color scheme
set_color("status",     $1,   "gray50");
set_color("region",	"yellow",  "blue");
set_color("operator",	$3, $2);
set_color("delimiter",	$3, $2);
set_color("number",	$4, $2);
set_color("string",	$6, $2);
set_color("keyword",	"yellow",  $2);
set_color("keyword1",	"green",   $2);
set_color("comment",	$3, "gray25");
set_color("preprocess", $5, $2);
set_color("dollar",     "yellow", "gray25");
set_color("...",	$5, $2);
set_color("message",	$3, $2);
set_color("error",	$4, $2);

% gray menu -
set_color("menu", $1, "gray50");
set_color("menu_char",	"yellow",	"gray50");
set_color("menu_popup", $1, "gray50");
set_color("menu_shadow", $4, "gray25");
set_color("menu_selection", $1, $2);
set_color("menu_selection_char", "yellow", $2);

set_color("cursor",    "black", "yellow");
set_color("cursorovr", "black", "red");
set_color("mouse",     "green", $2);

%% some of the following have been automatically generated:
set_color("linenum", "yellow", "blue");
set_color("trailing_whitespace", $3, "gray50");
set_color("tab", $3, "gray50");
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
