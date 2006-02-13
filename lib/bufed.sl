% -*- SLang -*-		bufed.sl
%
% Simple JED `bufed' mode by Mark Olesen <olesen@me.QueensU.CA>
%
% Bufed is a simple buffer manager -- patterned somewhat after dired.
% Provides easy, interactive switching, saving and killing of buffers.
%
% To invoke Bufed, do `M-x bufed'.
% Or re-bind to the key sequence which is normally bound to the
% `list_buffers' function `C-x C-b' (emacs)
%
% ------------------------------------------------------------------------
% TO USE THIS MODE: add the line
%
%	autoload ("bufed", "bufed");		to ~/.jedrc
%
% and optionally re-bind to the `C-x C-b' (emacs) key sequence
%
%	setkey ("bufed", "^X^B");
% ------------------------------------------------------------------------

variable Bufed_buf = "*BufferList*";	% as used by `list_buffers' (buf.sl)

% save the buffer
define bufed_savebuffer (buf)
{
   variable file, dir, flags, ch, this_buf;

   ch = int (buf);
   if ((ch == 32) or (ch == '*')) return;	% internal buffer or special

   this_buf = whatbuf ();
   (file,dir,,flags) = getbuf_info (buf);

   if (strlen (file) and (flags & 1))	% file assciated with it
     {
	setbuf (buf);
	ERROR_BLOCK { setbuf (this_buf); }
	() = write_buffer (dircat (dir, file));
     }
}

% extract the buffer name associated with the current line
% Note: The details of this routine will depend upon how buf.sl formats
%       the line.  Currently, this looks like:
% ----------- 0000    "*scratch*"		    /aluche/h1/davis/src/jed/lib/

define bufed_get ()
{
   variable buf;

   push_spot_bol ();
   EXIT_BLOCK { pop_spot (); }

   !if (ffind_char ('"'))
     return Null_String;

   go_right_1 ();
   push_mark ();
   !if (ffind_char ('"'))
     {
	pop_mark_1 ();
	return Null_String;
     }
   
   buf = bufsubstr ();
   !if (bufferp (buf)) buf = "";
   return buf;
}

define list_buffers ()
{
   variable i, j, tmp, this, name, flags, flag_chars, skip;
   variable umask;
   variable name_col, dir_col, mode_col;
   
   name_col = 21;
   mode_col = 13;
   dir_col = 45;
   
   skip = 0;
   if (prefix_argument(-1) == -1) skip = 1;
   tmp = "*BufferList*";
   this = whatbuf();
   pop2buf(tmp);
   set_readonly(0);
   erase_buffer();
   TAB = 8;
   
   flag_chars = "CBKN-UORDAM";
   insert ("  Flags");
   goto_column (mode_col);
   insert ("umask");
   goto_column (name_col);
   insert ("Buffer Name");
   goto_column(dir_col); insert("Dir/File\n");
   
   loop (buffer_list())
     {
	name = ();
	if (skip and (int(name) == ' ')) continue;   %% internal buffers begin with a space
	flags = getbuf_info (name);    % more on stack
	umask = set_buffer_umask (-1);
	bol();
	i = 0x400; j = 0;
	while (i)
	  {
	     if (flags & i) flag_chars[j]; else '-';
	     insert_char (());
	     i = i shr 1; j++;
	  }
	goto_column (mode_col);
	vinsert ("0%03o", umask);
	goto_column (name_col);
	
	% Since the buffername may contain whitespace, enclose it in quotes
	insert_char ('"');
	insert(()); %% buffer name
	insert_char ('"');

	goto_column(dir_col);
	!if (eolp())
	  {
	     eol(); insert_single_space();
	  }
	
	insert(()); insert(());               %% dir/file
	newline();
     }
   
   insert("\nU:Undo O:Overwrite R:Readonly D:Disk File Changed, A:Autosave, M:Modified\n");
   insert("C:CRmode, B:Binary File, K:Not backed up, N:No autosave");

   bob ();
   set_buffer_modified_flag (0);
   set_readonly (1);
   pop2buf(this);
}

private variable Line_Mark;
private define update_bufed_hook ()
{
   Line_Mark = create_line_mark (color_number ("menu_selection"));
}

