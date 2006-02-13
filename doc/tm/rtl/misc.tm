\variable{BATCH}
\synopsis{Non-Zero if in Batch Mode}
\usage{Int_Type BATCH}
\description
 \var{BATCH} is a read-only variable will be zero if the editor is run
 in interactive or full-screen mode.  It will be \1 if the editor is
 in batch mode (via the \exmp{-batch} comment line argument).  If the
 editor is in script mode (via \exmp{-script}), then the value of
 \var{BATCH} will be \2.
\seealso{}
\done

\variable{JED_ROOT}
\synopsis{Location of the JED root directory}
\usage{String_Type JED_ROOT}
\description
  This is a read-only string variable whose value indicates JED's root
  directory.  This variable may be set using the \var{JED_ROOT}
  environment variable.
\seealso{get_jed_library_path, set_jed_library_path, getenv}
\done

\function{_exit}
\synopsis{Exit the editor with a status value}
\usage{_exit (Int_Type status)}
\description
  This function performs the same actions as \var{quit_jed} except
  that the status code returned to the shell may be specified.
\seealso{quit_jed, exit, exit_jed}
\done

\variable{_jed_secure_mode}
\synopsis{Indicates if the editor is in secure mode}
\usage{Int_Type _jed_secure_mode}
\description
  The value of \var{_jed_secure_mode} will be non-zero if the editor
  is running in secure mode.  This mode does not allow any access to
  the shell.
\seealso{system}
\done

\variable{_jed_version}
\synopsis{The JED version number}
\usage{Int_Type _jed_version}
\description
  The value of \var{_jed_version} represents the version number of the
  editor.
\seealso{_jed_version_string, _slang_version}
\done

\variable{_jed_version_string}
\synopsis{The JED version number as a string}
\usage{String_Type _jed_version_string}
\description
  The value of \var{_jed_version_string} represents the version number
  of the editor.
\seealso{_jed_version, _slang_version_string}
\done

\function{call}
\synopsis{Execute an internal function}
\usage{Void call(String f)}
\description
  The \var{call} function is used to execute an internal function which is
  not directly accessable to the S-Lang interpreter.
\seealso{is_internal}
\done

\function{core_dump}
\synopsis{Exit the editor dumping the state of some crucial variables}
\usage{Void core_dump(String msg, Integer severity)}
\description
  \var{core_dump} will exit the editor dumping the state of some crucial
  variables. If \var{severity} is \var{1}, a core dump will result.  Immediately
  before dumping, \var{msg} will be displayed.
\seealso{exit, exit_jed, quit_jed, message, error}
\done

\function{define_word}
\synopsis{Define the set of characters that form a word}
\usage{Void define_word (String s)}
\description
  This function is used to define the set of characters that form a
  word. The string \var{s} consists of those characters or ranges of
  characters that define the word.  For example, to define only the
  characters \exmp{A-Z} and \exmp{a-z} as word characters, use:
#v+
        define_word ("A-Za-z");
#v-
  To include a hyphen as part of a word, it must be the first character
  of the control string \var{s}.  So for example,
#v+
        define_word ("-i-n");
#v-
  defines a word to consist only of the letters \var{i} to \var{n} and the
  hyphen character.
\done

\function{exit}
\synopsis{Exit the editor gracefully with a status value}
\usage{exit (Int_Type status)}
\description
  This function performs the same actions as \var{exit_jed} except
  that the status code returned to the shell may be specified.
\seealso{_exit, exit_jed}
\done

\function{exit_jed}
\synopsis{Exit JED in a graceful and safe manner}
\usage{Void exit_jed ()}
\description
  This function should be called to exit JED in a graceful and safe
  manner.  If any buffers have been modified but not saved, the user is
  queried about whether or not to save each one first.  \var{exit_jed} calls
  the S-Lang hook \var{exit_hook} if it is defined.  If \var{exit_hook} is
  defined, it must either call \var{quit_jed} or \var{exit_jed} to really exit
  the editor.  If \var{exit_jed} is called from \var{exit_hook}, \var{exit_hook} will
  not be called again.  For example:
#v+
        define exit_hook ()
        {
          flush ("Really Exit?");
       
          forever
            {
              switch (getkey () & 0x20)    % map to lowercase
               { case 'y': exit_jed (); }
               { case 'n': return; }
              beep ();
            }
        }
#v-
  may be used to prompt user for confirmation of exit.
\seealso{exit, quit_jed, suspend, flush, getkey}
\seealso{BATCH}
\done

