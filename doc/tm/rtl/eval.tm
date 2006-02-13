\function{_autoload}
\synopsis{Specify multiple functions to autoload}
\usage{Void _autoload (String fun, String fn, ..., Integer n)}
\description
  The \var{_autoload} function is like the \var{autoload} function except that
  it takes \var{n} pairs of function name (\var{fun}) / filename (\var{fn}) pairs.
  For example,
#v+
        _autoload ("fun_a", "file_a", "fun_b", "file_b", 2);
#v-
  is equivalent to
#v+
        autoload ("fun_a", "file_a");
        autoload ("fun_b", "file_b");
#v-
\seealso{autoload}
\done

\function{evalbuffer}
\synopsis{Eval the current buffer as S-Lang script}
\usage{Void evalbuffer ()}
\description
  This function causes the current buffer to be sent to the S-Lang
  interpreter for evaluation.  If an error is encountered while parsing
  the buffer, the cursor will be placed at the location of the error.
\seealso{evalfile}
\done

\function{get_jed_library_path}
\synopsis{Return the current search path for jed library files}
\usage{String get_jed_library_path ()}
\description
  This function returns the current search path for jed library files.
  The path may be set using the function \var{set_jed_library_path}.
\seealso{set_jed_library_path}
\done

\function{set_jed_library_path}
\synopsis{Set the search path for library files}
\usage{Void set_jed_library_path (String p)}
\description
  This function may be used to set the search path for library files.
  Its parameter \var{p} may be a comma separated list of directories to
  search.  When the editor is first started, the path is initialized
  from the \var{JED_ROOT}, or \var{JED_LIBRARY} environment variables.
\seealso{get_jed_library_path}
\done

