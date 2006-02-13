_debug_info = 1;
private define chgcase_reg ()
{
   narrow_to_region ();
   bob ();
   while (not eobp ())
     {
	push_mark ();
	skip_chars ("^\\u");
	insert (strup (bufsubstr_delete ()));
	push_mark ();
	skip_chars ("^\\l");
	insert (strlow (bufsubstr_delete ()));
     }
   widen_region ();
}

private define cap_region ()
{
   narrow_to_region ();
   bob ();
   forever 
     {
	skip_non_word_chars ();
	if (eobp ())
	  break;
	variable wch = what_char ();
	del ();
	insert (strup (char (wch)));
	push_mark ();
	skip_word_chars ();
	insert (strlow (bufsubstr_delete ()));
     }
   widen_region ();
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
   dupmark ();
   variable reg = (@f)(bufsubstr ());
   del_region ();
   insert (reg);
}


