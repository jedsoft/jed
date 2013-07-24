\function{Skip past all word characters}
\synopsis{bskip_word_chars}
\usage{Void bskip_word_chars ()}
\description
  This function moves the current editing point backward past all
  word characters until a non-word character is encountered.
  Characters that make up a word are set by the \var{define_word} function.
\seealso{define_word, skip_word_chars, bskip_chars, bskip_non_word_chars}
\done

\function{_get_point}
\synopsis{Get the current offset from the beginning of the line}
\usage{Int_Type _get_point ()}
\description
  The \var{_get_point} function returns the current character offset
  fro the beginning of the line.
\seealso{_set_point, what_column}
\done

\function{_set_point}
\synopsis{Move to a specified offset from the beginning of the line}
\usage{_set_point (Int_Type nth)}
\description
  The \var{_set_point} function moves the current editing position to
  the \var{nth} character of the current line.
\seealso{_get_point, goto_column}
\done

\function{backward_paragraph}
\synopsis{Move backward to a line that is a paragraph separator}
\usage{Void backward_paragraph ()}
\description
  This function moves the current editing point backward past the
  current paragraph to the line that is a paragraph separator.  Such a
  line is determined by the S-Lang hook \var{is_paragraph_separator}.  This
  hook can be modified on a buffer by buffer basis by using the
  function \var{set_buffer_hook}.
\seealso{forward_paragraph, set_buffer_hook}
\done

\function{bob}
\synopsis{Go to the beginning of the buffer}
\usage{Void bob ()}
\description
  The function \var{bob} is used to move the current editing point to the
  beginning of the buffer.  The function \var{bobp} may be used to determine
  if the editing point is at the beginning of the buffer or not.
\seealso{bobp, eob, bol, eol}
\done

\function{bol}
\synopsis{Go to the beginning of the line}
\usage{Void bol()}
\description
  This function moves the current editing point to the beginning of the
  current line.  The function \var{bolp} may be used to see if one is already
  at the beginning of a line.
\seealso{eol, bob, eob, bolp}
\done

\function{bskip_chars}
\synopsis{Skip past all characters defined by string "str"}
\usage{Void bskip_chars (String str)}
\description
  This function may be used to skip backward past all characters defined by the
  string \var{str}.  See \var{skip_chars} for the definition of \var{str}.
  The following example illustrates how to skip past all whitespace
  including newline characters:
#v+
        bskip_chars (" \t\n");
#v-
\seealso{skip_chars, left}
\done

\function{bskip_non_word_chars}
\synopsis{Skip past all non-word characters}
\usage{Void bskip_word_chars ()}
\description
  This function moves the current editing point backward past all
  non-word characters until a word character is encountered.
  Characters that make up a word are set by the \var{define_word} function.
\seealso{define_word, skip_non_word_chars, bskip_chars, bskip_word_chars}
\done

\function{down}
\synopsis{Move the editing point "n" lines down}
\usage{Integer down(Integer n)}
\description
  The \var{down} function is used to move the editing point down a number of
  lines specified by the integer \var{n}.  It returns the number of lines
  actually moved.  The number returned will be less than \var{n} only if the
  last line of the buffer has been reached.  The editing point will be
  left at the beginning of the line if it succeeds in going down more
  than one line.
  Example: The function
#v+
        define trim_buffer
        {
          bob ();
          do
            {
               eol (); trim ();
            }
          while (down (1));
        }
#v-
  removes excess whitespace from the end of every line in the buffer.
\seealso{down, left, right, goto_line}
\done

\function{eob}
\synopsis{Move to the end of the buffer}
\usage{Void eob()}
\description
  The \var{eob} function is used to move the current point to the end of the
  buffer.  The function \var{eobp} may be used to see if the current
  position is at the end of the buffer.
\seealso{eobp, bob, bol, eol}
\done

\function{eol}
\synopsis{Go to the end of the line}
\usage{Void eol()}
\description
  Moves the current position to the end of the current line.  The function
  \var{eolp} may be used to see if one is at the end of a line or not.
\seealso{eolp, bol, bob, eob}
\done

\function{forward_paragraph}
\synopsis{Go to the end of the current paragraph}
\usage{Void forward_paragraph ()}
\description
  This function moves the current editing point forward past the end of
  the current paragraph.  Paragraph delimiters are defined through either
  a buffer hook or via the hook \var{is_paragraph_separator}.
\seealso{backward_paragraph, set_buffer_hook}
\done

