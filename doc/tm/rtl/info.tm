\function{bobp}
\synopsis{Test if the current position is at the beginning of the buffer}
\usage{Integer bobp ();}
\description
  The \var{bolp} function is used to determine if the current position is at
  the beginning of the buffer or not.  If so, it returns a non-zero
  value.  However, if it is not, it returns zero.  This simple example,
#v+
        define is_buffer_empty ()
        {
          return bobp () and eobp ();
        }
#v-
  returns non-zero if the buffer is empty; otherwise, it returns zero.
\seealso{bob, eobp, bolp, eolp}
\done

\function{bolp}
\synopsis{Test if the current position is at the beginning of the line}
\usage{Integer bolp ();}
\description
  \var{bolp} is used to test if the current position is at the beginning of
  a line or not.  It returns non-zero if the position is at the
  beginning of a line or zero if not.
\seealso{bol, eolp, bobp, eobp}
\done

\function{count_chars}
\synopsis{Return information about size of and position in the buffer}
\usage{String count_chars ();}
\description
  This function returns information about the size of the current buffer
  and current position in the buffer.  The string returned is of the form:
#v+
        'h'=104/0x68/0150, point 90876 of 127057
#v-
\seealso{what_char}
\done

\function{eobp}
\synopsis{Test if the current position is at the end of the buffer}
\usage{Integer eobp ();}
\description
  The functio \var{eobp} is used to determine if the current position is at
  the end of the buffer or not.  It returns a non-zero value if at the
  end of the buffer or zero if not.
\seealso{eob, bolp, eolp}
\done

\function{eolp}
\synopsis{Test if the current position is at the end of the line}
\usage{Integer eolp ();}
\description
  This function may be used to determine whether or not the current
  position is at the end of a line ot not.  If it is, the routine
  returns a non-zero value; otherwise it returns zero.
\seealso{eol, bolp, eobp, bobp}
\done

\function{get_word_chars}
\synopsis{Get the currently defined word range}
\usage{String_Type get_word_chars ()}
\description
  The \var{get_word_chars} returns the currently defined set of
  characters that constitute a word.  The set may be returned as a
  character range.
\seealso{define_word}
\done

\function{what_char}
\synopsis{Return the ASCII-value of the character at the current position}
\usage{ULong_Type what_char ()}
\description
  The \var{what_char} function returns the value of the character at the
  current position as an unsigned long value.  If UTF-8 mode is active
  and the current character is part of an illegal byte sequence, then
  the function returns a negative value equal in magnitude to the
  value of the byte.
\example
#v+
        while (not (eolp ()))
          {
             if (what_char () == '_')
               {
                  del (); insert ("\\_");
                  continue;
               }
             go_right (1);
          }
#v-
  has the effect of replacing all underscore characters on the current
  line with a backslash-underscore combination.
\seealso{looking_at, blooking_at}
\done

\function{what_column}
\synopsis{Return the current column number}
\usage{Integer what_column ();}
\description
  The \var{what_column} function returns the current column number expanding
  tabs, control characters, etc...  The beginning of the line is at
  column number one.
\seealso{whatline, whatpos, goto_column, bolp, eolp}
\done

\variable{what_line}
\synopsis{Get the current line number}
\usage{Int_Type what_line}
\description
  The value of the \var{what_line} specifies the current line number.
  Lines are numbered from one. 

\notes
  This is a read-only variable.

  The actual number is measured from the top of the buffer which is
  itself is affected by whether the buffer is narrowed or not.  For
  example,
#v+
   define one ()
   {
     push_mark (); narrow ();
     return what_line;
   }
#v-
 always returns 1.
\seealso{what_column, goto_line}
\done

