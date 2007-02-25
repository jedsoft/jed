%  This particular email-editing mode borrows ideas from several other
%  such modes from contributers such as
%
%    Morten Bo Johansen		-> email
%    Paul Boekholt
%    Johann Botha		-> muttmail
%    Ulli Horlacher
%    Abraham vd Merwe
%    Thomas Roessler
%
%  To use this mode for editing a mail message from mutt, slrn, etc, set
%  the editor variable to:
%  
%     "jed %s -tmp --mailedit-mode"
%     
%   Variables supported by the mode include:
%
%     MailEdit_Fcc
%     MailEdit_Reply_To
%     MailEdit_Max_Signature_Lines
%     MailEdit_Quote_Chars,
%     
%   The colors may be controlled by
%   
%     MailEdit_Quote_Color
%     MailEdit_Signature_Color
%     MailEdit_To_Color
%     MailEdit_Subject_Color
%     MailEdit_From_Color
%     MailEdit_Header_Color
%   
%   via, e.g., set_color ("MailEdit_Header_Color", "red", "black");
%
%   The function mailedit_mode calls mailedit_mode_hook.
%

require ("mailutils");

custom_variable ("MailEdit_Fcc", NULL);
custom_variable ("MailEdit_Reply_To", NULL);
custom_variable ("MailEdit_Max_Signature_Lines", 4);
custom_variable ("MailEdit_Quote_Chars", ">:|");

custom_color ("MailEdit_Quote_Color", get_color ("string"));
custom_color ("MailEdit_Signature_Color", get_color ("preprocess"));
custom_color ("MailEdit_From_Color", get_color ("keyword"));
custom_color ("MailEdit_To_Color", get_color ("keyword"));
custom_color ("MailEdit_Subject_Color", get_color ("keyword"));
custom_color ("MailEdit_Header_Color", get_color ("keyword1"));

autoload ("mailalias_expand", "mailalias");

private variable Header_Color = color_number ("MailEdit_Header_Color");
private variable Quote_Color = color_number ("MailEdit_Quote_Color");
private variable Signature_Color = color_number ("MailEdit_Signature_Color");
private variable From_Color = color_number ("MailEdit_From_Color");
private variable To_Color = color_number ("MailEdit_To_Color");
private variable Subject_Color = color_number ("MailEdit_Subject_Color");

private define find_header_keyword_start ()
{
   do
     {
	bol_skip_white ();
	if (bolp ())
	  return;
     }
   while (up(1));
   bol ();
}

   

% If the header does not exist, it will be created. */
private define goto_header (header)
{
   if (-1 == mailutils_narrow_to_header ())
     return -1;

   variable exists = bol_fsearch (header);
   widen_region ();
   if (exists == 0)
     {
	() = mailutils_find_header_separator ();
	% Do it this way to avoid moving the line mark
	go_left (1);
	vinsert ("\n%s ", header);
	return 0;
     }
   go_right (strlen (header));
   skip_white ();
   return 0;
}

private define add_xxx_header (header, value)
{
   if (value == NULL)
     return;

   variable exists = mailutils_get_keyword_value (header);

   if ((exists != NULL) and (exists != ""))
     return;
   
   push_spot ();
   if (0 == goto_header (header))
     insert (value);
   pop_spot ();
}

% If not found, the point is left at eob
private define find_signature_start ()
{
   eob ();
   variable line = what_line ();
   !if (bol_bsearch ("-- \n"))
     return 0;
   
   if (line - what_line () > MailEdit_Max_Signature_Lines)
     {
	eob ();
	return 0;
     }
   return 1;
}

private define in_signature ()
{
   variable m = create_user_mark ();
   EXIT_BLOCK
     {
	goto_user_mark (m);
     }
   !if (find_signature_start ())
     return 0;
   return m >= create_user_mark ();
}

private define in_header ()
{
   variable m = create_user_mark ();
   variable ih = 0;

   if (mailutils_find_header_separator ())
     ih = (m <= create_user_mark ());

   goto_user_mark (m);
   return ih;
}

private define in_mail_body ()
{
   if (in_header ())
     return 0;
   return not in_signature ();
}

private define line_is_quoted ()
{
   push_spot();
   bol_skip_white ();
   variable c = what_column ();
   skip_chars (MailEdit_Quote_Chars);
   c = what_column () - c;
   pop_spot ();
   return c;
}

