% sendmail.sl	-*- mode: slang; mode: fold -*-
%
% (Thanks to olesen@weber.me.queensu.ca (mj olesen) for this)
%
% Sendmail interface for Unix.
%
%  Functions: 
%   mail_send             : send message
%   mail                  : initiate mail mode
%   mail_insert_signature : append contents of Mail_Signature_File
%   mail_kill_buffer      : Delete mail buffer
%
%  Variables:
%   Mail_Reply_To         : Set this to appropriate Reply-To value
%   Mail_Signature_File   : Filename of signature (~/.signature is default)
%   SendMail_Cmd          : Name of sendmail program including switches
%   Mail_Extra_Headers	  : Misc headers to insert
%   
% You might want something like the following in mail_hook
%
% define mail_hook ()
% {
%    local_setkey ("mail_send", "^C^C");
%    local_setkey ("mailalias_expand", "^C^E");
%    local_setkey ("mail_kill_buffer", "^Xk");
% }

autoload ("mailalias_expand", "mailalias"); % the mail-alias package

% Create a dummy function if set_line_readonly is not defined.
!if (is_defined ("set_line_readonly")) eval (".(pop) set_line_readonly");

%{{{ Public Variables 

$1 = "~/.signature";
!if (is_defined ("Mail_Signature_File"))
{
   variable Mail_Signature_File = expand_filename ($1);
}

!if (is_defined ("Mail_Header_Separator_String"))
  variable Mail_Header_Separator_String = 
"--- Do not modify this line.  Enter your message below ---";

!if (is_defined ("Mail_Extra_Headers"))
{
   variable Mail_Extra_Headers = NULL;
}


% The sendmail program

$1 = _stkdepth ();
NULL;
"/usr/bin/sendmail";		% places to look
"/usr/lib/sendmail";
"/usr/sbin/sendmail";

!if (is_defined ("SendMail_Cmd"))
{
   variable SendMail_Cmd;

   while (SendMail_Cmd = (), SendMail_Cmd != NULL)
     {
	if (1 == file_status (SendMail_Cmd))
	  {
	     SendMail_Cmd = strcat (SendMail_Cmd, " -t -oem -oi -odb");
	     break;
	  }
     }
   
}

_pop_n (_stkdepth () - $1);

!if (strlen (SendMail_Cmd)) error ("`sendmail' program not found!");

%!% always Reply-To: here instead.
!if (is_defined ("Mail_Reply_To"))
{
   variable Mail_Reply_To = Null_String;
}

%}}}

%{{{ Private Variables

private variable Mail_Previous_Buffer = Null_String;
private variable Mail_Previous_Windows = 1;
private variable Mail_This_Buffer = Null_String;
private variable Mail_Filename = dircat (getenv ("HOME"), ".__jed_mail__");

%}}}

%{{{ Private Functions


define mail_sw2_prev_buf ()
{
   if (bufferp (Mail_Previous_Buffer)) sw2buf (Mail_Previous_Buffer);
   if (1 == Mail_Previous_Windows) onewindow ();
   if (bufferp (Mail_This_Buffer) and strcmp (Mail_This_Buffer, Mail_Previous_Buffer))
     pop2buf (Mail_This_Buffer);
}

%}}}

