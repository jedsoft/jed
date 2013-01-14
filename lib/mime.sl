% Parse buffer full of MIME noise
% Note:  This is very primitive and should be fixed

try
{
   require ("mimemisc");
}
catch AnyError;

private variable Mime_Base64_Decode = __get_reference ("mime_base64_decode");
private variable Mime_Iconv = __get_reference ("mime_iconv");

define mime_parse_this_qp ()
{
   variable str;

   str = strcat ("0x", char (what_char ()));
   del ();
   str = strcat (str, char (what_char ()));
   del ();
   if (Integer_Type == _slang_guess_type (str))
     {
	insert_char (integer(str));
	return;
     }
   insert (str);
}

define mime_rfc1522_parse_buffer ()
{
   variable qpre;
   variable len;
   variable charset, encoding, str, encoded_string;

   % Look for things like =?iso-8859-1?Q?=E1end?=
   qpre = "=\\?\\([-_a-zA-Z0-9]+\\)\\?\\([QBqb]\\)\\?\\([^ \t]+\\)\\?=";

   push_spot ();
   bob ();
   while (len = re_fsearch (qpre), len)
     {
	len--;
	charset = regexp_nth_match (1);
	encoding = regexp_nth_match (2);
	str = regexp_nth_match (3);
	push_mark ();
	go_right (len);

	if (strup(encoding) == "B")
	  {
	     if (Mime_Base64_Decode == NULL)
	       continue;

	     encoded_string = bufsubstr_delete ();
	     variable e;
	     try (e)
	       {
		  str = (@Mime_Base64_Decode)(str);
	       }
	     catch AnyError:
	       {
		  vmessage ("Error [%S] encountered decoding mime: %S", e.descr, e.message);
	       }
	  }
	else
	  {
	     encoded_string = bufsubstr_delete ();
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
	     mark_buffer ();
	     str = bufsubstr_delete ();
	     widen_region ();
	  }
	if ((Mime_Iconv != NULL) && _slang_utf8_ok)
	  str = (@Mime_Iconv)("UTF-8", charset, str);

	insert (str);
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

