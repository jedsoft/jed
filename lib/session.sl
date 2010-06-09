% jed session save/restore
% 
% This file contains functions may be used to save and restore the
% partial state of a jed session.  By partial, it is meant that only
% the names of the files that are associated with buffers, the
% positions within those buffers, and some of the buffer flags are
% saved.  Other state information such as undo information,
% paste-buffers, etc are not saved.
% 
% To use this file, add the following near the end of your .jedrc file:
% 
%    require ("session");
%
% Then when jed starts up with no filenames specified on the command
% line, it will attempt to load a session file from the startup
% directory.  The files specified in the session file will be
% subsequently loaded.  If jed is started with one or more files given
% on the command-line, the previous session will not be loaded, nor
% will the current one be automatically saved.
% 
% By default, no session will be loaded or saved unless the startup
% directory contains a session file.  To initiate the saving of a
% session, you can either execute the save_session function, or create
% an empty session file in the startup directory.  If the save_session
% function is used, then the directory associated with the *scratch*
% buffer will be used for the session file.
% 
% The name of the session file may be set using
% the Session_Filename variable.  The default value is ".jedsession".
% 
% The Session_Exclude_Patterns variable is a list of regular
% expressions that are matched against a filename.  If a filename
% matches one of the patterns in the list, that filename will not be
% saved to the session file.  The function session_add_exclude_pattern
% may be used to add patterns to this variable.  Also, buffers that
% have the buffer-local variable save_session defined and set to 0
% will not be saved, as well as any buffer whose name begins with a
% space or '*' character.
%
% The format of the session file is very simple.  It consists of zero
% or more lines formatted as:
%
%    filename|linenumber|column|flags
%------------------------------------------------------------------------


%!%+
%\variable{Session_Filename}
%\synopsis{Name of the file where session information is saved}
%\usage{Session_Filename = ".jedsession";}
%\description
% The \svar{Session_Filename} function specifies the name of the file
% where session information is saved.  If the filename is a relative
% one, then it will be regarded as relative to the startup directory.
%\seealso{}
%!%-
custom_variable ("Session_Filename", ".jedsession");
custom_variable ("Session_Exclude_Patterns", NULL);

public define session_add_exclude_pattern (pat)
{
   if (Session_Exclude_Patterns == NULL)
     Session_Exclude_Patterns = {};
   list_append (Session_Exclude_Patterns, pat);
}

private variable This_Session_Filename = NULL;
private define expand_session_filename (file)
{
   if (path_is_absolute (file))
     return file;
   variable dir;
   (,dir,,) = getbuf_info ("*scratch*");
   return path_concat (dir, file);
}

private define exclude_filename (file)
{
   foreach (Session_Exclude_Patterns)
     {
	variable pat = ();
	if (string_match (file, pat, 1))
	  return 1;
     }
   return 0;
}

public define save_session ()
{
   variable session_file, do_error = 0;
   if (_NARGS == 0)
     {
	session_file = Session_Filename;
	do_error = 1;
     }
   else
     session_file = ();

   session_file = expand_session_filename (session_file);

   variable files = {}, lines = {}, columns = {}, flags = {};
   loop (buffer_list)
     {
	variable b = ();
	variable file = buffer_filename (b);
	if ((file == "") || (b[0] == ' ') || (b[0] == '*')
	    || exclude_filename (file))
	  continue;

	setbuf (b);
	ifnot (get_blocal_var ("save_session", 1))
	  continue;
	push_narrow ();
	widen_buffer();

	variable f; (,,,f) = getbuf_info ();
	list_append (flags, f);
	list_append (files, file);
	list_append (lines, what_line());
	list_append (columns, what_column());
	pop_narrow ();
     }

   variable fp = fopen (session_file, "w");
   if (fp == NULL)
     {
	variable msg = "Unable to save session to $file"$;
	if (do_error)
	  throw msg;
	message (msg);
	return;
     }

   () = chmod (file, 0600);
   _for (0, length(files)-1, 1)
     {
	variable i = ();
	() = fprintf (fp, "%s|%d|%d|%#lx\n", files[i], lines[i], columns[i], flags[i]);
     }
   () = fclose (fp);
}
add_completion ("save_session");

private define load_session ()
{
   variable do_error = 0, session_file;
   if (_NARGS == 0)
     session_file = Session_Filename;
   else
     {
	do_error = 1;
	session_file = ();
     }

   variable fp = fopen (session_file, "r");
   if (fp == NULL)
     {
	if (do_error)
	  throw OpenError, "Failed to open session file ${session_file}"$;
	return;
     }

   % Preserve the following flags:
   %   read-only (1<<3), overwrite (1<<4), crflag (1<<10)
   variable mask = (1<<3)|(1<<4)|(1<<10);
   variable str;
   while (-1 != fgets (&str, fp))
     {
	if (str[0] == '%')
	  continue;

	variable fields = strchop (str, '|', 0);
	variable file, line, col, flags;
	if ((length (fields) != 4)
	    || (1 != sscanf(fields[1], "%d", &line))
	    || (1 != sscanf(fields[2], "%d", &col))
	    || (1 != sscanf(fields[3], "0x%x", &flags)))
	  throw DataError, "session file appears corrupt";

	file = fields[0];

	if (NULL == stat_file (file))
	  continue;

	() = find_file (file);
	if (bobp())
	  {
	     variable f;
	     goto_line (line);
	     if (is_line_hidden ()
		 && (f = __get_reference ("fold_enter_fold"), f != NULL))
	       (@f)();

	     goto_column_best_try (col);
	  }

	_set_buffer_flag (flags&mask);
     }
   () = fclose (fp);
   This_Session_Filename = session_file;
}

private define startup_load_session_hook ()
{
   if (whatbuf () != "*scratch*")
     return;

   load_session ();
}
add_to_hook ("_jed_startup_hooks", &startup_load_session_hook);

private define exit_save_session_hook ()
{
   if (This_Session_Filename != NULL)
     save_session (This_Session_Filename);
   return 1;
}
add_to_hook ("_jed_exit_hooks", &exit_save_session_hook);
