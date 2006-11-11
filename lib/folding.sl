% -*- mode: slang; mode: fold -*-
_debug_info = 1;

!if (is_defined ("Fold_Bob_Eob_Error_Action")) %{{{
{
%!%+
%\variable{Fold_Bob_Eob_Error_Action}
%\synopsis{Fold_Bob_Eob_Error_Action}
%\usage{Integer Fold_Bob_Eob_Error_Action = 1;}
%\description
% This value of this variable determines the what will happen upon
% reaching the boundary of the current fold via the up/down keys.
% If the value is 0, an error will be generated; if the value is 1, 
% the fold will be exited; otherwise, the next/previous fold will be
% entered. 
%!%-
variable Fold_Bob_Eob_Error_Action = 1;
}

%}}}

%{{{ Associating fold marks with modes

%mode_set_mode_info ("score", "fold_info", "%{{{\r%}}}\r\r");


define fold_get_marks_for_mode () %{{{
{
   variable mode, fold_marks;
   
   fold_marks = mode_get_mode_info ("fold_info");

   if (fold_marks == NULL)
     return ("{{{", "}}}", "", "");

   % push the marks on stack
   foreach (strchop (fold_marks, '\r', 0))
     ;
}

%}}}

define fold_get_marks () %{{{
{
#ifdef HAS_BLOCAL_VAR
   ERROR_BLOCK
     {
	_clear_error ();
	error ("Folding mode not enabled for buffer.");
     }
   get_blocal_var ("fold_start");
   get_blocal_var ("fold_end");
   get_blocal_var ("fold_end_of_start");
   get_blocal_var ("fold_end_of_end");
#else
   fold_get_marks_for_mode ();
#endif
}

%}}}

%}}}

