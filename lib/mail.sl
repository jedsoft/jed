% Mail for VMS and Unix systems
%
% On VMS, this uses callable mail interface.  
% For Unix, uses /usr/ucb/mail
%
% See also 'sendmail.sl' for an alternative interface.
%
% Calls mail_hook if defined.  "mail_hook" can either load a file
% to override some of the functions here.

#ifdef UNIX
!if (is_defined ("Use_MailX"))
{
   variable Use_MailX = 0;
}

%%% University of California, Berkeley mail program
%%% "/usr/ucb/mail" on most unix, "/usr/sbin/Mail" on others

!if (is_defined("UCB_Mailer"))
{
   variable UCB_Mailer = "/usr/ucb/mail";
   if (1 != file_status(UCB_Mailer))
     {
	UCB_Mailer = "/bin/mailx";
	if (1 != file_status (UCB_Mailer))
	  {
	     UCB_Mailer = "/usr/bin/mailx";
	     if (1 != file_status (UCB_Mailer))
	       {
		  UCB_Mailer = "/usr/sbin/Mail";
		  if (1 != file_status (UCB_Mailer))
		    {
		       error ("UCB Mail program not found!");
		    }
	       }
	     else Use_MailX = 1;
	  }
	else Use_MailX = 1;
     }
}


define unix_send_mail (to, cc, subj)
{
   variable status;
   subj = str_quote_string (subj, "'", '\'');
   
   if (Use_MailX == 0)
     {
	% This is a total crock.  For ucb mail, we need to 'quote' lines beginning
	% with '~'.  Lets do it now:
	check_region (0);
	() = dupmark();
	narrow ();
	bob();
	while (bol_fsearch_char ('~')) insert_single_space ();
	bob ();
	if (strlen(cc))
	  {
	     push_spot ();
	     vinsert ("~c %s\n", cc);
	  }
	eob();
	widen();
	status = pipe_region (sprintf("%s -s '%s' '%s'", UCB_Mailer, subj, to));
	if (strlen(cc))
	  {
	     pop_spot ();
	     delete_line();
	  }
     }
   else status = pipe_region (sprintf ("%s -s '%s' -c '%s' '%s'",
				       UCB_Mailer, subj, cc, to));
   
   
   return (not(status));
}

#endif


variable Mail_Previous_Buffer = Null_String;
variable Mail_Previous_Windows = 1;

define mail_send ()
{
   variable mail_cmd, to = Null_String, subj = Null_String;
   variable dir, file, cc = Null_String, sent;
  
   push_spot ();
   bob ();
   
   !if (bol_fsearch ("---text follows this line---"))
     {
	pop_spot ();
	error ("Failed to find text divider.");
     }
   
   push_mark ();
   bob (); narrow ();
   if (re_fsearch("^To:[ \t]*\\(.*\\)"))
     {
	to = regexp_nth_match (1);
     }
   if (re_fsearch("^Cc:[ \t]*\\(.*\\)"))
     {
	cc = regexp_nth_match (1);
     }
   if (re_fsearch("^Subject:[ \t]*\\(.*\\)"))
     {
	subj = regexp_nth_match (1);
     }
   
   eob();
   widen ();

   !if (strlen(to))
     {
	pop_spot ();
	error ("Bad To: field.");
     }
#ifdef VMS
   if (strlen(cc))
     {
	to += "," + cc;
     }
#endif
      
   !if (down_1 ())
     {
	pop_spot();
	error ("No message!");
     }
   
   push_mark_eob ();
   flush("Sending...");
   
#ifdef UNIX
   sent = unix_send_mail (to, cc, subj);
#endif
#ifdef VMS
   narrow();
   sent = vms_send_mail (to, subj);
   widen ();
#endif
   if (sent) 
     {
	flush ("Sending...done");
	set_buffer_modified_flag (0);
	(file, dir,,) = getbuf_info();
	() = delete_file (make_autosave_filename(dir, file));
	() = delete_file (dir, file);
     } 
   else
     {
	message ("No message sent.");
     } 
   
   pop_spot ();
   whatbuf();
   if (bufferp(Mail_Previous_Buffer)) sw2buf (Mail_Previous_Buffer);
   if (1 == Mail_Previous_Windows) onewindow();
   bury_buffer(());
}
add_completion("mail_send");

define send ()
{
   mail_send ();
   message ("This function is obsolete.  Use 'mail_send'.");
}

define mail_format_buffer (erase)
{
   variable mail_map = "mail_map";
   text_mode();
   if (erase)
     {
	erase_buffer();
	insert("To: \nCc: \nSubject: \n---text follows this line---\n");
	bob(); eol();
	set_buffer_modified_flag(0);
     }

   set_buffer_undo(1);
   setbuf_info (getbuf_info () & ~(0x40)); %  turn off buried buffer flag
   
   !if (keymap_p(mail_map)) make_keymap(mail_map);
   use_keymap(mail_map);
}

define mail ()
{
   variable mail_buf, old, status;
   mail_buf = "*mail*";
   variable file, dir;
   variable do_format = 1;
   
   old = bufferp(mail_buf);
   Mail_Previous_Windows = nwindows();
   Mail_Previous_Buffer = pop2buf_whatbuf(mail_buf);
   
   %% if buffer is not old, turn autosave on
   if (old == 0)
     {
#ifdef VMS
	dir = "sys$login:";
#else
	dir = dircat(getenv("HOME"), Null_String);
#endif
	file = "__jed_mail__";
	setbuf_info(file, dir, mail_buf, 2);
	file = make_autosave_filename(dir, file);
	if (1 == file_status (file))
	  {
	     if (get_yes_no ("An autosave file exists.  Use it"))
	       {
		  erase_buffer ();
		  () = insert_file (file);
		  do_format = 0;
	       }
	  }
     }
   else
     {
	(,,,status) = getbuf_info();
	if (status & 1) return;
     }
   
   mail_format_buffer (do_format);
   run_mode_hooks("mail_hook");
}

define mail_insert_signature ()
{
   push_spot ();   
   eob ();
   insert ("\n-- \n");
   insert_file (expand_filename ("~/.signature"));
   pop_spot ();
}
