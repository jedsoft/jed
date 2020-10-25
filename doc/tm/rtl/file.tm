\variable{BACKUP_BY_COPYING}
\synopsis{Set the backup mode}
\usage{Int_Type BACKUP_BY_COPYING}
\description
 If non-zero, backup files will be made by copying the original file
 to the backup file.  If zero, the backup file will be created by
 renaming the original file to the backup file.  The default for
 \var{BACKUP_BY_COPYING} is zero because it is fastest.
\seealso{rename_file, copy_file}
\done

\function{IsHPFSFileSystem}
\synopsis{Test if drive of "path" is HPFS}
\usage{Int_Type IsHPFSFileSystem(String_Type path)}
\description
  Returns non-zero if drive of \var{path} (possibly the default drive) is
  HPFS.
\done

\function{change_default_dir}
\synopsis{Change the current working directory}
\usage{Int_Type change_default_dir (String_Type new_dir)}
\description
  This function may be used to change the current working directory
  of the editor to \var{new_dir}.  It returns zero upon success or \exmp{-1} upon
  failure.
  Note: Each buffer has its own working directory.  This function does not
  change the working directory of the buffer.  Rather, it changes the
  working directory of the whole editor.  This has an effect on functions
  such as \var{rename_file} when such functions are passed relative filenames.
\seealso{setbuf_info, getbuf_info, rename_file}
\done

\function{copy_file}
\synopsis{Copy a file "src" to "dest"}
\usage{Int_Type copy_file (String_Type src, String_Type dest)}
\description
  This function may be used to copy a file named \var{src} to a new file
  named \var{dest}.  It attempts to preserve the file access and modification
  times as well as the ownership and protection.

  It returns \exmp{0} upon success and \exmp{-1} upon failure.
\seealso{rename_file, file_status}
\done

\function{delete_file}
\synopsis{Delete the file "file"}
\usage{Int_Type delete_file (String_Type file)}
\description
  This function may be used to delete a file specified by the \var{file}
  parameter.  It returns non-zero if the file was successfully deleted
  or zero otherwise.
\seealso{rmdir}
\done

\function{directory}
\synopsis{Return number of files and list of files matching filename}
\description
   returns the number of files and a list of files which match filename.
      On unix, this defaults to filename*.  It is primarily useful for
      DOS and VMS to expand wildcard filenames
\done

\function{expand_filename}
\synopsis{Expand a file name to a canonical form}
\usage{String_Type expand_filename (String_Type file)}
\description
  The \var{expand_filename} function expands a file to a canonical form.
  For example, under Unix, if \var{file} has the value \exmp{"/a/b/../c/d"}, it
  returns \exmp{"/a/c/d"}.  Similarly, if \var{file} has the value
  \exmp{"/a/b/c//d/e"}, \exmp{"/d/e"} is returned.
\seealso{expand_symlink, path_concat}
\done

\function{expand_symlink}
\synopsis{Expand a symbolic link}
\usage{String_Type expand_symlink (String_Type pathname)}
\description
  The \var{expand_symlink} is like the \var{expand_filename} function
  except that it also expands components of the pathname that are
  symbolic links.
\seealso{expand_filename, path_concat, readlink}
\done

\function{extract_filename}
\synopsis{Separate the file name from "filespec"}
\usage{String_Type extract_filename (String_Type filespec)}
\description
  This function may be used to separate the file name from the path of
  of a file specified by \var{filespec}.  For example, under Unix, the
  expression
#v+
        extract_filename ("/tmp/name");
#v-
  returns the string \exmp{"name"}.
\seealso{expand_filename}
\done

\function{file_changed_on_disk}
\synopsis{Test if file "fn" is more recent than the current buffer}
\usage{Int_Type file_changed_on_disk (String_Type fn)}
\description
  This function may be used to determine if the disk file specified by the
  parameter \var{fn} is more recent than the current buffer.
\seealso{file_time_compare, check_buffers}
\done

\function{file_status}
\synopsis{Return information about file "filename"}
\usage{Int_Type file_status (String_Type filename)}
\description
  The \var{file_status} function returns information about a file specified
  by the name \var{filename}.  It returns an integer describing the file
  type:
   2     file is a directory
   1     file exists and is not a directory
   0     file does not exist.
  -1     no access.
  -2     path invalid
  -3     unknown error
\done

\function{file_time_compare}
\synopsis{Compares the modification times of two files}
\usage{Int_Type file_time_cmp (String_Type file1, String_Type file2)}
\description
  This function compares the modification times of two files,
  \var{file1} and \var{file2}. It returns an integer that is either
  positive, negative, or zero integer for \exmp{file1 > file2},
  \exmp{file1 < file2}, or \exmp{file1 == file2}, respectively.  In
  this context, the comparison operators are comparing file
  modification times.  That is, the operator \exmp{>} should be read
  ``is more recent than''.  The convention adopted by this routine is
  that if a file does not exist, its modification time is taken to be
  at the beginning of time.  Thus, if \var{f} exists, but \var{g} does
  not, the \exmp{file_time_compare (f, g)} will return a positive
  number. 
\seealso{file_status, time}
\done

\function{find_file}
\synopsis{Open the file "name" in a buffer (or just goto buffer)}
\usage{Int_Type find_file (String_Type name)}
\description
  The \var{find_file} function switches to the buffer associated with the
  file specified by \var{name}.  If no such buffer exists, one is created
  and the file specified by \var{name} is read from the disk and associated
  with the new buffer.  The buffer will also become attached to the
  current window.  Use the \var{read_file} function to find a file but not
  associate it with the current window.
\seealso{read_file}
\done

\function{insert_file}
\synopsis{Insert a file "f" into the current buffer}
\usage{Int_Type insert_file (String_Type f)}
\description
  This function may be used to insert the contents of a file named \var{f}
  into the buffer at the current position.  The current editing point
  will be placed at the end of the inserted text.  The function returns
  \exmp{-1} if the file was unable to be opened; otherwise it returns the
  number of lines inserted.  This number can be zero if the file is empty.
\seealso{read_file, find_file, insert}
\done

\function{msdos_fixup_dirspec}
\synopsis{remove trailing backslash from "dir"}
\usage{String_Type msdos_fixup_dirspec (String_Type dir)}
\description
  The motivation behind this is that DOS does not like a trailing
  backslash except if it is for the root dir.  This function makes
  \var{dir} conform to that.
\done

\function{read_file}
\synopsis{Read file "fn" but don't open in a window}
\usage{Int_Type read_file (string fn)}
\description
  The \var{read_file} function may be used to read a file specified by \var{fn}
  into its own buffer.  It returns a non-zero value upon success and
  signals an error upon failure.  The hook \var{find_file_hook} is called
  after the file is read in.  Unlike the related function, \var{find_file},
  this function does not create a window for the newly created buffer.
\seealso{find_file, file_status, write_buffer}
\done

\function{rename_file}
\synopsis{Change the name of a file}
\usage{Int_Type rename_file (String_Type old_name, String_Type new_name)}
\description
  This function may be used to change the name of a disk file from
  \var{old_name} to \var{new_name}.  Upon success, zero is returned.  Any other
  value indicates failure.
  Note: Both filenames must refer to the same file system.
\seealso{file_status, stat_file}
\done

\function{set_file_translation}
\synopsis{Set the way the next file is opened: 1 binary, 0 text mode}
\usage{set_file_translation (Int_Type n)}
\description
  This function affects only the way the next file is opened.  Its
  affect does not last beyond that.  If it the value of the parameter
  is 1, the next file will be opened in binary mode.  If the parameter is
  zero, the file will be opened in text mode.
\done

