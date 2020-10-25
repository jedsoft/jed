\variable{BLINK}
\synopsis{Set whether or not parentheses will be blinked}
\usage{Int_Type BLINK}
\description
  The \var{BLINK} variable controls whether or not matching
  parenthesis are blinked upon the insertion of a closing parenthesis.
  If its value is non-zero, the matching parenthesis will be blinked;
  otherwise, it will not.
\done

\variable{DISPLAY_EIGHT_BIT}
\synopsis{Set the display mode for eight-bit characters}
\usage{Int_Type DISPLAY_EIGHT_BIT}
\description
 This variable determines how characters with the high bit set are to
 be displayed.  Specifically, any character whose value is greater than
 or equal to the value of \var{DISPLAY_EIGHT_BIT} is output to the terminal
 as is.  Characters with the high bit set but less than this value are
 sent to the terminal in a multiple character representation. For Unix
 and VMS systems the value should be set to 160.  This is because many
 terminals use the characters with values between 128 and 160 as eight
 bit control characters.  For other systems, it can be set to zero.
\seealso{META_CHAR}
\done

\variable{DISPLAY_TIME}
\synopsis{Control the display of the current time}
\usage{Int_Type DISPLAY_TIME}
\description
 If this variable is non-zero, the current time will be displayed on the
 status line if the format for the status line permits it.  If it is zero,
 the time will not be displayed even if the \exmp{%t} format string is part
 of the status line format.
\seealso{set_status_line}
\done

\variable{DOLLAR_CHARACTER}
\synopsis{The line continuation character}
\usage{Int_Type DOLLAR_CHARACTER = '$'}
\description
  The character represented by \var{DOLLAR_CHARACTER} is used to
  indicate that text extends beyond the borders of the window.  This
  character is traditionally a dollar sign.  If the value of
  \var{DOLLAR_CHARACTER} is 0, no character will be used for this
  indicator.
\seealso{set_color}
\done

\variable{HIGHLIGHT}
\synopsis{Turn on/off region highlighting}
\usage{Int_Type HIGHLIGHT}
\description
  If this variable is non-zero, marked regions will be highlighted.
\seealso{WANT_SYNTAX_HIGHLIGHT, set_color}
\done

\variable{HORIZONTAL_PAN}
\synopsis{Set the horizontal panning mode}
\usage{Int_Type HORIZONTAL_PAN}
\description
  If the value of this variable is non-zero, the window will pan when
  the cursor goes outside the border of the window.  More precisely, 
  if the value is less than zero, the entire window will pan.  If the
  value is positive, only the current line will pan.  The
  absolute value of the number determines the panning increment.
\seealso{SCREEN_WIDTH}
\done

\variable{LINENUMBERS}
\synopsis{Enable the display of line or column numbers}
\usage{Int_Type LINENUMBERS}
\description
  The \var{LINENUMBERS} variable determines whether or not line or
  column numbers will be displayed on the status line.  If the value
  of \var{LINENUMBERS} is \0, then neither the line nor column number
  information will be displayed.  If \var{LINENUMBERS} is set to \1,
  then the current line number will be displayed but column numbers
  will not be.  If \var{LINENUMBERS} is \2, the both line a column
  numbers will be displayed.
\seealso{set_status_line}
\done

\variable{Simulate_Graphic_Chars}
\synopsis{Specifies whether or not graphic characters are to be used}
\usage{Int_Type Simulate_Graphic_Chars}
\description
  If the value of this variable is non-zero, graphic characters will
  be simulated by simple ascii characters instead of trying to use the
  terminal's alternate character set.
\notes
  This variable is not available on all platforms.
\done

\variable{Status_Line_String}
\synopsis{The string used for the status line}
\usage{String_Type Status_Line_String}
\description
 \var{Status_Line_String} is a read-only string variable that
 specifies the format of the status line for newly created buffers.
 To set the status line format, use the function \var{set_status_line}.
\seealso{set_status_line}
\done

\variable{TAB}
\synopsis{Set the current buffer TAB width}
\usage{Int_Type TAB}
\description
 This variable controls the tab width associated with the current
 buffer.  A value of zero means that tab characters are not expanded
 and that tabs are never used to produce whitespace.
\seealso{TAB_DEFAULT, USE_TABS, USE_TABS_DEFAULT}
\done

