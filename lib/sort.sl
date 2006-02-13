define sort_using_function (sort_fun)
{
   variable end_line, n, index, i, beg, begc, endc, keys;

   check_region(0);
   endc = what_column();
   end_line = what_line();
   pop_mark_1 ();
   beg = what_line();
  
   n = end_line - beg; ++n;	       %/* number of lines */
   begc = what_column();
  
   if (endc < begc)
     {
	endc; endc = begc; begc = ();
     }
   
   keys = String_Type [n];
  
  %
  % extract keys and fill array
  %

   goto_line(beg);
   for (i = 0; i < n; ++i)
     {
	goto_column(begc);
	push_mark_eol();
	if (what_column() > endc) goto_column(endc);
	keys[i] = bufsubstr();
	go_down_1 ();
     }
   
   index = array_sort(keys, sort_fun);

   %
   %  arrange region
   %
   goto_line (end_line);
   !if (down_1 ())
     {
	eol();
	newline();
     }
   
   push_spot();
   for(i = 0; i < n; ++i)
     {
	goto_line(index[i] + beg); 
	line_as_string (); 	       %  on stack-- also we are at eol now
	pop_spot();
	bol(); insert(()); newline();
	push_spot();
     }
   pop_spot();
   goto_line(beg);
   bol(); push_mark();
   goto_line(end_line + 1);
   bol(); del_region();
}

define sort ()
{
   sort_using_function (&strcmp);
   %flush("Done.");
}

