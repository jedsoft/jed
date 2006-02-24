%
% JED mouse interface 
%
% These routines assume a 2 button mouse
%  
%          left :  set point to mouse point
%         right :  paste from cut buffer moving point to mouse point
%     drag left :  mark a region and copy it to cut buffer
%    shift left :  split window mouse is in
%  control left :  delete window mouse is in
%
%  Other buttons are undefined.
%

setkey ("mouse_set_point_open",		"\e^@Dl");   % left button down
setkey ("mouse_set_point_close",	"\e^@Ul");   % left button up
setkey ("mouse_yank_cutbuffer",		"\e^@Dm");   % middle down
setkey ("mouse_yank_cutbuffer",		"\e^@Dr");   % right down
setkey ("mouse_null",			"\e^@Um");   % middle up
setkey ("mouse_null",			"\e^@Ur");   % right up

% dragging
setkey ("mouse_drag",			"\e^@^@l");  % left dragging
setkey ("mouse_drag",			"\e^@^@r");  % right dragging
setkey ("mouse_null",			"\e^@^@^L"); % C-left dragging
setkey ("mouse_drag",			"\e^@^@^R"); % C-right dragging
setkey ("mouse_null",			"\e^@^@L");  % S-left dragging
setkey ("mouse_null",			"\e^@^@R");  % S-right dragging

% shifted
setkey ("mouse_split_window",		"\e^@DL");   % Shift-left button down
setkey ("mouse_null",			"\e^@UL");   % left button up
setkey ("mouse_yank_from_jed",		"\e^@DM");   % middle down
setkey ("mouse_yank_from_jed",		"\e^@DR");   % right down
setkey ("mouse_null",			"\e^@UR");   % right up
setkey ("mouse_null",			"\e^@UM");   % middle up

% ctrl
setkey ("mouse_delete_window",		"\e^@D^L");  % left button down
setkey ("mouse_null",			"\e^@U^L");  % left button up
setkey ("mouse_null",			"\e^@D^M");  % middle down
setkey ("mouse_null",			"\e^@D^R");  % middle down
setkey ("mouse_null",			"\e^@U^M");  % middle up
setkey ("mouse_null",			"\e^@U^R");  % middle up

define mouse_null ();

% loop through windows moving the point to the mouse point
% if same_window is false, and mouse is on status line, call status_fun
% It returns zero if the status function was used, otherwise, it returns 1
define mouse_point_mouse (force, same_window, push, status_fun)
{
   variable n = nwindows ();
   variable top, bot, dy, col, want_col;
   
   while (n)
     {
        top = window_info('t');
	bot = window_info('r') + top;
	if ((MOUSE_Y >= top) and (MOUSE_Y < bot))
       	  { 
	     if (push) push_visible_mark ();
	     dy = MOUSE_Y - (top - 1 + window_line());
	     if (dy > 0) 
	       {       
	      	  dy -= down(dy);
	     	  eol();
	      	  if (force) loop (dy) newline();
	       }                                                   
	     else go_up(- dy);
	     
	     eol();
	     col = what_column ();
       	     want_col = window_info('c') + MOUSE_X - 1;
	     !if (force or (want_col <= col)) want_col = col;
	     () = goto_column_best_try(want_col);
	     return 1;
	  }
	
	if (same_window) 
	  {
	     if (push) push_visible_mark ();
       	     if (MOUSE_Y >= bot)
       	       {
       	     	  go_down(MOUSE_Y - bot + 1);
       	       }
       	     else
       	       {
       		  go_up(top - MOUSE_Y);
       	       }
	     x_warp_pointer ();
      	     return 1;
      	  }
	
      	if ((bot == MOUSE_Y) and (use_status_fun != NULL))
	  {
	     @status_fun ();
	     return 0;
	  }
	
	otherwindow();
	n--;
     }
   error ("Mouse not in a window.");
}
      
variable Mouse_Drag_Mode = 0;
variable Mouse_Buffer = " *Mouse buffer*";
variable Mouse_Delete_Region = 0;
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
   
   if (Mouse_Delete_Region) () = dupmark();
   () = dupmark();		       %/* for cut buffer */  
   x_copy_region_to_cutbuffer ();
   copy_region(Mouse_Buffer);
   if (Mouse_Delete_Region) 
     {
	Mouse_Delete_Region = 0;
	del_region();
     }
   message ("region copied.");
}


