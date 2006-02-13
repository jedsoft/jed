% Parse buffer full of MIME noise
% Note:  This is very primitive and should be fixed

define mime_parse_this_qp ()
{
   variable str;
   
   str = strcat ("0x", char (what_char ()));
   del ();
   str = strcat (str, char (what_char ()));
   del ();
   if (Integer_Type == _slang_guess_type (str))
     str = char (integer (str));
   
   insert (str);
}

define mime_rfc1522_parse_buffer ()
{
   variable qpre;
   variable len;
   variable charset, str;
   
   % Look for things like =?iso-8859-1?Q?=E1end?=
   qpre = "=\\?\\([-_a-zA-Z0-9]+\\)\\?Q\\?\\([^ \t]+\\)\\?=";
   
   push_spot ();
   bob ();
   while (len = re_fsearch (qpre), len)
     {
	len--;
	charset = regexp_nth_match (1);
	str = regexp_nth_match (2);
	push_mark ();
	go_right (len);
	del_region ();
	push_mark ();
	insert (str);
	narrow_to_region ();
	
	bob ();
	replace ("_", " ");
	while (fsearch ("="))
	  {
	     del ();
	     mime_parse_this_qp ();
	  }
	eob ();
	widen_region ();
     }
   pop_spot ();
}

define mime_qp_parse_buffer ()
{
   bob ();
   while (fsearch ("="))
     {
	del ();
	if (eolp ())
	  {
	     del ();
	     continue;
	  }
	
	mime_parse_this_qp ();
     }
}


	     
			   
	     
	     
	
