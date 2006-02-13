%
%  This function executes a query-replace across all buffers attached to
%  a file.
%

require ("search");

define replace_across_buffer_files ()
{
   variable cbuf = whatbuf ();
   variable n = buffer_list ();
   variable buf, file, flags;
   variable pat, rep;
   
   pat = read_mini ("Replace:", Null_String, Null_String);
   !if (strlen (pat)) return;
   rep = read_mini ("Replace with:", Null_String, Null_String);
   
   push_spot ();		       %  save our location

   REPLACE_PRESERVE_CASE_INTERNAL = REPLACE_PRESERVE_CASE;
   if ((strlen (rep) == strlen(pat)) and not (strcmp(strlow(rep), strlow(pat))))
     REPLACE_PRESERVE_CASE_INTERNAL = 0;
   
   if (-1 != prefix_argument (-1))
     REPLACE_PRESERVE_CASE_INTERNAL = not (REPLACE_PRESERVE_CASE_INTERNAL);
   
   ERROR_BLOCK 
     {
	sw2buf (cbuf);
	pop_spot ();
	_pop_n (n);               %  remove buffers from stack
	REPLACE_PRESERVE_CASE_INTERNAL = 0;
     }

   while (n)
     {
	buf = ();  n--;
	
	% skip special buffers
	if ((buf[0] == '*') or (buf[0] == ' ')) continue;

	sw2buf (buf);
	
	(file,,,flags) = getbuf_info ();
	
	% skip if no file associated with buffer, or is read only
	!if (strlen (file) or (flags & 8)) continue;  
	
	% ok, this buffer is what we want.
	
	push_spot_bob ();
	ERROR_BLOCK
	  {
	     pop_spot ();
	  }
	
	replace_with_query (&search_search_function, pat, rep, 1, 
			    &replace_do_replace);
	pop_spot ();
     }
   
   EXECUTE_ERROR_BLOCK;
   message ("Done.");
}

	
	
	
   
