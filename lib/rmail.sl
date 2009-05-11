% See jed/doc/rmail.txt for more information.
% Mark Olesen's patches added.

%!% variable that controls the location of the mailbox for new mail
!if (is_defined("Rmail_Spool_Mailbox_File"))
{
   variable Rmail_Spool_Mailbox_File = getenv ("MAIL");
   if (NULL == Rmail_Spool_Mailbox_File)
     {
	Rmail_Spool_Mailbox_File = "/var/spool/mail";
	if (2 != file_status (Rmail_Spool_Mailbox_File))
	  {
	     Rmail_Spool_Mailbox_File = "/usr/mail"; %  HP-UX
	  }
	Rmail_Spool_Mailbox_File = dircat (Rmail_Spool_Mailbox_File, getenv ("USER"));
     }
}



%!% where the JED mail reader should put the mail
!if (is_defined("Rmail_Directory"))
{
   variable Rmail_Directory = dircat(getenv("HOME"), "Mail");
}

%!% temporary mailbox where newmail is moved to before parsed and converted.
variable Rmail_Tmp_Mbox = dircat(Rmail_Directory, "#NewMail#");

variable Rmail_Master_Index_File = dircat(Rmail_Directory, "_Root_Index.index");
variable Rmail_Folder_Buffer = Null_String;
variable Rmail_Folder_Name = Null_String;
variable Rmail_Newmail_Folder = "New-Mail";
variable Rmail_Root_Buffer = "Rmail: Folder List";
variable Rmail_Mail_Buffer_Name = Null_String;

!if (is_defined("Rmail_Dont_Reply_To"))
{
   % comma separated list of name to not reply to.   
   variable Rmail_Dont_Reply_To;
   $1 = getenv("USER"); if ($1 == NULL) $1 = "";
   $2 = getenv("HOST"); if ($2 == NULL) $2 = "";
   
   Rmail_Dont_Reply_To = $1;
   if (strlen($1) and strlen ($2)) 
     Rmail_Dont_Reply_To = sprintf("%s,%s@%s", $1, $1, $2);
   else
     Rmail_Dont_Reply_To = $1;
}

Rmail_Dont_Reply_To = strlow (Rmail_Dont_Reply_To);

variable Rmail_Message_Number = Null_String;   %/* this is a string!! */


% check for existence of Rmail directory.  Create it if it does not exist.
!if (file_status(Rmail_Directory)) 
{
   if (mkdir(Rmail_Directory, 0700))
     error("Unable to create " + Rmail_Directory);

   if (chmod(Rmail_Directory, 0700)) 
     error ("chmod failed!");
}

!if (is_defined ("Rmail_Ask_About_Cc"))
{
   variable Rmail_Ask_About_Cc = 0;
}


define rmail_buffer_name(folder)
{
   "Rmail: " + folder;
}

!if (is_defined ("mime_rfc1522_parse_buffer"))
{
   autoload ("mime_rfc1522_parse_buffer", "mime");
   autoload ("mime_qp_parse_buffer", "mime");
}

define rmail_folder_file(folder)
{
   dircat (Rmail_Directory, folder) + ".index";
}
   
% set buffer flags so that autosave is off but save anyway is on.
% This way it gets saved in case of a power failure or something.

define rmail_set_buffer_flags()
{
   variable flags = getbuf_info();
   flags = (flags | 0x80) & ~(0x2);    % turn off autosave, turn on save
   flags = flags & ~(0x20);            % turn off undo
   flags = flags | 0x100;              % No backup file.
   setbuf_info(flags);

   () = set_buffer_umask (0077);
}

define rmail_save_buffer_secure ()
{
   variable dir, file, flags;
   (file, dir,,flags) = getbuf_info();

   !if (buffer_modified () and strlen (file))
     return;

   file = dircat(dir, file);
   () = write_buffer(file);
   () = chmod(file, 0600);
}

   
define rmail_update_master_index (folder, n, total, n_unread)
{
   variable cbuf = whatbuf(), f, fline;
   
   fline = sprintf("Folder:\t%20s\t(%d/%d) messages. %d unread.\n", 
		   folder, n, total, n_unread);
   
   variable flags;
   () = read_file(Rmail_Master_Index_File); 
   rmail_set_buffer_flags();
   push_spot_bob ();
   % only save if necessary.
   !if (bol_fsearch(fline))
     {
	!if (bol_fsearch(sprintf("Folder:\t%20s\t(", folder))) eob();
	set_readonly(0);
	delete_line();
	insert(fline);
	set_readonly(1);
     }
   rmail_save_buffer_secure();  % only if modified.
   pop_spot();
   setbuf(cbuf);
}


define rmail_save_folder (folder, delit)
{
   !if (bufferp(Rmail_Folder_Buffer)) return;
   
   variable file = rmail_folder_file(folder), n;
   variable total, n_ok, n_unread;
   
   setbuf(Rmail_Folder_Buffer);
   set_readonly(0);
   n = string(what_line());
   
   % This is a good place to update the master index.
   % calculate total messages and number of deleted ones.
   eob(); bskip_chars(" \t\n");
   total = what_line();
   
   if (bobp ())
     {
	total = 0; n = "0";
     }
   
   bob();
   n_ok = total;
   n_unread = 0;
   forever 
     {
	switch (what_char ())
	  { case 'D': n_ok--; }
	  { case '-': n_unread++; }
	
	!if (down_1 ()) break;
	bol();
     }
   
   rmail_update_master_index(folder, n_ok, total, n_unread);

   widen();
   bob();
   go_down_1 ();
   
   if (n != line_as_string ())
     {
	delete_line();
	insert(n);
	newline();
     }
   rmail_save_buffer_secure();
   
   if (delit) 
     {
	delbuf(whatbuf());
	return;
     }
   % 
   % narrow it back
   % 
   bob();
   () = bol_fsearch("\x1F\xC\n"); go_down_1 ();
   push_mark_eob (); narrow();
}

