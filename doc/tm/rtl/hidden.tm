\function{is_line_hidden}
\synopsis{Test if the current line is hidden}
\usage{Integer is_line_hidden ()}
\description
  This function returns a non-zero value if the current line is hidden.  It
  will return zero if the current line is visible.
\seealso{set_line_hidden}
\done

\function{set_line_hidden}
\synopsis{Set the hidden attribute: 1 hidden, 0 visible}
\usage{Void set_line_hidden (Integer flag)}
\description
  If the parameter \var{flag} is non-zero, the current line will be given
  the hidden attribute.  This means that it will not be displayed.  If the
  parameter is zero, the hidden attribute will be turned off.
\seealso{set_region_hidden, is_line_hidden}
\done

\function{set_region_hidden}
\synopsis{Set the hidden attribute for all lines in a region}
\usage{Void set_region_hidden (Integer flag)}
\description
  This function may be used to hide the lines in a region.  If \var{flag} is
  non-zero, all lines in the region will be hidden.  If it is zero, the
  lines in the region will be made visible.
\seealso{set_line_hidden, is_line_hidden, skip_hidden_lines_forward}
\done

\function{skip_hidden_lines_backward}
\synopsis{Move backward across either hidden or non-hidden lines}
\usage{Void skip_hidden_lines_backward (Integer type)}
\description
  This function may be used to move backward across either hidden or non-hidden
  lines depending upon whether the parameter \var{type} is non-zero or zero.
  If \var{type} is non-zero, the Point is moved backward across hidden lines
  until a visible line is reached.  If \var{type} is zero, visible lines will
  be skipped instead.  If the top of the buffer is reached before the
  appropriate line is reached, the Point will be left there.

  Note: The functions \var{up} and \var{down} are insensitive to whether or not
  a line is hidden.
\seealso{skip_hidden_lines_forward, is_line_hidden}
\done

\function{skip_hidden_lines_forward}
\synopsis{Move forward across either hidden or non-hidden lines}
\usage{Void skip_hidden_lines_forward (Integer type)}
\description
  This function may be used to move forward across either hidden or non-hidden
  lines depending upon whether the parameter \var{type} is non-zero or zero.
  If \var{type} is non-zero, the Point is moved forward across hidden lines
  until a visible line is reached.  If \var{type} is zero, visible lines will
  be skipped instead.  If the end of the buffer is reached before the
  appropriate line is reached, the Point will be left there.

  Note: The functions \var{up} and \var{down} are insensitive to whether or not
  a line is hidden.
\seealso{skip_hidden_lines_backward, is_line_hidden}
\done