%{{{ Basic functions:

private define fold_is_marker_line (start, end_of_start) %{{{
{
   bol_skip_white ();
   if (looking_at (start))
     {
	return 1;
     }
   eol ();
   !if (bfind (start))
     {
	return 0;
     }

   go_right (strlen (__tmp(start)));
%   skip_white ();
%   end_of_start = strtrim (__tmp(end_of_start));
%
%   !if (looking_at (end_of_start))
%     return 0;
   () = ffind (end_of_start);

   go_right (strlen (end_of_start));
   skip_white ();
   eolp ();
}

%}}}
private define fold_find_marker_line (start, end_of_start) %{{{
{
   bol ();
   while (fsearch (start))
     {
	if (fold_is_marker_line (start, end_of_start))
	  return 1;
	eol ();
     }
   return 0;
}

%}}}
private define fold_find_marker_line_reverse (start, end_of_start, hidden_check) %{{{
{
   eol ();
   while (bsearch (start))
     {
	if (fold_is_marker_line (start, end_of_start))
	  {
	     !if (hidden_check and is_line_hidden ())
	       return 1;
	  }
	bol ();
     }
   return 0;
}
%}}}
private define fold_this_fold (start, end, end_of_start, end_of_end, 
		       start_level, fold_level) %{{{
{
   variable level = start_level;
   
   while (down_1 ())
     {
   	set_line_hidden (level >= fold_level);
	
	if (fold_is_marker_line (start, end_of_start)) level++;
	else if (fold_is_marker_line (end, end_of_end)) 
	  {
	     if (level == start_level) break;
	     level--;
	  }
     }
}

%}}}
define fold_open_buffer () %{{{
{
   push_spot ();
   widen_buffer ();
   mark_buffer ();
   set_region_hidden (0);
   pop_spot ();
}

%}}}

define fold_whole_buffer () %{{{
{
   variable start, end, end_of_start, end_of_end;
   variable level, fold_level;
   
   flush ("folding buffer...");
   ERROR_BLOCK
     {
	pop_spot ();
     }
   push_spot ();
   
   fold_open_buffer ();
   
   bob ();
   (start, end, end_of_start, end_of_end) = fold_get_marks ();

   fold_level = prefix_argument (-1);
   if (fold_level <= 0) fold_level = 1;

   while (fold_find_marker_line (start, end_of_start))
     {
	fold_this_fold (start, end, end_of_start, end_of_end, 
			1, fold_level);
	!if (down_1 ())
	  break;
     }
   
   pop_spot ();
   if (is_line_hidden ())
     {
	skip_hidden_lines_backward (1);
	bol ();
     }
   flush ("folding buffer...done");
}

%}}}
private define fold_is_fold (start, end_of_start) %{{{
{
   push_spot ();
   EXIT_BLOCK
     {
	pop_spot ();
     }
   
   !if (fold_is_marker_line (start, end_of_start))
     return 0;
   
   % Check to make sure this is not the top of the current fold by making
   % sure that the next line is hidden.
   !if (down_1 ()) return 0;
   return is_line_hidden ();
}

%}}}
define fold_open_fold () %{{{
{
   variable start, end, end_of_start, end_of_end;
   
   (start, end, end_of_start, end_of_end) = fold_get_marks ();
   push_spot ();
   if (is_line_hidden ())
     skip_hidden_lines_backward (1);
   if (fold_is_fold (start, end_of_start))
     {
	fold_this_fold (start, end, end_of_start, end_of_end, 0, 1);
     }
   pop_spot ();
}

%}}}
define fold_enter_fold () %{{{
{
   variable start, end, end_of_start, end_of_end;
   variable h;
   
   (start, end, end_of_start, end_of_end) = fold_get_marks ();
   
   push_spot ();
   
   while (fold_find_marker_line_reverse (start, end_of_start, 1))
     {
	push_mark ();
	if (fold_is_fold (start, end_of_start))
	  {
	     fold_this_fold (start, end, end_of_start, end_of_end, 0, 1);
	     narrow ();
	     bob ();
	  }
	else 
	  {
	     pop_mark_1 ();
	     break;
	  }
	goto_spot ();
	!if (is_line_hidden ())
	  break;
     }
   
   pop_spot ();
}

%}}}

define fold_close_this_fold () %{{{
{
   variable start, end, end_of_start, end_of_end;
   
   (start, end, end_of_start, end_of_end) = fold_get_marks ();
   
   !if (fold_find_marker_line (start, end_of_start))
     error ("Unable to find fold-start");
   
   fold_this_fold (start, end, end_of_start, end_of_end, 1, 1);
   skip_hidden_lines_backward (1);
}
%}}}

#iffalse
define fold_close_fold () %{{{
{
   variable start, end, end_of_start, end_of_end;
   variable beg_mark, end_mark, orig_mark;
   variable not_in_a_fold = "Not in a fold.";
   variable end_line;
   
   (start, end, end_of_start, end_of_end) = fold_get_marks ();
   
   orig_mark = create_user_mark ();
   
   ERROR_BLOCK
     {
	goto_user_mark (orig_mark);
     }
   
   EXIT_BLOCK
     {
	fold_this_fold (start, end, end_of_start, end_of_end, 1, 1);
	skip_hidden_lines_backward (1);
     }
   
   if (fold_is_marker_line (start, end_of_start))
     {
	!if (down_1())
	  return;
	is_line_hidden();
	goto_user_mark (orig_mark);
	!if (())
	  return;
     }
 
   beg_mark = create_user_mark ();
   
   if (fold_is_marker_line (end, end_of_end))
     go_up_1 ();
   
   end_mark = create_user_mark ();

   forever
     {
	goto_user_mark (end_mark);
	
	end_line = 0;
	
	if (fold_find_marker_line_reverse (end, end_of_end, 0))
	  {
	     if (up_1 ())
	       end_line = what_line ();
	     
	     move_user_mark (end_mark);
	  }
	
	goto_user_mark (beg_mark);
	
	!if (up_1 ()) break;
	if (fold_find_marker_line_reverse (start, end_of_start, 0))
	  {
	     if (not(end_line)
		 or (what_line () > end_line))
	       break;
	     move_user_mark (beg_mark);
	  }
	else error (not_in_a_fold);
     }
}

%}}}
#else
define fold_close_fold () %{{{
{
   variable start, end, end_of_start, end_of_end;
   variable beg_mark, end_mark;
   variable not_in_a_fold = "Not in a fold.";
   variable end_line;
   
   (start, end, end_of_start, end_of_end) = fold_get_marks ();
   
   push_spot();
   
   if (
# if (_slang_version >= 20100)
       fold_is_marker_line (start, end_of_start) && down_1()
# else
       andelse {fold_is_marker_line (start, end_of_start)}{down_1()}
# endif
       )
       {
        is_line_hidden();
        go_up_1();
        !if (())
          {
             fold_this_fold (start, end, end_of_start, end_of_end, 1, 1);
             pop_spot();
             return;
          }
     }
 
   beg_mark = create_user_mark ();
   end_mark = create_user_mark ();

   forever
     {
        if (
# if (_slang_version >= 20100)
	    up_1() && fold_find_marker_line_reverse (end, end_of_end, 0)
# else
	    andelse {up_1()}{fold_find_marker_line_reverse (end, end_of_end, 0)}
# endif
	    )
          {
             end_line = what_line ();
             move_user_mark (end_mark);
          }
        else
          end_line= 0;

        goto_user_mark (beg_mark);
        
        if (
# if (_slang_version >= 20100)
	    up_1() && fold_find_marker_line_reverse (start, end_of_start, 0)
# else
	    andelse{up_1()}{fold_find_marker_line_reverse (start, end_of_start, 0)}
# endif
	    )
          {
             move_user_mark (beg_mark);
             if ( what_line () > end_line) break;
          }
        else
          {
             pop_spot ();
             error (not_in_a_fold);
          }
        goto_user_mark (end_mark);
     }
   
   fold_this_fold (start, end, end_of_start, end_of_end, 1, 1);
   pop_spot();
   goto_user_mark (beg_mark);
   bol();
}
%}}}
#endif

private define fold_exit_fold_internal () %{{{
{
   !if (count_narrows ())
     {
	error ("You are not in a fold.");
	return;
     }
   
   bob ();
   widen ();
   
   fold_close_this_fold ();
}

%}}}

define fold_exit_fold () %{{{
{
   fold_exit_fold_internal ();
   recenter (window_info ('r') / 2);
}

%}}}

define fold_fold_region () %{{{
{
   variable start, end, end_of_start, end_of_end;

   check_region (0);
   (start, end, end_of_start, end_of_end) = fold_get_marks ();
   
   % We have a canonical region with point at end.  See if this line
   % is the start of a fold.  If so, extend it to cover all of fold.
   
   if (fold_is_fold (start, end_of_start))
     {
	skip_hidden_lines_forward (1);
	!if (is_line_hidden ())
	  go_up_1 ();
     }
   
   narrow ();
   eob ();
   newline ();
   insert (end);
   insert (end_of_end);
   newline ();

   bob ();
   % Now look at position of beginning of region.  If it does not occur on
   % a blank line, put fold marks at end of line.
   skip_white ();
   if (eolp ())
     {
	bol ();
	insert (start);
	push_spot ();
	insert (end_of_start);
	newline ();
     }
   else
     {
	eol ();
	trim ();
	insert_single_space ();
	insert (start);
	insert (end_of_start);
	bol ();
	push_spot ();
     }

   fold_exit_fold_internal ();
   pop_spot ();
}

%}}}

%}}}

