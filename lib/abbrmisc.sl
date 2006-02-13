% Miscellaneous function for the abbrev tables

require ("abbrev");
   
define define_abbrev_for_table (table, word)
{
   variable n = what_line ();
   variable use_bskip = 1;
   variable exg = "exchange";
   variable abbrev, expans;
   
   if (markp ())
     {
	call (exg);
	if (n == what_line (), call (exg)) use_bskip = 0;
     }
   
   push_spot ();
   if (use_bskip)
     {
	push_mark ();
	bskip_chars (word);
     }
   expans = bufsubstr ();
   pop_spot ();

   !if (strlen (expans)) 
     {
	expans = read_mini("For what?", Null_String, Null_String);
	!if (strlen (expans)) return;
     }
   
   abbrev = read_mini ("Enter abbrev for '" + expans + "'", "", "");
   !if (strlen (abbrev)) return;
   
   define_abbrev (table,  abbrev, expans);
}

define define_abbreviation ()
{
   variable tbl, word;
   
   (tbl, word) = what_abbrev_table ();
   !if (strlen (tbl)) 
     {
	tbl = "Global";
	create_abbrev_table (tbl, Null_String);
	(tbl, word) = what_abbrev_table ();
     }

   define_abbrev_for_table (tbl, word);
}

private define quote_this_line ()
{
   push_spot ();
   while (ffind_char ('\\'))
     {
	insert_char ('\\');
	go_right_1 ();
     }
   pop_spot ();
   push_spot ();
   while (ffind_char ('"'))
     {
	insert_char ('\\');
	go_right_1 ();
     }
   pop_spot ();
}

define save_abbrevs ()
{
   variable file = read_file_from_mini ("Save abbrevs to:");
   variable n, table, word;
   
   !if (strlen (extract_filename (file)))
     {
	file = dircat (file, Abbrev_File);
     }
   
   !if (strlen (extract_filename (file))) error ("Invalid file.");
   
   n = list_abbrev_tables ();	       %  tables on stack
   !if (n) return;
   
   () = read_file (file);
   erase_buffer ();
   
   loop (n)
     {
	table = ();
	push_spot ();
	word = dump_abbrev_table (table);   %  buffer now contain abbrevs
	pop_spot ();
	
	vinsert("create_abbrev_table (\"%s\", \"%s\");\n", table, word);
	go_up_1 ();
	
	while (down_1 () and not(eobp()))
	  {
	     insert ("define_abbrev (\""); insert(table);
	     insert ("\",\t\"");
	     quote_this_line ();
	     () = ffind_char ('\t'); 
	     trim ();
	     insert ("\",\t\"");
	     eol ();
	     insert ("\");");
	  }
     }
   save_buffer ();
   delbuf (whatbuf);
}

	     
	
	
   
	
