% More emacs functions.  This file is autoloaded upon demand.

define find_buffer_other_window ()
{  
   variable n, buf, trybuf = Null_String;
   variable ch;
   
   n = buffer_list();
   loop (n)
     { 
	buf = ();
	n--;
	ch = buf[0];
	if ((ch == ' ') or (ch == '*') or 
	    not(strcmp (whatbuf (), buf)))
	  continue;
	trybuf = buf;
	break;
     }
   loop (n) pop ();
   
   trybuf = read_with_completion ("Switch to buffer:",
				  trybuf, Null_String, 'b');

   if (strlen (trybuf)) pop2buf (trybuf);
}

define find_file_other_window ()
{
   variable file;

   file = read_file_from_mini ("Find file:");
   !if (strlen(extract_filename(file))) return;
   
   !if (read_file(file)) message ("New file.");
   pop2buf (whatbuf());
}

define find_alternate_file ()
{
   variable file;

   file = read_file_from_mini ("Find alternate file:");
   !if (strlen(extract_filename(file))) return;
   
   delbuf (whatbuf());
   !if (find_file (file)) message ("New file.");
}

define delete_blank_lines () 
{
   variable white = " \t\r\n";
   bskip_chars(white);
   eol_trim(); go_down_1(); eol_trim(); bol();
   if (eolp()) 
     {
	go_down_1();
	push_mark ();
	skip_chars (white);
	bol ();
	del_region ();
     }
}


define forward_sexp () 
{
   skip_chars(" \t\n");
   if (looking_at_char ('(')
       or looking_at_char ('{')
       or looking_at_char ('['))
     {
	if (find_matching_delimiter(what_char ()) == 1) 
	  go_right_1 ();
	return;
     }
   skip_chars("^ \t\n()[]{}");
}

define backward_sexp () 
{
   bskip_chars(" \t\n");
   go_left_1 ();
   if (looking_at_char (')')
       or looking_at_char ('}')
       or looking_at_char (']'))
     {
	() = find_matching_delimiter(0);
	return;
     }
   
   bskip_chars("^ \t\n()[]{}");
}

define kill_sexp () 
{
   variable kr = "yp_kill_region";
   push_mark();
   forward_sexp();
   go_right (eolp ());
   if (is_defined (kr)) eval (kr);
   else call ("kill_region");
}

define scroll_up_in_place ()
{
   variable m;
   m = window_line ();
   if (down_1 ()) recenter (m);
   bol ();
}

define scroll_down_in_place ()
{
   variable m;
   m = window_line ();
   if (up_1 ()) recenter (m);
   bol ();
}

define string_rectangle ()
{
   variable str = read_mini ("String to replace rectangle:", "", "");
   if (str == "")
     return;

   check_region (1);
   exchange_point_and_mark ();
   variable col = what_column;
   variable line = what_line ();
   exchange_point_and_mark ();
   variable nlines = what_line () - line + 1;
   if (what_column == col)
     pop_mark (0);
   else
     kill_rect ();

   goto_line (line);
   loop (nlines)
     {
	goto_column (col);
	insert (str);
	go_down (1);
     }
   pop_spot ();
}

define list_directory ()
{
   variable pat = read_file_from_mini ("list directory");
   !if (strlen (pat))
     return;
   variable dir = path_dirname (pat);
   pat = path_basename (pat);
   !if (strlen(pat))
     pat = "*";

   if (file_status (dir) != 2)
     verror ("%s is not a directory", dir);

   variable files = listdir (dir);
   if (files == NULL) files = String_Type[0];

   pat = glob_to_regexp (pat);
   files = files[where(array_map (Int_Type, &string_match, files, pat, 1))];
   files = files[array_sort(files)];

   variable cbuf = whatbuf ();
   pop2buf ("*directory*");
   variable file, buf, flags;
   (file,,buf,flags) = getbuf_info ();
   setbuf_info (file, dir, buf, flags);

   erase_buffer ();
   if (length (files))
     {
	array_map (Void_Type, &vinsert, "%s\n", files);
	buffer_format_in_columns ();
     }
   bob ();
   vinsert ("Directory %s\n", dir);
   bob ();
   set_buffer_modified_flag (0);
   pop2buf (cbuf);
}