define fold_parse_errors () %{{{
{
   variable folded;
   
   % compile_parse_errors will widen buffer the buffer.  As a result, when it
   % returns, the buffer will be unfolded but some lines may be hidden.  Simply
   % unhide all lines.  Also take care to reenter a fold only if buffer is not
   % folded.
   compile_parse_errors ();
   
   push_spot ();
   eob ();
   skip_hidden_lines_backward (0);
   folded = is_line_hidden ();
   pop_spot ();
   
   
   if (folded)
     {
	push_spot ();
	fold_open_buffer ();
	fold_whole_buffer ();
	pop_spot ();
	if (is_line_hidden ()) fold_enter_fold ();
     }
}

%}}}

#ifdef HAS_BLOCAL_VAR
define fold_goto_bookmark_hook (mrk) %{{{
{
   variable folded;
   
   while (not (is_user_mark_in_narrow (mrk)))
     fold_exit_fold_internal ();
   
   goto_user_mark (mrk);
   fold_enter_fold ();
}

%}}}
#endif

define fold_bob_eob_error_hook (f) %{{{
{
   variable str = "Top Of Buffer.";
   variable start, end, end_of_start, end_of_end;
   
   if (f > 0) str = "End Of Buffer.";

   !if (Fold_Bob_Eob_Error_Action)
     error (str);
   
   !if (count_narrows () and (abs(f) == 1)) error (str);
   
   fold_exit_fold ();
   
   % The rest of this function is should be made optional, e.g.,
   % if (Optional_Flag) return;

   if (Fold_Bob_Eob_Error_Action == 1)
     return;

   bol ();
   if (f > 0)
     {
	skip_hidden_lines_forward (1);
	skip_chars (" \t\n");
     }
   else 
     {
	bskip_chars (" \t\n");
	skip_hidden_lines_backward (1);
     }
   
   (start, end, end_of_start, end_of_end) = fold_get_marks ();
   
   if (fold_is_fold (start, end_of_start))
     fold_enter_fold ();
}

%}}}

%{{{ mouse interface 
$1 = "mouse_goto_position";
$2 = "mouse";
!if (is_defined ($1)) autoload ($1, $2);

define fold_mouse_2click (line, col, but, shift) %{{{
{
   variable start, end, end_of_start, end_of_end;
   if (but == 1)
     {
	(start, end, end_of_start, end_of_end) = fold_get_marks ();
	mouse_goto_position (col, line);
	
	ERROR_BLOCK
	  {
	     _clear_error ();
	  }
	if (fold_is_fold (start, end_of_start))
	  fold_enter_fold ();
	else
	  fold_exit_fold ();
	
	return 1;
     }
   
   return -1;
}

%}}}

%}}}

define fold_goto_line ()
{
   variable n = read_mini ("Goto Line:", "", "");
   !if (strlen (n))
     return;
   n = integer (n);
   fold_open_buffer ();
   goto_line (n);
}