define rmail_find_folder (folder)
{
   variable file = rmail_folder_file(folder);
   rmail_save_folder(Rmail_Folder_Name, 1);
   Rmail_Folder_Buffer = rmail_buffer_name(folder);
   () = find_file(file);
   rmail_set_buffer_flags();

   rename_buffer(Rmail_Folder_Buffer);
   Rmail_Folder_Name = folder;

   % make sure buffer is properly formatted.  Routines expect this to be
   % widened.
   
   widen();
   bob();
   if (eobp())
     {
	% new, format it
	insert("0\n1\n\x1F\xC\n");
     }
}


define rmail_get_header (header, continue_flag, multi_flag)
{
   variable h, dh;

   bob();
   if (strlow (header) == "date: ")
     {
	% special treatment for date.  It always looks like:
	% Wed, 11 jun 1993
	% or: Sun Dec 04, 1994 11:05:52 GMT
	% The day and , are somtimes missing and 1993 may be just 93.
	if (re_fsearch("^Date:[ \t]+[a-zA-Z,]* *0?\\([1-3]?\\d\\) +\\([a-zA-Z]+\\) +"))
	  return sprintf("%s-%s", regexp_nth_match(1), regexp_nth_match(2));
	if (re_fsearch("^Date:[ \t]+[a-zA-Z]+ \\([a-zA-Z]+\\) +0?\\([1-3]?\\d\\)[ ,]+"))
	  return sprintf("%s-%s", regexp_nth_match(2), regexp_nth_match(1));
	return Null_String;
     }

   h = "";
   while (bol_fsearch(header))
     {
	go_right (strlen(header));
   
	skip_white();
	push_mark ();
	eol ();
   
	if (continue_flag)
	  {
	     while (down_1 ())
	       {
		  skip_white ();
		  if (bolp ())
		    {
		       go_left_1 ();
		       break;
		    }
	       }
	  }

	dh = bufsubstr();
	if (strlen (h)) h += ",";
	h += dh;
	!if (multi_flag) break;
     }
   
   return strtrim (strtrans (h, "\n", " "));
}

define rmail_narrow_to_headers ()
{
   bob ();
   push_mark ();
   !if (bol_fsearch("\n")) eob();
   narrow();
   bob ();
}

   
define rmail_extract_headers ()
{
   variable from = Null_String, date = Null_String, subject = Null_String;
   
   push_spot ();
   rmail_narrow_to_headers ();
   
   from = rmail_get_header("From: ", 0, 0);
   !if (strlen(from)) from = rmail_get_header("From ", 0, 0);
   
   date = rmail_get_header("Date: ", 0, 0);
   subject = rmail_get_header("Subject: ", 0, 0);
   
   widen();
   pop_spot();
   %                RAFE  (flags, the - means it has not been read.)
   % The 'from' string may have multibyte characters, which will not
   % will not be handled properly via the printf width specifier.  So
   % pad it outside the printf call.
   variable width = 25;
   variable len = strwidth (from);   
   if (len > width)
     {
	variable ch, new_from = "";
	foreach ch (from) using ("chars")
	  {
	     ch = char (ch);
	     if (strwidth (new_from + ch) > width)
	       break;
	     new_from += ch;
	  }
	from = new_from;
	len = strwidth (from);
     }
   loop (width-len) from += " ";

   sprintf(" %6s       %s  %s\n", date, from, subject);
   % Note: The column widths may not be correct when UTF-8 characters are 
   % present.
}


% This routine assumes that the buffer is narrow. 
define rmail_make_filename()
{
   variable cbuf = whatbuf(), n;
   setbuf(Rmail_Folder_Buffer);
   bob();   
   push_mark_eol ();
   n = bufsubstr_delete();
   if (strlen(n)) n = integer(n); else n = 0;
   ++n;
   n = string(n);
   bob();
   insert(n); 
   if (eobp())
     {
	insert("\n1\n\x1F\xC\n");
     }
   
   setbuf(cbuf);
   return n;
}

   
   

define rmail_create_folder(folder)
{
   variable dir = dircat(Rmail_Directory, folder);
   if (file_status(dir)) return dir;
   if (mkdir(dir, 0700))
     error ("Unable to create folder directory.");
   () = chmod(dir, 0700);
   rmail_update_master_index(folder, 0, 0, 0);
   return (dir);
}

define rmail_parse_mime ()
{
   variable h;

   push_spot ();
   
   rmail_narrow_to_headers ();
   if (fsearch ("?Q?"))
     mime_rfc1522_parse_buffer ();

   h = rmail_get_header ("Content-Transfer-Encoding: ", 1, 0);
   
   eob ();
   widen ();
   
   if (is_substr (strlow (h), "quoted-printable"))
     {
	push_mark ();
	eob ();
	narrow ();
	mime_qp_parse_buffer ();
	widen ();
     }
   pop_spot ();
}


define rmail_output_newmail()
{
   variable headers;
   variable cbuf = whatbuf();
   variable n = rmail_make_filename(), file;
   
   file = rmail_create_folder (Rmail_Folder_Name);
   file = dircat(file, n);
   bob(); push_mark(); skip_chars("\n \t"); del_region();
   push_mark_eob();
   () = write_region_to_file(file);
   % () = chmod(file, 0600);
   
   rmail_parse_mime ();
   headers = rmail_extract_headers();

   setbuf(Rmail_Folder_Buffer);
   eob();
   % If this format is changed, change it in xpunge as well.
   vinsert ("-%3s", n); insert(headers);
   setbuf(cbuf);
}


