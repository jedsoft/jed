\variable{ADD_NEWLINE}
\synopsis{Add a newline to a buffer when saving}
\usage{Int_Type ADD_NEWLINE}
\description
  If the value of \var{ADD_NEWLINE} is non-zero and the buffer if the
  buffer does not end with a newline character, a newline character
  will be silently added to the end of a buffer when the buffer is
  written out to a file.
\done

\variable{MAX_HITS}
\synopsis{Set the Autosave Interval}
\usage{Int_Type MAX_HITS}
\description
  The value of the \var{MAX_HITS} variable specifies how many ``hits''
  a buffer can take before it is autosaved.   A hit is defined as a
  single key sequence that could modify the buffer.
\seealso{}
\done

\function{autosave}
\synopsis{Save the current buffer to an autosave file}
\usage{Void autosave ()}
\description
  The \var{autosave} function saves the current buffer in an auto save file
  if the buffer has been marked for the auto save operation.
\seealso{setbuf_info, autosaveall}
\seealso{MAX_HITS}
\done

\function{autosaveall}
\synopsis{Save all buffers to autosave files}
\usage{Void autosaveall ()}
\description
  This function is like \var{autosave} except that it causes all files
  marked for the auto save operation to be auto-saved.
\seealso{autosave, setbuf_info}
\seealso{MAX_HITS}
\done

\function{buffer_list}
\synopsis{Return the names of buffers}
\usage{Integer buffer_list ()}
\description
  This function returns an integer indicating the number of buffers and
  leaves the names of the buffers on the stack.  For example, the
  following function displays the names of all buffers attached to
  files:
#v+
        define show_buffers ()
        {
           variable b, str = "", file;
           loop (buffer_list ())
             {
                 b = ();
                 (file,,,) = getbuf_info (b);
                 if (strlen (file)) str = strcat (str, strcat (" ", b));
             }
           message (str);
        }
#v-
\seealso{getbuf_info, whatbuf}
\done

\function{buffer_visible}
\synopsis{Return the number of windows containing a specified buffer}
\usage{Integer buffer_visible (String buf)}
\description
  This function is used to determine whether or not a buffer with name
  specified by the string \var{buf} is in a window or not.  More explicitly,
  it returns the number of windows containing \var{buf}.  This means that if
  \var{buf} does not occupy a window, it returns zero.  For Example,
#v+
        define find_buffer_in_window (buf)
        {
           ifnot (buffer_visible (buf)) return 0;
           pop2buf (buf);
           return 1;
        }
#v-
  is a function that moves to the window containing \var{buf} if \var{buf} is in
  a window.
\seealso{bufferp, nwindows}
\done

\function{bufferp}
\synopsis{Test if a buffer exists or not}
\usage{Integer bufferp (String buf)}
\description
  This function is used to see if a buffer exists or not.  If a buffer with
  name \var{buf} exists, it returns a non-zero value.  If it does not exist,
  it returns zero.
\seealso{setbuf, getbuf_info}
\done

\function{bury_buffer}
\synopsis{Make it unlikely for a specified buffer to appear in a window}
\usage{Void bury_buffer (String name)}
\description
  The \var{bury_buffer} function may be used to make it unlikely for the
  buffer specified by the paramter \var{name} to appear in a window.
\seealso{sw2buf, getbuf_info}
\done

\function{check_buffers}
\synopsis{Check if any buffers have been changed on disk}
\usage{check_buffers ()}
\description
  The \var{check_buffers} function checks to see whether or not any of
  the disk files that are associated with the editor's buffers have been
  modified since the assocation was made.  The buffer flags are
  updated accordingly.
\seealso{file_time_compare, file_changed_on_disk}
\done

\function{delbuf}
\synopsis{Delete a named buffer}
\usage{Void delbuf (String buf)}
\description
  \var{delbuf} may be used to delete a buffer with the name specified by
  \var{buf}.  If the buffer does not exist, a S-Lang error will be generated.