% Moves to bol, skips the quote characters leaving the point and returns their number
private define count_quotes ()
{
   bol_skip_white ();
   variable count = 0;
   forever 
     {
	push_mark ();
	skip_chars (MailEdit_Quote_Chars);
	variable dcount = strlen (bufsubstr ());
	!if (dcount)
	  break;
	count += dcount;

	!if (looking_at (" "))
	  break;
	go_right_1 ();
     }
   return count;
}
   
private define skip_quotes ()
{
   bol_skip_white ();
   skip_chars (MailEdit_Quote_Chars);
}

private define extract_quotes ()
{
   push_spot ();
   () = count_quotes ();
   push_mark ();
   bol ();
   bufsubstr ();		       %  on stack
   pop_spot ();
}

% What constitutes a paragraph?

private define mark_paragraph ()
{
   !if (in_mail_body ())
     {
	push_mark ();
	return;
     }

   push_spot ();
   if (mailutils_find_header_separator ())
     go_down_1 ();
   else
     bob ();

   variable body_start = create_user_mark ();

   goto_spot ();
   % Goto paragraph start
   variable nquotes = count_quotes ();
   while (up_1 ())
     {
	if (
#if (_slang_version >= 20100)
	    (create_user_mark () >= body_start) && (nquotes == count_quotes ())
#else
	    andelse{create_user_mark () >= body_start}{nquotes == count_quotes ()}
#endif
	    )
	  {
	     skip_white ();
	     !if (eolp ())
	       continue;
	  }
	go_down_1 ();
	break;
     }
   bol ();
   push_mark ();
   
   % Now goto end
   pop_spot ();
   eol ();
   while (down_1 ())
     {
	if (nquotes == count_quotes ())
	  {
	     skip_white ();
	     !if (eolp ())
	       {
		  eol ();
		  continue;
	       }
	  }
	bol ();
	break;
     }
   if (in_signature ())
     {
	() = find_signature_start ();
     }
}

private define format_paragraph_hook ();

private define format_header ()
{
   push_spot ();
   find_header_keyword_start ();
   push_mark ();
   while (down_1 ())
     {
	skip_white ();
	if (bolp ())
	  break;
     }
   narrow_to_region ();
   bob ();
   unset_buffer_hook ("format_paragraph_hook");
   call ("format_paragraph");
   set_buffer_hook ("format_paragraph_hook", &format_paragraph_hook);

   bob ();
   while (down_1 () and not (eobp ()))
     insert (" ");
   
   eob ();
   !if (bolp ())
     newline ();
   
   widen_region ();
   pop_spot ();
}

private define format_paragraph_hook ()
{
   if (in_header ())
     return format_header ();
   
   if (in_signature ())
     return;
   
   % In the body
   push_spot ();

   variable nquotes = count_quotes ();
   variable quotes = extract_quotes ();

   mark_paragraph ();
   narrow_to_region ();
   bob ();
   if (nquotes)
     {
	while (nquotes == count_quotes ())
	  {
	     push_mark ();
	     bol ();
	     del_region ();
	     !if (down_1 ())
	       break;
	  }
     }

   pop_spot ();
   unset_buffer_hook ("format_paragraph_hook");
   call ("format_paragraph");
   set_buffer_hook ("format_paragraph_hook", &format_paragraph_hook);
   
   push_spot ();

   eob ();
   if (not bolp ())
     newline ();

   if (nquotes)
     {
	bob ();
	do 
	  {
	     insert (quotes);
	  }
	while (down_1 () and not (eobp ()));
     }

   pop_spot ();
   widen_region ();   
}

private define is_paragraph_separator ()
{
   !if (in_mail_body ())
     return 1;

   if (line_is_quoted ())
     return 1;

   bol_skip_white ();
   return eolp ();
}

private define wrap_hook ()
{
   push_spot ();
   EXIT_BLOCK
     {
	pop_spot ();
     }

   if (in_header ())
     {
	bol_trim ();
	insert (" ");
	trim ();
     }
   if (in_mail_body ())
     {
#iftrue
	indent_line ();
#else	
	go_up_1 ();
	bol_skip_white ();
	variable col = what_column ();
	go_down_1 ();
	bol_trim ();
	whitespace (col - 1);
#endif
	return;
     }
}

