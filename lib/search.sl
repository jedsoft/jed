%
%  forward and backward search functions.  These functions can search across
%  lines.
%

require ("srchmisc");

define search_across_lines (str, dir)
{
   variable n, s, s1, fun, len;

   len = strlen (str);
   fun = &fsearch;
   if (dir < 0) fun = &bsearch;

   n = is_substr (str, "\n");

   !if (n)
     {
	if (@fun (str)) return len;
	return -1;
     }
   s = substr (str, 1, n);
   s1 = substr (str, n + 1, strlen(str));
   n = strlen(s);
   
   push_mark ();
   
   while (@fun(s))
     {
	% we are matched at end of the line.
	go_right (n);
	if (looking_at(s1)) 
	  {
	     go_left(n);
	     pop_mark_0 ();
	     return len;
	  }
	if (dir < 0) go_left (n);
     }
   pop_mark_1 ();
   -1;
}

private define setup_case_search (pat)
{
   variable cs = CASE_SEARCH;
   if (strlow (pat) != pat)
     {
	CASE_SEARCH = 1;
     }
   
   return cs;
}

define search_generic_search (prompt, dir, line_ok_fun)
{
   variable str, not_found = 1;

   str = read_mini(prompt, LAST_SEARCH, Null_String);
   !if (strlen(str)) return;
   
   push_mark ();
   
   variable cs = setup_case_search (str);
   ERROR_BLOCK 
     {
	pop_mark (not_found);
	CASE_SEARCH = cs;
     }

   if ((dir > 0) and looking_at(str)) go_right_1 ();
   
   save_search_string (str);
   
   not_found = not (search_maybe_again (&search_across_lines, str, dir,
					line_ok_fun));
   if (not_found) verror ("%s: not found.", str);
   
   EXECUTE_ERROR_BLOCK;
}

define search_forward ()
{
   search_generic_search ("Search forward:", 1, &_function_return_1);
}

define search_backward ()
{   
   search_generic_search ("Search backward:", -1, &_function_return_1);
}


define replace_do_replace (str, len)
{
   replace_chars (len, str);	       %  returns strlen (str)
#iffalse
   push_mark ();
   go_right(len);
   del_region ();
   insert (str);
   strlen (str);
#endif
}
define search_search_function (pat)
{
   variable cs = setup_case_search (pat);
   EXIT_BLOCK
     {
	CASE_SEARCH = cs;
     }
   variable len = strlen (pat);
   if (search_across_lines (pat, 1) > 0) return len;
   return -1;
}

define replace_cmd ()
{
   variable pat, prompt, rep;
   variable has_prefix;
   
   pat = read_mini("Replace:", Null_String, Null_String);
   !if (strlen (pat)) return;
   
   prompt = strcat (strcat ("Replace '", pat), "' with:");
   rep = read_mini(prompt, "", "");
   
   variable cs = setup_case_search (pat);
   ERROR_BLOCK
     {
	CASE_SEARCH = cs;
     }

   replace_with_query (&search_search_function, pat, rep, 1, &replace_do_replace);
   
   EXECUTE_ERROR_BLOCK;

   message ("done.");   
}

provide ("search");
