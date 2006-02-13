% This file contains some routines for dealing with buffers that are used for
% composing email

% Returns 1 if found, 0 otherwise
public define mailutils_find_header_separator ()
{
   bob ();
   if (bol_fsearch ("--- Do not modify this line.  Enter your message below ---\n"))
     return 1;
   return bol_fsearch ("\n");
}

% Returns 0 upon sucess, -1 upon failure
public define mailutils_narrow_to_header ()
{
   bob ();
   push_mark ();
   if (mailutils_find_header_separator ())
     {
	narrow_to_region ();
	bob ();
	return 0;
     }
   pop_mark (0);
   return -1;
}

private define mark_this_keywords_value ()
{
   () = ffind (":");
   go_right_1 ();
   push_mark ();
   while (down (1))
     {
	skip_chars ("\t ");
	if (bolp ())
	  break;
	eol ();
     }
   go_left (1);
}

% If the header does not exist, the function returns NULL, else its value
public define mailutils_get_keyword_value (kw)
{
   push_spot ();
   EXIT_BLOCK 
     {
	pop_spot ();
     }
   if (-1 == mailutils_narrow_to_header ())
     return NULL;

   variable exists = bol_fsearch (kw);
   widen_region ();
   
   if (exists == 0)
     return NULL;
   
   mark_this_keywords_value ();
   return strtrim (bufsubstr ());
}

% If the field does not exist, it will be created.
public define mailutils_set_keyword_value (field, value)
{
   if (-1 == mailutils_narrow_to_header ())
     return;

   variable exists = bol_fsearch (field);
   widen_region ();
   if (exists == 0)
     {
	() = mailutils_find_header_separator ();
	% Do it this way to avoid moving a line mark
	go_left (1);
	vinsert ("\n%s", field);
	bol ();
     }
   mark_this_keywords_value ();
   del_region ();
   insert (" ");
   insert (strtrim (value));
}

provide ("mailutils");
