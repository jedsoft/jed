%% EDT emulation for JED  -- Application Keypad.

% See doc/txt/edt.txt for information about this emulation.

_Jed_Emulation = "edt";

#ifdef IBMPC_SYSTEM
!if (is_defined ("NUMLOCK_IS_GOLD"))
  eval ("variable NUMLOCK_IS_GOLD = 0;");
if (NUMLOCK_IS_GOLD)
  custom_variable ("Key_Gold", "\eOP");
else
  custom_variable ("Key_Gold", "^@;");  %  IBM F1 key
#else
custom_variable ("Key_Gold", "\eOP");
#endif
if ((Key_Gold == NULL) or (Key_Gold == ""))
  Key_Gold = "\eOP";

#ifdef VMS
%define_word("!-~");
#endif

set_status_line("(Jed %v) EDT: %b   (%m%a%n%o)  %p   Advance   %t", 1);

%%
%% Escape sequences for EDT keypad:
%%
%% FP1 = \eOP       FP2 = \eOQ      FP3 = \eOR       PF4 = \eOS
%%   7 = \eOw         8 = \eOx        9 = \eOy         - = \eOm
%%   4 = \eOt         5 = \eOu        6 = \eOv         , = \eOl
%%   1 = \eOq         2 = \eOr        3 = \eOs
%%            0 = \eOp            . = \eOn         enter = \eOM

WANT_EOB = 1;
!if (is_defined("Edt_Loaded"))
{
   variable Edt_Loaded;
   variable edt_pbuf;    %% a real buffer
   edt_pbuf = " <edt>";
   whatbuf();
   setbuf(edt_pbuf);
   sw2buf(());
}

define quit() {exit_jed();}
%define exit() {exit_jed();}

private variable Edt_Keypad_State;
private define application_keypad_mode (state)
{
   Edt_Keypad_State = state;
#ifdef UNIX VMS
   variable t = "ke", e = "\e>";
   if (state)
     {
	t = "ks";
	e = "\e=";
     }
# ifdef UNIX
   variable s =  get_termcap_string (t);
   if (strlen (s))
     e = s;
# endif
   tt_send (e);
#elifdef IBMPC_SYSTEM
   if ((Key_Gold != "\eOP") and (Key_Gold != "^[OP"))
     NUMLOCK_IS_GOLD = state;
#endif
}

define edt_togglekp ()
{
   variable on, off;
#ifdef VMS UNIX
   off = "Numeric.";  on =  "Application.";
#else
   on = "Gold ON"; off = "Gold OFF";
#endif

   Edt_Keypad_State = not(Edt_Keypad_State);

   if (Edt_Keypad_State)
     message (on);
   else
     message (off);

   application_keypad_mode (Edt_Keypad_State);
}

private variable EDT_Dir; EDT_Dir = 1;

%!%+
%\function{edt_advance}
%\synopsis{edt_advance}
%\description
% ADVANCE - (4)
% Sets the current direction to forward for the CHAR, WORD, LINE, EOL, PAGE,
% SECT, SUBS, FIND, and FNDNXT keys.  ADVANCE means that movement will be
% toward the end of the buffer; that is, to the right and down.
%!%-
define edt_advance()
{
   EDT_Dir = 1;
   set_status_line("(Jed %v) EDT: %b   (%m%n%o)  %p    Advance   %t", 1);
}

%!%+
%\function{edt_backup}
%\synopsis{edt_backup}
%\description
% BACKUP - (5)
% Sets the cursor direction to backward for the CHAR, WORD, LINE, EOL, PAGE,
% SECT, SUBS, FIND, and FNDNXT keys.  BACKUP means that movement will be
% toward the beginning of the buffer% that is, to the left and up.
%!%-
define edt_backup()
{
   EDT_Dir = -1;
   set_status_line("(Jed %v) EDT: %b   (%m%n%o)  %p    Backup    %t", 1);
}

%% other buffers:  not buffers but strings except the char buffer which is int
variable edt_wbuf, edt_lbuf, edt_cbuf;
edt_wbuf = Null_String; edt_lbuf = Null_String; edt_cbuf = 0;

%% character (un)deletion
define edt_cdel()
{
   !if (eobp()) { edt_cbuf = what_char(); del(); }
}
define edt_ucdel()
{
   if (edt_cbuf) { insert_char (edt_cbuf); go_left_1 (); }
}

define edt_word()
{
   if (EDT_Dir == 1)
     {
	if (eolp()) return(go_right_1 ());   %  trick-- nothing returned
	skip_word_chars(); skip_non_word_chars();
     }
   else
     {
	if (bolp()) return (go_left_1 ());
	bskip_non_word_chars(); bskip_word_chars();
     }
}

