private variable Failed = 0;
private define test_rebsearch ()
{
   setbuf ("*scratch*");
   erase_buffer ();
   variable n = 20;
   loop(20)
     newline ();
   
   loop (n)
     {
	if (0 == re_bsearch ("^\n"))
	  {
	     ()=fprintf (stderr, "re_bsearch for newlines failed");
	     Failed++;
	     return;
	  }
     }
   ifnot (bobp ())
     {
	()=fprintf(stderr,"re_bsearch: expected to be at the bob");
	Failed++;
	return;
     }
}
test_rebsearch ();

private define test_search_char ()
{
   if (0 == _slang_utf8_ok ())
     {
	() = fprintf (stdout, "UTF-8 mode not enabled-- test_search_char not run.\n");
	return;
     }

   setbuf ("*scratch*");
   erase_buffer ();
   insert ("foo\x{ABCD}bar\n");
   bob ();
   if (1 != ffind_char (0xABCD))
     {
	() = fprintf (stderr, "ffind_char: failed to find a wide-character\n");
	Failed++;
     }
   eol ();
   if (1 != bfind_char (0xABCD))
     {
	() = fprintf (stderr, "ffind_char: failed to find a wide-character\n");
	Failed++;
     }
   if (1 != looking_at_char (0xABCD))
     {
	() = fprintf (stderr, "looking_at_char: failed\n");
	Failed++;
     }
}
test_search_char ();

exit (Failed);