define rmail_newmail_narrow (from)
{
   variable content_length = 0;
   
   push_mark_eol ();
   !if (bol_fsearch_char ('\n'))
     eob ();
   narrow ();
   if (re_bsearch ("^\\CContent-Length:[ \t]+\\(\\d+\\)$"))
     content_length = integer (regexp_nth_match (1));
   
   bob ();
   widen ();
   push_mark_eol ();
   
   if (content_length)
     {
	if (bol_fsearch_char ('\n'))
	  {
	     content_length++;
	     if ((content_length == right (content_length))
		 and (skip_chars ("\n"), 
		      (re_looking_at (from) or eobp ())))
	       {
		  !if (eobp ()) go_up_1 ();
		  narrow ();
		  return;
	       }
	  }
	
	pop_mark_1 ();
	push_mark_eol ();
     }

   forever
     {
	if (re_fsearch (from))
	  {
	     if (blooking_at ("\n\n"))
	       {
		  go_up_1 ();
		  break;
	       }
	     eol ();
	     continue;
	  }
	
	eob ();
	break;
     }

   narrow();
}


private define rmail_process_newmail (mbox, newmail_folder)
{
   variable n = 0, from = "^\\cFrom ";
   
   rmail_find_folder(newmail_folder);
   eob(); push_spot();
   setbuf(" *rmail-newmail*");
   erase_buffer();

   if (insert_file(mbox) <= 0) error ("File not inserted.");   

   bob(); push_mark();
   
   if (0 == re_fsearch(from)) 
     {
	message ("New mail with no messages!");
	pop_mark_1 ();
	pop_spot ();
	return 0;
     }
	
   del_region();
   () = set_buffer_umask (0077);
   forever
     {
	rmail_newmail_narrow (from);
	++n;
	flush (sprintf ("Processing %d messages...", n));
	rmail_output_newmail();
	erase_buffer();
	widen();
	if (eobp()) break;
	del();
     }
   
   % pop back to beginning of the newest messages in the folder.
   % in addition, save_folder expects the buffer to be narrowed.
   whatbuf();   
   setbuf(Rmail_Folder_Buffer);
   bob();
   () = bol_fsearch("\x1F\xC\n"); 
   go_down_1 (); push_mark_eob(); narrow();
   pop_spot();
   
   setbuf(());
   
   rmail_save_folder(newmail_folder, 1);
   return n;
}
   

define rmail_get_newmail_from_file (file, tmp_mbox, folder)
{
   variable n, st;
   variable nomail, cmd;
   variable getmail = dircat(Jed_Bin_Dir, "getmail");
   
   nomail = sprintf ("No new mail in %s", file);
   
   st = stat_file (file);
   if (
#if (_slang_version >= 20100)
       (st == NULL) || (st.st_size <= 0)
#else
       orelse{NULL == st}{st.st_size <= 0}
#endif
       )
     {
	message (nomail);
	return (0);
     }

   flush("Getting new mail...");
   
   if (file_status(getmail) != 1)
     {
	error (getmail + " not found!");
     }
   
   cmd = sprintf("%s %s %s", getmail, file, tmp_mbox);
   push_mark(); 
   if (pipe_region(cmd))
     error ("getmail returned error.");
   
   n = rmail_process_newmail (tmp_mbox, folder);
   % delete_file(Rmail_Tmp_Mbox);
   return n;
}

define rmail_get_newmail ()
{
   rmail_get_newmail_from_file (Rmail_Spool_Mailbox_File, Rmail_Tmp_Mbox, Rmail_Newmail_Folder);
}

define rmail_folder_mode()
{
   variable flags;
   () = find_file(Rmail_Master_Index_File); 
   set_readonly(1);
   rmail_set_buffer_flags();
   rename_buffer(Rmail_Root_Buffer);
   
   bob();
   use_keymap("Rmail-Folder");
}

define rmail_exit_folder ()
{
   rmail_save_folder(Rmail_Folder_Name, 1);
   if (bufferp(Rmail_Root_Buffer)) pop2buf(Rmail_Root_Buffer); 
   else rmail_folder_mode();
   
   onewindow();
   if (bufferp(Rmail_Mail_Buffer_Name))
     {
	delbuf(Rmail_Mail_Buffer_Name);
	Rmail_Message_Number = Null_String;
     }
   Rmail_Mail_Buffer_Name = Null_String;
   clear_message ();
}

variable Rmail_Headers_Hidden = 0;

define rmail_unhide_headers ()
{
   variable mark = "\x1F\xC\n";

   Rmail_Headers_Hidden = 0;
   set_readonly(0);
   widen();
   bob(); push_mark();
   !if (bol_fsearch("\n")) eob();
   narrow();
   bob();
   
   % make a mark if not found
   if (bol_fsearch(mark))
     {
	go_down_1 ();  push_spot();
   
	% delete visible headers
	push_mark_eob(); del_region();

	% restore old ones
	bob(); push_mark(); () = bol_fsearch(mark); 
	bufsubstr(); eob(); insert();
	pop_spot();
     }
   else
     {
	insert(mark);
	push_mark_eob();
	bskip_chars("\n");
	bufsubstr();
	bob();
	insert();
	newline();
	go_down(2);
     }
   push_mark(); widen(); eob(); narrow();
   bob(); set_readonly(1);
}



define rmail_hide_headers()
{
   variable ok_headers = "From,Subj,Cc,Organ,In-Reply,Date,Reply-To,To";
   variable header;
   
   rmail_unhide_headers();
   Rmail_Headers_Hidden = 1;
   widen(); bob(); push_mark();
   !if (bol_fsearch("\n")) eob();
   narrow();
   bob();
   !if (bol_fsearch("\x1F\xC\n")) error ("Unable to find marker.");
   set_readonly(0);
   push_spot();

   % Unwrap lines 
   forever
     {
	eol (); trim ();
	!if (looking_at ("\n ") or looking_at ("\n\t"))
	  break;
	  
	del ();
	trim ();
	insert_single_space ();
     }
   
   goto_spot ();
	
   % mark ok lines
   
   foreach (strtok (ok_headers, ","))
     {
	header = ();

	while (bol_fsearch(header))
	  {
	     insert("\xFF");
	  }
#ifnfalse
	while (down_1 () and 
	       not (skip_white (), bolp ()))
	  {
	     bol ();
	     insert ("\xFF");
	  }
#endif
	goto_spot ();
     }
   pop_spot();
   eol(); go_right_1 ();
   % delete unmarked lines
   while(not(eobp()))
     {
	bol();
	if (looking_at("\xFF"))
	  {
	     del();
	     eol(); go_right_1 ();
	     continue;
	  }
	
	push_mark_eol(); skip_chars("\n"); del_region();
	eol();
     }
   %newline();
   widen();
   bob();
   () = bol_fsearch("\x1F\xC\n"); 
   go_down_1 ();
   push_mark_eob();   narrow();
   bob();
}