%!%+
%\function{edt_wdel}
%\synopsis{edt_wdel}
%\description
% DEL W - (-)
% Deletes text from the cursor to the beginning of the next word, storing the
% text in the delete word buffer.
%!%-
define edt_wdel()
{
   push_mark();
   edt_word();            %% use whatever edt_word does as a region
   edt_wbuf = bufsubstr_delete ();
}

%% another one from Clive Page.
define edt_specins()
{
   insert_char (integer(read_mini("char code (decimal):", "27", Null_String)));
}

%!%+
%\function{edt_uwdel}
%\synopsis{edt_uwdel}
%\description
% UND W - (GOLD -)
% Inserts the contents of the delete word buffer directly to the left of the
% cursor.
%!%-
define edt_uwdel()
{
   push_spot(); insert (edt_wbuf); pop_spot();
}

%% aparantly deleol also saves what it did in buffer...
define edt_deleol()
{
   push_mark_eol(); edt_lbuf = bufsubstr_delete();
}
define edt_delbol()
{
   push_mark(); bol(); edt_lbuf = bufsubstr_delete ();
}

%% the line

%!%+
%\function{edt_ldel}
%\synopsis{edt_ldel}
%\description
% DEL L - (PF4)
% Deletes text from the cursor position to the end of the current line, including
% the line terminator.  If the cursor is positioned at the beginning of a line,
% the entire line is deleted.  The deleted text is saved in the delete line
% buffer.
%!%-
define edt_ldel()
{
   mark_to_visible_eol ();
   go_right_1 ();
   edt_lbuf = bufsubstr_delete ();
}

%!%+
%\function{edt_uldel}
%\synopsis{edt_uldel}
%\description
% UND L - (GOLD PF4)
% Inserts the contents of the delete line buffer directly to the left of the
% cursor.
%!%-
define edt_uldel()
{
   push_spot(); insert (edt_lbuf); pop_spot();
}

%!%+
%\function{edt_find}
%\synopsis{edt_find}
%\description
% FIND - (GOLD PF3)
% Searches for an occurrence of a string.  Press the FIND key and then enter the
% string using the main keyboard.  End the string by pressing either the ADVANCE
% or BACKUP key to set the direction of the search, or the ENTER key to search in
% the current direction.
%!%-
define edt_find()
{
   if (EDT_Dir == 1) search_forward (); else search_backward ();
}

%!%+
%\function{edt_findnxt}
%\synopsis{edt_findnxt}
%\description
% FNDNXT - (PF3)
% Searches for the next occurrence of the search string previously entered with
% the FIND key.  The direction of the search is the current one (ADVANCE or
% BACKUP).
%!%-
define edt_findnxt()
{
   variable r, found;
   r = 0;
   if (strlen(LAST_SEARCH))
     {
	if (EDT_Dir == 1)
	  {
	     r = right(1);
	     found = fsearch(LAST_SEARCH);
	  }
	else found = bsearch(LAST_SEARCH);
	!if (found)
	  {
	     go_left(r);
	     error("Not Found.");
	  }
     }
   else error ("Find What?");
}

define edt_go_down_n (n)
{
   ERROR_BLOCK
     {
	_clear_error ();
     }
   loop (n) call ("next_line_cmd");
}

define edt_go_up_n (n)
{
   ERROR_BLOCK
     {
	_clear_error ();
     }
   loop (n) call ("previous_line_cmd");
}

%!%+
%\function{edt_sect}
%\synopsis{edt_sect}
%\description
% SECT - (8)
% Moves the cursor 16 lines (one section) forward or backward, depending on the
% current direction (see ADVANCE and BACKUP).  The cursor is moved to the
% beginning of the appropriate line.
%!%-
define edt_sect()
{
   if (EDT_Dir == 1) edt_go_down_n(16); else edt_go_up_n(16);
   bol();
}

define edt_eol()
{
   if (EDT_Dir == 1)
     {
	if (eolp()) edt_go_down_n (1);
     }
   else edt_go_up_n (1);
   eol();
}

define edt_line()
{
   if (EDT_Dir > 0) edt_go_down_n (1);
   else if (bolp()) edt_go_up_n (1);
   bol();
}

define edt_char()
{
   if (EDT_Dir == 1) go_right_1 (); else go_left_1 ();
}

define edt_oline()
{
   newline(); go_left_1 ();
}

%% reset also pops marks.
define edt_reset()
{
   edt_advance();
   while(markp()) pop_mark_0 ();
   call ("kbd_quit");
}

%% edt page searches for a form feed.  However, real edt allows user to
%% change this.

