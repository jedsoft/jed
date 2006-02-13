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

\function{set_buffer_hook}
\synopsis{Set current buffer hook "hook" to function "f"}
\usage{set_buffer_hook (String_Type hook, String_Type f)}
\description
  Set current buffer hook \var{hook} to function \var{f}. \var{f} is a user
  defined S-Lang function.  Currently supported hooks include:
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
\seealso{unset_buffer_hook, mouse_set_default_hook}
\done

\function{unset_buffer_hook}
\synopsis{Remove a buffer hook}
\usage{unset_buffer_hook (String_Type name)}
\description
  The \var{unset_buffer_hook} function removes a specified buffer hook
  from the current buffer.  If \var{name} is the empty string, then
  all the buffer hooks of the current buffer will be unset.
\seealso{set_buffer_hook}
\done

