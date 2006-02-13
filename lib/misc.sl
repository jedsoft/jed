% misc functions that should be in site.sl but they are not because we want
% jed to start up fast and they are not always needed.


%!%+
%\function{make_tmp_buffer_name}
%\synopsis{make_tmp_buffer_name}
%\usage{String make_tmp_buffer_name (String base);}
%\description
% Generates a unique buffer name using the string 'base' for the beginning
% of the name.  The buffer name is returned.  The buffer is not created.
%!%-
define make_tmp_buffer_name (tmp)
{
   variable n = 0, buf;
   variable t = time ();
   
   tmp = strcat (tmp, time);
   do 
     {
	buf = sprintf("%s%d", tmp, n);
	n++;
     }
   while (bufferp(buf));
   buf;
}

define misc_do_write_to_file (str, file, write_function)
{
   variable ret = -1;   
   variable buf = make_tmp_buffer_name (Null_String);
   variable cbuf = whatbuf ();
   
   setbuf (buf);
   insert (str);
   set_buffer_modified_flag (0);
   push_mark ();
   bob ();
#iffalse   
   ERROR_BLOCK 
     {
	_clear_error ();
     }
#endif
   ret = @write_function (file);
   
   setbuf (cbuf);
   delbuf (buf);
   ret;
}

%!%+
%\function{append_string_to_file}
%\synopsis{append_string_to_file}
%\usage{Integer append_string_to_file (String str, String file);}
%\description
% The string 'str' is appended to file 'file'.  This function returns -1
% upon failure or the number of lines written upon success.
% See append_region_to_file for more information.
%!%-
define append_string_to_file (str, file)
{
   misc_do_write_to_file (str, file, &append_region_to_file);
}

%!%+
%\function{write_string_to_file}
%\synopsis{write_string_to_file}
%\usage{Integer write_string_to_file (String str, String file);}
%\description
% The string 'str' is written to file 'file'.  This function returns -1
% upon failure or the number of lines written upon success.
% This function does not modify a buffer visiting the file.
%!%-
define write_string_to_file (str, file)
{
   misc_do_write_to_file (str, file, &write_region_to_file);
}

#ifnexists glob_to_regexp
define glob_to_regexp (glob)
{
   variable regexp;
#ifdef UNIX
   regexp = "^";
#else
   regexp = "^\\C";		       %  case-insensitive
#endif

   foreach (glob)
     {
	variable ch = ();
	
	switch (ch)
	  {
	   case '.':
	     ch = "\\.";
	  }
	  {
	   case '?':
	     ch = ".";
	  }
	  {
	   case '*':
	     ch = ".*";
	  }
	  {
	     % default
	     ch = char (ch);
	     if (is_substr ("[]\\^$+", ch))
	       ch = strcat ("\\", ch);
	  }
	
	regexp = strcat (regexp, ch);
     }
   strcat (regexp, "$");
}
#endif

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

define directory (dirspec)
{
   variable buf_dir, dir;
   variable pattern;
   variable files, i;

   !if (strlen (dirspec))
     dirspec = "*";

   (,buf_dir,,) = getbuf_info ();
   pattern = extract_filename (dirspec);
   dir = substr (dirspec, 1, strlen (dirspec) - strlen (pattern));
   dir = dircat (buf_dir, dir);
   !if (strlen (dir))
     dir = ".";

   pattern = glob_to_regexp (pattern);

   files = listdir (dir);

   if ((files == NULL) or (length (files) == 0))
     return 0;

   i = array_map (Int_Type, &string_match, files, pattern, 1);
   files = files[where (i)];

   % Push them onto the stack to mimick 
   foreach (files)
     ;
   length (files);
}

   
   
   