%{{{ Interactive searching functions 

define fold_search_line_ok () %{{{
{
   not (is_line_hidden ());
}

%}}}

$1 = "search_generic_search";
$2 = "search";
!if (is_defined ($1)) autoload ($1, $2);

define fold_search_backward () %{{{
{
   search_generic_search ("Fold search backward:", -1, &fold_search_line_ok);
}

%}}}

define fold_search_forward () %{{{
{
   search_generic_search ("Fold search forward:", 1, &fold_search_line_ok);
}

%}}}

%}}}


define folding_mode () %{{{
{
   variable s, s1, e, e1;
   
   if (Fold_Mode_Ok == 0)
     {
	if (1 != get_yes_no ("Folding mode not enabled.  Enable it"))
	  return;
	Fold_Mode_Ok = 1;
     }

#ifdef HAS_BLOCAL_VAR
   (s, e, s1, e1) = fold_get_marks_for_mode ();
   
   define_blocal_var ("fold_start", s);
   define_blocal_var ("fold_end_of_start", s1);
   define_blocal_var ("fold_end", e);
   define_blocal_var ("fold_end_of_end", e1);
   define_blocal_var ("bookmark_narrow_hook", ".fold_goto_bookmark_hook");
#endif
   
   local_setkey_reserved ("fold_whole_buffer",		"^W");
   local_setkey_reserved ("fold_enter_fold",		">");
   local_setkey_reserved ("fold_exit_fold",		"<");
   local_setkey_reserved ("fold_open_buffer",		"^O");
   local_setkey_reserved ("fold_open_fold",		"^S");
   local_setkey_reserved ("fold_close_fold",		"^X");
   local_setkey_reserved ("fold_fold_region",		"^F");
   local_setkey_reserved ("fold_search_forward",	"f");
   local_setkey_reserved ("fold_search_backward",	"b");
   
   set_buffer_hook ("bob_eob_error_hook", "fold_bob_eob_error_hook");
#iffalse
   set_buffer_hook ("mouse_2click", "fold_mouse_2click");
#endif
   loop (which_key ("compile_parse_errors"))
     {
	local_setkey ("fold_parse_errors", exch ());
     }
   
   fold_whole_buffer ();

   run_mode_hooks ("fold_mode_hook");
}
%}}}

define c_fold_buffer () %{{{
{
   bob ();
   !if (ffind ("mode: fold"))
     {
	insert ("/* -*- mode: C; mode: fold; -*- */\n");
     }
   folding_mode ();
   fold_open_buffer ();
   
   while (bol_fsearch ("{"))
     {
	eol ();
	push_mark ();

	go_up_1 ();		       %  at eol
	!if (re_bsearch ("^[a-z_A-Z]"))
	  {
	     pop_mark_1 ();
	     continue;
	  }
	
	if (ffind ("{{{"))
	  {
	     pop_mark_1 ();
	     continue;
	  }
	
	exchange_point_and_mark ();
	
	!if (bol_fsearch ("}"))
	  {
	     pop_mark_1 ();
	     error ("Matching } at bol not found.");
	  }
	go_down_1 ();
	trim ();
	!if (eolp ())
	  {
	     newline ();
	     go_up_1 ();
	  }
	push_spot ();
	fold_fold_region ();
	pop_spot ();
     }
   
   fold_whole_buffer ();
   bob ();
}

%}}}

% Fold menu support

private define fold_menu_callback (m)
{
#ifdef HAS_BLOCAL_VAR
   !if (blocal_var_exists ("fold_start"))
     {
	menu_append_item (m, "Enable &Folding", "folding_mode");
	return;
     }
#endif
   menu_append_item (m, "&Fold Buffer", "fold_whole_buffer");
   menu_append_item (m, "&Unfold Buffer", "fold_open_buffer");
   menu_append_item (m, "&Enter Fold", "fold_enter_fold");
   menu_append_item (m, "E&xit Fold", "fold_exit_fold");
   menu_append_item (m, "&Open Fold", "fold_open_fold");
   menu_append_item (m, "&Close Fold", "fold_close_fold");
   menu_append_item (m, "Fold &Region", "fold_fold_region");
   menu_append_item (m, "&Search Forward", "fold_search_forward");
   menu_append_item (m, "S&earch Backward", "fold_search_backward");
}


private define install_fold_menus ()
{
   menu_delete_item ("Global.&Buffers.Enable &Folding");
   menu_append_popup ("Global.&Buffers", "&Folding");
   menu_set_select_popup_callback ("Global.&Buffers.&Folding", &fold_menu_callback);
}

if (Menu_Popups_Loaded)
  install_fold_menus ();
else
  add_to_hook ("load_popup_hooks", &install_fold_menus);
   
