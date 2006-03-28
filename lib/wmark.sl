%--------------------------------*-SLang-*--------------------------------
% wmark.sl
% Implements Windows style of marking - for Windows users
% Author: Luchesar Ionkov
%
% Modified By JED
% modified by mj olesen
%
% Holding down Shift key and using arrow keys selects text
% Delete Key cuts the block of text.
% Inserting a character will replace the block with the character.
% Yanking will replace the block with the text in the yank buffer
% 
% Note: If you are having problems with this under linux, then read the
% jed/doc/txt/linux-keys.txt file.

require ("keydefs");

private variable Wmark_Region_Funs = Assoc_Type[Ref_Type];

private define backward_delete_fun ()
{
   call ("kill_region");
   insert ("X");
}

private define forward_delete_fun ()
{
   backward_delete_fun ();
   go_left_1 ();
}

public define wmark_add_region_function (fun, region_fun)
{
   Wmark_Region_Funs[fun] = region_fun;
}

wmark_add_region_function ("self_insert_cmd", &del_region);
wmark_add_region_function ("yank", &del_region);
wmark_add_region_function ("yp_yank", &del_region);

wmark_add_region_function ("backward_delete_char_untabify", &backward_delete_fun);
wmark_add_region_function ("backward_delete_char", &backward_delete_fun);
wmark_add_region_function ("delete_char_cmd", &forward_delete_fun);

private variable Wmark_Movement_Flag = 0;
private variable Wmark_Buffer;

private define before_key_hook ();
private define after_key_hook ();
private define end_wmark ()
{
   if (bufferp (Wmark_Buffer))
     {
	variable cbuf = whatbuf();
	setbuf (Wmark_Buffer);
	if (is_visible_mark ())
	  pop_mark_0 ();
	setbuf (cbuf);
     }

   remove_from_hook ("_jed_before_key_hooks", &before_key_hook);
   remove_from_hook ("_jed_after_key_hooks", &after_key_hook);
}


private define before_key_hook (fun)
{
   if (typeof (fun) == Ref_Type)
     fun = "&";

   if (0 == strncmp (fun, "wmark_", 6))
     return;

   Wmark_Movement_Flag = 0;
   if (is_visible_mark ()
       and assoc_key_exists (Wmark_Region_Funs, fun))
     {
	(@Wmark_Region_Funs[fun]) ();
	end_wmark ();
	return;
     }
}

private define after_key_hook ()
{
   if (Wmark_Movement_Flag == 0)
     end_wmark ();
}

private define wmark_prefix ()
{
   !if (is_visible_mark ()) push_visible_mark ();
   if (Wmark_Movement_Flag == 0)
     {
	add_to_hook ("_jed_before_key_hooks", &before_key_hook);
	add_to_hook ("_jed_after_key_hooks", &after_key_hook);
	Wmark_Movement_Flag = 1;
	Wmark_Buffer = whatbuf ();
     }
}


private define wmark_eval (fun)
{
   wmark_prefix ();
   if (typeof (fun) == Ref_Type)
     @fun ();
   else
     call (fun);
}

% the various functions

define wmark_up () { wmark_eval ("previous_line_cmd"); }
define wmark_down () { wmark_eval ("next_line_cmd"); }
define wmark_left () { wmark_eval ("previous_char_cmd"); }
define wmark_right () { wmark_eval ("next_char_cmd"); }
define wmark_page_up () { wmark_eval ("page_up"); }
define wmark_page_down () { wmark_eval ("page_down"); }
define wmark_bol () { wmark_eval (&bol); }
define wmark_eol () { wmark_eval (&eol); }
define wmark_bob () { wmark_eval (&bob); }
define wmark_eob () { wmark_eval (&eob); }
define wmark_skip_word () { wmark_eval (&skip_word); }
define wmark_bskip_word () { wmark_eval (&bskip_word); }

setkey ("wmark_up",		Key_Shift_Up);		% S-Up
setkey ("wmark_down",		Key_Shift_Down);	% S-Down
setkey ("wmark_left",		Key_Shift_Left);	% S-Left
setkey ("wmark_right",		Key_Shift_Right);	% S-Right
setkey ("wmark_page_up", 	Key_Shift_PgUp);	% S-PageUp
setkey ("wmark_page_down",	Key_Shift_PgDn);	% S-PageDown
setkey ("wmark_bol",		Key_Shift_Home);	% S-Home
setkey ("wmark_eol",		Key_Shift_End);		% S-End
setkey ("yank",			Key_Shift_Ins);		% S-Insert
setkey ("kill_region",		Key_Shift_Del);		% S-Delete
setkey ("copy_region",		Key_Ctrl_Ins);		% C-Insert
setkey ("del_region",		Key_Ctrl_Del);		% C-Delete

#ifndef IBMPC_SYSTEM
private define wmark_reset_display_hook ()
{
   tt_send("\e[?35h");
}

private define wmark_init_display_hook ()
{
   tt_send("\e[?35l");
}

$1 = getenv ("TERM"); if ($1 == NULL) $1 = "";

if (string_match ($1, "^xterm.color", 1)
    or string_match ($1, "^rxvt", 1))
{
   % rxvt: bypass XTerm shift keys and allow S-Prior, S-Next, S-Insert
   add_to_hook ("_jed_reset_display_hooks", &wmark_reset_display_hook);
   add_to_hook ("_jed_init_display_hooks", &wmark_init_display_hook);
   setkey ("wmark_bol",	"\e[7$");	% S-Home
   setkey ("wmark_eol",	"\e[8$");	% S-End
}
#endif

provide ("wmark");
