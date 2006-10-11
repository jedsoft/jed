% This file contains some routines for dealing with buffers that are used for
% composing email
%
% Public Functions:
%  Int_Type mailutils_find_header_separator ()
%  Int_Type mailutils_narrow_to_header ()
%  mailutils_get_keyword_value (kw)
%  mailutils_set_keyword_value (field, value)


%!%+
%\function{mailutils_find_header_separator}
%\synopsis{Searches for the header/body separator}
%\usage{Int_Type mailutils_find_header_separator ()}
%\description
%  This function searches for the line separating the mail headers
%  from the body.  It returns 1 if found, and 0 otherwise.
%\seealso{mailutils_narrow_to_header}
%!%-
public define mailutils_find_header_separator ()
{
   bob ();
   if (bol_fsearch ("--- Do not modify this line.  Enter your message below ---\n"))
     return 1;
   !if (re_looking_at ("[-A-Za-z0-9_]+: "))
     return 0;
   return bol_fsearch ("\n");
}


%!%+
%\function{mailutils_narrow_to_header}
%\synopsis{Narrow to the mail headers}
%\usage{Int_Type mailutils_narrow_to_header ()}
%\description
% This function narrows the buffer to the mail headers.  It returns 0
% if sucessful (headers were found), or -1 upon failure.
%\seealso{mailutils_find_header_separator, mailutils_set_keyword_value, widen}
%!%-
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


%!%+
%\function{mailutils_get_keyword_value}
%\synopsis{Obtain the value of a header keyword}
%\usage{String_Type mailutils_get_keyword_value (kw)}
%\description
% This function returns the value of a mail header specified by the
% \exmp{kw} parameter.  If no such header exists, \NULL will be
% returned.
%\notes
% This function does not preserve the editing point.
%\seealso{mailutils_set_keyword_value}
%!%-
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


%!%+
%\function{mailutils_set_keyword_value}
%\synopsis{Set the value of a mail header}
%\usage{mailutils_set_keyword_value (String_Type kw, String_Type val)}
%\description
% This function sets the value of the header keyword specified by the
% \exmp{kw} argument to that of the \exmp{val} argument.  If no such
% header exists, one will be created.  The editing point will be left
% at the end of the header's value.  If the header exists and has a
% value, then that value will be replace.
%\notes
% This function does not preserve the editing point.
%\seealso{mailutils_get_keyword_value, mailutils_find_header_separator}
%!%-
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
