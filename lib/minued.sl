% ------------------------------------------- -*- mode:SLang; mode:folding; -*-
%
% MINUED-MODE FOR JED
%
% $Id: minued.sl,v 2.15 2000/09/09 22:16:42 rocher Exp $
%
% --------------------------------------------------------------------- %{{{
%
% DESCRIPTION
%	'Minued' is a MINi-bUffer EDitor with wich you can view all
%	previously lines entered in it. Select and modify one of them, press
%	press return and that line will be updated and evaluated.
%
% USAGE
%	Simply add the line:
%
%		autoload ("minued_mode", "minued");
%
%	somewhere in your startup file (.jedrc or jed.rc). You can optionally
%	bind 'minued_mode' to "Ctrl-X Ctrl-E" (quite useful in emacs mode) with
%
%		setkey ("minued_mode", "^X^E");
%
%	See the 'COLORS' section to see how you can customize colors
%	used by minued.
%
% CHANGELOG
%	2000/09/09: 'minued_mode' is now called 'minued_mode' (as it should
%	be). Some minor changes added.
%
%	2000/05/30: Modified by JED for inclusion into 0.99-11.  Until
%	Francesc's is_color has been implemented, the menu_selection colors
%	are used.
%
% AUTHOR
%	Francesc Rocher <f.rocher@computer.org>
%	Feel free to send comments, suggestions or improvements.
%
% ------------------------------------------------------------------------ %}}}

implements ("Minued");

% PRIVATE VARIABLES %{{{
private variable
   minued_cbuf,        % Current buffer before minued-mode was called
   minued_nwin,        % Number of windows (before minued)
   minued_ncoms,       % Number of commands
   minued_mark,
   minued_lnum = 1,    % Line number
   minued_line,        % Line contents
   minued_cnums,       % Color used for numbers
   minued_cline,       % Color used in the current line
   Max_Num_Lines = 32; % The same as in 'mini.sl'

%}}}

% USER VARIABLES

%!%+
%\variable{Minued_Lines}
%\synopsis{Minued_Lines}
%\usage{Integer_Type Minued_Lines = 0;}
%\description
% This variable controls the number of lines shown in the minued
% buffer. If the value of \var{Minued_Lines} is -1, then the entire
% screen is used. If it is 0, the window splits vertically. A positive
% value selects the exact number of lines to show. This value shouldn't
% be greater than \var{SCREEN_HEIGHT}-4, and is only useful while you are
% working with one window.
%
% Default value is 0.
%\seealso{minued}
%\seealso{SCREEN_HEIGHT}
%!%-
custom_variable ("Minued_Lines", 0);

% PRIVATE FUNCTIONS
private define insert_mini_commands ()       %{{{
{
   % Returns the number of inserted lines.
   variable l, line = mini_get_lines (NULL), n = 0;

   foreach (line)
     {
        l = ();
        if (strlen (l))
          {
             n++;
             vinsert ("%3d  %s\n", n, l);
          }
     }

   return n;
}

%}}}
private define remove_mini_command  (n)      %{{{
{
   % Remove the n-th Mini_Previous_Line
   % ('n' is the n-th line from the user point of view)
   variable lines = mini_get_lines (NULL);
   variable i = [0:length (lines)-1];

   mini_set_lines (lines [where (i != n-1)]);
}

%}}}
private define update_mini_command  (n, cmd) %{{{
{
   % Replace n-th Mini_Previous_Line with 'cmd'
   % ('n' is the n-th line from the user point of view)
   if (n > minued_ncoms)
     {
        variable c = String_Type [1];
        c [0] = cmd;
        mini_store_lines (c);
     }
   else
     {
        variable l = mini_get_lines (NULL);
        l [n-1] = cmd;
        mini_set_lines (l);
     }
}

%}}}
private define minued_eval          ()       %{{{
{
   variable mtr;

   if (is_defined (minued_line))
     {
        eval (minued_line);
        return;
     }
   mtr = strtrans (minued_line, "-", "_");
   if (is_internal (mtr))
     {
        call (mtr);
        return;
     }
   if (is_defined (mtr))
     {
        eval (mtr);
        return;
     }
   eval (minued_line);
}

%}}}
private define minued_get_line      ()       %{{{
{
   minued_line = line_as_string ();
   minued_line = strtrim (minued_line [[5:]]);
}

%}}}
private define minued_show          ()       %{{{
{
   erase_buffer ();

   minued_ncoms = insert_mini_commands ();
   if (minued_ncoms == Max_Num_Lines)
     {
        minued_ncoms--;
        () = up (1);
        delete_line ();
     }
   insert ("add> ");
   goto_line (minued_lnum);
   set_column_colors (minued_cnums,1,5);
   set_buffer_modified_flag (0);
}

%}}}
private define minued_adjust_window ()       %{{{
{
   if (Minued_Lines == -1)
      onewindow ();

   if (Minued_Lines)
     {
        if (Minued_Lines > SCREEN_HEIGHT-4)
           onewindow ();
        else
          {
             variable n = window_info ('r');
             if (n < Minued_Lines)
                  Minued_Lines - n;
             else
               {
                  otherwindow ();
                  n - Minued_Lines;
               }
             loop ()
                enlargewin ();
             pop2buf ("*minued*");
          }
     }
}

%}}}
private define minued_update        ()       %{{{
{
   update_mini_command (minued_lnum, minued_line);
}

%}}}
private define minued_update_hook   ()       %{{{
{
   minued_mark = create_line_mark (minued_cline);

   if (what_column () < 6)
      goto_column (6);

   if (andelse
         {what_line () <= minued_ncoms}
         {buffer_modified ()})
     {
        push_spot ();
        goto_column (4);
        del ();
        insert_char ('*');
        pop_spot ();
        set_buffer_modified_flag (0);
     }

   minued_lnum = what_line ();
}

%}}}