define bufed_list ()
{
   Line_Mark = NULL;
   check_buffers ();
   list_buffers ();
   pop2buf (Bufed_buf);
   set_buffer_hook ("update_hook", &update_bufed_hook);
   set_readonly (0);
   bob();
   insert ("Press '?' for help.  Press ENTER to select a buffer.\n\n");
   set_readonly (0);
   set_buffer_modified_flag(0);
   go_down (1);
   %goto_column (21);
}

% kill a buffer, if it has been modified then pop to it so it's obvious
define bufed_kill ()
{
   variable file, dir, flags, buf = bufed_get ();
   variable line;

   !if (strlen (buf)) return;

   line = what_line ();
   (file,dir,,flags) = getbuf_info (buf);

   if (flags & 1)		% modified
     {
	pop2buf (buf);
	update (1);
     }
   delbuf (buf);
   if (strcmp (buf, Bufed_buf))
     bufed_list ();
   goto_line (line);
}

define bufed_save ()
{
   variable buf = bufed_get ();
   !if (int (buf)) return;
   bufed_savebuffer (buf);
}

% try to re-load the file from disk
define bufed_update ()
{
   variable file, dir, flags;
   (file,dir,,flags) = getbuf_info ();
   if (flags & 2)		% file on disk modified?
     {
	!if (find_file (dircat (dir, file)))
	  error ("Error reading file");
     }
}


define bufed_pop2buf ()
{
   variable buf = bufed_get ();

   !if (int (buf)) return;

   % if the buffer is already visible, scroll down
   buffer_visible (buf);	% leave on the stack
   pop2buf (buf);
   if (() and not(eobp ())) 
     call ("page_down");

   bufed_update ();
   pop2buf (Bufed_buf);
}

define bufed_sw2buf (one)
{
   variable buf = bufed_get ();
   !if (int (buf)) return;
   sw2buf (buf);
   bufed_update ();
   if (one) onewindow ();
}

define bufed_exit ()
{
   delbuf (whatbuf ());
}

variable Bufed_help;
Bufed_help = "k:kill, s:save, g:refresh, SPC,f:pop2buf, CR,TAB:sw2buf, q:quit, h:help, ?:this help";

define bufed_help ()
{
   message (Bufed_help);
}

$1 = "bufed";
!if (keymap_p ($1)) make_keymap ($1);
definekey ("bufed_list",	"g",	$1);
definekey ("describe_mode",	"h",	$1);
definekey ("bufed_kill",	"k",	$1);
definekey ("bufed_save",	"s",	$1);
definekey ("bufed_pop2buf",	"f",	$1);
definekey ("bufed_pop2buf",	" ",	$1);
definekey (".0 bufed_sw2buf",	"\r",	$1);
definekey (".1 bufed_sw2buf",	"\t",	$1);
definekey ("bufed_exit",	"q",	$1);
definekey ("bufed_help",	"?",	$1);

% Also possible,
%  U	toggle_undo
%  O	toggle_overwrite
%  R	toggle_readonly
%  C	toggle_crmode

%!%+
%\function{bufed}
%\synopsis{bufed}
%\description
% Mode designed to aid in navigating through multiple buffers
% patterned somewhat after dired.
% 
% To invoke Bufed, do \var{M-x bufed} or bind to \var{C-x C-b} (emacs)
% 
% \var{g}	Update the buffer listing.
% 
% \var{k}	Kill the buffer described on the current line, like typing
% 	\var{M-x kill_buffer} and supplying that buffer name.
% 
% \var{s}	Save the buffer described on the current line.
% 
% \var{f}, \var{SPC}, \var{CR}, \var{TAB}
% 	Visit the buffer described on the current line. 
% 	\var{f} and \var{SPC} will create a new window if required.
% 	\var{CR} will use the current window.
% 	\var{TAB} will revert to a single window.
% 
% \var{Q}	Quit bufed mode.
%!%-
define bufed ()
{
   variable mode = "bufed";
   variable this_buf;
   
   this_buf = sprintf ("\"%s\"", whatbuf ());
   bufed_list ();
   () = fsearch (this_buf);

   bufed_help ();
   use_keymap (mode);
   set_mode (mode, 0);
   run_mode_hooks ("bufed_hook");
}
