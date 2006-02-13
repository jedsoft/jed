\function{get_hostname}
\synopsis{Get the name of the host computer}
\usage{String_Type get_hostname ()}
\description
  The \var{get_hostname} function returns the name of the host
  computer.  If the editor is unable to determine the name, and the
  user has not specified a name, then \exmp{"localhost"} is returned.
\seealso{set_hostname, get_realname, get_username}
\done

\function{get_realname}
\synopsis{Get the user's real name}
\usage{String_Type get_realname}
\description
 The \var{get_realname} returns the user's real name.  If the editor
 is unable to determine this value, an empty string is returned.
\seealso{set_realname, get_username, get_hostname}
\done

\function{get_username}
\synopsis{Get the username}
\usage{String_Type get_username ()}
\description
 The \var{get_username} function returns the username associated with
 the current process.  If is is unable to determine this value,
 \exmp{"unknown"} will be returned.
\seealso{set_username, get_realname, get_hostname}
\done

\function{set_hostname}
\synopsis{Set the name of the host}
\usage{set_hostname (String_Type hostname)}
\description
 \var{set_hostname} may be used to set set the name of the host that
 the editor will associate with the current process.
\seealso{get_hostname, set_username, set_realname}
\done

\function{set_realname}
\synopsis{Set the user's realname}
\usage{set_realname (String_Type realname)}
\description
 The \var{set_realname} function sets the editor's notion of what the
 user's real name is such that subsequent calls to \var{get_realname}
 will return the specified value.
\seealso{get_realname, get_username, set_username, set_hostname}
\done

\function{set_username}
\synopsis{Set the username of the editor process}
\usage{set_username (String_Type username)}
\description
 This function may be used to specify the username associated with the
 editor process.
\seealso{get_username, set_realname, set_hostname}
\done

