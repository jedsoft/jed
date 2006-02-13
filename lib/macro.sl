
define macro_save_macro ()
{
   variable str = get_last_macro ();
   variable file;
   variable macro_name;
   
   file = read_file_from_mini ("Macro filename:");
   !if (strlen (file)) return;
   
   macro_name = read_mini ("Name of macro:", Null_String, Null_String);
   !if (strlen (macro_name)) return;
   
   str = sprintf ("%%%%%%MACRO NAME: %s\n@%s\n\n", macro_name, str);
   
   if (-1 == append_string_to_file (str, file))
     {
	error ("Error appending macro to file");
     }
}

   
define macro_to_function ()
{
   variable macro = get_last_macro ();
   variable i, imax, ch, num;
   variable last_was_insert = 0, f, ftype;
   
   pop2buf ("*macro*");
   eob ();
   
   newline ();
   insert ("\ndefine macro_");
   push_spot ();
   insert ("EDITME ()\n{\n");
   
   imax = strlen (macro);
   num = 0;
   i = 0;
   % Push macro characters on stack 
   while (i < imax)
     {
	ch = macro[i];
	if (ch == '\\')
	  {
	     i++;
	     ch = macro[i];
	  }
	else if (ch == '^') 
	  {
	     i++;
	     ch = macro[i];
	     ch -= '@';
	  }
	
	ch;			       %  push it on stack
	num++;
	i++;
     }
   
   while (num)
     {
	num--;
	ungetkey (());
     }
   
   while (input_pending (0))
     {
	(ftype, f) = get_key_binding ();

	if (typeof (f) == Ref_Type)
	  {
	     vinsert ("(@%S)();  %%<--- Ref may be to a private function\n", f);
	     last_was_insert = 0;
	     continue;
	  }

	if (f == "self_insert_cmd")
	  {
	     !if (last_was_insert)
	       {
		  insert ("  insert (\"\");  % insert text here\n");
		  last_was_insert = 1;
	       }
	     continue;
	  }

	if (ftype)
	  {
	     insert (sprintf ("  call (\"%s\");\n", f));
	  }
	else insert (sprintf ("  %s ();\n", f));

	last_was_insert = 0;
     }

   insert ("}\n");
   pop_spot ();
}

   
   
define macro_assign_macro_to_key ()
{
   variable key = Null_String;
   variable ch = 0;
   flush ("Press key to assign macro to:");
   
   do
     {
	ch = getkey ();
	if (ch == 0) ch = "^@"; else ch = char (ch);
	key = strcat (key, ch);
     }
   while (input_pending (5));
   
   local_setkey (strcat ("@", get_last_macro ()), key);
   message (strcat ("Macro set to ", expand_keystring (key)));
}

