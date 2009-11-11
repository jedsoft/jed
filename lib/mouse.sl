unsetkey ("\e^@");
setkey ("mouse_cmd", "\e^@");

custom_variable ("Mouse_Wheel_Scroll_Lines", 3);


%!%+
%\variable{Mouse_Selection_Word_Chars}
%\synopsis{Characters that delimit double-click selections}
%\usage{String_Type Mouse_Selection_Word_Chars}
%\description
% The value of this variable represents a set of characters that serve
% to delimit double-click selections.  The default value of this
% variable is 
%#v+
%    Mouse_Selection_Word_Chars = "^ \n\"%&'()*,;<=>?@[]^`{|}";
%#v-
% If the value of this variable is NULL, the word characters
% associated with the buffer will be used.
%\seealso{define_word}
%!%-
custom_variable ("Mouse_Selection_Word_Chars","^ \n\"%&'()*,;<=>?@[]^`{|}");

private variable Mouse_Drag_Mode = 0;
private variable Mouse_Save_Point_Mark;
private variable Mouse_Buffer = " *Mouse buffer*";
private variable Mouse_Delete_Region = 0;


define mouse_goto_position (col, line)
{
   goto_line (line);
   () = goto_column_best_try (col);
}

define mouse_yank_from_jed ()
{
   if (bufferp(Mouse_Buffer)) 
     insbuf(Mouse_Buffer);
}

define copy_kill_to_mouse_buffer ()
{
   variable cbuf = whatbuf ();
   variable pnt, n;
   % 
   % We are not going to copy to the pastebuffer if the region is nil
   %
   n = what_line(); pnt = _get_point (); 
   push_spot();
   pop_mark_1 ();
   if ((what_line() == n) and (_get_point () == pnt)) 
     {
	pop_spot();
	return;
     }
   push_mark();
   pop_spot();
   
   setbuf(Mouse_Buffer);
   erase_buffer ();
   setbuf (cbuf);
   
   if (Mouse_Delete_Region) 
     () = dupmark();
   () = dupmark();		       %/* for cut buffer */  
   x_copy_region_to_selection ();
   copy_region(Mouse_Buffer);
   if (Mouse_Delete_Region) 
     {
	Mouse_Delete_Region = 0;
	del_region();
     }
}

define mouse_down_hook (line, col, but, shift)
{
   variable l;  
   if (but == 8)
     {
	l = window_line();
	loop (Mouse_Wheel_Scroll_Lines) skip_hidden_lines_backward (1);

	bol();
	recenter(l);
	return 0;
     }
   if (but == 16)
     {
	l = window_line();
	loop (Mouse_Wheel_Scroll_Lines) skip_hidden_lines_forward (1);
	bol();
	recenter(l);
	return 0;
     }
   
   if (shift == 0)
     {
	if (but == 2)
	  {
	     mouse_set_current_window ();
	     () = x_insert_selection ();
	     return 0;
	  }
	
	if (is_visible_mark ())
	  {
	     if (but == 1)
	       {
		  pop_mark (0);
		  return 0;
	       }
	     
	     if (but == 4)
	       Mouse_Delete_Region = 1;
	       
	     copy_kill_to_mouse_buffer ();
	     return 0;
	  }

	Mouse_Drag_Mode = 0;
	Mouse_Save_Point_Mark = create_user_mark ();
	mouse_goto_position (col, line);
	return 0;
     }
   
   if (shift == 1)
     {
	if (but == 2)
	  {
	     mouse_set_current_window ();
	     mouse_yank_from_jed ();
	     return 0;
	  }
     }
   
   return -1;
}

define mouse_up_hook (line, col, but, shift)
{
   if (shift == 0)
     {
	if (but == 1)
	  {
	     if (Mouse_Drag_Mode)
	       {
		  Mouse_Drag_Mode = 0;
		  copy_kill_to_mouse_buffer ();
		  
		  if (whatbuf () == Mouse_Save_Point_Mark.buffer_name)
		    goto_user_mark (Mouse_Save_Point_Mark);

		  return 0;
	       }
	     return 1;
	  }
	
	if (but == 4)
	  {
	     Mouse_Drag_Mode = 0;
	     return 1;
	  }
     }
   
   return -1;
}

define mouse_drag_hook (line, col, but, shift)
{
   variable top, bot;
   variable y;
   
   !if (Mouse_Drag_Mode)
     {
	!if (is_visible_mark ())
	  {
	     push_visible_mark ();
	  }
	Mouse_Drag_Mode = 1;
     }
   mouse_goto_position (col, line);
   
   % only warp if pointer is outside window.
   top = window_info ('t');
   bot = top + window_info ('r');
   
   (,y, ) = mouse_get_event_info ();
   
   if ((y < top) or (y > bot))
     x_warp_pointer ();
   
   return 0;
}

define mouse_next_buffer ()
{
   variable n, buf, cbuf = whatbuf ();
   
   n = buffer_list ();		       %/* buffers on stack */
   loop (n)
     {
	buf = ();
	n--;
	if (buf[0] == ' ') continue;
	sw2buf (buf);
	loop (n) pop ();
	return;
     }
}

define mouse_status_up_hook (line, col, but, shift)
{
   if (shift == 2)
     {
	if (but == 1)
	  {
	     delbuf (whatbuf ());
	     return 0;
	  }
     }
   
   return -1;
}

define mouse_status_down_hook (line, col, but, shift)
{
   if (shift == 0)
     {
	if (but == 1) mouse_next_buffer ();
	else if (but == 2) splitwindow ();
	else
	  call ("delete_window");
	return 0;
     }
   
   if (shift == 1)
     {
	try
	  {
	     if (but == 1)
	       call ("page_down");
	     else if (but == 4)
	       call ("page_up");
	  }
	catch AnyError;
	return 0;
     }
      
   return -1;
}


define mouse_2click_hook (line, col, but, shift)
{
   variable word_chars = Mouse_Selection_Word_Chars;
   if (word_chars == NULL)
     word_chars = get_word_chars ();

   if (but == 1)
     {
	mouse_goto_position (col, line);
	push_spot ();
	bskip_chars (word_chars);
	push_visible_mark ();
	skip_chars (word_chars);
	update_sans_update_hook (1);
	usleep (500);
	
	copy_kill_to_mouse_buffer ();
	pop_spot ();
	return 0;
     }
   return -1;
}	


mouse_set_default_hook ("mouse_2click", "mouse_2click_hook");
mouse_set_default_hook ("mouse_up", "mouse_up_hook");
mouse_set_default_hook ("mouse_down", "mouse_down_hook");
mouse_set_default_hook ("mouse_drag", "mouse_drag_hook");
mouse_set_default_hook ("mouse_status_down", "mouse_status_down_hook");
mouse_set_default_hook ("mouse_status_up", "mouse_status_up_hook");