%!%+
%\function{edt_page}
%\synopsis{edt_page}
%\description
% PAGE - (7)
% Moves the cursor to the top of a page.  A page is defined by a delimiter
% string, which can be set by the SET ENTITY command.  The default page
% delimiter is the formfeed character (CTRL/L).
%!%-
define edt_page()
{
   variable ret, ff;
   ff = char(12);
   if (EDT_Dir == 1) ret = fsearch(ff); else ret = bsearch(ff);
   if (ret) recenter(1);
}

define edt_cut()
{
   variable b;
   !if (dupmark()) return;
   b = whatbuf();
   setbuf(edt_pbuf);
   erase_buffer();
   setbuf(b);
   copy_region(edt_pbuf);
   del_region();
}
define edt_paste()
{
   insbuf(edt_pbuf);
}

%% Although not documented in EDT online help, this deletes the region.
%!%+
%\function{edt_append}
%\synopsis{edt_append}
%\description
% APPEND - (9)
% Moves the select range to the end of the PASTE buffer.  The select range is all
% the text between the selected position (see SELECT) and the current cursor
% position.  If no SELECT has been made and the cursor is positioned on the
% current search string, that string is appended.
%!%-
define edt_append()
{
   variable b;
   b = whatbuf();
   setbuf(edt_pbuf);
   eob();
   setbuf(b);
   if (dupmark())
     {
	copy_region(edt_pbuf);
	del_region();
     }
   else error("No Region.");
}

define edt_replace()
{
   del_region(); edt_paste();
}

%% a real edt_subs function
%% deletes search string, substitutes what is in the pastebuffer and finds
%% the next.
define edt_subs()
{
   if (looking_at(LAST_SEARCH))
     {
	deln (strlen(LAST_SEARCH));
	edt_paste();
	!if (looking_at (LAST_SEARCH)) edt_findnxt();
     }
   else error("Select range not active.");
}

%% a help for the help key
define edt_help()
{
   jed_easy_help("edt.hlp");
}

%% Chngcase
define edt_chgcase()
{
   variable n;
   !if (markp())
     {
	push_mark();
	n = strlen(LAST_SEARCH);
	if (n and looking_at(LAST_SEARCH)) go_right(n);
	else if (EDT_Dir == 1) go_right_1 ();
	else go_left_1 ();
     }
   xform_region('X');
}

%% is this a better defn of what edt_replace should be?
%!%+
%\function{edt_replace}
%\synopsis{edt_replace}
%\description
% REPLACE - (GOLD 9)
% Deletes the select range and replaces it with the contents of the PASTE
% buffer.
%!%-
define edt_replace()
{
   variable n;
   n = strlen(LAST_SEARCH);

   if (n and looking_at(LAST_SEARCH)) deln (n);
   else
     {
	!if (markp()) error("Select range not active.");
	del_region();
     }
   edt_paste();
}

variable EDT_Scroll_Begin = 3;
define edt_check_scroll ()
{
   variable n, max_n, w_rows;
   w_rows = window_info('r');
   if (w_rows > 2 * EDT_Scroll_Begin - 2 )
     {
	n = window_line();
	max_n = w_rows - EDT_Scroll_Begin + 1;
	if (n < EDT_Scroll_Begin) recenter (EDT_Scroll_Begin);
	if (n > max_n)        recenter (max_n);
     }
}

private define GOLD(x)
{
   return strcat (Key_Gold, x);
}

%% unset some of default jed keys--- lose window capability on "^W" one
%% unsetkey ("^W");
%% setkey ("redraw", "^W");
%% unsetkey("^K"); unsetkey %%-- unset this, we lose kill line in emacs.sl
unsetkey("^U");             %% emacs.sl rebinds this to digit-arg

