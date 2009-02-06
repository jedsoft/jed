%enable_profiling ();
private define fixup_line ()
{
   variable col;

   !if (parse_to_point ())
     {
	col = what_column ();
	bol_skip_white ();
	if (col != what_column ())
	  {
	     goto_column (col);
	     indent_line ();
	     if (C_BRA_NEWLINE) newline ();
	  }
	go_right_1 ();
	trim ();
	!if (eolp () or looking_at_char (',') or looking_at_char (';'))
	  {
	     indent_line ();
	     newline ();
	  }
     }
   go_right_1 ();
}

define c_indent_buffer ()
{
   variable line = -50, max_line;
   push_spot ();
   eob ();
   max_line = what_line ();
   bob ();
   do
     {
	bol_skip_white ();
	if (looking_at ("/*"))
	  {
	     % skip the comment 
	     () = fsearch ("*/");
	     continue;
	  }
	
	eol ();
	if (blooking_at ("\\"))
	  {
	     indent_line ();
	     % skip all continuation lines.
	     while (down (1))
	       {
		  eol ();
		  !if (blooking_at ("\\")) break;
	       }
	     continue;
	  }
	trim ();
	bol_skip_white ();
	!if (looking_at_char ('{'))
	  {
	     variable this_line = what_line ();
	     % I do not want to touch constructs such as x = {1, 3};
	     while (ffind_char ('{')
		    and parse_to_point ())
	       go_right_1 ();
	     
	     if (looking_at_char ('{'))
	       {
		  variable m = create_user_mark ();
		  if (find_matching_delimiter ('{')
		      and (this_line == what_line ()))
		    {
		       eol ();
		       indent_line ();
		       continue;
		    }
		  goto_user_mark (m);
	       }
	     bol ();
	     while (ffind_char ('}'))
	       fixup_line ();
	     bol ();
	     while (ffind_char ('{'))
	       fixup_line ();
	  }

	indent_line ();
	if (line + 50 < what_line ())
	  {
	     line = what_line ();
	     flush (sprintf ("processed %d/%d lines.", line, max_line));
	  }
	
	%update_sans_update_hook (1);
     }
   while (down_1 ());
   trim_buffer ();
   flush (sprintf ("processed %d/%d lines.", what_line (), max_line));
   pop_spot ();
}

	
	     
