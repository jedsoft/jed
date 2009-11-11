% tags.sl	-*- SLang -*-
%
% read a tags file produced by ctags/etags programs
% 
% Public Functions:
%   ctags_forward
%      Go forward to the previous tag
%   ctags_backward
%      Go back to the previous position
%   ctags_find
%      Lookup a tag in the tags file and jump to the indicated position
%   ctags_popup_tag
%      Like ctags_find, except popup in a separate window
%
% Public Variables:
% 
%   Tags_file:  The name of the tags file to use.   The default is
%   "tags".
%
% Buffer Local Variables:
%   Tags_File: The name of the tag file to use
%   Word_Chars: The characters that make up a word
custom_variable ("Tags_File", "tags");

private variable Position_Stack = NULL;
private variable Position_Stack_Ptr = NULL;

private define create_position ()
{
   variable s = struct
     {
	file, line, mark, next, prev
     };
   s.mark = create_user_mark ();
   s.file = buffer_filename ();
   s.line = what_line ();
   return s;
}

private define save_position (s)
{
   if (Position_Stack_Ptr != NULL)
     {
	Position_Stack_Ptr.next = s;
	s.prev = Position_Stack_Ptr;
     }
   else
     Position_Stack = s;

   Position_Stack_Ptr = s;
}

private define goto_position ()
{
   variable s = Position_Stack_Ptr;
   variable buf;
   EXIT_BLOCK
     {
	sw2buf (buf);
     }

   try
     {
	buf = user_mark_buffer (s.mark);
	goto_user_mark (s.mark);
	return;
     }
   catch AnyError;

   () = read_file (s.file);
   goto_line (s.line);
   s.mark = create_user_mark ();
   buf = whatbuf ();
}
   
private define back_position ()
{
   if (Position_Stack_Ptr == NULL)
     verror ("Can't go back more");
   variable s = Position_Stack_Ptr.prev;
   if (s == NULL)
     verror ("Can't go back more");
   Position_Stack_Ptr = s;
   goto_position ();
}

private define forw_position ()
{
   if (Position_Stack_Ptr == NULL)
     verror ("Can't go forward");
   variable s = Position_Stack_Ptr.next;
   if (s == NULL)
     verror ("Can't go forward");
   Position_Stack_Ptr = s;
   goto_position ();   
}

% ctags format:
%  function-name\tfilename\t/^function-prototype/
%  typedef-name\tfilename\tline-number
% Note: an extended format file will look like:
%  function-name\tfilename\t/^function-prototype$/;"TAB...
private define _ctags_find (tag, sinfo)
{
   variable n, file, proto;

   !if ((n = re_fsearch (strcat ("\\c^", tag, "\t+\\([^\t]+\\)\t+"))), n)
     return NULL;
   file = regexp_nth_match (1);

   variable dir;
   (,dir,,) = getbuf_info ();
   sinfo.file = dircat (dir, file);

   n--;
   go_right (n);
   if (looking_at ("/^"))
     {
	go_right (2);
	push_mark ();
	!if (ffind ("/;\"\t"))
	  {
	     eol (); bskip_chars ("\\\\$/");
	  }
	if (blooking_at ("$"))
	  go_left (1);

#ifexists strreplace
	proto = str_replace_all (bufsubstr (), "\\/", "/");
#else
	% Versions of slang prior to 1.4.1 do not have strreplace and
	% str_replace_all will fail unless something is done to trick it.
	proto = str_replace_all (bufsubstr (), "\\/", "\001\002\x7f");
	proto = str_replace_all (proto, "\001\002\x7f", "/");
#endif
	sinfo.line = proto;
     }
   else
     {
	push_mark ();
	eol ();
	sinfo.line = atoi (bufsubstr ());
     }
   return sinfo;
}


% etags format:
%  ^L
%  filename,some-number
%  [function-type] function-name ^?line-name,some-number
private define etags_find (tag)
{
   variable file, line, tmptag, msg = "Tag file needs updated?";

   % we do the re_fsearch in order of preference: user->function->array
   tmptag = strcat ("[: ]", tag);
   !if (re_fsearch (strcat (tmptag, "[\t ]+\x7F\\(\\d+\\),")))
     !if (re_fsearch (strcat (tmptag, "[\t \\(]+\x7F\\(\\d+\\),")))
       !if (re_fsearch (strcat (tmptag, "[\t \\[]+\x7F\\(\\d+\\),")))
	 error (msg);
   line = integer (regexp_nth_match (1));

   () = bol_bsearch (char (014));	% previous ^L
   go_down_1 ();
   push_mark (); skip_chars ("^,\n");
   file = bufsubstr ();

   !if (read_file (file)) error ("File not found.");
   goto_line (line);
}

private define goto_tag (s, tag)
{
   () = read_file (s.file);
   variable line = s.line;
   if (String_Type == typeof (line))
     {
	bob ();
	if (0 == bol_fsearch (line))
	  {
	     () = fsearch (tag);
	     message ("Your tags file needs to be updated");
	  }
     }
   else goto_line (line);
}