%% The default binding for the quote keys (", ') is 'text_smart_quote'.
%% Most users do not seem to like this so it is unset here.
setkey("self_insert_cmd",	"\"");
setkey("self_insert_cmd",	"'");
%%
%% In addition, if you want the ^H key to move to the beginning of line
%% then uncomment the next two lines.
%    unsetkey("^H");
%  setkey("beg_of_line",	"^H");  %% beginning of line
%% By default, these are bound
%% to help functions (man page, etc...).

%% conventional subs key definition:
unsetkey (Key_Gold);
setkey("edt_subs",	GOLD("\eOM"));  %% subs (edt style)

%% Give user ability to exit via GOLD-Q, GOLD-E
setkey("exit_jed", 	GOLD("Q"));
setkey("exit_jed", 	GOLD("E"));

setkey("edt_togglekp",	GOLD(Key_Gold));	%% Gold-Gold toggles keypad
%  setkey("self_insert_cmd", "^I");	%% tab inserts tab.
setkey("edt_delbol",	"^U");	%% delete to bol
setkey("edt_help",	"\eOQ");	%% help
setkey("edt_findnxt",	"\eOR");	%% findnxt
setkey("edt_ldel",	"\eOS");	%% del l
setkey("edt_cdel",	"\eOl");	%% del c
setkey("edt_wdel",	"\eOm");	%% del w
setkey("smart_set_mark_cmd",	"\eOn");	%% select
setkey("edt_line",	"\eOp");	%% line
setkey("edt_word",	"\eOq");	%% word
setkey("edt_eol",	"\eOr");	%% eol
setkey("edt_char",	"\eOs");	%% char
setkey("edt_advance",	"\eOt");	%% advance
setkey("edt_backup",	"\eOu");	%% backup
setkey("edt_cut",	"\eOv");	%% cut
setkey("edt_page",	"\eOw");	%% page
setkey("edt_sect",	"\eOx");	%% sect
setkey("edt_append",	"\eOy");	%% append
setkey("edt_help",	GOLD("\eOQ"));	%% help
setkey("edt_find",	GOLD("\eOR"));	%% find
setkey("edt_uldel",	GOLD("\eOS"));	%% udel l
setkey("edt_ucdel",	GOLD("\eOl"));	%% udel c
setkey("edt_uwdel",	GOLD("\eOm"));	%% udel w
setkey("edt_reset",	GOLD("\eOn"));	%% reset
setkey("edt_oline",	GOLD("\eOp"));	%% open line
setkey("edt_chgcase",	GOLD("\eOq"));	%% chgcase
setkey("edt_deleol",	GOLD("\eOr"));	%% deleol
%%
%% There are two possible definitions for the specins key.  Let's
%% choose the edt one though I prefer the other.
%%
setkey("edt_specins",   GOLD("\eOs"));	%% specins
%  setkey("quoted_insert",	GOLD("\eOs"));	%% specins
setkey("eob",		GOLD("\eOt"));	%% bottom
setkey("beg_of_buffer",	GOLD("\eOu"));	%% top
setkey("edt_paste",	GOLD("\eOv"));	%% paste
setkey("evaluate_cmd",	GOLD("\eOw"));	%% cmd
setkey("format_paragraph", GOLD("\eOx"));	%% fill
setkey("edt_replace",	GOLD("\eOy"));	%% replace
setkey("exit_mini",	"\eOM");	%% enter

%% The enter key requires some care--- it MUST be put in the
%% minibuffer keymap.  But this is not created until AFTER the init
%% files are loaded so that it inherits user definitions.  The above
%% line puts it in the global map so that it behaves properly there.
%% The same applies to the 'reset' command.
%%
%% The above comment WAS true in versions prior to 0.81.  Now it is possible
%% to achieve the desired effect in the startup_hook.  I think it is time to
%% think of a way to chain functions.  That is, I would like to define a
%% function that startup_hook will call without explicitly modifying startup
%% hook and not destroying other hooks in the process.  Thus, startup_hook
%% should consist of a chain of functions to execute.
if (keymap_p("Mini_Map"))
{
   undefinekey (Key_Gold, "Mini_Map");
   definekey("exit_mini", "\eOM", "Mini_Map");    %% enter
   definekey("edt_reset", GOLD("\eOn"),"Mini_Map"); %% reset
}
%%
%%  In EDT, a command may be repeated by GOLD number.  Lets have that too
%%
_for (0, 9, 1)
{
   $0 = ();
   setkey("digit_arg", GOLD(string($0)));
}

%% These are the keys on the vt220 keyboard
setkey("edt_find",      	"\e[1~");	%%differs from vt220.sl
setkey("yank",			"\e[2~");
setkey("kill_region",		"\e[3~");
setkey("smart_set_mark_cmd",	"\e[4~");
%setkey(evaluate_cmd",		"\e[29~");
setkey("emacs_escape_x",	"\e[29~");
setkey("edt_help",		"\e[28~");
%%
%%  Finally some definitions for scrolling the screen left/right
%%
setkey("beg_of_buffer", GOLD("\e[A"));	% gold ^
setkey("eob",           GOLD("\e[B"));	% gold v
setkey("scroll_left",   GOLD("\e[C"));	% gold ->
setkey("scroll_right",  GOLD("\e[D"));	% gold <-

setkey(".\"page_up\" call edt_check_scroll",		"\e[5~");
setkey(".\"page_down\" call edt_check_scroll",		"\e[6~");
setkey(".\"previous_line_cmd\" call edt_check_scroll",	"\e[A");
setkey(".\"previous_line_cmd\" call edt_check_scroll",	"\eOA");
setkey(".\"next_line_cmd\" call edt_check_scroll",	"\e[B");
setkey(".\"next_line_cmd\" call edt_check_scroll",	"\eOB");

application_keypad_mode (1);
runhooks ("keybindings_hook", _Jed_Emulation);
