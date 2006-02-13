% untabify region function



define untab_convert_to_tabs ()
{
   variable c;
   while (fsearch_char (' '))
     {
	c = what_column ();
	push_mark ();
	skip_white ();
	c = what_column () - c;
	if (c > 1)
	  {
	     del_region ();
	     whitespace (c);
	  }
	else pop_mark_0 ();
     }
}

%!%+
%\function{untab}
%\synopsis{untab}
%\usage{Void untab ();}
%\description
% This function may be used either to convert tabs to spaces or, if called
% with a prefix argument, it will perform the opposite conversion from 
% spaces to tabs.  This function operates on a region.
%!%-
define untab ()
{
   check_region (0);
   narrow ();
   bob ();
   if (-1 != prefix_argument (-1))
     {
	untab_convert_to_tabs ();
     }
   else
     {
	while (fsearch ("\t")) 
	  {
	     TAB;                       % on stack
	     skip_white ();
	     what_column ();   % on stack
	     bskip_chars ("\t ");
	     () - what_column ();   % on stack
	     trim ();
	     TAB = 0;
	     whitespace (());
	     TAB = ();
	  }
     }
   
   widen ();
}

