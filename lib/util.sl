%%
%%  utility functions which Must be available (e.g., autoload)
%%

%%
%%  A routine that trims the buffer by:  removing excess whitespace at the
%%  end of lines and removing excess newlines
%%

define trim_buffer()
{
   push_spot();
   bob();
   do
     {
	eol_trim(); bol();
	if (eolp())
	  {
	     go_down_1 ();
	     while (eol_trim(), bol(), 
		    eolp() and not(eobp())) del();
	  } 
     }
   while (down_1 ());

   bob(); eol_trim(); bol(); 
   if (eolp() and not(eobp ())) del();
   pop_spot();
   !if (BATCH) message ("done.");
}


