static variable Failed = 0;
static define new_buffer (str)
{
   setbuf ("*scratch*");
   erase_buffer ();
   insert (str);
   bob ();
}
static define buffer_contents ()
{
   bob ();
   push_mark ();
   eob ();
   return bufsubstr ();
}

static define test_replace (str, s, t)
{
   variable new_s;

   new_buffer (str);

   replace (s, t);
   (new_s,) = strreplace (str, s, t, strlen (str));
   
   if (new_s != buffer_contents ())
     {
	(s,) = strreplace (s, "\n", "\\n", strlen(s));
	(t,) = strreplace (t, "\n", "\\n", strlen(t));
	() = fprintf (stderr, "failed to replace \"$s\" with \"$t\"\n"$, s, t);
	Failed++;
     }
}
static define main ()
{
   test_replace ("\n\n\n\n\nx\n\ny", "x", "t");
   test_replace ("\n\n\n\n\nx\n\ny", "\n", "t");
   test_replace ("\n\n\n\n\nx\n\ny", "\n", "\ny");
   test_replace ("\n\n\n\n\nx\n\ny", "\n", "x");
}
main ();

exit (Failed);
