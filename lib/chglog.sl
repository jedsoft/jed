% Maintain ChangeLog files

custom_variable ("ChangeLog_Filename",
#ifdef VMS
		 "$CHANGE_LOG$.TXT"
#else
		 "ChangeLog"
#endif
		 );

custom_variable ("ChangeLog_User",
		 sprintf ("%s  <%s>", get_realname (), get_emailaddress ()));
custom_variable ("ChangeLog_Indent_Amount", 8);

private define get_changelog_date ()
{
   variable tm, day, month, year;
   tm = localtime (_time ());
   
   sprintf ("%d-%0d-%0d", 1900 + tm.tm_year, 1+tm.tm_mon, tm.tm_mday);
}

private define format_changelog_heading ()
{
   variable date = get_changelog_date ();
   
   return sprintf ("%s  %s", date, ChangeLog_User);
}

private define locate_changelog_file ()
{
   variable file, dir;
   
   (,dir,,) = getbuf_info ();
   
   forever
     {
	file = dircat (dir, ChangeLog_Filename);
	if (1 == file_status (file))
	  return file;
	
	% This may need modified for non-Unix systems...
#ifdef UNIX
	dir = expand_filename (dircat (dir, "../"));
	if (dir == "/")
	  break;
#elifdef IBMPC_SYSTEM
	dir = expand_filename (dircat (dir, "..\\"));
	if ((dir == "/") or (dir == "\\"))
	  break;
	if (strlen (dir) == 3)
	  {
	     if (dir[1] == ':')
	       break;
	  }
#elifdef VMS
	% Does this work?
	dir = expand_filename (dircat (dir, "[-]"));
#endif
     }

   verror ("Unable to find a ChangeLog file");
}

private define get_changelog_file_item ()
{
   variable dir, file;
   
   (file, dir,,) = getbuf_info ();
   !if (strlen (file))
     return "";
   if (file == ChangeLog_Filename)
     return "";
   
   return dircat (dir, file);
}

private define get_changelog_function ()
{
   variable fun = mode_get_mode_info ("chglog_get_item");
   if (fun != NULL)
     fun = @fun ();
   if (fun == NULL)
     return "";
   return fun;
}

private define wrap_hook ()
{
   push_spot ();
   bol_trim (); whitespace (ChangeLog_Indent_Amount + 2);
   pop_spot ();
}

public define changelog_add_change ()
{
   variable heading = format_changelog_heading ();
   variable file = get_changelog_file_item ();
   variable function = get_changelog_function ();
   variable changelog = locate_changelog_file ();
   
   if (strlen (file))
     {
	% Make it with respect to the changelog directory
	variable i = 0;
	while (changelog[i] == file[i]) % can this fail in practice?
	  i++;
	file = file [[i:]];
     }
   
   () = read_file (changelog);
   set_buffer_no_backup ();
   pop2buf (whatbuf ());
   text_mode ();

   set_buffer_hook ("wrap_hook", &wrap_hook);

   bob ();
   !if (bol_fsearch (heading))
     {
	vinsert ("%s\n\n", heading);
	bob ();
     }
   eol ();
   variable m = create_user_mark ();

   skip_chars (" \t\n*");

   variable create_new_entry = 1;
   
   if (looking_at (file))
     {
	go_right (strlen (file));
	_get_point ();		       %  on stack
	skip_chars (" :");
	create_new_entry = (() == _get_point ());
     }
   
   if (create_new_entry)
     {
	goto_user_mark (m);
	insert ("\n\n");
	whitespace (ChangeLog_Indent_Amount);
	vinsert ("* %s ", file);
     }
   else 
     {
	!if (re_fsearch ("^[ \t]*$"))
	  {
	     eob ();
	  }
	trim ();
	newline ();
	go_up_1 ();
	whitespace (8);
     }
   
   if (strlen (function))
     vinsert ("(%s): ", function);
   else if (create_new_entry and strlen (file))
     {
	trim ();
	insert (": ");
     }
}
