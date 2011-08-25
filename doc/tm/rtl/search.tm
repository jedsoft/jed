\variable{CASE_SEARCH}
\synopsis{Control the case-sensitivity for searches in the current buffer}
\usage{Int_Type CASE_SEARCH}
\description
 If the value of \var{CASE_SEARCH} is non-zero, text searches
 performed in the current buffer will be case-sensitive, otherwise
 case-insensitive searches will be performed.  The value of this
 variable may vary from buffer to buffer.
\seealso{CASE_SEARCH_DEFAULT, fsearch, ffind}
\done

\variable{CASE_SEARCH_DEFAULT}
\synopsis{The default CASE_SEARCH setting for newly created buffers}
\usage{Int_Type CASE_SEARCH_DEFAULT}
\description
 Searches in a buffer are performed according to the value of the
 \var{CASE_SEARCH} variable.  Newly created buffers are given a
 \var{CASE_SEARCH} value of \var{CASE_SEARCH_DEFAULT}.  The default
 value of \var{CASE_SEARCH_DEFAULT} is 0.
\seealso{CASE_SEARCH, fsearch, ffind}
\done

\function{bfind}
\synopsis{Search backward to the beginning of the line}
\usage{Integer bfind (String str)}
\description
  \var{bfind} searches backward from the current position to the beginning
  of the line for the string \var{str}.  If a match is found, the length of
  \var{str} is returned and the current point is moved to the start of the
  match. If no match is found, zero is returned.
  Note: This function respects the setting of the \var{CASE_SEARCH} variable.
\seealso{bsearch, ffind, bol_bsearch, re_bsearch}
\seealso{CASE_SEARCH}
\done

\function{bfind_char}
\synopsis{Search backward on the current line for a character}
\usage{Integer fsearch_char (Integer ch)}
\description
  This function searches backward on the current line for a character
  \var{ch}.  If it is found, \var{1} is returned; otherwise \exmp{0} is returned.
\seealso{fsearch_char, ffind_char, fsearch}
\seealso{CASE_SEARCH}
\done


\function{blooking_at}
\synopsis{Test if the characters immediately preceding the point match a string}
\usage{Int_Type blooking_at (String_Type str)}
\description
  This function returns non-zero if the characters immediately preceding
  the current editing point match the string specified by \exmp{str}.  Whether
  the match is case-sensitive or not depends upon the value of the
  variable \var{CASE_SEARCH}.  The function returns a non-zero value
  if there is a match; otherwise zero will be returned to indicated no
  match.
\seealso{ffind, fsearch, re_fsearch, bfind, looking_at, re_looking_at}
\done

\function{bol_bsearch}
\synopsis{Search backward for "str" at the beginning of a line}
\usage{Integer bol_bsearch (str)}
\description
  \var{bol_bsearch} searches backward from the current point until the
  beginning of the buffer for the occurrences of the string \var{str} at
  the beginning of a line.  If a match is found, the length of \var{str} is
  returned and the current point is moved to the start of the match. If
  no match is found, zero is returned.

  Note: \var{bol_bsearch} is much faster than using \var{re_bsearch} to perform
  a search that matches the beginning of a line.
\seealso{bol_fsearch, bsearch, bfind, re_bsearch}
\seealso{CASE_SEARCH}
\done

\function{bol_bsearch_char}
\synopsis{Search backward for character "ch" at the beginning of a line}
\usage{Integer bol_fsearch_char (Integer ch)}
\description
  This function searches backward for a character \var{ch} at the beginning
  of a line.  If it is found, \var{1} is returned; otherwise \exmp{0} is returned.
\seealso{bol_bsearch, bol_fsearch_char, bsearch_char}
\seealso{CASE_SEARCH}
\done

\function{bol_fsearch}
\synopsis{Search forward for "str" at the beginning of a line}
\usage{Integer bol_fsearch (str)}
\description
  \var{bol_fsearch} searches forward from the current point until the end
  of the buffer for occurrences of the string \var{str} at the beginning of
  a line.  If a match is found, the length of \var{str} is returned and the
  current point is moved to the start of the match.  If no match is
  found, zero is returned.
  Note: \var{bol_fsearch} is much faster than using \var{re_fsearch} to perform
  a search that matches the beginning of a line.
