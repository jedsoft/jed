% Here is a new version of isearch.sl reworked by John Burnell <johnb@whio.grace.cri.nz>.
% This one is said to be more emacs-like.
%
% %% with modifications and comments by Guenter Milde (G.Milde web.de)
%
%% Further modifications by Lloyd Zusman <ljz@asfast.com>
%% as well as JED to get rid of marks which were riding on the stack.

%% isearch.sl
%% Routines here perform incremental forward/backward searches
%%
%% Default bindings:
%%
%% ESCAPE    quits the search
%% ^G        aborts the search (returns to starting point)
%% ESCAPE or ENTER as first char: switch to non incremental search
%%           with the last isearch match as default
%% BACKSPACE deletes last character entered and returns to previous point
%% ^S        finds next match (and switches to forward search)
%% ^R        finds previous match (and switches to backward search)
%%
%% You may use the following variables to change this behaviour,
%%  either here or (better!) in your keybinding defining file (e.g. ".jedrc")
%%
%% This code fragment checks to see what the isearch keys are bound to

require ("srchmisc");
custom_variable ("Isearch_Highlight", 1);
private define get_bound_key (search_func, default)
{
   foreach (["", which_key (search_func), pop()])
     {
	variable key = ();
	if (strlen (key) == 2)
	  {
	     if (key [0] == '^')
	       return (key [1] - '@');
	  }
     }
   return default;
}

custom_variable ("Isearch_Forward_Char", 
		 get_bound_key ("isearch_forward", 19));
custom_variable ("Isearch_Backward_Char", 
		 get_bound_key ("isearch_backward", 18));
custom_variable ("Isearch_Quit_Char",    '\e' );
custom_variable ("Isearch_Abort_Char",     7  ); % ^G

%% Cause isearch_{forward,backward} to behave in the traditional
%% manner by default.  Otherwise, wrap around to the start/end
%% of the buffer and keep going after a forward/reverse search
%% fails.
custom_variable("SEARCH_WRAP", 0);

variable Isearch_Last_Search = "";

private variable Last_Search_Failed = 0;

private define isearch_simple_search (dir)
{
   Last_Search_Failed = 0;

   if (dir < 0)
     search_backward ();
   else
     search_forward ();
   Isearch_Last_Search = LAST_SEARCH;
}

private define perform_search (str, dir)
{
   variable cs = CASE_SEARCH;
   if (strlow (str) != str)
     CASE_SEARCH = 1;

   if (dir > 0)
     dir = fsearch (str);
   else
     dir = bsearch (str);

   CASE_SEARCH = cs;
   return dir;
}

private variable Position_Stack;
private define push_position (attached_to_char)
{
   variable s = struct
     {
	mark, attached_to_char, next
     };
   s.mark = create_user_mark ();
   s.attached_to_char = attached_to_char;
   s.next = Position_Stack;
   Position_Stack = s;
}

private define delete_position_stack ()
{
   Position_Stack = NULL;
}

private define isearch_del (str)
{
   variable attached_to_char = 1;

   if (Position_Stack != NULL)
     {
	variable s = Position_Stack;
	Position_Stack = s.next;
	attached_to_char = s.attached_to_char;
	goto_user_mark (s.mark);
     }

   if (attached_to_char)
     {
	variable n = strlen (str);
	if (n)
	  str = substr (str, 1, n-1);
     }
   
   return str;
}


