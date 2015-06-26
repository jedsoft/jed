private define skip_chars_to_mark (chars, mark)
{
   skip_chars (chars);
   if (create_user_mark () >= mark)
     {
	goto_user_mark (mark);
	return 1;
     }
   return 0;
}

private define skip_word_chars_to_mark (mark)
{
   skip_word_chars ();
   if (create_user_mark () > mark)
     goto_user_mark (mark);
}

private define chgcase_reg ()
{
   check_region (0);
   variable end = create_user_mark ();
   pop_mark_1 ();
   while (create_user_mark () < end)
     {
	push_mark ();
	variable at_end = skip_chars_to_mark ("^\\u", end);
	insert (strup (bufsubstr_delete ()));
	if (at_end) break;
	push_mark ();
	at_end = skip_chars_to_mark ("^\\l", end);
	insert (strlow (bufsubstr_delete ()));
	if (at_end) break;
     }
}

private define cap_region ()
{
   check_region (0);
   variable end = create_user_mark ();
   pop_mark_1 ();
   while (create_user_mark () < end)
     {
	skip_non_word_chars ();
	if (create_user_mark () >= end)
	  break;
	variable wch = what_char ();
	% The insertion must be done before the deletion to handle a single
	% character word.  This is because the "end" mark follows the character
	% and if the deletion took place first, the insertion would happen
	% AFTER the mark, which is not what is wanted.
	insert (strup (char (wch)));
	del ();
	push_mark ();
	skip_word_chars_to_mark (end);
	insert (strlow (bufsubstr_delete ()));
     }
}

public define xform_region (how)
{
   check_region (0);
   variable f;

   switch (how)
     {
      case 'u':
	f = &strup;
     }
     {
      case 'd':
	f = &strlow;
     }
     {
      case 'c':
	return cap_region ();
     }
     {
	% default
	return chgcase_reg ();
     }
   () = dupmark ();
   variable reg = (@f)(bufsubstr ());
   del_region ();
   insert (reg);
}