% PUBLIC FUNCTIONS ------------------------------------------------------------

%!%+
%\function{minued_mode}
%\synopsis{minued_mode}
%\usage{Void minued_mode ();}
%\description
% \var{minued_mode} is a mini-buffer's contents editor with which you can view all
% previously entered lines in it. Select and modify one of them. Press return
% and that line will be updated in the mini-buffer and evaluated in the buffer
% from which minued was called.
%
% All lines appears numbered, in the same order that they are stored in the
% mini-buffer. Use the last line of minued if you want to add a new one.
% Instead of a number, that line shows "add> ".
%
% The following key-bindings are defined in \var{minued_mode}:
%#v+
%    Return        Update and evaluate the current line.
%    Meta-Return   Evaluate the current line.
%    Ctrl-U        Update the current line, don't leave 'minued_mode'.
%    Meta-R        Remove the current line, don't leave 'minued_mode'.
%    Ctrl-C        Quit.
%#v-
% Note: Minued cannot be invoked from the mini-buffer.
%\seealso{history_load, history_save}
%\seealso{Minued_Lines}
%!%-
public  define minued_mode             () %{{{
{
   if (MINIBUFFER_ACTIVE or (whatbuf () == "*minued*"))
      return;

   minued_nwin = nwindows ();
   minued_cbuf = pop2buf_whatbuf ("*minued*");
   minued_adjust_window ();
   minued_show ();

   set_buffer_undo (1);
   set_mode ("minued", 0);
   use_keymap ("minued");
   set_buffer_hook ("update_hook", &minued_update_hook);
   run_mode_hooks ("minued_mode");
}

%}}}
public  define minued_cancel           () %{{{
{
   setbuf ("*minued*");
   set_buffer_modified_flag (0);
   sw2buf (minued_cbuf);
   delbuf ("*minued*");

   if (minued_nwin == 1)
      onewindow ();
   else
      otherwindow ();
}

%}}}
public  define minued_eval_line        () %{{{
{
   minued_get_line ();
   minued_cancel ();

   if (strlen (minued_line))
      minued_eval ();
}

%}}}
public  define minued_remove_line      () %{{{
{
   if (orelse
      {minued_lnum > minued_ncoms}
      {minued_ncoms == 1})
      return;

   remove_mini_command (minued_lnum);
   minued_show ();
}

%}}}
public  define minued_update_line      () %{{{
{
   if (what_line () > minued_ncoms)
      return;

   variable c = what_column ();

   minued_get_line ();
   if (strlen (minued_line))
     {
        minued_update ();
        delete_line ();
        vinsert ("%3dU %s\n", minued_lnum, minued_line);
        () = up (1);
        goto_column (c);
        set_buffer_modified_flag (0);
        flush ("Line updated.");
     }
}
%}}}
public  define minued_update_eval_line () %{{{
{
   minued_get_line ();
   minued_cancel ();

   !if (strlen (minued_line))
      return;

   minued_update ();
   minued_eval ();
}

%}}}

% COLORS %{{{

% DISABLED while 'is_color' and 'set_color' are not available
%
% !if (is_color ("minued_nums"))
%    set_color ("minued_nums", "brightred", "black");
%
% !if (is_color ("minued_line"))
%    set_color ("minued_line", "black", "green");
%
% minued_cnums = color_number ("minued_nums");
% minued_cline = color_number ("minued_line");

% Temporary solution
#iftrue
minued_cline = color_number ("menu_selection");
minued_cnums = color_number ("menu_selection_char");
#else
set_color_object (100, "brightred", "black");
set_color_object (101, "black", "green");
minued_cnums = 100;
minued_cline = 101;
#endif
%}}}

% KEYMAP %{{{

$0 = "minued";

!if (keymap_p ($0)) {
   make_keymap ($0);
   if ("^C" == _Reserved_Key_Prefix)
     {
	undefinekey ("^C", $0);
	definekey ("minued_cancel",           "^C",   $0);
     }
   definekey ("minued_eval_line",        "\e\r", $0);
   definekey ("minued_remove_line",      "\er",  $0);
   definekey ("minued_update_eval_line", "\r",   $0);
   definekey ("minued_update_line",      "^U",   $0);
}

%}}}