define isearch_dir (dir)
{
   variable prompt, str = "";
   variable c, first = 1;
   variable len = 0;

   delete_position_stack ();
   variable start_mark = create_user_mark ();

   EXIT_BLOCK
     {
	delete_position_stack ();
        Last_Search_Failed = 0;
     }
   ERROR_BLOCK
     {
	delete_position_stack ();
        Last_Search_Failed = 0;
     }

   forever
     {
	variable prompt_prefix;
	variable prompt_suffix;
	variable h = is_line_hidden ();
	set_line_hidden (0);

	if (Last_Search_Failed)
	  {
	     prompt_prefix = "Failed: i";
	     if (strlen(str) > 0)
	       prompt_suffix = ": ";
	     else
	       prompt_suffix = "";
	  }
	else
	  {
	     prompt_prefix = "I";
	     prompt_suffix = ": ";
	  }
	if (dir > 0)
	  prompt = prompt_prefix + "search forward" + prompt_suffix;
	else
	  prompt = prompt_prefix + "search backward" + prompt_suffix;

	message (prompt + str);

	push_spot ();

	IGNORE_USER_ABORT++;
	if (Isearch_Highlight)
	  {
	     if (looking_at (str) and (Last_Search_Failed == 0))
	       mark_next_nchars (strlen(str), dir);
	  }
	else
	  {
	     if ((dir > 0) and looking_at (str))
	       go_right (strlen (str));
	  }
	update_sans_update_hook (0);
	pop_spot ();

#ifexists AnyError
	try
	  {
#endif
	     c = getkey();
#ifexists AnyError
	  }
	finally
#endif
	IGNORE_USER_ABORT--;

	set_line_hidden (h);
	switch (c)
	  { case Isearch_Quit_Char and first :
	     isearch_simple_search (dir); break;
	  }
	  { case Isearch_Forward_Char :       % ^S
	     push_position (0);
	     if (dir < 0)
	       {
		  % Clear 'failed' indicator if we've changed direction.
		  Last_Search_Failed = 0;
		  dir = 1;
	       }
	     else
	       {
		  go_right_1 ();
		  !if (strlen (str))
		    {
		       str = Isearch_Last_Search;
		       len = strlen (str);
		    }
	       }
	  }
	  { case Isearch_Backward_Char :  %^R
	     push_position (0);
	     if (dir > 0)
	       {
		  % Clear 'failed' indicator if we've changed direction.
		  Last_Search_Failed = 0;
		  dir = -1;
		  c = ' ';                      % use this to fool test (*) below
	       }
	     else
	       {
		  !if (strlen (str)) str = Isearch_Last_Search;
	       }
	  }
	  { case 127 :
	     % Clear 'failed' indicator.
	     Last_Search_Failed = 0;
	     str = isearch_del (str);
	     continue;
	  }
	  { case Isearch_Abort_Char : % ^G go back to start
	     goto_user_mark (start_mark);
	     beep ();
	     return;
	  }
	  {
	   case '\r' and first:
	     if (dir > 0) return search_forward ();
	     else return search_backward ();
	  }
	  {
	   case '\e':
	     if (input_pending (3))
	       ungetkey (c);
	     break;
	  }
#ifdef IBMPC_SYSTEM
	  {
	   case 0xE0:
	     ungetkey (c);
	     break;
	  }
#endif
	  { c < 32 :
	     if (c != '\r') ungetkey (c);
	     break; 	       % terminate search
	  }

	  { str += char (c);             % any other char
	     push_position (1);
	     % Clear 'failed' indicator.
	     Last_Search_Failed = 0;
	  }

	first = 0;

	if (Last_Search_Failed and (SEARCH_WRAP > 0))
	  {
	     % The _next_ C-s or C-r after a previous C-s or C-r that failed
	     % will now redo the search from either the beginning or end of
	     % the buffer, depending on whether we've been going forwards or
	     % backwards.
	     Last_Search_Failed = 0;
	     push_mark ();
	     if (dir > 0)
	       bob();
	     else
	       eob();
	     pop_mark (not (perform_search (str, dir)));
	     continue;
	  }

	% test (*), see ^R switch above
	% NOTE: This test used to include a check to make sure that the 
	%       position stack was not empty.  Does it matter?  --JED
	if ((dir < 0) and looking_at (str) and (c >= ' '))
	  continue;

	if (perform_search (str, dir))
	  len = strlen (str);
	else
	  {
	     variable msg;
	     if (c == Isearch_Forward_Char) go_left_1();
	     if (strlen(str) > 0)
	       msg = strcat (str, " not found.");
	     else
	       msg = "No search string.";
	     flush (msg);
	     % Only beep if we're not wrapping.
	     if (SEARCH_WRAP < 1)
	       beep ();
	     () = input_pending (10);
	     % str = isearch_del (str);
	     if (EXECUTING_MACRO)
	       error ("Not found.");
	     % This piece of state information needs to be set as late
	     % as possible after a failed search attempt.
	     Last_Search_Failed = 1;
	  }
     }

   EXECUTE_ERROR_BLOCK;
   if (strlen (str))
     Isearch_Last_Search = str;
   if (dir > 0)
     go_right (strlen (str) - len);
   message ("Done.");
}

define isearch_forward()
{
   %variable save_abort = IGNORE_USER_ABORT;
   %IGNORE_USER_ABORT = 1;
   isearch_dir (1);
   %IGNORE_USER_ABORT = save_abort;
}

define isearch_backward()
{
   %variable save_abort = IGNORE_USER_ABORT;
   %IGNORE_USER_ABORT = 1;
   isearch_dir (-1);
   %IGNORE_USER_ABORT = save_abort;
}

