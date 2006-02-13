\variable{KILL_ARRAY_SIZE}
\synopsis{The size of the internal kill buffer array}
\usage{Int_Type KILL_ARRAY_SIZE}
\description
 This variable contains the value of the size of the internal kill
 array of character strings.  Any number from zero up to but not
 including the value of \var{KILL_ARRAY_SIZE} may be used as an
 argument in the functions that manipulate this array.
\notes
 This variable is a read-only varaible and may not available on 16
 bit systems.
\seealso{insert_from_kill_array, copy_region_to_kill_array, append_region_to_kill_array}
\done

\function{append_region_to_file}
\synopsis{Append the region to "file"}
\usage{Integer append_region_to_file (String file)}
\description
  Appends a marked region to \var{file} returning number of lines
  written or \-1 on error.  This does NOT modify a buffer visiting the
  file; however, it does flag the buffer as being changed on disk.
\done

\function{append_region_to_kill_array}
\synopsis{Append the region to the element "n" of the kill array}
\usage{Void append_region_to_kill_array (Integer n)}
\description
  This function appends the currently defined region to the contents of
  nth element, specified by \var{n}, of an internal array of character strings.

  Note: This function is not available on 16 bit systems.
\seealso{insert_from_kill_array, copy_region_to_kill_array}
\seealso{KILL_ARRAY_SIZE}
\done

\function{bufsubstr}
\synopsis{Return the region as a string}
\usage{String bufsubstr ()}
\description
  This function returns a string that contains the characters in the
  region specified by a mark and the current editing point.
  If the region crosses lines, the string will contain newline
  characters.
\seealso{insbuf, push_mark}
\done

\function{check_region}
\synopsis{Test if a region is defined and ensure the mark comes before point}
\usage{Void check_region (Integer ps)}
\description
  This function checks to see if a region is defined and may exchange
  the current editing point and the mark to define a canonical region.
  If the mark is not set, it signals an S-Lang error.  A canonical
  region is one with the mark set earlier in the buffer than than the
  editing point.  Always call this if using a region which requires
  such a situation.

  If the argument \var{ps} is non-zero, \var{push_spot} will be called,
  otherwise, \var{ps} is zero and it will not be called.

  As an example, the following function counts the number of lines in
  a region:
#v+
        define count_lines_region ()
        {
           variable n;
           check_region (1);   % spot pushed
           narrow ();
           n = what_line ();
           widen ();
           pop_spot ();
           return n;
         }
#v-
\seealso{markp, push_mark}
\done

\function{copy_region}
\synopsis{copy a region to the buffer "buf"}
\usage{Void copy_region (String buf)}
\description
  This function may be used to copy a region defined by a mark and the
  current position to the buffered specified by the name \var{buf}. It does
  not delete the characters in region but it does pop the mark that
  determines the region.
\seealso{insbuf, bufsubstr, push_mark, pop_mark, bufferp}
\done

\function{copy_region_to_kill_array}
\synopsis{Copy the defined region to element "n" of the kill array}
\usage{Void copy_region_to_kill_array (Integer n)}
\description
  This function copies the currently defined region to the nth element,
  specified by \var{n}, of an internal array of character strings replacing
  what is currently there.

  Note: This function is not available on 16 bit systems.
\seealso{insert_from_kill_array, append_region_kill_array}
\seealso{KILL_ARRAY_SIZE}
\done

\function{count_narrows}
\synopsis{Return the narrow depth of the current buffer}
\usage{Integer count_narrows ()}
\description
  This function returns the narrow depth of the current buffer.
\seealso{narrow, widen, widen_buffer, push_narrow}
\done

\function{narrow}
\synopsis{Restict editing to the region (complete lines)}
\usage{Void narrow ()}
\description
  This function may be used to restict editing to the region of lines
  between the mark and the editing point.  The region includes the line
  containing the mark as well as the line at the current point. All
  other lines outside this region are completely inacessable without
  first lifting the restriction using the \var{widen} function. As a simple
  example, suppose that there is a function called \var{print_buffer} that
  operates on the entire buffer.  Then the following function will work
  on a region of lines:
#v+
        define print_region ()
        {
           narrow ();
           print_buffer ();
           widen ();
        }
