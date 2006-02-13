\variable{MESSAGE_BUFFER}
\synopsis{The Contents of the Message Buffer}
\usage{String_Type MESSAGE_BUFFER}
\description
  The \var{MESSAGE_BUFFER} variable is a read-only string variable
  whose value indicates the text to be displayed or is currently
  displayed in the message buffer.
\seealso{message, vmessage, error, verror}
\done

\function{beep}
\synopsis{Beep (according to the value of \var{IGNORE_BEEP})}
\usage{Void beep ();}
\description
  The \var{beep} function causes the terminal to beep according to the value
  of the variable \var{IGNORE_BEEP}.
\seealso{tt_send}
\seealso{IGNORE_BEEP}
\done

\function{clear_message}
\synopsis{Clear the message line of the display}
\usage{Void clear_message ();}
\description
  This function may be used to clear the message line of the display.
\seealso{message, update, error, flush}
\done

\function{flush}
\synopsis{Immediately display "msg" as a message in the mini-buffer}
\usage{Void flush (String msg);}
\description
  The \var{flush} function behaves like \var{message} except that it immediately
  displays its argument \var{msg} as a message in the mini-buffer.  That is,
  it is not necessary to call \var{update} to see the message appear.
\seealso{message, error}
\done

\function{tt_send}
\synopsis{Send "s" directly to the terminal}
\usage{Void tt_send (String s);}
\description
  This function may be used to send a string specified by \var{s} directly
  to the terminal with no interference by the editor.  One should
  exercise caution when using this routine since it may interfere with
  JED's screen management routines forcing one to redraw the screen.
  Nevertheless, it can serve a useful purpose.  For example, when run in
  an XTerm window, using
#v+
        tt_send ("\e[?9h");
#v-
  will enable sending mouse click information to JED encoded as
  keypresses.
\seealso{beep}
\done

