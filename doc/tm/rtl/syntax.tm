\function{create_syntax_table}
\synopsis{Create a new syntax table "name"}
\usage{Void create_syntax_table (String name);}
\description
  This the purpose of this function is to create a new syntax table
  with the name specified by \var{name}.  If the table already exists,
  this clears the table of its current syntax entries.
\seealso{define_syntax, use_syntax_table, define_keywords, what_syntax_table}
\done

\function{define_keywords_n}
\synopsis{Define a set of keywords}
\usage{String define_keywords_n (String table, String kws, Integer len, Integer N);}
\description
  This function is used to define a set of keywords for the syntax
  table \var{table} to be color syntax highlighted in the
  \exmp{"keywordN"} color,  The first parameter, \var{table}, specifies which
  syntax table is to be used for the definition. The second parameter,
  \var{kws}, is a string that is the concatenation of keywords of length
  specified by the last parameter \var{len}.  The list of keywords specified
  by \var{kws} must be in alphabetic order.  The function returns the
  previous list of keywords of length \var{len}. For example, C mode uses
  the statement
#v+
        () = define_keywords_n ("C", "asmforintnewtry", 3, 0);
#v-
  to define the four three-letter keywords \var{asm}, \var{for},
  \var{int}, \var{new}, and \var{try} to be given the
  \exmp{"keyword0"} color.  Note that in the above example, the return
  value is not used.
\seealso{define_syntax, set_color}
\seealso{WANT_SYNTAX_HIGHLIGHT,USE_ANSI_COLORS}
\done

