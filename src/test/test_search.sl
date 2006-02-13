static variable Failed = 0;
static define test_rebsearch ()
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
   !if (bobp ())
     {
	()=fprintf(stderr,"re_bsearch: expected to be at the bob");
	Failed++;
	return;
     }
}
test_rebsearch ();
exit (Failed);