define mail_send () %{{{
{
   variable dir, file, sent, buf = "*mail*";
   variable sep_mark, fcc_mark, fcc_file;

   if (buf != whatbuf ())
     error ("not *mail* buffer");

   flush ("Sending...");

   push_spot ();

   bob ();
   !if (bol_fsearch (Mail_Header_Separator_String))
     {
	!if (bol_fsearch ("\n"))
	  {
	     pop_spot ();
	     verror ("Cannot find %s\n", Mail_Header_Separator_String);
	  }
     }

   set_line_readonly (0);
   del_eol ();
   sep_mark = create_user_mark ();

   bob ();
   fcc_mark = NULL;
   if (bol_fsearch ("Fcc: "))
     {
	push_mark_eol ();
	go_right_1 ();
	fcc_file = strtrim (substr (bufsubstr_delete (), 6, -1));
	fcc_mark = create_user_mark ();
     }
  
  
   mark_buffer ();
   sent = not (pipe_region (SendMail_Cmd));
   
   if (fcc_mark != NULL)
     {
	if (sent and strlen (fcc_file))
	  {
	     variable user = getenv ("USER");
	     if (user == NULL)
	       {
		  user = getenv ("LOGNAME");
		  if (user == NULL)
		    user = "user";
	       }
	     () = append_string_to_file (sprintf ("From %s %s\n", user, time ()), fcc_file);

	     % Make sure the buffer ends with two newline characters
	     eob ();
	     push_mark ();
	     bskip_chars (" \t\n");
	     del_region ();
	     newline (); newline ();
	     
	     mark_buffer ();
	     () = append_region_to_file (fcc_file);
	  }
	goto_user_mark (fcc_mark);
	vinsert ("Fcc: %s\n", fcc_file);

	% If the marks are on top of one another, make sure sep_mark gets
	% updated.  I added the binary operations on User Marks just for
	% this purpose.
	if (fcc_mark == sep_mark)
	  move_user_mark (sep_mark);
     }

   goto_user_mark (sep_mark);
   insert (Mail_Header_Separator_String);

   pop_spot ();

   if (sent)
     {
	set_buffer_modified_flag (0);
	flush ("Sending...done");
	(file, dir,,) = getbuf_info ();
	() = delete_file (make_autosave_filename (dir, file));
	file = dircat (dir, file);
	!if (strcmp (Mail_Filename, file))
	  () = delete_file (Mail_Filename);
	mail_sw2_prev_buf ();
	bury_buffer (buf);
	return;
     }

   flush ("Error sending message");
   beep ();
}
add_completion ("mail_send");

%}}}

define mail_format_buffer () %{{{
{
   variable erase, opt_headers=NULL;
   if (_NARGS == 2)
     opt_headers = ();
   erase = ();

   variable km = "mail_map";

   text_mode ();

   setbuf_info (getbuf_info () & ~(0x40)); %  turn off buried buffer flag
   
   !if (keymap_p(km)) make_keymap(km);
   use_keymap(km);

   if (erase == -1)
     {
	if (buffer_modified ())
	  {
	     !if (get_yes_no("Mail already being composed.  Erase it"))
	       erase = 0;
	  }
     }

   if (erase)
     {
	erase_buffer ();

	insert ("To: \nCc: \nBcc: \nSubject: \n");
	if (strlen (Mail_Reply_To))
	  vinsert ("Reply-To: %s\n", Mail_Reply_To);

	if (opt_headers != NULL)
	  {
	     insert (opt_headers);
	  }

	if (Mail_Extra_Headers != NULL)
	  {
	     insert (Mail_Extra_Headers);
	     newline ();
	  }
	insert (Mail_Header_Separator_String);
	newline ();

	go_up_1 ();
	set_line_readonly (1);

	bob (); eol ();
	set_buffer_modified_flag(0);
   
     }

   push_spot ();
   run_mode_hooks("mail_hook");
   pop_spot ();

   set_buffer_undo(1);
}

%}}}

define mail_kill_buffer () %{{{
{
   call ("kill_buffer");
   mail_sw2_prev_buf ();
}

%}}}

define mail () %{{{
{
   variable status, dir, file, buf = "*mail*";
   variable do_format = -1;
   
   status = bufferp (buf);
   Mail_Previous_Windows = nwindows ();
   Mail_This_Buffer = whatbuf ();
   
   if (BATCH)
     {
	% Mail_Previous_Buffer = whatbuf ();
	sw2buf (buf);
     }
   else
     {
	Mail_Previous_Buffer = pop2buf_whatbuf (buf);
     }

   % if buffer is not old, turn autosave on
   if (status)
     {
	(,,,status) = getbuf_info ();
	if (buffer_modified ())
	  return;
     }
   else
     {
	(dir, file) = parse_filename (Mail_Filename);
	setbuf_info (file, dir, buf, 2);	% autosave
	
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

   () = set_buffer_umask (0077);
   mail_format_buffer (do_format);
}

%}}}

define mail_insert_signature () %{{{
{
   !if (strlen (Mail_Signature_File))
     return;
   
   push_spot ();
   eob ();
   insert ("\n-- \n");
   () = insert_file (Mail_Signature_File);
   pop_spot ();
}

%}}}