% This routines returns the file number of current summary line as a string.
% If the line is invalid, it returns the null string.
define rmail_extract_file_number()
{
   bol();
   go_right_1 (); skip_white();
   push_mark();
   skip_chars("0-9");
   bufsubstr();
}

define rmail_add_flag(flag)
{
   variable n = 13, flags = "RFE";
   
   flag = char(flag);
   n += is_substr(flags, flag);
   goto_column(n);
   !if (looking_at(flag))
     {
	insert(flag);
	del();
     }
}


% this routine returns 0 if line does not contain a valid message
% 1 if the message is already in the window, 2 if not but it exists, -1
% if it had to be read in.
define rmail_get_message(flag)
{
   variable buf, file, n, n1, pop_buf, ret;
   
   !if (bufferp(Rmail_Folder_Buffer)) return (0);
   pop2buf(Rmail_Folder_Buffer);
   
   file = rmail_extract_file_number ();
   !if (strlen(file)) return 0;
   Rmail_Message_Number = file;
   set_readonly(0);
   bol(); if (looking_at_char ('-')) 
     {
	del(); insert_single_space ();
     }
   
   if (flag) rmail_add_flag(flag);
   
   set_readonly(1);
   
   buf = sprintf("Rmail: %s(%s)", Rmail_Folder_Name, file);
   pop_buf = buf;
   ret = 2;

   if ((buf != Rmail_Mail_Buffer_Name) or not(bufferp(buf)))
     {
	%
	% we have to read it in.
	%
	ret = -1;
	if (bufferp(Rmail_Mail_Buffer_Name)) 
	  {
	     pop_buf = Rmail_Mail_Buffer_Name;
	  }
     }
   
   if (buffer_visible(pop_buf))
     {
	pop2buf(pop_buf);
	if (ret == 2) ret = 1;
     }
   else
     {
	% I want summary window at top. There is no nice way to do it
	% at present.
	onewindow();
	n = window_info('r');
	pop2buf(pop_buf);
	if (TOP_WINDOW_ROW == window_info('t')) 
	  {
	     pop2buf(Rmail_Folder_Buffer);
	     sw2buf(pop_buf);
	  }
	n1 = window_info('r');	
	%  we want n to be 4 lines
	%  now it is:
	n -= n1 + 1;
	loop (n - 4) enlargewin();
	pop2buf(Rmail_Folder_Buffer);
	pop2buf(pop_buf);
     }
   
   if (ret != -1) return ret;
   
   () = set_buffer_umask (077);
   set_readonly(0);
   widen();
   erase_buffer();
   Rmail_Mail_Buffer_Name = buf;
   file = dircat(dircat(Rmail_Directory, Rmail_Folder_Name), file);
   n = insert_file(file);

   ERROR_BLOCK
     {
	set_readonly(1);
	set_buffer_modified_flag(0);
     }
   
   if (n < 0)
     {
	mark_buffer ();
	() = write_region_to_file (file); 
	error ("File not found!");
     }
   

   bob();
   rename_buffer(buf);
#iftrue
   rmail_parse_mime ();
#endif
   rmail_hide_headers();
   EXECUTE_ERROR_BLOCK;
   
   return (ret);
}

  
define rmail_toggle_headers()
{
   !if (rmail_get_message(0)) return;

   if (Rmail_Headers_Hidden)
     {
	rmail_unhide_headers();
     }
   else rmail_hide_headers();
   set_buffer_modified_flag(0);
   set_readonly(1);
   pop2buf(Rmail_Folder_Buffer);
}

define rmail_scroll_forward ()
{
   ERROR_BLOCK 
     {
	pop2buf(Rmail_Folder_Buffer);
     }
   if (rmail_get_message(0) == 1) 
     {
	update_sans_update_hook(0);
	call("page_down");
     }

   EXECUTE_ERROR_BLOCK;
}


define rmail_skip_quotes ()
{
   variable str = "^ *[>:|=]";
   ERROR_BLOCK 
     {
	pop2buf(Rmail_Folder_Buffer);
     }

   if (1 == rmail_get_message (0))
     {
	update(0);
	goto_top_of_window ();
	go_down_1 ();
	if (re_fsearch (str))
	  {
	     while (
#if (_slang_version >= 20100)
		    (push_mark (), narrow (), bol (), re_fsearch (str), widen ())
		    || (skip_white (), eolp ())
#else
		    orelse { push_mark (), narrow (), bol (), re_fsearch (str), widen () }
		      { skip_white (), eolp ()}
#endif
		    )
	       {
		  !if (down_1 ()) break;
	       }
	     recenter (1);
	  }
     }

   EXECUTE_ERROR_BLOCK;
}

define rmail_scroll_backward()
{
   ERROR_BLOCK 
     {
	   pop2buf(Rmail_Folder_Buffer);
     }
   if (rmail_get_message(0) == 1) 
     {
	update(0);
	call("page_up");
     }

   EXECUTE_ERROR_BLOCK;
}

define rmail_format_mail_buffer ()
{   
   variable opt_headers = NULL;
   if (_NARGS == 1)
     opt_headers = ();

   pop2buf (Rmail_Folder_Buffer);
   mail();
   %onewindow();
   
   if (buffer_modified ())
     {
	!if (get_yes_no("Mail already being composed.  Erase it")) return 0;
     }

   mail_format_buffer (1, opt_headers);
   1;
}