\seealso{bol_bsearch, fsearch, ffind, re_fsearch}
\seealso{CASE_SEARCH}
\done

\function{bol_fsearch_char}
\synopsis{Search forward for character "ch" at the beginning of a line}
\usage{Integer bol_fsearch_char (Integer ch)}
\description
  This function searches forward for a character \var{ch} at the beginning
  of a line.  If it is found, \var{1} is returned; otherwise \exmp{0} is returned.
\seealso{bol_fsearch, bol_bsearch_char, fsearch_char}
\seealso{CASE_SEARCH}
\done

\function{bsearch}
\synopsis{Search backward for "str"}
\usage{Integer bsearch (String str)}
\description
  The \var{bsearch} function searches backward from the current position
  for the string \var{str}.  If \var{str} is found, this function will return
  the length of \var{str} and move the current position to the beginning of
  the matched text.  If a match is not found, zero will be returned and
  the position will not change.  It respects the value of the variable
  \var{CASE_SEARCH}.
\seealso{fsearch, bol_bsearch, re_bsearch}
\done

\function{bsearch_char}
\synopsis{Search backward for a character}
\usage{Integer bsearch_char (Integer ch)}
\description
  This function searches backward for a character \var{ch}.  If it is
  found, \var{1} is returned; otherwise \exmp{0} is returned.
\seealso{fsearch_char, ffind_char, fsearch}
\seealso{CASE_SEARCH}
\done

\function{ffind}
\synopsis{Search forward to the end of the line for the string "str"}
\usage{Integer ffind (String s)}
\description
  \var{ffind} searches forward from the current position to the end of the
  line for the string \var{str}.  If a match is found, the length of \var{str}
  is returned and the current point is moved to the start of the match.
  If no match is found, zero is returned.
  Note: This function respects the setting of the \var{CASE_SEARCH} variable.
  To perform a search that includes multiple lines, use the \var{fsearch}
  function.
\seealso{fsearch, bfind, re_fsearch, bol_fsearch}
\done

\function{ffind_char}
\synopsis{Search forward on the current line for character "ch"}
\usage{Integer ffind_char (Integer ch)}
\description
  This function searches forward on the current line for the character
  \var{ch}.  If it is found, \var{1} is returned; otherwise \exmp{0}
  is returned.
\seealso{fsearch_char, bfind_char, fsearch}
\seealso{CASE_SEARCH}
\done

\function{find_matching_delimiter}
\synopsis{Look for the delimiter that matches "ch"}
\usage{Integer find_matching_delimiter (Integer ch)}
\description
  This function scans either forward or backward looking for the
  delimiter that matches the character specified by \var{ch}.  The actual
  direction depends upon the syntax of the character \var{ch}.  The
  matching delimiter pair must be declared as such by a prior call to
  \var{define_syntax}.  This function returns one of the following values:
#v+
         1    Match found
         0    Match not found
        -1    A match was attempted from within a string.
        -2    A match was attempted from within a comment
         2    No information
#v-
  In addition, the current point is left either at the match or is left
  at the place where the routine either detected a mismatch or gave up.
  In the case of a comment or a string (return values of -2 or -1), the
  current point is left at the beginning of a comment.
  Note: If the of \var{ch} is zero, the character at the current point will be
  used.
\seealso{blink_match, create_syntax_table, define_syntax, parse_to_point}
\done

\function{fsearch}
\synopsis{Search forward for the string "str"}
\usage{Integer fsearch (String str)}
\description
  This function may be used to search forward in buffer looking for the
  string \var{str}.  If not found, this functions returns zero.  However,
  if found, the length of the string is returned and the current point
  is moved to the to the start of the match.  It respects the setting
  of the variable \var{CASE_SEARCH}.  If the string that one is searching
  for is known to be at the beginning of a line, the function
  \var{bol_fsearch} should be used instead.

  Note: This function cannot find a match that crosses lines.
\seealso{ffind, fsearch_char, bsearch, bol_fsearch, re_fsearch, looking_at}
\seealso{CASE_SEARCH}
\done