private define tags_find (find_method, tag)
{
   variable s = Struct_Type[0];
   forever 
     {
	variable s1 = struct
	  {
	     file, line
	  };
	s1 = (@find_method) (tag, s1);
	if (s1 == NULL)
	  break;
	s = [s, s1];
     }

   if (length (s) == 0)
     verror ("Unable to find %s.  Perhaps your tags file needs updated.", tag);
   
   return s;
}

private define locate_tags_file (tags_file)
{
   variable dir;

   if (path_is_absolute (tags_file))
     {
	if (1 == file_status (tags_file))
	  return tags_file;
	
	return NULL;
     }

   (,dir,,) = getbuf_info ();
   
   forever
     {
	variable file = dircat (dir, tags_file);
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

   return NULL;
}

private define find_tags_file ()
{
   variable file, dir, dir1;
   variable tbuf = " *tags*";

   file = get_blocal_var("Tags_File", Tags_File);
   file = locate_tags_file (file);
   if (file == NULL)
     error ("Unable to find a tags file");

   (dir1,) = parse_filename (file);

   if (bufferp (tbuf))
     {
	(,dir,,) = getbuf_info (tbuf);
	if (dir == dir1)
	  return;
     }

   setbuf (tbuf);
   
   erase_buffer ();
   if (insert_file (file) < 0)
     error ("File tags not found!");
   
   variable flags;

   (file,,tbuf,flags) = getbuf_info ();
   setbuf_info (file,dir1,tbuf,flags);
}

private define get_word_at_point (word_chars)
{
   push_spot ();
   skip_white ();
   bskip_chars (word_chars);
   push_mark ();
   skip_chars (word_chars);
   variable tag = bufsubstr ();
   pop_spot ();
   return tag;
}

private define get_tag_at_point ()
{
#ifexists _slang_utf8_ok
   variable word_chars = "\\w_";
#else
   variable word_chars = "0-9A-Za-z_";
#endif
#ifdef VMS
   word_chars = strcat (word_chars, "$");
#endif
   word_chars = get_blocal_var ("Word_Chars", word_chars);
   return read_mini ("Find tag:", get_word_at_point (word_chars), "");
}

define ctags_find ()
{
   variable tag;
   if (_NARGS == 1)
     tag = ();
   else
     tag = get_tag_at_point ();
   
   tag = strtrim (tag);
   !if (strlen (tag))
     return;

   variable cbuf = whatbuf ();
   variable cpos = create_position ();

   find_tags_file ();
   
   bob ();
   variable find_method = &_ctags_find;

   if (looking_at_char (014))	% if first char is ^L (etags)
     find_method = &etags_find;
   
   variable s = tags_find (find_method, tag);
   goto_tag (s[0], tag);

   save_position (cpos);
   variable tag_pos = create_position ();
   save_position (tag_pos);
   sw2buf (whatbuf ());
}

define ctags_forward ()
{
   forw_position ();
}
define ctags_backward ()
{
   back_position ();
}

% This function pops up a window containing a specified position but
% leaves the point in the current window/buffer.
private define popup_window_containing_buffer_position (m1)
{
   variable m0 = create_user_mark ();
   variable m0buf = user_mark_buffer (m0);
   variable m1buf = user_mark_buffer (m1);
   variable cwindow = window_info ('t');

   if (m0buf == m1buf)
     {
	% We may need to split this window if it is the only one
	% containing m0buf
	variable nwin = 0;
	loop (nwindows ())
	  {
	     if (m0buf == whatbuf ())
	       nwin++;
	     otherwindow ();
	  }
	variable total_windows = nwindows ();
	if (MINIBUFFER_ACTIVE)
	  total_windows--;

	if (nwin == 1)
	  {
	     if (total_windows == 1)
	       splitwindow ();
	     else loop (nwindows ())
	       {
		  if ((whatbuf () != m0buf)
		      and (whatbuf() != " <mini>"))
		    {
		       sw2buf (m0buf);
		       break;
		    }
		  otherwindow ();
	       }
	  }
     }
   pop2buf (m1buf);
   goto_user_mark (m1);
   
   % Now go back to the original window.
   % If there are several windows that contain m0buf, find the one we
   % started from.
   loop (nwindows ())
     {
	otherwindow ();
	if ((window_info ('t') == cwindow)
	    and (m0 == create_user_mark ()))
	  return;
     }
   % We could not find the window _and_ mark, so just look for the mark
   loop (nwindows ())
     {
	if (m0 == create_user_mark ())
	  return;
	otherwindow ();
     }
   % Should not get here
}

define ctags_popup_tag ()
{
   variable m0 = create_user_mark ();
   ctags_find ();
   variable m1 = create_user_mark ();
   if (m0 == m1)
     return;

   % Restore the previous position
   sw2buf (user_mark_buffer (m0));
   goto_user_mark (m0);

   % Popup a window containing the new position.
   popup_window_containing_buffer_position (m1);
}

% Compatibility
define find_tag ()
{
   ctags_popup_tag ();
}