\seealso{whatbuf, bufferp, sw2buf}
\done

\function{getbuf_info}
\synopsis{Get basic information about a buffer}
\usage{(file, dir, name, flags) = getbuf_info ([ buf ])}
#v+
   String_Type buf;  % optional argument -- name of buffer
   Int_Type flags;   % buffer flags
   String_Type name; % name of buffer
   String_Type dir;  % directory associated with buffer
   String_Type file; % name of file associated with buffer (if any).
#v-
\description
  This function may be used to get some basic information about a
  specified buffer.  If the optional argument \var{buf} is not
  present, the current buffer will be used, otherwise \var{buf} must
  be the name of an existing buffer.

  The integer that corresponds to the buffer flags are encoded as:
#v+
        bit 0: (0x001) buffer modified
        bit 1: (0x002) auto save mode
        bit 2: (0x004) file on disk modified
        bit 3: (0x008) read only bit
        bit 4: (0x010) overwrite mode
        bit 5: (0x020) undo enabled
        bit 6: (0x040) buffer buried
        bit 7: (0x080) Force save upon exit.
        bit 8: (0x100) Do not backup
        bit 9: (0x200) File associated with buffer is a binary file
        bit 10: (0x400) Add CR to end of lines when writing buffer to disk.
        bit 11: (0x800) Abbrev mode
#v-
  For example,
#v+
        (file,,,flags) = getbuf_info();
#v-
  returns the file and the flags associated with the current buffer.
\seealso{setbuf_info, whatbuf}
\done

\function{pop2buf}
\synopsis{Open a specified buffer in a second window}
\usage{Void pop2buf (String buf)}
\description
  The \var{pop2buf} function will switch to another window and display the
  buffer specified by \var{buf} in it.  If \var{buf} does not exist, it will be
  created. If \var{buf} already exists in a window, the window containing
  \var{buf} will become the active one.  This function will create a new
  window if necessary.  All that is guaranteed is that the current
  window will continue to display the same buffer before and after the
  call to \var{pop2buf}.
\seealso{whatbuf, pop2buf_whatbuf, setbuf, sw2buf, nwindows}
\done

\function{pop2buf_whatbuf}
\synopsis{pop2buf and return the old buffers name}
\usage{String pop2buf_whatbuf (String buf)}
\description
  This function performs the same function as \var{pop2buf} except that the
  name of the buffer that \var{buf} replaced in the window is returned.
  This allows one to restore the buffer in window to what it was before
  the call to \var{pop2buf_whatbuf}.
\seealso{pop2buf, whatbuf}
\done

\function{set_buffer_umask}
\synopsis{Set the process file creation mask for the current buffer}
\usage{Integer set_buffer_umask (Integer cmask)}
\description
  The function may be used to set the process file creation mask
  for the appropriate operations associated with the current
  buffer.  This makes it possible to have a buffer-dependent
  umask setting. The function takes the desired umask setting and
  returns the previous setting.  If \var{cmask} is zero, the default
  process umask setting will be used for operations while the buffer
  is current.  If \var{cmask} is -1, the umask associated with the buffer
  will not be changed.
\done

\function{set_mode}
\synopsis{Set mode flags and name}
\usage{ Void set_mode(String mode, Integer flags)}
\description
  This function sets buffer mode flags and status line mode name.  \var{mode}
  is a string which is displayed on the status line if the \exmp{%m} status
  line format specifier is used. The second argument, \var{flags} is an
  integer with the possible values:
#v+
        0 : no mode. Very generic
        1 : Wrap mode.  Lines are automatically wrapped at wrap column.
        2 : C mode.
        4 : Language mode.  Mode does not wrap but is useful for computer
            languages.
        8 : S-Lang mode
        16: Fortran mode highlighting
        32: TeX mode highlighting
#v-
\seealso{whatmode, getbuf_info, setbuf_info.}
\done

