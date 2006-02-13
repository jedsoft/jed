% This function sorts a region of lines of the form:
%    keyname
%     text...
%    delimeter string
%    keyname
%     text...
%    delimeter string

define sort_region_internal (line_break_str, delim)
{
   check_region (0);
   narrow (); 
   
   bob ();

   do
     {
	push_mark ();
	!if (bol_fsearch (delim)) eob ();
	narrow ();
	ERROR_BLOCK
	  {
	     widen(); widen ();
	  }
	
	bob ();
	while (eol(), not(eobp()))
	  {
	     del (); insert (line_break_str);
	  }
	widen ();
     }
   while (down_1 ());
   
   mark_buffer ();
   goto_column (32);
   
   ERROR_BLOCK
     {
	variable len = strlen (line_break_str);
	bob ();
	while (fsearch (line_break_str)) 
	  {
	     deln (len); newline ();
	  }
	widen ();
     }
   
   sort ();
   EXECUTE_ERROR_BLOCK;
}
