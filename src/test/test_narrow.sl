private variable Failed = 0;

public define test_narrow (str, narrow_str)
{
   setbuf ("*scratch*");
   
   c_mode ();

   erase_buffer ();
   insert (str);
   bob ();
   
   ifnot (fsearch (narrow_str))
     error ("fsearch failed");
   
   push_mark ();
   go_right (strlen (narrow_str));
   narrow_to_region ();
   
   () = parse_to_point ();

   bob ();
   push_mark ();
   eob ();
   variable b = bufsubstr ();

   if (b != narrow_str)
     {
	Failed++;
	() = fprintf (stderr, "narrow_to_region produced %s\n", b);
     }
   
   widen_region ();
   bob ();
   push_mark ();
   eob ();
   b = bufsubstr ();

   if (b != str)
     {
	Failed++;
	() = fprintf (stderr, "widen_region failed %s\n", b);
     }
}
	
test_narrow ("A single line", "single");
test_narrow ("\nNewline + A single line", "single");
test_narrow ("\nNewline + A single line+newline", "single");
exit (Failed);