\function{setbuf}
\synopsis{Temporary change the default buffer to another}
\usage{Void setbuf(String buf)}
\description
  Changes the default buffer to one named \var{buf}. If the buffer does not
  exist, it will be created.
  Note: This change only lasts until top
  level of editor loop is reached at which point the the buffer
  associated with current window will be made the default.  That is this
  change should only be considered as temporary.  To make a long lasting
  change, use the function \var{sw2buf}.
\seealso{sw2buf, pop2buf, whatbuf, pop2buf_whatbuf}
\done

\function{setbuf_info}
\synopsis{Change attributes for a buffer}
\usage{setbuf_info([ buf, ] file, dir, name, flags)}
#v+
   String_Type buf;  % optional argument -- name of buffer
   Int_Type flags;   % buffer flags
   String_Type name; % name of buffer
   String_Type dir;  % directory associated with buffer
   String_Type file; % name of file associated with buffer (if any).
#v-
\description
  This function may be used to change attributes regarding the buffer
  \var{buf}.  If the optional argument \var{buf} is not present, the
  current buffer will be used.

  \var{setbuf_info} performs the opposite function of the related
  function \var{getbuf_info}.  Here \var{file} is the name of the file
  to be associated with the buffer; \var{dir} is the directory to be
  associated with the buffer; \var{buf} is the name to be assigned to
  the buffer, and \var{flags} describe the buffer attributes.  See
  \var{getbuf_info} for a discussion of \var{flags}.  Note that the
  actual file associated with the buffer is located in directory
  \var{dir} with the name \var{file}. For example, the function
#v+
        define set_overwrite_mode ()
        {
           variable dir, file, flags, name;
           (file, dir, name, flags) = getbuf_info ();
           flags = flags | (1 shl 4);
           setbuf_info (file, dir, name, flags);
        }
#v-
  may be used to turn on overwrite mode for the current buffer. 

  Advanced S-Lang programmers exploit the fact that S-Lang is a stack
  based language and simply write the above function as:
#v+
        define set_overwrite_mode ()
        {
           setbuf_info (getbuf_info () | 0x10);
        }
#v-
  Here, \exmp{(1 shl 4)} has been written as the hexidecimal number
  \exmp{0x10}.
\seealso{getbuf_info, setbuf, whatbuf}
\done

\function{sw2buf}
\synopsis{Switch to a buffer (more permanent than setbuf)}
\usage{Void sw2buf (String buf)}
\description
  This function is used to switch to another buffer whose name is
  specified by the parameter \var{buf}.  If the buffer specified by \var{buf}
  does not exist, one will be created.
  Note: Unlike \var{setbuf}, the change
  to the new buffer is more permanent in the sense that when control
  passed back out of S-Lang to the main editor loop, if the current
  buffer at that time is the buffer specified here, this buffer will be
  attached to the window.
\seealso{setbuf, pop2buf, bufferp}
\done

\function{what_mode}
\synopsis{Get mode flags and name of the current buffer}
\usage{(String name, Integer flags) = Integer what_mode ()}
\description
  This function may be used to obtain the mode flags and mode name of the
  current buffer.  See \var{set_mode} for more details.
\seealso{set_mode, getbuf_info, setbuf_info}
\done

\function{whatbuf}
\synopsis{Return the name of the current buffer}
\usage{String what_buffer()}
\description
  \var{whatbuf} returns the name of the current buffer.  It is usually used
  in functions when one wants to work with more than one buffer.  The
  function \var{setbuf_info} may be used to change the name of the buffer.
\seealso{getbuf_info, bufferp}
\done

\function{write_buffer}
\synopsis{Write the current buffer to a file}
\usage{Integer write_buffer (String filename)}
\description
  This function may be used to write the current buffer out to a file
  specified by \var{filename}.  The buffer will then become associated with
  that file.  The number of lines written to the file is returned.  An
  error condition will be signaled upon error.
\seealso{write_region_to_file, setbuf_info}
\done