#v-
  The \var{narrow} function will signal an error if the mark is not set.
  Note also that the narrow function may be used recursively in the
  sense that a narrowed region may be further restricted using the
  \var{narrow} function.  For each narrow, the \var{widen} function must be called
  to lift each restriction.
\seealso{widen, narrow_to_region}
\done

\function{narrow_to_region}
\synopsis{Restict editing exactly to the region}
\usage{Void narrow_to_region (void)}
\description
  The \var{narrow_to_region} function behaves like the \var{narrow} function
  that \var{narrow} operates on lines and \var{narrow_to_region} restricts
  editing to only characters within the region.
\seealso{widen_region, narrow.}
\done

\function{pipe_region}
\synopsis{Execute "cmd" as subprocess and sends the region to its stdin}
\usage{Integer pipe_region (String cmd)}
\description
  The \var{pipe_region} function executes \var{cmd} in a separate process and
  sends the region of characters defined by the mark and the current
  point to the standard input of the process.  It successful, it returns
  the exit status of the process.  Upon failure it signals an error.
  Note: This function is only available for Unix and OS/2 systems.
\seealso{run_shell_cmd, push_mark}
\done

\function{pop_narrow}
\synopsis{restore the last narrow context}
\usage{Void pop_narrow ()}
\description
  The purpose of this function is to restore the last narrow
  context that was saved via \var{push_narrow}.
\seealso{push_narrow, widen, widen_buffer}
\done

\function{push_narrow}
\synopsis{Save the current narrow context}
\usage{Void push_narrow ()}
\description
  This function saves the current narrow context.  This is useful when
  one wants to restore this context after widening the buffer.
\seealso{pop_narrow, narrow, widen, widen_buffer}
\done

\function{translate_region}
\synopsis{translate the characters in the region according to "a"}
\usage{Void translate_region (String_Type[256] a)}
\description
  This function uses the 256 element array of strings to translate the
  characters in a region based on the mapping defined by the array.
  If an array element is \var{NULL}, then the corresponding character
  will not be replaced.
  
  The \var{translate_region} function leaves the editing point at the
  end of the region.
\example
#v+
    variable a = String_Type[256];
    a['&'] = "&amp;";
    a['<'] = "&lt;";
    a['>'] = "&gt;";
    a['$'] = "&dollar;";
    bob (); push_mark (); eob ();
    translate_region (a);
#v-
  uses \var{translate_region} to replace the characters \var{'&'},
  \var{'<'}, \var{'>'}, and \var{'$'} by the strings 
  \exmp{"&amp;"}, \exmp{"&lt;"}, \exmp{"&gt;"}, and \exmp{"&dollar;"},
  respectively.
\seealso{insert, delete, what_char, replace}
\done

\function{widen}
\synopsis{Undo the effect of "narrow"}
\usage{Void widen ()}
\description
  This function undoes the effect of \var{narrow}.  Consult the documentation
  for \var{narrow} for more information.
\seealso{widen_region, narrow}
\done

\function{widen_buffer}
\synopsis{Widen the whole buffer}
\usage{Void widen_buffer ()}
\description
  This function widens the whole buffer.  If one intends to restore the
  narrow context after calling this function, the narrow context should be
  saved via \var{push_narrow}.
\seealso{narrow, widen, push_narrow, pop_narrow}
\done

\function{widen_region}
\synopsis{Undo the effect of "narrow_to_region"}
\usage{Void widen_region ()}
\description
  This function undoes the effect of \var{narrow_to_region}.  Consult the
  documentation for \var{narrow_to_region} for more information.
\seealso{widen, narrow_to_region}
\done

\function{write_region_to_file}
\synopsis{Write the region to the file "filename"}
\usage{Integer write_region_to_file (String filename)}
\description
  This function may be used to write a region of the current buffer to
  the file specified by \var{filename}.  It returns the number of lines
  written to the file or signals an error upon failure.
\seealso{write_buffer, append_region_to_file, push_mark}
\done

\function{xform_region}
\synopsis{Change the characters in the region according to "how"}
\usage{Void xform_region (Integer how)}
\description
  This function changes the characters in the region in a way specified
  by the parameter \var{how}.  This is an integer that can be any of of the
  following:
#v+
        'u'       Upcase_region
        'd'       Downcase_region
        'c'       Capitalize region
#v-
  Anything else will change case of region.
\seealso{translate_region, define_case}
\done

