%!%+
%\function{make_tmp_file}
%\synopsis{make_tmp_file}
%\usage{String make_tmp_file (String base);}
%\description
% This function returns a unique file name that begins with \var{base}.
% If \exmp{base} does not specify an absolute path, the value of
% \svar{Jed_Tmp_Directory} will be used for the directory.
%!%-
define make_tmp_file (base)
{
   if ((Jed_Tmp_Directory != NULL)
       and (path_is_absolute (base) == 0))
     base = path_concat (Jed_Tmp_Directory, base);

   base = path_sans_extname (base);

   () = random (-1, 0);
   variable pid = getpid ();

   loop (1000)
     {
	variable file = sprintf ("%s%d.%d", base, random (0, 0x7FFF), pid);
	!if (file_status(file)) return (file);
     }
   error ("Unable to create a tmp file!");
}

%}}}


%!%+
%\function{open_unique_filename}
%\synopsis{Generate and unique filename and open the file}
%\usage{structure = open_unique_filename (base, extname)}
%\description
% This function generates a unique filename of the form
% \exmp{baseXXXXXX.extname} and returns a structure with fields
%#v+
%    filename     : The name of the file
%    fd           : The FD_Type file descriptor
%    fp           : The FP_Type file pointer
%#v-
% If \exmp{base} represents an absolute path, then the file
% will be opened in the correspondind directory.  Otherwise the value
% of the \svar{Jed_Tmp_Directory} variable will be used.
% 
% If this function fails, an exception will be thrown.
%\notes
% The value of the \exmp{fp} structure field is generated from the
% value of the \exmp{fd} field using the \ifun{fdopen} function.  See the
% documentation of the \ifun{fdopen} function for the relationship of
% these two types and their semantics.
%\seealso{fdopen}
%!%-
define open_unique_filename (base, ext)
{
   if ((Jed_Tmp_Directory != NULL)
       && (0 == path_is_absolute (base)))
     base = path_concat (Jed_Tmp_Directory, base);

   variable dir = path_dirname (base);
   if ((dir != ".")
       && (2 != file_status (dir)))
     throw IOError, "tmpfile directory $dir does not exist"$;
   
   () = random (-1, 0);

   variable fmt = "%s%X%s";
   if (ext == NULL)
     ext = "";
   else if (ext[0] != '.')
     ext = "." + ext;

   variable flags = O_RDWR|O_CREAT|O_EXCL;
   variable mode = S_IRUSR|S_IWUSR;

   loop (5000)
     {
	variable err;

	variable file = sprintf (fmt, base, random (0, 0x7FFFFFFF), ext);
	variable fd = open (file, flags, mode);
	if (fd != NULL)
	  {
	     variable fp = fdopen (fd, "r+");
	     if (fp == NULL)
	       {
		  err = errno_string ();
		  () = close (fd);
		  () = remove (file);
		  throw IOError, "fdopen failed: $err"$;
	       }
	     return struct 
	       {
		  file = file, fp = fp, fd = fd
	       };
	  }

	if ((errno != EEXIST) && (errno != EINTR))
	  {
	     err = errno_string ();
	     throw IOError, "open failed: $err"$;
	  }
     }

   throw IOError, "Unable to open a unique file";
}
