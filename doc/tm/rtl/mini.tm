\variable{MINIBUFFER_ACTIVE}
\synopsis{Non-zero is the mini-buffer is in use}
\usage{Int_Type MINIBUFFER_ACTIVE}
\description
 The \var{MINIBUFFER_ACTIVE} variable will be non-zero if the
 mini-buffer is in use.
\seealso{read_mini}
\done

\function{_add_completion}
\synopsis{"add_completion" for "n" names "f1", ... "fn"}
\usage{Void _add_completion (String f1, String f2, ..., Integer n);}
\description
  The \var{_add_completion} function is like the \var{add_completion} function
  except that it takes \var{n} names \var{f1}, ... \var{fn}.
  For example,
#v+
        _add_completion ("fun_a", "fun_b", 2);
#v-
  is equivalent to
#v+
        add_completion ("fun_a");
        add_completion ("fun_b");
#v-
\seealso{add_completion}
\done

\function{add_completion}
\synopsis{Add the function "f" to the list for mini-buffer completion}
\usage{Void add_completion(String f);}
\description
  The \var{add_completion} function adds the user defined S-Lang function
  with name specified by the string \var{f} to the list of functions that
  are eligible for mini-buffer completion.  The function specified by
  \var{f} must be already defined before this function is called.  The
  S-Lang function \var{is_defined} may be used to test whether or not the
  function is defined.
\seealso{read_with_completion, _add_completion}
\done

\function{get_mini_response}
\synopsis{Prompt for a key}
\usage{Int_Type get_mini_response (String_Type str)}
\description
  The \var{get_mini_response} function display the text \var{str} at
  the bottom of the screen and waits for the user to press a key.  The
  key is returned.
\seealso{read_mini, getkey, flush}
\done

\function{get_y_or_n}
\synopsis{Prompt for a y or n response}
\usage{Int_Type get_y_or_n (String_Type str)}
\description
  The \var{get_y_or_n} function forms a y/n question by
  concatenating \exmp{"? (y/n)"} to \var{str} and displays the result
  at the bottom of the display.  It returns \1 if the user responds
  with \exmp{y}, \0 with \exmp{n}, or \exmp{-1} if the user cancelled
  the prompt.
\seealso{get_yes_no, get_mini_response}
\done

\function{get_yes_no}
\synopsis{Get a yes or no response from the user}
\usage{Integer get_yes_no (String s);}
\description
  This function may be used to get a yes or no response from the
  user.  The string parameter \var{s} will be used to construct the prompt
  by concating the string \exmp{"? (yes/no)"} to \var{s}.
  It returns \var{1} if the answer is yes or \exmp{0} if the answer is no.
\seealso{getkey, flush, message}
\done

\function{read_mini}
\synopsis{Read input from the mini-buffer}
\usage{String read_mini (String prompt, String dflt, String init);}
\description
  The \var{read_mini} function reads a line of input from the user in the
  mini-buffer.  The first parameter, \var{prompt}, is used to prompt the
  user.  The second parameter, \var{dflt}, is what is returned as a default
  value if the user simply presses the return key.  The final parameter,
  \var{init}, is stuffed into the mini-buffer for editing by the user.
  For example,
#v+
        define search_whole_buffer ()
        {
          variable str;
          str = read_mini ("Search for:", "", "");
          ifnot (strlen (str)) return;
          ifnot (fsearch (str))
             {
               push_mark (); bob ();
               if (fsearch (str)) pop_mark (0);
               else pop_mark (1);
                 {
                    pop_mark (1);
                    error ("Not found");
                 }
             }
        }
#v-
  reads a string from the user and then searches forward for it and if
  not found, it resumes the search from the beginning of the buffer.
\notes
  If the user aborts the function \var{mini_read} by pressing the
  keyboard quit character (e.g., Ctrl-G), an error is signaled.  This
  error can be caught by a \var{try}-\var{catch} statement and the appropriate action
  taken. Also if the mini-buffer is already in use, this function should
  not be called.  The variable \var{MINIBUFFER_ACTIVE} may be checked to
  determine if this is the case or not.
\seealso{read_with_completion, getkey, input_pending}
\seealso{MINIBUFFER_ACTIVE}
\done

\function{read_with_completion}
\synopsis{Read input from the minibuffer (with completion)}
\usage{Void read_with_completion (String prt, String dflt, String s, Integer type);}
\description
  This function may be used to read one of the objects specified by the
  last parameter \var{type}.  The first parameter, \var{prt}, is used as a
  prompt, the second parameter, \var{dflt}, is used to specify a default,
  and the third parameter, \var{s}, is used to initialize the string to
  be read.
  \var{type} is an integer with the following meanings:
#v+
        'f'   file name
        'b'   buffer name
        'F'   function name
        'V'   variable name.

#v-
  Finally, if \var{type} has the value \exmp{'s'}, then the set of completions
  will be defined by a zeroth parameter, \var{list}, to the function call.
  This parameter is simple a comma separated list of completions.
  For example,
#v+
        read_with_completion ("Larry,Curly,Moe", "Favorite Stooge:",
                              "Larry", "", 's');
#v-
  provides completion over the set of three stooges.
  The function returns the string read.
\seealso{read_mini}
\done

\function{set_expansion_hook}
\synopsis{Specify a function to expand a filename upon TAB completion}
\usage{Void set_expansion_hook (String fname);}
\description
  This function may be used to specify a function that will be called to
  expand a filename upon TAB completion.  The function \var{fname} must
  already be defined.  When \var{fname} is called, it is given a string to
  be expanded. If it changes the string, it must return a non-zero value
  and the modified string.  If the string is not modified, it must simply
  return zero.
\done

