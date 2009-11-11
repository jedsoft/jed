% These routines user 'user_marks' to implement book marks.  A book mark 
% may be placed in any buffer and returning to the mark may cause a change
% of buffer.

% The only functions that are considered external are 'bkmrk_set_mark'
% and 'bkmrk_goto_mark'.  These functions prompt for a a key '0' - '9'
% or a SPACE.


private variable Book_Marks = Mark_Type [10];
private variable Bkmrk_Last_Position = NULL;

private define bkmrk_get_or_set_mark (get)
{
   variable n;
   variable prompt;
   variable m;
   
   if ((Bkmrk_Last_Position != NULL) and get)
     get = 2;

   prompt = "Bookmark number:";
   if (get == 2)
     prompt = "Bookmark number or SPACE for last position:";
   
   n = get_mini_response (prompt);
   
   if ((get == 2) and (n == ' '))
     return Bkmrk_Last_Position;

   n -= '0';

   if ((n < 0) or (n > 9)) error ("Number must be less than 10");
   
   if (get)
     return Book_Marks[n];

   Book_Marks[n] = create_user_mark ();
   vmessage ("Bookmark %d set.", n);
}

define bkmrk_set_mark ()
{
   bkmrk_get_or_set_mark (0);
}

define bkmrk_goto_mark ()
{
   variable mrk = bkmrk_get_or_set_mark (1);

   if (mrk == NULL)
     error ("Bookmark has not been set");
   

   Bkmrk_Last_Position = create_user_mark ();

   sw2buf (mrk.buffer_name);
   !if (is_user_mark_in_narrow (mrk))
     {
#ifdef HAS_BLOCAL_VAR
	variable fun;
	try
	  {
	     fun = get_blocal_var ("bookmark_narrow_hook");
	     mrk; eval (fun);
	  }
	catch AnyError:
	  throw UsageError, "Mark lies outside visible part of buffer.";
#else
	error ("Mark lies outside visible part of buffer.");
#endif
     }
   

   goto_user_mark (mrk);
   message ("done");
}