define rmail_forward_message ()
{
   variable cbuf, subj;
   !if (rmail_get_message('F'))
     {
	beep();
	return;
     }
   
   cbuf = whatbuf ();
   
   push_spot ();
   bob ();
   subj = "Fwd: ";
   if (bol_fsearch ("Subject: "))
     {
	go_right (9);
	push_mark ();
	eol ();
	subj = sprintf ("Fwd: [%s]", bufsubstr ());
     }
   pop_spot ();
   
   !if (rmail_format_mail_buffer ())
     return;
   
   eob();
   insert ("----Begin Forwarded Message----\n");
   insbuf(cbuf);
   insert ("----End Forwarded Message----\n");
   
   bob();
   if (bol_fsearch ("Subject: "))
     {
	eol ();
	insert (subj);
     }
   bob ();
   eol();
}

% This function assumes that the buffer has been narrowed about the headers.
define rmail_unfold_headers ()
{
   variable flags;
   
   flags = getbuf_info ();
   setbuf_info (0);
   
   push_spot ();
   bob ();
   
   do
     {
	while (eol (),
	       (looking_at ("\n ") or looking_at ("\n\t")))
	  del ();
     }
   while (down_1 ());
   
   () = getbuf_info ();
   setbuf_info (flags);
   pop_spot ();
}

define rmail_insert_attribution (from, date, cc)
{
   variable you = "you";
   
   if (strlen (cc) and strlen(from)) you = from;
   if (strlen (date))
     vinsert ("On %s, %s said:\n", date, you);
   else
     vinsert ("%s said:\n", you);
}

define rmail_parse_email_address (from)
{
   variable pos, len;

   % First try to remove comments.  We have 2 forms to consider:
   % "not@a.comment, This is a comment <this@is.not>, this@is_not (This is)"
   
   % Knock out parenthesis form.  Try to handle nested ones
   while (pos = string_match (from, "\\(([^()]*)\\)", 1), pos)
     {
	(pos, len) = string_match_nth (1);
	from = substrbytes (from, 1, pos) + substrbytes (from, pos + len + 1, -1);
     }
   
   % Remove double quote type comments
   from = str_uncomment_string (from, "\"", "\"");
   % Now try to get rid of ", This is a comment <this is not> bla" form
   % In general, we must handle commas:
   % 1.  "not comment,is comment <not comment> is comment, not comment"
   % So we do the following:
   %  (However, we still incorrectly handle commas in comments!)
   while (pos = string_match (from, ",?\\([^,]*<\\([^>]*\\)>[^,]*\\)", 1), pos)
     {
	variable pos1, len1;

	(pos, len) = string_match_nth (1);
	(pos1, len1) = string_match_nth (2);
	
	from = substrbytes (from, 1, pos)
	  + substrbytes (from, pos1 + 1, len1)
	    + substrbytes (from, pos + len + 1, -1);
     }
   
   % Now replace all space, tab, and newline by commas
   return strcompress (strtrans (strcompress(from, " \t\n") , " ", ","), ",");
}

define rmail_apply_dont_reply_to (from)
{
   variable sub_f, n, num;
   
   % add to list, see if any part of it is in the list.
   
   ",";				       %  delimiter for create_delimited_string  
   n = 0;
   num = 0;

   from = strlow (from);
   while (sub_f = extract_element (from, n, ','), sub_f != NULL)
     {
	n++;
	variable pats;
	variable matches;

	pats = "^" + strchop (Rmail_Dont_Reply_To, ',', '\\') + "$";
	matches = array_map (Int_Type, &string_match, sub_f, pats, 1);
	!if (any (matches))
	  {
	     sub_f;
	     num++;
	  }
     }
   create_delimited_string (num);
}

define rmail_reply ()
{
   !if (rmail_get_message('R'))
     {
	beep();
	return;
     }
   
   variable subj, cc, to, replyto, from, msgid=NULL;
   variable date = "";

   variable cbuf = whatbuf ();
   
   push_spot ();
   variable headers_hidden = Rmail_Headers_Hidden;
   rmail_unhide_headers ();
   rmail_narrow_to_headers ();
   set_buffer_modified_flag(0);
   set_readonly(1);

   replyto = rmail_get_header ("Reply-To: ", 1, 0);
   !if (strlen (replyto))
     replyto = rmail_get_header ("From: ", 1, 0);
   
   to = rmail_get_header ("To: ", 1, 1);

   subj = rmail_get_header ("Subject: ", 1, 0);
   !if (strlen (subj))
     subj = "(No Subject)";

   msgid = rmail_get_header ("Message-ID: ", 1, 0);

   cc = rmail_get_header ("Cc:", 1, 1);

   if (Rmail_Ask_About_Cc and strlen (cc))
     {
	ERROR_BLOCK
	  {
	     cc = "";
	     _clear_error ();
	  }
	!if (get_yes_no ("Include the cc: header"))
	  cc = Null_String;
     }

   cc += "," + to;
   cc = rmail_apply_dont_reply_to (rmail_parse_email_address (cc));

   replyto = rmail_parse_email_address (replyto);

   from = rmail_get_header ("From: ", 1, 0);

   eob ();
   % get the date for the attribution string
   if (re_bsearch("^Date:[ \t]*\\(.*\\)$"))  
     date = regexp_nth_match (1);

   widen();

   if (headers_hidden)
     {
	rmail_hide_headers ();
	set_buffer_modified_flag(0);
	set_readonly(1);
     }

   pop_spot();

   variable opt_headers = NULL;
   if (msgid != "")
     opt_headers = sprintf ("In-Reply-To: %s\n", msgid);

   !if (rmail_format_mail_buffer (opt_headers))
     return;

   bob();
   do
     {
	if (looking_at("To:")) 
	  {
	     eol ();
	     insert (replyto);
	  }
	else if (looking_at("Cc:")) 
	  {
	     eol (); 
	     insert (cc);
	  }
	else if (looking_at("Subject:"))
	  {
	     eol ();
	     push_spot ();
	     insert (subj);
	     pop_spot ();
	     !if (looking_at("Re:")) insert ("Re: ");
	  }
     }
   while (down_1 () and not (looking_at ("X-Mailer:")));

   eob();
   
   if (bol_bsearch ("-- \n")) % find signature
     up_1 ();
   
   push_mark();
   narrow();
   % This trick allows a single undo to remove the later insertion:
   erase_buffer();
   
   insbuf(cbuf);
   bob();

   rmail_insert_attribution (from, date, cc);
   push_spot ();
   do
     {
	bol();
	insert_char ('>');
     }
   while (down_1 ());
   newline();
   pop_spot ();
   widen();
   
   !if (strlen (to))
     {
	bob(); eol();
     }
}