private define add_fcc ()
{
   add_xxx_header ("Fcc:", MailEdit_Fcc);
}

private define add_replyto ()
{
   add_xxx_header ("Reply-To:", MailEdit_Reply_To);
}

private define mark_header_separator ()
{
   !if (mailutils_find_header_separator ())
     return;

   %set_line_readonly (1);
}

private variable Header_Color_Map = Assoc_Type[Int_Type, Header_Color];
Header_Color_Map["TO"] = To_Color;
Header_Color_Map["CC"] = To_Color;
Header_Color_Map["BCC"] = To_Color;
Header_Color_Map["SUBJECT"] = Subject_Color;
Header_Color_Map["FROM"] = From_Color;

private define color_header_line ()
{
   push_spot ();
   find_header_keyword_start ();
   push_mark ();
   () = ffind (":");
   variable color = Header_Color_Map[strup (bufsubstr())];
   pop_spot ();
   set_line_color (color);
}

private define color_buffer (min_line, max_line)
{
   push_spot ();
   EXIT_BLOCK
     {
	pop_spot ();
     }

   % Optimization for the most common case
   if (min_line == max_line)
     {
	goto_line (min_line);
	if (in_header ())
	  {
	     bol ();
	     color_header_line ();
	     return;
	  }
	if (in_signature ())
	  {
	     set_line_color (Signature_Color);
	     return;
	  }
	if (line_is_quoted ())
	  {
	     set_line_color (Quote_Color);
	     return;
	  }
	
	set_line_color (0);
	return;
     }

   variable header_line, signature_line;

   if (mailutils_find_header_separator ())
     header_line = what_line ();
   else
     header_line = 0;

   if (find_signature_start ())
     signature_line = what_line ();
   else
     signature_line = max_line + 1;
   
   goto_line (min_line);
   if (max_line < header_line)
     header_line = max_line + 1;
   if (min_line <= header_line)
     {
	loop (header_line - min_line)
	  {
	     color_header_line ();
	     %set_line_color (Header_Color);
	     go_down_1 ();
	  }   
	% skip header
	go_down_1 ();
	min_line = header_line;
	min_line++;
     }

   if (max_line < signature_line)
     signature_line = max_line + 1;

   loop (signature_line - min_line)
     {
	if (line_is_quoted ())
	  set_line_color (Quote_Color);
	else
	  set_line_color (0);
	
	go_down_1 ();
     }
   min_line = signature_line;

   loop (max_line - min_line + 1)
     {
	set_line_color (Signature_Color);
	go_down_1 ();
     }
}

private define newline_indent_hook ()
{
   if (line_is_quoted ())
     {
	!if (bolp () or eolp ())
	  {
	     variable quotes = extract_quotes ();
	     
	     insert ("\n\n\n");
	     insert (quotes);
	     call ("format_paragraph");
	     go_up (2);
	     return;
	  }
     }
   newline ();
   indent_line ();
}

!if (keymap_p ("mailedit"))
  make_keymap ("mailedit");

public define mailedit_mode ()
{
   mark_header_separator ();
   add_fcc ();
   add_replyto ();

   variable start_mark = NULL;
   bob ();
   if (bol_fsearch ("To: "))
     {
	go_right (3);
	skip_chars ("\t ");
	if (eolp ())
	  start_mark = create_user_mark ();
     }

   if (start_mark == NULL)
     {
	if (mailutils_find_header_separator ())
	  {
	     go_down (2);
	     start_mark = create_user_mark ();
	  }
     }
   bob ();

   if (start_mark != NULL)
     goto_user_mark (start_mark);

   set_buffer_modified_flag (0);
   
   use_keymap ("mailedit");
   set_mode ("mailedit", 1);
   unset_buffer_hook ("");
   %set_buffer_hook ("par_sep", &is_paragraph_separator);
   set_buffer_hook ("format_paragraph_hook", &format_paragraph_hook);
   set_buffer_hook ("wrap_hook", &wrap_hook);
   set_buffer_hook ("color_region_hook", &color_buffer);
   set_buffer_hook ("newline_indent_hook", &newline_indent_hook);

   %set_buffer_hook ("mark_paragraph_hook", &mark_paragraph);
   set_comment_info ("mailedit", ">", "", 0);
   run_mode_hooks ("mailedit_mode_hook");
}
