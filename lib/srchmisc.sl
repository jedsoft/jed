%  These routines are common to both regular expression searches and ordinary
%  searches.

define mark_next_nchars (n, dir)
{
   variable h;
   ERROR_BLOCK 
     {
	set_line_hidden (h); pop_mark_0 ();
     }

   h = is_line_hidden ();
   set_line_hidden (0);
   push_visible_mark ();
   go_right (n);
   if (dir < 0) exchange_point_and_mark ();
   update(1);
   ungetkey(getkey());
   EXECUTE_ERROR_BLOCK;
}



% The search function is to return: 0 if non-match found or the length of the 
% item matched.
% search_fun takes the pattern to search for and returns the length of the 
% pattern matched.  If no match occurs, return -1.
% rep_fun returns the length of characters replaced.

define replace_with_query (search_fun, pat, rep, query, rep_fun)
{
   variable n, prompt, doit, err, ch, pat_len;
   variable undo_stack_type = struct
     {
	rep_len,
	prev_string,
	user_mark,
	next
     };
   variable undo_stack = NULL;
   variable tmp;
   variable replacement_length = strlen (rep);

   prompt =  sprintf ("Replace '%s' with '%s'? (y/n/!/+/q/h)", pat, rep);
   
   while (pat_len = @search_fun (pat), pat_len >= 0)
     {
	!if (query)
	  {
	     %tmp = create_user_mark ();
	     () = @rep_fun (rep, pat_len);
	     if ((pat_len == 0)
		 %and (tmp == create_user_mark ())
		 )
	       go_right_1 ();
	     continue;
	  }

	do 
	  {
	     message(prompt);
	     mark_next_nchars (pat_len, -1);
	     
	     ch = getkey ();
	     if (ch == 'r')
	       {
		  recenter (window_info('r') / 2);
	       }
	     
	  } while (ch == 'r');
	
	switch(ch)
	  { case 'u' and (undo_stack != NULL) :
	     goto_user_mark (undo_stack.user_mark);
	     push_spot ();
	     () = @rep_fun (undo_stack.prev_string, undo_stack.rep_len);
	     pop_spot ();
	     undo_stack = undo_stack.next;
	  }   
	  { case 'y' :
	     tmp = @undo_stack_type; 
	     tmp.next = undo_stack;
	     undo_stack = tmp;

	     push_spot(); push_mark ();
	     go_right (pat_len); undo_stack.prev_string = bufsubstr ();
	     pop_spot (); 
	     undo_stack.user_mark = create_user_mark ();
	     undo_stack.rep_len  = @rep_fun (rep, pat_len);
	     if (pat_len == 0)
	       go_right(1);
	  }
	  { case 'n' : go_right_1 ();}
	  { case '+' : () = @rep_fun (rep, pat_len);
	     if (pat_len == 0)
	       go_right(1);
	     break;
	  }
	  { case '!' :
	     query = 0;
	  }
          { case 'q' : break; }
          {
	     flush ("y:replace, n:skip, !:replace all, u: undo last, +:replace then quit, q:quit");
	     () = input_pending (30); 
	  }
     }
}


define search_maybe_again (fun, str, dir, match_ok_fun)
{
   variable ch, len;
   
   while (len = @fun (str, dir), len >= 0)
     {	
	if (@match_ok_fun ())
	  {
	     if (EXECUTING_MACRO or DEFINING_MACRO) return 1;
	     message ("Press RET to continue searching.");
	     mark_next_nchars (len, -1);
	     ch = getkey ();
	     if (ch != '\r')
	       {
		  ungetkey (ch);
		  return 1;
	       }
	  }
	if (dir > 0) go_right_1 ();
     }
   return 0;
}

%!%+
%\function{toggle_case_search}
%\synopsis{Toggle the CASE_SEARCH variable}
%\usage{Void toggle_case_search ()}
%\seealso{CASE_SEARCH}
%!%-
public define toggle_case_search ()
{
   variable off_on = ["Off", "On"];
   CASE_SEARCH = not(CASE_SEARCH);
   vmessage("Case Search %s for this buffer", off_on[CASE_SEARCH]);
}

provide ("srchmisc");
