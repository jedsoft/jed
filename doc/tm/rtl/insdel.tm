\variable{USE_TABS}
\synopsis{Control use of tabs in whitespace}
\usage{Int_Type USE_TABS}
\description
  \var{USE_TABS} is a buffer-local variable which defaults to the
  current value of \var{USE_TABS_DEFAULT}. If \var{USE_TABS} is
  non-zero, the editor may use tab characters when creating
  whitespace.  If the value of this variable is zero, no tabs will be
  used.
\seealso{TAB, TAB_DEFAULT, USE_TABS_DEFAULT}
\done

\variable{USE_TABS_DEFAULT}
\synopsis{Control use of tabs in whitespace}
\usage{Int_Type USE_TABS_DEFAULT}
\description
  The \var{USE_TABS_DEFAULT} variable is used to set the default value
  of the buffer-local variable \var{USE_TABS} for newly created buffers.
  See its documentation for more information.
\seealso{TAB, TAB_DEFAULT, USE_TABS}
\done

\variable{WRAP}
\synopsis{Set the column at which wrapping occurs}
\usage{Int_Type WRAP}
\description
 The \var{WRAP} variable determines the column number at which
 wrapping will occur.  When entering text, if the current point goes
 beyond this column, the text will automatically wrap to the next
 line.  This will only happen for those buffers for which the wrap flag
 is set. 
\seealso{WRAP_INDENTS, getbuf_info, set_mode}
\done

\variable{WRAP_INDENTS}
\synopsis{Control indentation after wrapping}
\usage{Int_Type WRAP_INDENTS}
\description
 If this variable is non-zero, after a line is wrapped, the new line
 will start at the same indentation as the current one.  On the other
 hand, if the value of \var{WRAP_INDENTS} is zero, the new line will
 begin in the first column.
\done

\function{del}
\synopsis{Delete the character at the current editing position}
\usage{Void del ()}
\description
  The \var{del} function deletes the character at the current editing
  position.  If the position is at the end of the buffer, nothing happens.
  If the deletion occurs at the end of a line, the next line will be joined
  with the current one.
\seealso{eobp, erase_buffer, insert}
\done

\function{del_region}
\synopsis{Delete the region}
\usage{Void del_region ()}
\description
  This function deletes the region defined by the mark and the current
  editing point.  For example,
#v+
        define delete_this_line ()
        {
          bol (); push_mark (); eol ();
          del_region ();
        }
#v-
  defines a function that deletes all characters on the current line
  from the beginning of the line until the end of the line.  It does not
  delete the line itself.
\seealso{push_mark, markp, check_region}
\done

\function{erase_buffer}
\synopsis{Erase all text from the current buffer}
\usage{erase_buffer ()}
\description
  The \var{erase_buffer} function erases all text from the current buffer.
  However, it does not delete the buffer itself.

  Note: This function destroys all undo information associated with the
  buffer making it impossible to undo the result of this function.
\seealso{delbuf, del}
\done

\function{indent_line}
\synopsis{Indent the current line (using the \var{indent_hook})}
\usage{Void indent_line ()}
\description
  The \var{indent_line} line function indents the current line in a manner
  which depends upon the current buffer.  The actual function that gets
  called is set via a prior call the \var{set_buffer_hook} to set the indent
  hook.  The default value is to indent the line to the indentation
  level of the previous line.
\seealso{set_buffer_hook}
\done

\function{insbuf}
\synopsis{Insert buffer "buf" into the current buffer}
\usage{Void insbuf (String buf)}
\description
  This function may be used to insert the contents of a buffer specified
  by the name \var{buf} into the current buffer.  The editing position is
  advanced to the end of the insertion.
\seealso{copy_region, narrow, narrow_to_region}
\done

\function{insert}
\synopsis{Insert string "str" into buffer at the current position}
\usage{Void insert (String str)}
\description
  Inserts string \var{str} into buffer at the current position.  The editing
  point is moved to the end of the of the string that was inserted.
\seealso{insert_char, del, insert_file, insbuf}
\done

\function{insert_char}
\synopsis{Insert a character at the current position}
\usage{Void insert_char (ULong_Type ch)}
\description
 This function inserts the specified character into the buffer.
\seealso{what_char, insert, insert_byte}
\done

\function{insert_byte}
\synopsis{Insert a byte into the buffer}
\usage{Void insert_byte (UChar_Type ch)}
\description
 This function inserts the specified byte into the buffer at the
 current position.
\seealso{what_char, insert_char, insert}
\done

\function{insert_file_region}
\synopsis{Insert a region of the file "file"}
\usage{Integer insert_file_region (String file, String beg, String end)}
\description
  This function may be used to insert a region specified by the strings
  \var{beg} and \var{end} of the file with name \var{file} into the current buffer.
  The file is scanned line by line until a line that begins with the
  string given by \var{beg} is encountered.  Then, that line and all
  successive lines up to the one that starts with the string specified
  by \var{end} is inserted into the buffer.  The line beginning with the
  value of \var{end} is not inserted although the one beginning with \var{beg} is.
  The function returns the number of lines inserted or \exmp{-1} upon failure
  to open the file.

  Note that a zero length \var{beg} corresponds to the first line
  and that a zero length \var{end} corresponds to the last line.
\seealso{insert_file}
\done

\function{insert_from_kill_array}
\synopsis{Insert the contents of element "n" of the kill array}
\usage{Void insert_from_kill_array (Integer n)}
\description
  This function inserts the contents of the nth element, specified by
  \var{n}, of an internal array of character strings.

  Note: This function is not available on 16 bit systems.
\seealso{insert_from_kill_array, copy_region_to_kill_array}
\seealso{KILL_ARRAY_SIZE}
\done

\function{trim}
\synopsis{Remove all whitespace around the current editing point}
\usage{Void trim ()}
\description
  The \var{trim} function removes all whitespace around the current editing
  point.  In this context, whitespace is considered to be any
  combination of tab and space characters.  In particular, it does not
  include the newline character.  This means that the \var{trim} function
  will not delete across lines.
\seealso{skip_chars, skip_white, del, del_region}
\done

\function{whitespace}
\synopsis{Insert white space of length "n"}
\usage{whitespace (Integer n)}
\description
  The \var{whitespace} function inserts white space of length \var{n} into the
  current buffer using a combination of spaces and tabs.  The actual
  combination of spaces and tabs used depends upon the buffer local
  variable \var{TAB}.  In particular, if \var{TAB} is zero, no tab characters
  will be used for the expansion.
\seealso{insert, trim, goto_column}
\seealso{TAB,TAB_DEFAULT}
\done

