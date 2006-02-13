% Note the functions used here are not available on 16 bit systems.

private variable Kill_Buffer_Number = -1;
private variable Kill_Buffer_Yank_Number = -1;
private variable Kill_Buffer_Max_Number = -1;

% Instead of using a new user mark, I should probably set aside a bookmark
% to allow the user to goto it.
private variable Kill_Buffer_User_Mark;

private define append_or_prepend_copy_as_kill (fun)
{
   variable kill_fun = "%kill%";
   if (strcmp (LAST_KBD_COMMAND, kill_fun))
     {
	Kill_Buffer_Number++;
	if (Kill_Buffer_Number == KILL_ARRAY_SIZE)
	  {
	     Kill_Buffer_Number = 0;
	  }
	
	if (Kill_Buffer_Number > Kill_Buffer_Max_Number)
	  Kill_Buffer_Max_Number = Kill_Buffer_Number;

	copy_region_to_kill_array (Kill_Buffer_Number);
	Kill_Buffer_Yank_Number = Kill_Buffer_Number;
     }
   else
     {
	@fun (Kill_Buffer_Number);
     }
   
   set_current_kbd_command (kill_fun);
}

define yp_copy_region_as_kill ()
{
   append_or_prepend_copy_as_kill (&append_region_to_kill_array);
}


define yp_kill_region ()
{
   () = dupmark ();
   yp_copy_region_as_kill ();
   del_region ();
}

define yp_prepend_copy_region_as_kill ()
{
   append_or_prepend_copy_as_kill (&prepend_region_to_kill_array);
}

define yp_prepend_kill_region ()
{
   () = dupmark ();
   yp_prepend_copy_region_as_kill ();
   del_region ();
}


define yp_kill_line ()
{
   variable one;
   variable kill_fun = "%kill%";

   one = eolp () or (KILL_LINE_FEATURE and bolp ());

   mark_to_visible_eol ();
   go_right (one);
   yp_kill_region ();
}

define yp_yank ()
{
   Kill_Buffer_User_Mark = create_user_mark ();
   insert_from_kill_array (Kill_Buffer_Yank_Number);
   set_current_kbd_command ("%yank%");
}

define yp_yank_pop ()
{
   if (strcmp (LAST_KBD_COMMAND, "%yank%"))
     {
	error ("The last command must be a yank one.");
     }

   Kill_Buffer_Yank_Number--;
   if (Kill_Buffer_Yank_Number < 0)
     {
	Kill_Buffer_Yank_Number = Kill_Buffer_Max_Number;
     }
   
   %  Delete the previous yank 
   push_mark ();
   goto_user_mark (Kill_Buffer_User_Mark);
   del_region ();
   
   yp_yank ();
}   

define yp_kill_word ()
{
   push_mark(); skip_word(); 
   yp_kill_region ();
}

define yp_bkill_word ()
{
   push_mark(); bskip_word(); 
   yp_prepend_kill_region ();
}


provide ("yankpop");