define rmail_edit ()
{
   variable file;

   !if (get_y_or_n ("Are you sure you want to edit")) return;

   !if (rmail_get_message ('E'))
     return;
   
   % Now we are in the message buffer.  The global variable Rmail_Message_Number
   % should be correct.
   file = dircat(dircat(Rmail_Directory, Rmail_Folder_Name), Rmail_Message_Number);
   
   if (1 != file_status (file))
     verror ("Unable to read %s", file);
   
   set_buffer_modified_flag (0);
   delbuf (whatbuf ());
   
   () = find_file (file);
   text_mode ();
   
   set_buffer_no_backup ();
}


define rmail_find_next_message ()
{
   push_mark();
   while (down_1 () and looking_at_char ('D'));
   if (eobp())
     {
	pop_mark_1 ();
	message("No more undeleted messages.");
	return 0;
     }
   pop_mark_0 ();
   1;
}

define rmail_find_prev_message()
{
   push_mark();
   while (up_1 (), bol(), () and looking_at_char ('D'));
   if (bobp() and looking_at_char ('D'))
     {
	pop_mark_1 ();
	message("No more undeleted messages.");
	return 0;
     }
   pop_mark_0 ();
   1;
}

define rmail_delete_cmd ()
{
   bol();
   if (eolp()) return beep();
   !if (looking_at_char ('D'))
     {
	set_readonly(0);
	del(); insert_char ('D');
	set_readonly(1);
     }
}

define rmail_delete_backward ()
{
   rmail_delete_cmd ();
   () = rmail_find_prev_message ();
}

define rmail_delete_forward ()
{
   rmail_delete_cmd ();
   () = rmail_find_next_message();
}

define rmail_undelete ()
{
   push_mark_eol();
   if (bol_bsearch_char ('D'))
     {
	set_readonly(0);
	del(); insert_single_space ();
	set_readonly(1);
	pop_mark_0 ();
	return;
     }
   pop_mark_1 ();
   beep();
}

define rmail_next_message()
{
   if (rmail_find_next_message())
     {
	rmail_scroll_forward();
     }
}

define rmail_prev_message()
{
   if (rmail_find_prev_message())
     {
	rmail_scroll_forward();
     }
}

define rmail_select_folder (folder)
{
   variable n;
   rmail_find_folder(folder);
   bob();
   go_down_1 ();
   n = integer(line_as_string ());
   () = bol_fsearch("\x1F\xC\n"); 
   go_down_1 ();
   push_mark_eob();
   narrow();
   pop2buf(whatbuf());
   goto_line(n); bol();
   use_keymap("Rmail-Read");
   set_status_line("Jed %b Folder.      (%p)  %t", 0);
   set_readonly(1);
   run_mode_hooks ("rmail_folder_hook");
}

define rmail_build_folder_list ()
{
   variable slist = Null_String;
   variable cbuf = whatbuf ();
   
   setbuf (Rmail_Root_Buffer);
   push_spot_bob ();
   while (bol_fsearch("Folder:\t"))
     {
	go_right (8);
	skip_white ();
	push_mark ();
	() = ffind ("\t"); 
	slist += "," + bufsubstr ();
     }
   pop_spot ();
   setbuf (cbuf);
   slist;
}

private define folder_exists (folder)
{
   folder = dircat(Rmail_Directory, folder);
   return file_status(folder) == 2;
}
   
private define query_create_folder (default_folder)
{
   variable folder, new_dir;

   folder = read_string_with_completion ("Move to folder:", 
					 default_folder,
					 rmail_build_folder_list ());

   if (not (strlen (folder)))
     return NULL;

   if (folder == Rmail_Folder_Name)
     return folder;

   if (0 == folder_exists (folder))
     {
	if (1 != get_yes_no(sprintf("Folder %s does not exist, create it", folder)))
	  return NULL;

	() = rmail_create_folder(folder); 
     }
   
   return folder;
}

define rmail_folder_newmail ()
{
   variable n;
   variable file = Rmail_Spool_Mailbox_File;
   variable folder;

   folder = Rmail_Newmail_Folder;

   if (-1 != prefix_argument (-1))
     {
	file = read_file_from_mini ("Input MailBox:");
	!if (strlen (file)) return;

	folder = query_create_folder (folder);
	if (folder == NULL)
	  return;
     }
   
   n = rmail_get_newmail_from_file (file, Rmail_Tmp_Mbox, folder);
   
   if (n)
     {
	flush(sprintf("%d new messages.", n));
	rmail_select_folder(folder);
     }
}


variable Rmail_Last_Folder = Null_String;


% This routine deletes buffer containing message n if it exists
% it returns 1 if is exists and is visible otherwise it returns 0.
define rmail_validate_message ()
{
   variable vis = 0;
   variable buf = sprintf("Rmail: %s(%s)", Rmail_Folder_Name, Rmail_Message_Number);
   
   if (bufferp(buf))
     {
	vis = buffer_visible(buf);
	delbuf(buf);
     }
   return vis;
}