\variable{TAB_DEFAULT}
\synopsis{Set the default tab width}
\usage{Int_Type TAB_DEFAULT}
\description
 The value of \var{TAB_DEFAULT} is the default tab setting given to
 all newly created buffers.  A value of zero means that tab characters
 are not expanded and that tabs are never used to produce whitespace.
\notes
 A related variable \var{TAB} may be used to change the current
 buffer's tab setting.
\seealso{TAB, USE_TABS, TAB_DEFAULT, USE_TABS_DEFAULT}
\done

\variable{TOP_WINDOW_ROW}
\synopsis{Top window's starting row}
\usage{Int_Type}
\description
 This read-only variable gives the value of the starting row of the top
 window.  If a menubar is present, the value will be \2, otherwise it
 will be \1.
\seealso{enable_top_status_line, window_info}
\done

\variable{WANT_EOB}
\synopsis{Control the display of the end of buffer indicator}
\usage{Int_Type}
\description
 If this value of this variable is non-zero, the end of buffer
 indicator \exmp{"[EOB]"} will be displayed at the end of the buffer.  Such
 an indicator is used for various editor emulations such as the
 VAX/VMS EDT editor.
\done

\variable{WANT_SYNTAX_HIGHLIGHT}
\synopsis{Enable or disable syntax highlighting}
\usage{Int_Type WANT_SYNTAX_HIGHLIGHT}
\description
  If the value of this variable is non-zero, syntax highlighting will
  be enabled.  Otherwise, syntax highlighting will be turned off.
\seealso{HIGHLIGHT, set_color}
\done

\function{blink_match}
\synopsis{Blink the matching delimiter}
\usage{Void blink_match ();}
\description
  This function will attempt to blink the matching delimiter immediately
  before the editing point.
\seealso{find_matching_delimiter, define_syntax}
\done

\function{enlargewin}
\synopsis{Increase the size of the current window}
\usage{Void enlargewin ()}
\description
  This function increases the size of the current window by one line by
  adjusting the size of the other windows accordingly.
\seealso{window_info, onewindow}
\done

\function{nwindows}
\synopsis{Return the number of windows currently visible}
\usage{Integer nwindows ();}
\description
  The \var{nwindows} function returns the number of windows currently visible.
  If the variable \var{MINIBUFFER_ACTIVE} is non-zero, the minibuffer is busy and
  contributes to the number of windows.
\seealso{splitwindow, onewindow, window_size}
\seealso{MINIBUFFER_ACTIVE}
\done

\function{onewindow}
\synopsis{Make current window the only one}
\usage{Void onewindow ();}
\description
  This function deletes all other windows except the current window and
  the mini-buffer window.
\seealso{nwindows, splitwindow, enlargewin}
\seealso{MINIBUFFER_ACTIVE}
\done

\function{otherwindow}
\synopsis{Make the next window the default window}
\usage{Void otherwindow ()}
\description
  This function will make the next window in the ring of windows as the
  default window. For example,
#v+
        define zoom_next_window ()
        {
          otherwindow (); onewindow ();
        }
#v-
  defines a function that moves to the next window and then makes it the
  only window on the screen.
\seealso{nwindows, onewindow}
\seealso{MINIBUFFER_ACTIVE}
\done

\function{recenter}
\synopsis{Scroll the window to make the "nth" line contain the current line}
\usage{Void recenter (Integer nth);}
\description
  This function may be used to scroll the window such that the \var{nth} line
  of the window contains the current line.  If \var{nth} is zero, the current
  line will be placed at the center of the window and the screen will be
  completely redrawn.
\seealso{nwindows, window_info}
\done

\function{set_status_line}
\synopsis{Customize the status line of the current window}
\usage{set_status_line (String format, Integer flag)}
\description
  This function may be used to customize the status line of the current
  window according to the string \var{format}.  If the second parameter
  \var{flag} is non-zero, \var{format} will apply to the global format string;
  otherwise it applies to current buffer only.  Newly created buffer
  inherit the global format string when they appear in a window.
  The format string may contain the following format specifiers:
#v+
        %b   buffer name
        %f   file name (without the directory part)
        %F   file name with directory
        %v   JED version
        %t   current time --- only used if variable DISPLAY_TIME is non-zero
        %p   line number or percent string. If LINENUMBERS is 2, this
              expands to "line number,column number"
        %c   column number
        %%   literal '%' character
        %m   mode string
        %a   If abbrev mode, expands to "abbrev"
        %n   If buffer is narrowed, expands to "Narrow"
        %o   If overwrite mode, expands to "Ovwrt"
        %O   Overwrite/Insert flag - like %o, but shows INS/OVR
        %l   Shows current line number
        %L   Shows number of lines in the file
        %C   Use the colour specified by the numeric prefix for the
             remainder of the status line. For instance, %5C selects colour 5.
        %T   Expands to 'tab:' or 'spc:' (depending on whether tabs or spaces are
             being used to indent the current buffer) followed by the indentation
             size
        %W   If wrap mode is enabled, expands to 'wrap:' followed by the wrap size,
             otherwise, expands to 'nowrap'
        %S   The current S-Lang stack depth
#v-
  For example, the default status line used by JED's EDT emulation uses
  the format string:
#v+
        "(Jed %v) EDT: %b   (%m%a%n%o)  %p,%c   Advance   %t"
#v-
  All the format specifiers can include an optional number between the
  '%' and the format character to indicate the width of the field into
  which the value should be formatted, using an optional leading '-'
  character to indicate that the data should be left-justified. For
  example '%5c' will format the current column into a field at least 3
  characters wide.
\seealso{set_mode, narrow, whatbuf, getbuf_info}
\seealso{DISPLAY_TIME,LINENUMBERS, Global_Top_Status_Line, Status_Line_String}
\done

\function{splitwindow}
\synopsis{Split the current window vertically}
\usage{Void splitwindow ();}
\description
  This function splits the current window vertically creating another
  window that carries the current window's buffer.
\seealso{onewindow, enlargewin, window_info}
\done

\function{update}
\synopsis{Update the display}
\usage{Void update (Integer f);}
\description
  This function may be called to update the display.  If the parameter
  \var{f} is non-zero, the display will be updated even if there is input
  pending.  If \var{f} is zero, the display may only be partially updated if
  input is pending.
\seealso{input_pending, flush}
\done

\function{update_sans_update_hook}
\synopsis{Update the display without running the update hooks}
\usage{update_sans_update_hook (Int_Type force)}
\description
  The \var{update_sans_update_hook} function performs the same
  function as \var{update}, except that the buffer's update hook will
  not be run.  See \var{update} for more information.
\seealso{update, set_buffer_hook, unset_buffer_hook}
\done

\function{w132}
\synopsis{Set the number of columns on a vtxxx compatable terminal to 132.}
\usage{Void w132 ()}
\description
  This function may be used to set the number of columns on a vtxxx
  compatable terminal to 132.
\seealso{w80, set_term_vtxxx}
\done

\function{w80}
\synopsis{Set the number of columns on a vtxxx compatable terminal to 80}
\usage{Void w80 ()}
\description
  This function may be used to set the number of columns on a vtxxx
  compatable terminal to 80.
\seealso{w132, set_term_vtxxx}
\done

\function{window_info}
\synopsis{Return information concerning the current window}
\usage{Integer window_info(Integer item);}
\description
  The \var{window_info} function returns information concerning the current
  window.  The actual information that is returned depends on the \var{item}
  parameter.  Acceptable values of \var{item} and the description of the
  information returned is given in the following table:
#v+
        'r'  : Number of rows
        'w'  : Width of window
        'c'  : Starting column (from 1)
        't'  : Screen line of top line of window (from 1)
#v-
\seealso{otherwindow, nwindows}
\seealso{SCREEN_HEIGHT,SCREEN_WIDTH}
\done

\function{window_line}
\synopsis{Return the number of rows from the top of the window}
\usage{Integer window_line ();}
\description
  This function returns the number of rows from the top of the current
  window for the current line.  If the current line is the very first line
  in the window, a value of \var{1} will be returned, i.e., it is the first
  line of the window.
\seealso{window_info, nwindows}
\seealso{TOP_WINDOW_ROW}
\done

\function{get_scroll_column}
\synopsis{Get the scroll column for the current window}
\usage{Int_Type get_scroll_column ()}
\description
 This function returns the scroll column for the current window.
\seealso{set_scroll_column}
\done

\function{set_scroll_column}
\synopsis{Set the scroll column for the current window}
\usage{set_scroll_column (Int_Type col)}
\description
 This function may be used to set the scroll column of the current
 window.
\seealso{get_scroll_column}
\done
