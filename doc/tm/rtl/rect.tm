\function{Replace all text in the rectangle by spaces}
\synopsis{Replace the rectangle defined by point and mark with spaces}
\usage{Void blank_rect ()}
\description
   The \var{blank_rect} function replaces all text in the rectangle defined by
  the current editing point and the mark by spaces.
\seealso{push_mark, kill_rect, insert_rect, copy_rect}
\done

\function{copy_rect}
\synopsis{Copy the contents of the rectangle to the rectangle buffer}
\usage{Void copy_rect ()}
\description
  The \var{copy_rect} function is used to copy the contents of the
  currently defined rectangle to the rectangle buffer.  It overwrites
  the previous contents of the rectangle buffer.  A rectangle is defined
  by the diagonal formed by the mark and the current point.
\seealso{insert_rect, kill_rect, blank_rect}
\done

\function{insert_rect}
\synopsis{Insert the contents of the rectangle buffer}
\usage{insert_rect ()}
\description
  The \var{insert_rect} function inserts the contents of the rectangle buffer
  at the current editing point.  The rectangle buffer is not modified.
  Any text that the rectangle would overwrite is moved to the right by an
  amount that is equal to the width of the rectangle.
\seealso{kill_rect, blank_rect, copy_rect}
\done

\function{kill_rect}
\synopsis{Delete the rectangle and place a copy in the rectangle buffer}
\usage{Void kill_rect ()}
\description
  This function deletes the rectangle defined by the mark and the current
  point.  The contents of the rectangle are saved in the rectangle buffer
  for later retrieval via the \var{insert_rect} function.  The previous
  contents of the rectangle buffer will be lost.
\seealso{insert_rect, blank_rect, copy_rect}
\done

\function{open_rect}
\synopsis{Insert a blank rectangle determined by mark and point}
\usage{Void open_rect ()}
\description
  The \var{open_rect} function may be used to insert a blank rectangle whose
  size is determined by the mark and the current editing point.  Any text
  that lies in the region of the rectangle will be pushed to the right.
\seealso{insert_rect, kill_rect, copy_rect}
\done

