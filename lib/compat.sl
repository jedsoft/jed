define create_array ()
{
   variable n, dims, type;
   variable i, size, dim;
   variable a;

   n = ();
   dims  = Integer_Type [n];
   
   i = n;
   size = 1;
   while (i)
     {
	i--;
	dim = ();
	dims[i] = dim;
	size = size * dim;
     }
   type = ();
   switch (type)
     {
      case 'i':
	a = Integer_Type [size];
     }
#ifdef SLANG_DOUBLE_TYPE
     {
      case 'f':
	a = Double_Type [size];
     }
#endif
     {
      case 's':
	a = String_Type [size];
     }
     {
      case 128:
	a = Mark_Type [size];
     }
     {
	% default
	verror ("create_array: Type %d not supported.", type);
     }
   
   
   reshape (a, dims);
   
   return a;
}

%{{{ strncat (n)
%!%+
%\function{strncat}
%\synopsis{strncat}
%\usage{Void strncat (String a, String b, ..., Integer n);}
%\description
% Returns concatenated string "abc..."
%\notes
% This function is obsolete.
%!%-
define strncat (n)
{
   "";
   _stk_roll (n + 1);
   create_delimited_string (n);
}