\function{fsearch_char}
\synopsis{Search forward for a character}
\usage{Integer fsearch_char (Integer ch)}
\description
  This function searches forward for the character \var{ch}.  If it is
  found, \var{1} is returned; otherwise \exmp{0} is returned.
\seealso{fsearch, ffind_char, bsearch_char}
\seealso{CASE_SEARCH}
\done

\function{looking_at}
\synopsis{Test if the characters immediately following the point match "s"}
\usage{Integer looking_at (String s)}
\description
  This function returns non-zero if the characters immediately following
  the current editing point match the string specified by \var{s}.  Whether
  the match is case-sensitive or not depends upon the value of the
  variable \var{CASE_SEARCH}.  The function returns zero if there is no match.
\seealso{ffind, fsearch, re_fsearch, bfind}
\done

\function{re_bsearch}
\synopsis{Search backward for regular expression "pattern"}
\usage{Integer re_bsearch(String pattern)}
\description
  Search backward for regular expression \var{pattern}.  This function returns
  the 1 + length of the string  matched.  If no match is found, it returns
  0.
\seealso{bsearch, bol_bsearch, re_fsearch}
\done

\function{re_fsearch}
\synopsis{Search forward for regular expression "pattern"}
\usage{Integer re_fsearch(String pattern)}
\description
  Search forward for regular expression \var{pattern}.  This function returns
  the 1 + length of the string  matched.  If no match is found, it returns
  0.
\seealso{fsearch, bol_fsearch, re_bsearch}
\done

\function{regexp_nth_match}
\synopsis{Return the nth sub-expression from the last re search}
\usage{String regexp_nth_match (Integer n)}
\description
  This function returns the nth sub-expression matched by the last regular
  expression search.  If the parameter \var{n} is zero, the entire match is
  returned.
  Note: The value returned by this function is meaningful only if the
  editing point has not been moved since the match.
\seealso{re_fsearch, re_bsearch}
\done

\function{replace}
\synopsis{Replace all occurances of "old" with "new"}
\usage{Void replace(String old, String new)}
\description
  This function may be used to replace all occurances of the string
  \var{old} with the string, \var{new}, from current editing point to the end
  of the buffer. The editing point is returned to the initial location.
  That is, this function does not move the editing point.
\seealso{replace_chars, fsearch, re_fsearch, bsearch, ffind, del}
\seealso{REPLACE_PRESERVE_CASE}
\done

\function{replace_chars}
\synopsis{Replace the next "n" characters with another string}
\usage{Int_Type replace_chars (Int_Type n, String_Type new)}
\description
  This function may be used to replace the next \exmp{n} characters at the
  editing position by the string \exmp{new}.  After the replacement, the editing
  point will be moved to the end of the inserted string.  The length of
  the replacement string \exmp{new} is returned.
\seealso{fsearch, re_fsearch, bsearch, ffind, del}
\seealso{REPLACE_PRESERVE_CASE}
\done

\function{replace_match}
\synopsis{Replace text previously matched with "re_fsearch" or "re_bsearch"}
\usage{Int_Type replace_match(String_Type str, Int_Type method)}
\description
  This function replaces text previously matched with \var{re_fsearch}
  or \var{re_bsearch} at the current editing point with string
  \exmp{str}.  If \exmp{method} is zero, \exmp{str} is a specially formatted
  string of the form described below. If \exmp{method} is non-zero,
  \exmp{str} is regarded as a simple string and is used literally.  If
  the replacement fails, this function returns zero otherwise, it
  returns a non-zero value.
\notes
  This function should be used at the position of the corresponding
  match and nowhere else.
\done

\function{search_file}
\synopsis{Regular expression search for strings in a disk file}
\usage{Integer search_file (String filename, String re, Integer nmax)}
\description
  This function may be used to search for strings in a disk file
  matching the regular expression \exmp{re}.  The first argument \var{filename}
  specifies which file to search.  The last argument \var{nmax} specifies
  how many matches to return.  Each line that is matched is pushed onto
  the S-Lang stack.  The number of matches (limited by \var{nmax}) is returned.
  If the file contains no matches, zero is returned.
\done