define rmail_update_folder_and_save ()
{
   variable new_n;
   
   eob ();
   bskip_chars ("\n \t");
   bol ();
   new_n = rmail_extract_file_number ();
   !if (strlen(new_n)) new_n = "0";
   widen(); 
   
   bob(); push_mark_eol(); 
   if (bufsubstr() != new_n)
     {
	delete_line();
	insert(new_n);
	newline();
     }
   () = bol_fsearch("\x1F\xC\n"); 
   go_down_1 (); push_mark_eob(); narrow();
   
   pop_mark_1 ();
   rmail_save_folder(Rmail_Folder_Name, 0);
}


define rmail_resequence_folder ()
{
   variable n, file, new_file, dir, new_n = "0";
   variable update_message = 0;
   variable res_fmt = "Resequencing folder [%d/%d]...";
   variable nmax;
   
   ERROR_BLOCK
     {
	rmail_update_folder_and_save ();
	pop_spot ();
     }

   push_spot();
   set_readonly(0);
   dir = dircat(Rmail_Directory, Rmail_Folder_Name);

   n = 0;
   eob ();
   nmax = what_line () - 1;
   bob();
      
   flush (sprintf (res_fmt, n, nmax));

   while (file = rmail_extract_file_number (), strlen(file))
     {
	n++;
	new_n = string(n);
	
	!if (n mod 10) 
	  flush (sprintf (res_fmt, n, nmax));
	
	if (file != new_n)
	  {
	     file = dircat(dir, file);
	     new_file = dircat(dir, new_n);
	     if (file_status (new_file)) 
	       verror ("File %s exists.  Cannot rename %s to it.",
		       new_file, file);
		 
	     if (rename_file(file, new_file)) 
	       error ("Error renaming ", file);

	     bol();
	     go_right_1 ();
	     push_mark();
	     skip_white();
	     skip_chars("0-9");
	     del_region();
	     vinsert ("%3s", new_n);
	     if (new_n == Rmail_Message_Number)
	       {
		 update_message = rmail_validate_message ();
	       }
	  }

	!if (down_1 ()) break;
     }

   % update the last file number
   rmail_update_folder_and_save ();
   
   pop_spot();
   eol (); bskip_chars("\n"); bol ();
   set_readonly(1);
   if (update_message or (integer(new_n) < integer(Rmail_Message_Number)))
     {
	Rmail_Message_Number = Null_String;
	rmail_scroll_forward ();
     }
   
   message("done.");
}



define rmail_output_to_folder()
{
   variable folder = NULL, header, old_n, new_n, new_file, old_file;
   variable old_folder = Rmail_Folder_Name;
   variable buf, vis;
   
   if (_NARGS)
     folder = ();

   variable new_dir, old_dir = dircat(Rmail_Directory, Rmail_Folder_Name);
   
   old_n = rmail_extract_file_number ();
   !if (strlen(old_n)) return;

   if (
#if (_slang_version >= 20100)
       (folder == NULL) || (0 == folder_exists (folder))
#else
       orelse {(folder == NULL)}{0 == folder_exists (folder)}
#endif
       )
     folder = query_create_folder (Rmail_Last_Folder);
     
   if (folder == NULL)
     return;

   new_dir = dircat(Rmail_Directory, folder);
   
   % lets get this header
   header = line_as_string ();
   
   rmail_find_folder (folder);

   %
   % generate a new filename
   %
   new_n = rmail_make_filename();
   new_file = dircat(new_dir, new_n);
   old_file = dircat(old_dir, old_n);
   
   if (rename_file(old_file, new_file))
     {
	rmail_select_folder(old_folder);
	error("Unable to rename file.");
     }
   eob();
   insert(header);
   bol();
   go_right_1 ();
   push_mark();
   skip_white();
   skip_chars("0-9");
   del_region();
   vinsert ("%3s", new_n);
   Rmail_Last_Folder = folder;
   
   % Now narrow it so next routine can process it
   bob();
   () = bol_fsearch("\x1F\xC\n"); 
   go_down_1 ();
   push_mark_eob();
   narrow();

   rmail_select_folder(old_folder);
   set_readonly(0);
   bob(); 
   () = bol_fsearch(header); 
   delete_line();
   rmail_resequence_folder();
}

define rmail_unhide_deleted ()
{
   push_spot ();
   mark_buffer ();
   set_region_hidden (0);
   pop_spot ();
}

define rmail_hide_deleted ()
{
   push_spot ();
   rmail_unhide_deleted ();
   bob ();
   while (bol_fsearch_char ('D'))
     {
	set_line_hidden (1);
	eol ();
     }
   pop_spot ();
}

   
   

% This function is assumed to be called from within a folder.  It REALLY
% deletes files marked with the D.

define rmail_xpunge_deletions ()
{
   variable file, dir, n, new_file;
   % variable old_n = Null_String;
   flush("Expunging messages...");
   push_mark();
   ERROR_BLOCK
     {
	rmail_resequence_folder ();
	pop_mark_0 ();
	set_readonly(1);
     }
   set_readonly(0);
   dir = dircat(Rmail_Directory, Rmail_Folder_Name);
   bob();
   while (bol_fsearch_char ('D'))
     {
	file = rmail_extract_file_number();
	!if (strlen(file)) continue;
	% !if (strcmp(file, Rmail_Message_Number))
	%  {
	%     old_n = file;
	%  }
	
	file = dircat(dir, file);
	if (1 != file_status(file))
	  error ("Unable to access ", file);

	!if (delete_file(file))
	  error ("Unable to delete ", file);

	delete_line ();
     }
   pop_mark_1 ();
   rmail_resequence_folder ();
}


