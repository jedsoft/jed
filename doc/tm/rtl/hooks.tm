\function{add_to_hook}
\synopsis{Add a function to a hook (as first one)}
\usage{add_to_hook (String_Type hook_name, Ref_Type funct)}
\description
  \var{add_to_hook} adds the function \var{funct} to the beginnning of the
  list of hooks associated with \var{hook_name}.  The currently
  supported hooks include:
#v+
    _jed_append_region_hooks
    _jed_exit_hooks
    _jed_find_file_after_hooks
    _jed_find_file_before_hooks
    _jed_init_display_hooks
    _jed_insert_file_hooks
    _jed_quit_hooks
    _jed_read_file_hooks
    _jed_reset_display_hooks
    _jed_resume_hooks
    _jed_save_buffer_after_hooks
    _jed_save_buffer_before_hooks
    _jed_set_mode_hooks
    _jed_switch_active_buffer_hooks
    _jed_suspend_hooks
    _jed_write_region_hooks
#v-
 See the file \var{hooks.txt} in the main \jed distribution for more
 information.
\seealso{append_to_hook, remove_from_hook}
\done

\function{append_to_hook}
\synopsis{Append a function to a hook}
\usage{append_to_hook (String_Type hook_name, Ref_Type funct)}
\description
  \var{append_to_hook} adds the function \var{funct} to the end of the
  list of hooks associated with \var{hook_name}.  See
  \var{add_to_hook} for more information.
\seealso{add_to_hook, remove_from_hook}
\done

\function{remove_from_hook}
\synopsis{Remove a function from a list of hooks}
\usage{remove_from_hook (String_Type hook_name, Ref_Type funct)}
\description
  \var{remove_from_hook} removes the function \var{funct} from the
  list of hooks associated with \var{hook_name}.
\seealso{add_to_hook, append_to_hook}
\done

\function{_jed_run_hooks}
\synopsis{Execute the functions in a hook chain}
\usage{_jed_run_hooks (String_Type hook_name, Integer_Type mode [,Array_Type args])}
\description
 \ifun{_jed_run_hooks} executes the functions registered for \var{hook_name}.
 The argument \exmp{mode} defines which functions are called:
#v+
  JED_HOOKS_RUN_ALL
     all functions; no function should return a value.
  JED_HOOKS_RUN_UNTIL_0
     Run until a function returns the value 0; every function must return 
     an integer value.
  JED_HOOKS_RUN_UNTIL_NON_0
     Run until a function returns a non-zero
      value; every function must return an integer value.
#v-
 If the third argument is given, then it must be an array of strings
 to be passed as arguments to the the hook-functions. 
\seealso{append_to_hook, add_to_hook}
\done


\function{get_buffer_hook}
\synopsis{Get the value of a specified buffer hook}
\usage{Ref_Type get_buffer_hook (String_Type hook_name)}
\description
  The \ifun{get_buffer_hook} function returns the value of the
  specified hook name as a function reference.  If no hook was defined
  or the hook does not exist, then \NULL will be returned.  See the
  documentation for \ifun{set_buffer_hook} for a list of hook-names.
\example
#v+
   % temporarily unset the indent hook
   fun = get_buffer_hook ("indent_hook");
   unset_buffer_hook ("indent_hook");
      .
      .
   % restore the indent hook
   if (fun != NULL)
     set_buffer_hook ("indent_hook", fun);
#v-
\seealso{set_buffer_hook, unset_buffer_hook}
\done

\function{set_buffer_hook}
\synopsis{Set a specified buffer hook}
\usage{set_buffer_hook (String_Type hook_name, Ref_Type func)}
\description
  This function sets the specified hook for the current buffer to the
  function \exmp{func}.  The value of \exmp{func} may either be a
  string or a function reference (e.g., \exmp{&my_hook}).  The use of
  a function reference is preferred since that allows hooks to be
  static or private functions.

  Currently supported hooks include:
#v+
         "par_sep"  -- returns zero if the current line does not
              constitute the beginning or end of a paragraph.
              It returns non-zero otherwise.  The default value of hook is
              is_paragraph_separator.
         "indent_hook" -- returns nothing.  It is called by the indent line
              routines.
         "wrap_hook"   hook that is called after a line is wrapped.  Returns
              nothing.
         "wrapok_hook"  hook that is called prior to wrapping a line.
              If it returns non-zero, the line will be wrapped,
              otherwise the line will not be wrapped.
         "newline_indent_hook"  --- returns nothing.  If this hook is defined,
              it will be called instead of the internal function
              newline_and_indent is called.
         "bob_eob_error_hook"  --- returns nothing.  If this hook is defined,
              it will be called whenever an error one of the internal cursor
              movement functions would have generated an end of buffer or beginning of
              buffer error.  It is passed an integer that indicates which function
              would have generated the error.  Specifically:
       
                       -1  previous_line_cmd
                       -2  previous_char_cmd
                       -3  page_up
                        1  next_line_cmd
                        2  next_char_cmd
                        3  page_down
         "mouse_down", "mouse_up", "mouse_drag" "mouse_2click" "mouse_3click"
              These hooks are used to override default hooks defined by the
              mouse_set_default_hook function.
         "mark_paragraph_hook"
              This hook hook is called by the format_paragraph
              function to mark a paragraph.
         "forward_paragraph_hook", "backward_paragraph_hook"
         "format_paragraph_hook"
#v-
 See the file jed/doc/hooks.txt for more information and examples.
\seealso{unset_buffer_hook, mouse_set_default_hook, get_buffer_hook}
\done

\function{unset_buffer_hook}
\synopsis{Remove a buffer hook}
\usage{unset_buffer_hook (String_Type name)}
\description
  The \ifun{unset_buffer_hook} function removes a specified buffer hook
  from the current buffer.  If \exmp{name} is the empty string, then
  all the buffer hooks of the current buffer will be unset.  See the
  documentation for \ifun{set_buffer_hook} for a list of hook-names.
\seealso{set_buffer_hook, get_buffer_hook}
\done