\function{get_last_macro}
\synopsis{Return characters composing the last keyboard macro}
\usage{String get_last_macro ()}
\description
  This function returns characters composing the last keyboard macro.  The
  charactors that make up the macro are encoded as themselves except the
  following characters:
#v+
        '\n'    ---->   \J
        null    ---->   \@
         \      ---->   \\
         '"'    ---->   \"
#v-
\done

\function{get_passwd_info}
\synopsis{Return information about the user "username"}
\usage{(dir, shell, pwd, uid, gid) = get_passwd_info (String username)}
\description
  This function returns password information about the user with name
  \var{username}.  The returned variables have the following meaning:
#v+
        dir:     login directory
        shell:   login shell
        pwd:     encripted password
        uid:     user identification number
        gid:     group identification number
#v-
  If the user does not exist, or the system call fails, the function
  returns with \var{uid} and \var{gid} set to \exmp{-1}.
\done

\function{getpid}
\synopsis{Return the process identification number for the editor}
\usage{Integer getpid ()}
\description
  This function returns the process identification number for the current
  editor process.
\done

\function{is_internal}
\synopsis{Tst if function "f" is defined as an internal function}
\usage{Integer is_internal(String f)}
\description
  \var{is_internal} returns non-zero is function \var{f} is defined as an
  internal function or returns zero if not.  Internal functions not
  immediately accessable from S-Lang; rather, they must be called using
  the \var{call} function.  See also the related S-Lang function
  \var{is_defined} in the S-Lang Programmer's Reference.
\seealso{call}
\done

\function{quit_jed}
\synopsis{Quit the editor immediately: no autosave, no hooks}
\usage{Void quit_jed ()}
\description
  This function quits the editor immediately.  No buffers are
  auto-saved and no hooks are called.  The function \var{exit_jed} should be
  called when it is desired to exit in a safe way.
\seealso{exit_jed}
\done

\function{random}
\synopsis{Return a random number in the range [0, "nmax"[}
\usage{Integer random (Integer seed, Integer nmax)}
\description
  The \var{random} function returns a random number in the range 0 to, but
  not including, \var{nmax}.  If the first parameter \var{seed} is 0, the
  number generated depends on a previous seed.  If \var{seed} is -1, the
  current time and process id will be used to seed the random number
  generator; otherwise \var{seed} will be used.

  Example: generate 1000 random integers in the range 0-500 and insert
  them into buffer:
#v+
        () = random (-1, 0);  % seed generator usingtime and pid
        loop (1000)
          insert (Sprintf ("%d\n", random (0, 500), 1));
#v-
  Note: The random number is generated via the expression:
#v+
        r = r * 69069UL + 1013904243UL;
#v-
\done

\function{set_line_readonly}
\synopsis{Turn on or off the read-only state of the current line}
\usage{Void set_line_readonly (Integer flag)}
\description
  This function may be used to turn on or off the read-only state of the
  current line.  If the integer parameter \var{flag} is non-zero, the line
  will be made read-only.  If the paramter is zero, the read-only state
  will be turned off.
\seealso{getbuf_info}
\done

\function{suspend}
\synopsis{Suspend the editor}
\usage{Void suspend ()}
\description
  The action of this command varies with the operating system.
  Under Unix, the editor will be suspended and control will pass to the
  parent process.  Under VMS and MSDOS, a new subprocess will be spawned.
  Before suspension, \var{suspend_hook} is called.  When the editor is
  resumed, \var{resume_hook} will be called.  These hooks are user-defined
  functions that take no arguments and return no values.
\done

\function{usleep}
\synopsis{Pause for "ms" milliseconds}
\usage{Void usleep (Integer ms)}
\description
  A call to usleep will cause the editor to pause for \var{ms} milliseconds.
\seealso{input_pending}
\done

\function{vms_get_help}
\synopsis{Interact with the VMS help system from within the editor}
\usage{Void vms_get_help (String hlp_file, String hlp_topic)}
\description
  This function may be used on VMS systems to interact with the VMS help
  system from within the editor.  \var{hlp_file} is the name of the help file
  to use and \var{hlp_topic} is the topic for which help is desired.
\done

\function{vms_send_mail}
\synopsis{interface to the VMS callable mail facility}
\usage{Integer vms_send_mail (String recip_lis, String subj)}
\description
  This VMS specific function provides an interface to the VMS callable
  mail facility.  The first argument, \var{recip_lis}, is a comma separated list
  of email addresses and \var{subj} is a string that represents the subject of
  the email.  The current buffer will be emailed.  It returns \var{1} upon
  success and \exmp{0} upon failure.
\done

