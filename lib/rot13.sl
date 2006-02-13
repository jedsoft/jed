%%
%%  rot13.sl---- rotates text by 13 characters
%%
define rot13 ()
{
   variable i, j;

   check_region (1);		       %  spot pushed

   variable a = String_Type[256];
   _for ('A', 'M', 1)
     {
	i = ();
	a[i] = char (i + 13);
	% Now take care of lower case ones
	i = i | 0x20;
	a[i] = char (i + 13);
     }
   
   _for ('N', 'Z', 1)
     {
	i = ();
	a[i] = char (i - 13);
	% Now take care of lower case ones
	i = i | 0x20;
	a[i] = char (i - 13);
     }
   translate_region (a);
   pop_spot ();
}
