%
%  Miscellaneous text functions
%

define text_justify_line ()
{
   variable min, max, r, count;
   count = 100;
   push_spot(); bol_skip_white();
   if (eolp) {pop_spot(); return; }
   min = what_column();
   eol_trim();
   max = what_column();
   while ((max < WRAP) and count)
     {
	r = random(0, max);
	--count;
	if (r < min) continue;
	goto_column(r); skip_white();
	if (ffind_char (' ')) { insert_single_space(); ++max; eol();}
     }
  pop_spot();
}


define format_paragraph_hook()
{
   variable n;
   push_spot();
   
   backward_paragraph();
   n = what_line();
   forward_paragraph ();
   go_up(2);
   if (n - what_line() > 0) {pop_spot(); return; }
   push_mark();
   backward_paragraph();
   if (what_line() != 1) go_down_1 ();
   narrow();
   bob();
   do text_justify_line (); while (down_1 ());
   widen();
   pop_spot();
}

