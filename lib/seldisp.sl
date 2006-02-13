#ifdef HAS_LINE_ATTR

define set_selective_display ()
{
   variable c, arg, h;
   variable msg;

   c = prefix_argument (-1);

   % Allow the current column be used to set the indent level.
   if (c == -1)
     c = what_column ();

   if (c <= 1) 
     {
	arg = 0;
	msg = "Cancelling selective display mode...";
     }
   else
     {
	arg = 1;
	c--;
	msg = sprintf ("Hiding all lines beyond column %d...", c);
     }
   flush (msg);

   push_spot ();
   bob ();
   h = 0;
   do
     {
	bol_skip_white ();
	!if (eolp ())
	  h = arg * (what_column () > c);
	% Otherwise, a blank line to if the last line was hidden, then
	% hide this one too.
	set_line_hidden (h);
     }
   while (down_1 ());

   pop_spot ();
   message (msg + "done");
}
