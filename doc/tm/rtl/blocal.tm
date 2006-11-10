\function{blocal_var_exists}
\synopsis{Determine whether a buffer-local variable exists}
\usage{Int_Type blocal_var_exists (String_Type name)}
\description
  The \var{blocal_var_exists} function returns non-zero if the
  specified buffer-local variable exists.  It returns zero of the
  variable does not exists.
\seealso{get_blocal_var, create_blocal_var, set_blocal_var, define_blocal_var}
\done

\function{create_blocal_var}
\synopsis{Create a buffer local variable "name"}
\usage{Void create_blocal_var (String name)}
\description
  This function is used to create a buffer local variable named
  \var{name}. A buffer local variable is a variable whose value is
  local to the current buffer.
\seealso{get_blocal_var, set_blocal_var, define_blocal_var}
\done

\function{set_blocal_var}
\synopsis{Set the buffer local variable "v" to value "val"}
\usage{Void set_blocal_var (val, String v)}
\description
  This function sets the value of the buffer local variable with name \var{v}
  to value \var{val}.  The buffer local variable specified by \var{v} must have
  been previously created by the \var{create_blocal_var} function.  \var{val} must
  have the type that was declared when \var{create_blocal_var} was called.
\seealso{get_blocal_var, create_blocal_var}
\done

