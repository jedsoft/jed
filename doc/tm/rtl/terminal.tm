\variable{CHEAP_VIDEO}
\synopsis{Control flicker on old video cards}
\usage{Int_Type CHEAP_VIDEO}
\description
  Some old video cards for MSDOS systems, most notably the CGA card,
  display snow when updating the card.  This variable should be set to
  \1 to avoid the presence of snow when used with such a card.
\notes
  This variable is not available on all systems.
\seealso{SCREEN_WIDTH, SCREEN_HEIGHT}
\done

\variable{IGNORE_BEEP}
\synopsis{Control beeping of the terminal}
\usage{Int_Type IGNORE_BEEP}
\description
  This variable determines how the terminal is to be beeped.  It may
  be any one of the following values:
#v+
   0    Do not beep the terminal in any way.
   1    Produce an audible beep only.
   2    Produce an visible beep only by flashing the display.
   3    Produce both audible and visible bells.
#v-
\notes
  Not all terminals support visible bells.
\seealso{beep}
\done

\variable{SCREEN_HEIGHT}
\synopsis{Number of display rows}
\usage{Int_Type SCREEN_HEIGHT}
\description
  This is a read-only variable whose value represents the number of
  rows of the display or terminal.
\seealso{SCREEN_WIDTH, window_info}
\done

\variable{SCREEN_WIDTH}
\synopsis{Number of display columns}
\usage{Int_Type SCREEN_WIDTH}
\description
  This is a read-only variable whose value represents the number of
  columns of the display or terminal.
\seealso{SCREEN_HEIGHT, window_info}
\done

\variable{TERM_BLINK_MODE}
\synopsis{Enable the use of high intensity background colors}
\usage{Int_Type TERM_BLINK_MODE}
\description
  If the value of this variable is non-zero, \jed will interpret
  high-intensity background colors as blinking characters.  On some
  terminals, e.g., \exmp{rxvt}, the blink bit will be mapped to an
  actual high intensity background color.
\notes
  This variable is not available on all systems.
\seealso{}
\done

\variable{TERM_CANNOT_INSERT}
\synopsis{Control the use of terminal insertion}
\usage{Int_Type TERM_CANNOT_INSERT}
\description
  The value of this variable indicates whether or not the terminal is
  able to insert.  Do disable the use of the insertion capability, set
  the value of this variable to \0.
\notes
  This variable is not available on all systems.  It is a good idea
  not to set this variable.
\seealso{TERM_CANNOT_SCROLL}
\done

\variable{TERM_CANNOT_SCROLL}
\synopsis{Control the use of the terminal's scrolling capability}
\usage{Int_Type TERM_CANNOT_SCROLL}
\description
  If this variable is set to \0, the hardware scrolling capability of
  the terminal will not be used.  This also means that the window will
  be recentered if the cursor moves outside the top or bottom rows of
  the window.
\notes
  This variable is not available on all systems. 
\seealso{TERM_CANNOT_INSERT}
\done

\variable{USE_ANSI_COLORS}
\synopsis{Enable the use of colors}
\usage{Int_Type USE_ANSI_COLORS}
\description
  The variable \var{USE_ANSI_COLORS} may be used to enable or disable
  color support.  If set to a non-zero value, the terminal will be
  assumed to support ANSI colors.  This value of this variable is
  initially determined by examining the terminal's terminfo file, or
  by looking for the existence of a \var{COLORTERM} environment
  variable.
\notes
  This variable is not available on all platforms.
\seealso{HIGHLIGHT}
\done

\function{get_termcap_string}
\synopsis{Return the keystring associated with the termcap capability "cap"}
\usage{String get_termcap_string (String cap);}
\description
  This function may be used to extract the string associated with the
  termcap capability associated with \var{cap}.
  Note: This function is only available on Unix systems.
\done

\function{set_term_vtxxx}
\synopsis{Set terminal display appropriate for a vtxxx terminal}
\description
  Set terminal display appropriate for a vtxxx terminal.  This function
  takes a single integer parameter.  If non-zero, the terminal type is set
  for a vt100.  This means the terminal lacks the ability to insert/delete
  lines and characters.  If the parameter is zero, the terminal is assumed
  to be vt102 compatible.  Unless you are using a VERY old terminal or
  a primitive emulator, use zero as the parameter.
\done

