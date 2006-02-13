%
%  This is supposed to be an easy to use help facility
%

$1 = " *EZhelp*";

!if (keymap_p($1))
{
   make_keymap($1);
   definekey("page_up",		"^?", $1);
   definekey("page_up",		"u", $1);
   definekey("page_up",		"U", $1);
   definekey("page_down",	" ", $1);
   definekey("page_down",	"d", $1);
   definekey("page_down",	"D", $1);
   definekey("search_forward",	"S", $1);
   definekey("search_forward",	"F", $1);
   definekey("search_forward",	"f", $1);
   definekey("search_forward",	"s", $1);
   definekey("search_backward",	"b", $1);
   definekey("search_backward",	"B", $1);
   definekey("ezhelp_quit",	"q", $1);
   definekey("ezhelp_quit",	"Q", $1);
}

variable EZhelp_Last_Buffer;

define ezhelp_quit()
{
   sw2buf(EZhelp_Last_Buffer);
}

define jed_easy_help(file)
{
   variable c, hlpbuf, hlpfile, err, flags, dir;
   hlpbuf = " *EZhelp*";
   !if (strcmp(hlpbuf, whatbuf())) return;
   EZhelp_Last_Buffer = whatbuf();
   err = strcat("Help file not found: ", file);
   ERROR_BLOCK 
     {
	ezhelp_quit();
     }
   
   sw2buf(hlpbuf);
   (hlpfile, dir, hlpbuf, flags) = getbuf_info();
   
   if (strcmp(hlpfile, file))
     {
	hlpfile = expand_jedlib_file(file);
	!if (strlen(hlpfile)) error(err);
	erase_buffer();
	set_readonly(0);
	if (insert_file(hlpfile) <= 0) error(err);
	setbuf_info(file, dir, hlpbuf, flags);
	bob();
	set_readonly(1);
     }
   use_keymap(hlpbuf);
   set_status_line(" u:Page Up, d:Page Down, s:Search, q:Quit Help  (%p)", 0);
}


