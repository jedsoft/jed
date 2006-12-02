private variable Input_File 
  = path_concat (path_dirname (__FILE__), "test_cmode.dat");

private variable Log_File = path_sans_extname (__FILE__) + ".log";

define test_cmode (file)
{
   setbuf ("*test*");
   erase_buffer ();
   c_set_style ("jed");
   c_mode ();
   TAB = 0;

   () = insert_file (file);
   bob ();
   variable failures = {};
   do
     {
	bol_skip_white ();
	if (not eolp ())
	  {
	     variable col = what_column ();
	     indent_line ();
	     bol_skip_white ();
	     if (col != what_column ())
	       list_append (failures, what_line());
	  }
     }
   while (down_1());
   
   if (length (failures))
     {
	() = fprintf (stderr, "cmode.sl failed to properly indent the following lines:\n");
	foreach (failures)
	  {
	     variable line = ();
	     () = fprintf (stdout, " %d", line);
	  }
	() = fprintf (stdout, "\n");
	() = write_buffer (Log_File);
	exit (1);
     }
   exit (0);
}

test_cmode (Input_File);

	
