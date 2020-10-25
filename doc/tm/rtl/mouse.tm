\function{gpm_disable_mouse}
\synopsis{Disable support for the GPM mouse}
\usage{gpm_disable_mouse ()}
\description
  The \var{gpm_disable_mouse} function may be used to inactivate
  support for the GPM mouse.
\notes 
  This function may not be available on all systems.
\seealso{mouse_get_event_info}
\done

\function{mouse_get_event_info}
\synopsis{Return information about the last processed mouse event}
\usage{(x, y, state) = mouse_get_event_info ()}
\description
  This function returns the position of the last processed
  mouse event, and the state of the mouse buttons and shift
  keys before the event.

  \var{x} and \var{y} represent the column and row, respectively, where
  the event took place. They are measured with relative to the
  top left corner of the editor's display.

  \var{state} is a bitmapped integer whose bits are defined as follows:
#v+
         1  Left button pressed
         2  Middle button pressed
         4  Right button pressed
         8  Shift key pressed
        16  Ctrl key pressed
#v-
  Other information such as the button that triggered the event is
  available when the mouse handler is called.  As a result, this information
  is not returned by \var{mouse_get_event_info}.
\seealso{mouse_set_default_hook, set_buffer_hook}
\done

\function{mouse_map_buttons}
\synopsis{Map one mouse button to another}
\usage{Void mouse_map_buttons (Integer x, Integer y)}
\description
  This function may be used to map one mouse button to another.  The
  button represented by \var{x} will appear as \var{y}.
\done

\function{mouse_set_current_window}
\synopsis{Change to the window to that of the the mouse event}
\usage{Void mouse_set_current_window ()}
\description
  Use of this function results in changing windows to the window that
  was current at the time of the mouse event.
\seealso{mouse_set_default_hook}
\done

\function{mouse_set_default_hook}
\synopsis{Associate a function "fun" with the mouse event "name"}
\usage{Void set_default_mouse_hook (String name, String fun)}
\description
  This function associates a slang function \var{fun} with the mouse event
  specified by \var{name}.  The first parameter \var{name} must be one of the
  following:
#v+
            "mouse_up"          "mouse_status_up"
            "mouse_down"        "mouse_status_down"
            "mouse_drag"        "mouse_status_drag"
            "mouse_2click"      "mouse_status_2click"
            "mouse_3click"      "mouse_status_3click"
#v-
  The meaning of these names should be obvious.  The second parameter,
  \var{fun} must be defined as
#v+
           define fun (line, column, btn, shift)
#v-
  and it must return an integer.  The parameters \var{line} and
  \var{column} correspond to the line and column numbers in the
  buffer where the event took place. \var{btn} is an integer that
  corresponds to the button triggering the event.  It can take
  on values \var{1}, \var{2}, and \var{4} corresponding to the left,
  middle, and right buttons, respectively.  \var{shift} can take on
  values \exmp{0}, \var{1}, or \var{2} where \exmp{0} indicates that no modifier
  key was pressed, \var{1} indicates that the SHIFT key was
  pressed, and \var{2} indicates that the CTRL key was pressed.
  For more detailed information about the modifier keys, use
  the function \var{mouse_get_event_info}.

  When the hook is called, the editor will automatically change
  to the window where the event occurred.  The return value of
  the hook is used to dictate whether or not hook handled the
  event or whether the editor should switch back to the window
  prior to the event.  Specifically, the return value is interpreted
  as follows:

#v+
          -1     Event not handled, pass to default hook.
           0     Event handled, return active window prior to event
           1     Event handled, stay in current window.
#v-
\seealso{mouse_get_event_info, mouse_set_current_window, set_buffer_hook}
\done