\function{goto_column}
\synopsis{Move to the column "n"}
\usage{Void goto_column (Integer n)}
\description
  This function moves the current editing point to the column specified
  by the parameter \var{n}.  It will insert a combination of spaces and tabs
  if necessary to achieve the goal.
  Note: The actual character number offset from the beginning of the
  line depends upon tab settings and the visual expansion of other
  control characters.
\seealso{goto_column_best_try, what_column, left, right, goto_line}
\seealso{TAB,TAB_DEFAULT,DISPLAY_EIGHT_BIT}
\done

\function{goto_column_best_try}
\synopsis{like "goto_column" but don't insert whitespace}
\usage{Integer goto_column_best_try (Integer c)}
\description
  This function is like \var{goto_column} except that it will not insert
  whitespace.  This means that it may fail to achieve the column number
  specified by the argument \var{c}.  It returns the current column number.
\seealso{goto_column, what_column}
\done

\function{goto_line}
\synopsis{Go to line number "n"}
\usage{Void goto_line (Integer n)}
\description
  The \var{goto_line} function may be used to move to a specific line number
  specified by the parameter \var{n}.
  Note: The actual column that the editing point will be left in is
  indeterminate.
\seealso{what_line, goto_column, down, up}
\done

\function{left}
\synopsis{Move the editing point backward "n" characters}
\usage{Integer left(Integer n)}
\description
  \var{left} moves the editing point backward \var{n} characters and returns the
  number actually moved.  The number returned will be less than \var{n} only
  if the top of the buffer is reached.
\seealso{right, up, down, bol, bob}
\done

\function{right}
\synopsis{Move the editing position forward forward "n" characters}
\usage{Integer right(Integer n)}
\description
  This function moves the editing position forward forward \var{n}
  characters. It returns the number of characters actually moved.  The
  number returned will be smaller than \var{n} if the end of the buffer is
  reached.
\seealso{left, up, down, eol, eob}
\done

\function{skip_chars}
\synopsis{Go forward past all characters defined by "s"}
\usage{Void skip_chars(String s)}
\description
  This fnction may be used to move the editing point forward past all
  characters in string \var{s} which contains the chars to skip, or a range
  of characters.  A character range is denoted by two charcters
  separated by a hyphen.  If the first character of the string \var{s} is a
  \exmp{'^'} character, then the list of characters actually denotes the
  complement of the set of characters to be skipped.  To explicitly
  include the hyphen character in the list, it must be either the first
  or the second character of the string, depending upon whether or not
  the \exmp{'^'} character is present. So for example,
#v+
        skip_chars ("- \t0-9ai-o_");
#v-
  will skip the hyphen, space, tab, numerals \exmp{0} to \var{9}, the letter \var{a},
  the letters \var{i} to \var{o}, and underscore.  An example which illustrates
  the complement of a range is
#v+
        skip_chars("^A-Za-z");
#v-
  which skips all characters except the letters.
  Note: The backslash character may be used to escape only the first
  character in the string.  That is, \exmp{"\\\\^"} is to be used to skip over
  \exmp{^} characters.
\seealso{bskip_chars, skip_white}
\done

\function{skip_non_word_chars}
\synopsis{Go forward until a word character is encountered}
\usage{Void skip_non_word_chars ()}
\description
  This function moves the current editing point forward past all
  non-word characters until a word character is encountered.
  Characters that make up a word are set by the \var{define_word} function.
\seealso{define_word, skip_word_chars, skip_chars, bskip_non_word_chars}
\done

\function{skip_white}
\synopsis{Go forward until a non-whitespace character or the end of the line}
\usage{Void skip_white ()}
\description
  The \var{skip_white} function moves the current point forward until it
  reaches a non-whitespace character or the end of the current line,
  whichever happens first.  In this context, whitespace is considered to
  be any combination of space and tab characters.  To skip newline
  characters as well, the function \var{skip_chars} may be used.
\seealso{bskip_chars, what_char, trim, right}
\done

\function{skip_word_chars}
\synopsis{Go forward until a non-word character is
  encountered}
\usage{Void skip_word_chars ()}
\description
  This function moves the current editing point forward across all
  characters that constitute a word until a non-word character is
  encountered. Characters that make up a word are set by the
  \var{define_word} function.
\seealso{define_word, skip_non_word_chars, skip_chars, bskip_word_chars}
\done

\function{up}
\synopsis{Go up "n" lines}
\usage{Integer up(Integer n)}
\description
  This function moves the current point up \var{n} lines and returns the
  number of lines actually moved.  The number returned will be less than
  \var{n} only if the top of the buffer is reached.
\seealso{down, left, right}
\done

