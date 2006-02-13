%
%  Interactive Regular expression searches.  These highlight region matched
%  until next key is pressed.
%

require ("srchmisc");

define re_search_dir (pat, dir)
{
   variable ret;
   if (dir > 0) ret = re_fsearch (pat); else ret = re_bsearch (pat);
   ret--; ret;
}

define re_search_forward()
{
   variable pat, not_found = 1;
   pat = read_mini("Search (Regexp):", Null_String, Null_String);
   !if (strlen(pat)) return;
   
   push_mark();
   ERROR_BLOCK 
     {
	pop_mark (not_found);
     }
   
   not_found = not (search_maybe_again (&re_search_dir, pat, 1, 
					&_function_return_1));
   if (not_found) error ("Not found.");
   EXECUTE_ERROR_BLOCK;
}

define re_search_backward()
{
   variable pat, not_found;
   pat = read_mini("Backward Search (Regexp):", Null_String, Null_String);
   !if (strlen(pat)) return;
   
   push_mark();
   ERROR_BLOCK 
     {
	pop_mark (not_found);
     }
   
   not_found = not (search_maybe_again (&re_search_dir, pat, -1, 
					&_function_return_1));

   if (not_found) error ("Not found.");
   EXECUTE_ERROR_BLOCK;
}

private define research_search_function (pat)
{
   re_fsearch (pat) - 1;
}


private define re_replace_function (str, len)
{
   !if (replace_match(str, 0))
     error ("replace_match failed.");
   -2;
}


define query_replace_match()
{
   variable pat, n, rep, prompt, doit, err, ch;
   
   err = "Replace Failed!";
   pat = read_mini("Regexp:", Null_String, Null_String);
   !if (strlen(pat)) return;
   prompt = strcat (strcat ("Replace '", pat), "' with:");
   rep = read_mini(prompt, Null_String, Null_String);

   replace_with_query (&research_search_function, pat, rep, 1, 
		       &re_replace_function);
}