$1 = "Rmail-Read";
!if (keymap_p($1))
{
   make_keymap($1);
   _for (' ', 127, 1) 
     {
	$2 = char(());
	undefinekey($2, $1);
     }
   
   definekey("rmail_skip_quotes", "\t", $1);
   definekey("rmail_delete_backward", "D", $1);
   definekey("rmail_delete_forward", "d", $1);
   definekey("rmail_exit_folder", "q", $1);
   definekey("rmail_exit_folder", "Q", $1);
   definekey("rmail_folder_newmail", "g", $1);
   definekey("rmail_folder_newmail", "G", $1);
   definekey("rmail_next_message", "n", $1);
   definekey("rmail_next_message", "N", $1);
   definekey("rmail_prev_message", "p", $1);
   definekey("rmail_prev_message", "P", $1);
   definekey("rmail_toggle_headers", "t", $1);
   definekey("rmail_toggle_headers", "T", $1);
   definekey("rmail_scroll_forward", " ", $1);
   definekey("rmail_scroll_forward", "\r", $1);
   definekey("rmail_scroll_backward", "^?", $1);
   definekey("rmail_xpunge_deletions", "x", $1);
   definekey("rmail_xpunge_deletions", "X", $1);
   definekey("rmail_reply", "r", $1);
   definekey("rmail_reply", "R", $1);
   definekey("rmail_forward_message", "f", $1);
   definekey("rmail_forward_message", "F", $1);
   definekey("rmail_output_to_folder", "o", $1);
   definekey("rmail_undelete", "u", $1);
   definekey("rmail_undelete", "U", $1);
   definekey("rmail_edit", "E", $1);
   definekey("mail", "m", $1);
   definekey("mail", "M", $1);
}
   



define rmail_select_this_folder ()
{
   bol();
   !if (looking_at("Folder:\t")) return beep();
   () = ffind("\t"); 
   skip_white();
   push_mark(); 
   !if (ffind("\t("))
     {
	pop_mark(0);
	beep();
	return;
     }
   
   rmail_select_folder (bufsubstr());
}

	




$1 = "Rmail-Folder";
!if (keymap_p($1))
{
   make_keymap($1);
   _for (' ', 127, 1) 
     {
	$2 = char(());
	undefinekey($2, $1);
     }
   definekey("rmail_folder_newmail", "g", $1);
   definekey("rmail_folder_newmail", "G", $1);
   definekey("rmail_quit_rmail", "q", $1);
   definekey("rmail_quit_rmail", "Q", $1);
   definekey("rmail_select_this_folder", " ", $1);
   definekey("rmail_select_this_folder", "\r", $1);
   definekey("mail", "m", $1);
   definekey("mail", "M", $1);
}

% if parameter is 0, unlock it if it is ours.  Otherwise, lock or steal it.
% it returns 1 if the lock was made, 0 if not.
define rmail_check_lock_file(lck)
{
   variable lpid, this_pid = string(getpid()), do_del;
   variable lfn = "___LOCKED___";
   variable lock_file = dircat (Rmail_Directory, lfn);
   variable ret = 0, write_it = 0;
   variable errbuf;
   variable ourhost = getenv ("HOST"); if (ourhost == NULL) ourhost = "";
   variable lockhost = Null_String;
   variable lock_buffer;
   variable flags;
   
   lock_buffer = " " + lfn;
   ERROR_BLOCK
     {
	if (bufferp (lock_buffer)) 
	  {
	     setbuf (lock_buffer);
	     set_buffer_modified_flag(0);
	     delbuf (lock_buffer);
	  }
     }
   EXECUTE_ERROR_BLOCK;
   
   
   () = read_file(lock_file);  clear_message ();
   (,flags) = getbuf_info ();
   setbuf_info (lock_buffer, flags);
   set_buffer_no_backup ();
   
   % extract the pid, it will be null if file was created
   bob(); push_mark();
   if (ffind_char (':'))
     {
	lockhost = bufsubstr ();
	go_right_1 ();
	push_mark ();
     }
   eol(); lpid = bufsubstr();
   
   delete_line();

   vinsert ("%s:%s", ourhost, this_pid);
   
   if (lck)
     {
	if (strlen (lockhost))
	  {
	     if (lockhost != ourhost)
	       {
		  verror ("Pid %s on host %s is locking the mail file.  Sorry.",
			  lpid, lockhost);
	       }
	  }
	
	% check existence of pid
	if (kill(integer (lpid), 0)) lpid = "";
   
	if (strlen (lpid) and (lpid != this_pid))
	  {
	     verror ("Pid %s is locking mail directory.  Sorry.",
		     lpid);
	  }
	save_buffer ();
	
	% Now that we have created a lock file, we use a trick to unlock it
	% in case jed is exited ungracefully.  
	erase_buffer ();
	rmail_set_buffer_flags ();
	return 1;
     }
   
   
   % We get here if we are unlocking the file.
#iffalse
   if ((lockhost != ourhost) or (lpid != this_pid))
     return 0;

   () = delete_file(lock_file);
#else
   if ((lockhost == ourhost) and (lpid == this_pid))
     () = delete_file(lock_file);
#endif

   if (bufferp (lock_buffer))
     {
	setbuf(lock_buffer);
	set_buffer_modified_flag (0);
	delbuf (lock_buffer);
     }
   return 0;
}

define rmail_quit_rmail()
{
   rmail_exit_folder();
   % This puts us at the root again so save and get out.
   rmail_save_buffer_secure();
   delbuf(Rmail_Root_Buffer);
   () = rmail_check_lock_file(0);   % deletes the lock file.
}
	

define rmail ()
{
   variable n;
   !if (rmail_check_lock_file(1)) return;

   rmail_save_folder(Rmail_Folder_Name, 1);
   Rmail_Folder_Buffer = Null_String;
   
   rmail_folder_mode();
	
   n = rmail_get_newmail();
   if (n)
     {
	flush(sprintf("%d new messages.", n));
	rmail_select_folder(Rmail_Newmail_Folder);
     }
}

   
  
