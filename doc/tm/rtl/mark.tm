\function{create_line_mark}
\synopsis{Return a line mark (of the type User_Mark)}
\usage{User_Mark create_line_mark (Integer c)}
\description
  The function \var{create_line_mark} returns an object of the type
  \var{User_Mark}.  This object contains information regarding the current
  position and current buffer.  The parameter \var{c} is used to specify the
  color to use when the line is displayed.
\seealso{create_user_mark, set_color_object}
\done

\function{create_user_mark}
\synopsis{Return an object of the type User_Mark}
\usage{User_Mark create_user_mark ()}
\description
  The function \var{create_user_mark} returns an object of the type
  \var{User_Mark}. This object contains information regarding the current
  position and current buffer.
\seealso{move_user_mark, goto_user_mark, user_mark_buffer}
\done

\function{dupmark}
\synopsis{Duplicate the mark (if set), return success}
\usage{Integer dupmark ()}
\description
  This function returns zero if the mark is not set or, if the mark is
  set, a duplicate of it is pushed onto the mark stack and a non-zero
  value is returned.
\seealso{push_mark, markp, pop_mark}
\done

\function{goto_user_mark}
\synopsis{Return to the position of the User Mark "mark"}
\usage{Void goto_user_mark (User_Mark mark)}
\description
  This function returns to the position of the User Mark \var{mark}.  Before
  this function may be called, the current buffer must be the buffer
  associated with the makr.
\seealso{move_user_mark, create_user_mark, user_mark_buffer}
\done

\function{is_user_mark_in_narrow}
\synopsis{Test if the user mark "m" is within the narrowed buffer.}
\usage{Integer is_user_mark_in_narrow (User_Mark m)}
\description
  This function returns non-zero if the user mark \var{m} refers to a
  position that is within the current narrow restriction of the current
  buffer.  It returns zero if the mark lies outside the restriction.
  An error will be generated if \var{m} does not represent a mark for the current
  buffer.
\seealso{goto_user_mark, move_user_mark}
\done

\function{is_visible_mark}
\synopsis{Test if the mark is a visible mark}
\usage{is_visible_mark ()}
\description
  This function may be used to test whether or not the mark is a visible
  mark.  A visible mar is one which causes the region defined by it to
  be highlighted.
  It returns \var{1} is the mark is visible, or \exmp{0} if the mark
  is not visible or does not exist.
\seealso{markp, push_mark}
\done

\function{markp}
\synopsis{Test if a mark is set}
\usage{Void markp ()}
\description
  This function returns a non-zero value if the mark is set; otherwise, it
  returns zero.  If a mark is set, a region is defined.
\seealso{push_mark, pop_mark, check_region, push_spot}
\done

\function{move_user_mark}
\synopsis{Move the User Mark "mark" to the current position}
\usage{Void move_user_mark (User_Mark mark)}
\description
  This function call takes a previously created User Mark, \var{mark}, and
  moves it to the current position and buffer.  This means that if one
  subsequently calls \var{goto_user_mark} with this mark as an argument, the
  the position will be set to the new position.

  Note: This function call is not equivalent to simply using
#v+
        mark = create_user_mark ();
#v-
  because independent copies of a User Mark are not created upon
  assignment.  That is, if one has
#v+
        variable mark1, mark2;
        setbuf ("first");
        mark1 = create_user_mark ();
        mark2 = mark1;
        setbuf ("second");
#v-
  and then calls
#v+
        move_user_mark (mark1);
#v-
  both user marks, \var{mark1} and \var{mark2} will be moved since
  they refer to the same mark.
\seealso{goto_user_mark, create_user_mark, user_mark_buffer}
\done

\function{pop_mark}
\synopsis{Pop the most recent mark (if "g" is non-zero, go there}
\usage{pop_mark (Integer g)}
\description
  \var{pop_mark} pops the most recent mark pushed onto the mark stack.  If
  the argument \var{g} is non-zero, the editing position will be moved to
  the location of the mark.  However, if \var{g} is zero, the editing
  position will be unchanged.
\seealso{push_mark, pop_spot, markp, check_region, goto_user_mark}
\done

\function{pop_spot}
\synopsis{Pop the last spot (and go there)}
\usage{Void pop_spot ()}
\description
  This function is used after a call to \var{push_spot} to return to the
  editing position at the last call to \var{push_spot} in the current buffer.
\seealso{push_spot, pop_mark}
\done

\function{push_mark}
\synopsis{Mark the current position as the beginning of a region}
\usage{Void push_mark()}
\description
  This function marks the current position as the beginning of a region.
  and pushes other marks onto a stack.  A region is defined by this
  mark and the editing point.  The mark is removed from the stack only
  when the function \var{pop_mark} is called.
  For example,
#v+
        define mark_buffer ()
        {
          bob ();
          push_mark ();
          eob ();
        }
#v-
  marks the entire buffer as a region.
\seealso{pop_mark, push_spot, markp, dupmark, check_region}
\done

\function{push_spot}
\synopsis{Push the current buffer location onto a stack}
\usage{Void push_spot ()}
\description
  \var{push_spot} pushes the location of the current buffer location onto a
  stack.  This function does not set the mark.  The function \var{push_mark}
  should be used for that purpose. The spot can be returned to using the
  function \var{pop_spot}.
  Note: Spots are local to each buffer.  It is not
  possible to call \var{push_spot} from one buffer and then subsequently
  call \var{pop_spot} from another buffer to return to the position in the
  first buffer.  For this purpose, one must use user marks instead.
\seealso{pop_spot, push_mark, create_user_mark}
\done

\function{user_mark_buffer}
\synopsis{Return the name of the buffer with User Mark "m"}
\usage{String user_mark_buffer (User_Mark m)}
\description
  This function returns the name of the buffer associated with the
  User Mark specified by \var{m}.
\seealso{goto_user_mark, create_user_mark, move_user_mark, is_user_mark_in_narrow}
\done