define mouse_next_buffer ()
{
   variable n, buf, cbuf = whatbuf ();
   
   n = buffer_list ();		       %/* buffers on stack */
   loop (n)
     {
	=buf;
	n--;
	if (buf[0] == ' ') continue;
	if (buffer_visible (buf)) continue;
	sw2buf (buf);
	loop (n) pop ();
	return;
     }
   error ("All buffers are visible.");
}

	
custom_variable ("Mouse_Save_Point_Mode", 1);
%!%+
%\variable{Mouse_Save_Point_Mode}
%\synopsis{Mouse_Save_Point_Mode}
%\usage{Integer Mouse_Save_Point_Mode = 1;}
%\description
% If this variable is non-zero, the editing point will be restored to its
% original position when the left button is used to copy a region to the 
% cutbuffer.  If the variable is zero, the current point is left at the 
% end of the copied region.
%!%-

private variable Mouse_Save_Point_Window;
private variable Mouse_Save_Point_Mark;

define mouse_set_point_open ()
{
   Mouse_Drag_Mode = 0;
   Mouse_Save_Point_Window = window_info ('t');
   Mouse_Save_Point_Mark = create_user_mark ();
   
   if (Mouse_Save_Point_Mode) Mouse_Save_Point_Mode = -1;
   
   if (mouse_point_mouse (0, 0, 0, &mouse_next_buffer)
       and Mouse_Save_Point_Mode)
     {
	Mouse_Save_Point_Mode = 1;
     }
}

define mouse_set_point_close ()
{
   if (Mouse_Drag_Mode) 
     {
	copy_kill_to_mouse_buffer ();
	Mouse_Drag_Mode = 0;
	if ((MOUSE_BUTTON == 1) and (Mouse_Save_Point_Mode > 0))
	  {
	     loop (nwindows ())
	       {
		  if (window_info ('t') == Mouse_Save_Point_Window)
		    {
		       if (whatbuf () == Mouse_Save_Point_Mark.buffer_name)
			 goto_user_mark (Mouse_Save_Point_Mark);
		       break;
		    }
		  otherwindow ();
	       }
	     Mouse_Save_Point_Mode = -1;
	  }
     }
   else if (MOUSE_BUTTON == 3)
     {
	dupmark(); pop();
	copy_kill_to_mouse_buffer ();
	pop_mark_1 ();
     }
}

define mouse_yank_from_jed ()
{
   () = mouse_point_mouse (0, 0, 0, NULL);
   if (bufferp(Mouse_Buffer)) insbuf(Mouse_Buffer);
}

define mouse_yank_cutbuffer ()
{
   variable w = window_info ('t');
   variable max_loops = nwindows ();
   variable m = create_user_mark ();
   if (mouse_point_mouse (0, 0, 1, &splitwindow))
     {
	pop_mark_1 ();		       %  since we set it in the function call
	while (max_loops and (w != window_info ('t')))
	  {
	     otherwindow ();
	     max_loops--;
	  }
	
	if (max_loops)
	  {
	     goto_user_mark (m);
	     () = x_insert_cutbuffer ();
	  }
     }
}
 
define delete_window ()
{
   call("delete_window");
}

define mouse_mark_and_copy ()
{
   Mouse_Drag_Mode = 0;
   () = mouse_point_mouse (0, 0, 1, &delete_window);
}

   
define mouse_drag ()
{
   !if (Mouse_Drag_Mode)
     {
	push_visible_mark ();
	Mouse_Drag_Mode = 1;
     }
   () = mouse_point_mouse (0, 1, 0, NULL);
   update_sans_update_hook (not(input_pending(0)));
}

   
define mouse_kill_region ()
{
   Mouse_Delete_Region = 1;
   mouse_mark_and_copy (0, 0, 1); 
}


define mouse_split_window ()
{
   () = mouse_point_mouse (0, 0, 0, NULL);
   splitwindow();
}

define mouse_delete_window ()
{
   () = mouse_point_mouse (0, 0, 0, NULL);
   delete_window ();
}

   