\function{define_syntax}
\synopsis{Add a syntax entry to the table "name"}
\usage{Void define_syntax (..., Integer type, String name);}
\description
  This function adds a syntax entry to the table specified by the last
  parameter \var{name}.  The actual number of parameters vary according to
  the next to the last parameter \var{type}.

  If \var{type} is \exmp{'"'} or \exmp{'\\''}, a string or character delimiter syntax is
  defined. In this case, \var{define_syntax} only takes three parameters
  where the first parameter is an integer that represents the character
  for which the syntax is to be applied.

  Similarly, if \var{type} is \exmp{'\\\\'}, then a quote syntax is defined and
  again \var{define_syntax} only takes three parameters where the first
  parameter is an integer that represents the character for which the
  syntax is to be applied.  A quote character is one in which the
  syntax of the following character is not treated as special.

  If \var{type} is \exmp{'('}, then \var{define_syntax} takes four parameters where
  the first two parameters are strings that represent a matching set of
  delimiters.  The first string contains the set of opening delimiters
  and the second string specifies the set of closing delimiters that
  match the first set.  If a character from the closing set is entered
  into the buffer, the corresponding delimiter from the opening set
  will be blinked.  For example, if the C language syntax table is
  called \exmp{"C"}, then one would use
#v+
        define_syntax ("([{", ")]}", '(', "C");
#v-
  to declare the matching delimiter set.  Note that the order of the
  characters in the two strings must correspond.  That is, the above
  example says that \exmp{'('} matches \exmp{')'} and so on.

  If \var{type} is \exmp{'%'}, a comment syntax is defined.  As in the
  previous case, \var{define_syntax} takes four parameters where there
  first two parameters are strings that represent the begin and end
  comment delimiters.  If the comment syntax is such that the comment
  ends at the end of a line, the second string must either be the
  empty string, \exmp{""}, or a newline \exmp{"\\n"}.  The current
  implementation supports at most two such types of comments.

  If \var{type} is \exmp{'+'}, the first parameter is a string whose characters
  are given the operator syntax.  If type is \exmp{','}, the first parameter
  is a string composed of characters that are considered to be
  delimiters.  If type is '0', the first parameter is a string composed
  of characters that make up a number.

  If \var{type} is \exmp{<}, the first parameter is a string whose successive
  characters form begin and end keyword highlight directives.

  Finally, if \var{type} is \exmp{'#'}, the first parameter is an integer whose
  value corresponds to the character used to begin preprocessor lines.

  As an example, imagine a language in which the dollar sign character
  \exmp{$} is used as a string delimiter, the backward quote character \exmp{`}
  is used as a quote character, comments begin with a semi-colon and
  end at the end of a line, and the characters \exmp{'<'} and \exmp{'>'} form
  matching delimiters.  The one might use
#v+
        create_syntax_table ("strange");
        define_syntax ('$',        '"',  "strange");
        define_syntax ('`',        '\\', "strange");
        define_syntax (";", "",    '%',  "strange");
        define_syntax ("<", ">",   '(',  "strange");
#v-
  to create a syntax table called \exmp{"strange"} and define the
  syntax entries for appropriate this example.
\seealso{create_syntax_table, use_syntax_table, find_matching_delimiter}
\seealso{BLINK}
\done

\function{dfa_build_highlight_table}
\synopsis{Build a DFA table for the syntax table "n"}
\usage{Void dfa_build_highlight_table (String n);}
\description
  This function builds a DFA table for the enhanced syntax
  highlighting scheme specified for the syntax table specified
  by the name \var{n}. This must be called before any syntax
  highlighting will be done for that syntax table.
\seealso{create_syntax_table, use_syntax_table, dfa_define_highlight_rule, dfa_enable_highlight_cache}
\seealso{WANT_SYNTAX_HIGHLIGHT, USE_ANSI_COLORS}
\done

\function{dfa_define_highlight_rule}
\synopsis{Add an DFA rule to the syntax table "n"}
\usage{Void dfa_define_highlight_rule (String rule, String color, String n);}
\description
  This function adds an enhanced highlighting rule to the
  syntax table specified by the name \var{n}. The rule is described
  as a regular expression by the string \var{rule}, and the
  associated color is given by the string \var{color}, in the same
  format as is passed to \var{set_color}. For example:
#v+
        create_syntax_table ("demo");
        dfa_define_highlight_rule ("[A-Za-z][A-Za-z0-9]*", "keyword", "demo");
        dfa_define_highlight_rule ("//.*$", "comment", "demo");
        dfa_build_highlight_table ("demo");
#v-
  causes a syntax table to be defined in which any string of
  alphanumeric characters beginning with an alphabetic is
  highlighted in keyword color, and anything after "//" on a
  line is highlighted in comment color.

  The regular expression syntax understands character classes
  like \exmp{[a-z]} and \exmp{[^a-z0-9]}, parentheses, \exmp{+}, \exmp{*}, \exmp{?}, \exmp{|}
  and \exmp{.}. Any metacharacter can be escaped using a backslash
  so that it can be used as a normal character, but beware that
  due to the syntax of S-Lang strings the backslash has to be
  doubled when specified as a string constant. For example:
#v+
        dfa_define_highlight_rule ("^[ \t]*\\*+[ \t].*$", "comment", "C");
#v-
  defines any line beginning with optional whitespace, then one
  or more asterisks, then more whitespace to be a comment. Note
  the doubled backslash before the \exmp{*}.

  Note also that \var{dfa_build_highlight_table} must be called before
  the syntax highlighting can take effect.
\seealso{create_syntax_table, use_syntax_table, dfa_build_highlight_table}
\seealso{WANT_SYNTAX_HIGHLIGHT, USE_ANSI_COLORS}
\done

\function{dfa_enable_highlight_cache}
\synopsis{Enable caching of the DFA table}
\usage{Void dfa_enable_highlight_cache (String file, String n);}
\description
  This function enables caching of the DFA table for the
  enhanced syntax highlighting scheme belonging to the syntax
  table specified by the name \var{n}. This should be called before
  any calls to \var{dfa_define_highlight_rule} or to
  \var{dfa_build_highlight_table}. The parameter \var{file}
  specifies the name of the file (stored in the directory set by the
  \var{set_highlight_cache_dir} function) which should be used as a cache.

  For example, in \exmp{cmode.sl} one might write
#v+
        dfa_enable_highlight_cache ("cmode.dfa", "C");
#v-
  to enable caching of the DFA. If caching were not enabled for
  C mode, the DFA would take possibly a couple of seconds to
  compute every time Jed was started.

  Transferring cache files between different computers is
  theoretically possible but not recommended. Transferring them
  between different versions of Jed is not guaranteed to work.
\seealso{create_syntax_table, use_syntax_table, dfa_define_highlight_rule, dfa_build_highlight_table}
\seealso{WANT_SYNTAX_HIGHLIGHT, USE_ANSI_COLORS}
\done

\function{dfa_set_init_callback}
\synopsis{Set a callback to initialize a DFA syntax table}
\usage{Void dfa_set_init_callback (Ref_Type func, String_Type tbl)}
\description
  This function defines a callback function \var{func} that will be
  used to build a DFA syntax table for the syntax table \var{tbl}.
  When the \var{use_dfa_syntax} function is called to enable syntax
  highlighting, the callback function \var{func} will be called to to
  create the specified syntax table if it does not already exist.
\seealso{create_syntax_table, use_syntax_table, dfa_define_highlight_rule, dfa_enable_highlight_cache}
\seealso{WANT_SYNTAX_HIGHLIGHT, USE_ANSI_COLORS}
\done

\function{parse_to_point}
\synopsis{Attempt to determine the syntactic context of the point}
\usage{Integer parse_to_point ();}
\description
  This function attempts to determine the syntactic context of the
  current editing point.  That is, it tries to determine whether or not
  the current point is in a comment, a string, or elsewhere.
  It returns:
#v+
        -2   In a comment
        -1   In a string or a character
         0   Neither of the above
#v-
  Note: This routine is rather simplistic since it makes the assumption
  that the character at the beginning of the current line is not in a
  comment nor is in a string.
\seealso{define_syntax, find_matching_delimiter}
\done

\function{set_fortran_comment_chars}
\synopsis{Specify characters for fortran-like comments}
\usage{Void set_fortran_comment_chars (String_Type table, String_Type list}
\description
  This function may be used to specify the set of characters that
  denote fortran style comments.  The first parameter \var{table} is
  the name of a previously defined syntax table, and \var{list}
  denotes the set of characters that specify the fortran-style
  comment.
  
  The string \var{list} is simply a set of characters and may include
  character ranges.  If the first character of \var{list} is
  \var{'^'}, then the meaning is that only those characters that do
  not specify Fortran style comments are included in the list.
\example
  Fortran mode uses the following:
#v+
     set_fortran_comment_chars ("FORTRAN", "^0-9 \t\n");
#v-
  This means that if any line that begins with any character
  except the characters \exmp{0} to \exmp{9}, the space, tab, and
  newline characters will denote a comment.
\notes
  The usefulness of this function is not limited to fortran modes.  In
  fact, many languages have fortran-style comments.  

  This function is meaningful only if the syntax table has
  fortran-style comments as specified via the \var{set_syntax_flags}
  function.
\seealso{define_syntax, set_syntax_flags}
\done

\function{set_highlight_cache_dir}
\synopsis{Set the directory for the dfa syntax highlighting cache files}
\usage{Void set_highlight_cache_dir (String dir);}
\description
  This function sets the directory where the dfa syntax highlighting
  cache files are located.
  See also: \var{dfa_enable_highlight_cache}
\done

\function{set_syntax_flags}
\synopsis{Set the flags in the syntax table "table"}
\usage{Void set_syntax_flags (String table, Integer flag);}
\description
  This function may be used to set the flags in the syntax table
  specified by the \var{table} parameter.  The \var{flag} parameter may take
  any of the following values or any combination bitwise or-ed together:
#v+
      0x01     Keywords are case insensitive
      0x02     Comments are Fortran-like
      0x04     Ignore leading whitespace in C comments
      0x08     Keywords are TeX-like
      0x10     EOL style comments must be surrounded by whitespace.
      0x20     Syntax highlight whole preprocessor line in same color.
      0x40     Leading whitespace allowed for preprocessor lines.
      0x80     Strings do not span lines
#v-
  A Fortran-like comment means that any line that begins with certain
  specified characters is considered to be a comment.  This special
  subset of characters must be specified via a call to the
  \var{set_fortran_comment_chars} function.

  If the \exmp{0x04} bit is set, then whitespace at the beginning of a
  line in a C comment preceding a \exmp{'*'} character will not be 
  highlighted.

  A TeX-like keyword is any word that follows the quote character.
  
  An EOL style comment is one that ends at the end of the line.
\seealso{define_syntax, set_fortran_comment_chars}
\done

\function{use_dfa_syntax}
\synopsis{Turn on/off DFA syntax highlighting for the current mode}
\usage{use_syntax_table (Int_Type on_off)}
\description
  This function may be used to turn on or off DFA syntax highlighting
  for the current mode according to whether or not the \var{on_off}
  parameter is non-zero.  The most useful way of using this function
  is from within a mode hook.
\example
  The following example illustrates how to use this function to enable
  DFA syntax highlighting for C mode:
#v+
    define c_mode_hook ()
    {
       use_dfa_syntax (1);
    }
#v-
\seealso{enable_dfa_syntax_for_mode, disable_dfa_syntax_for_mode}
\done

\function{use_syntax_table}
\synopsis{Associate the current buffer with the syntax table "n"}
\usage{Void use_syntax_table (String n);}
\description
  This function associates the current buffer with the syntax table
  specified by the name \var{n}.  Until another syntax table is associated
  with the buffer, the syntax table named \var{n} will be used in all
  operations that require a syntax.  This includes parenthesis matching,
  indentation, etc.
\seealso{create_syntax_table, define_syntax, what_syntax_table}
\done

\function{what_syntax_table}
\synopsis{Get the name of the active syntax table}
\usage{String_Type what_syntax_table ()}
\description
  This function returns of the name of the syntax table used by the
  current buffer.  If no table is in effect it returns \NULL.
\seealso{define_syntax, use_syntax_table, define_keywords}
\done

