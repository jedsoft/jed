\variable{ALT_CHAR}
\synopsis{Controls the Alt character prefix}
\usage{Int_Type ALT_CHAR}
\description
  If this variable is non-zero, characters pressed in combination the
  \exmp{Alt} key will generate a two character sequence: the first
  character is the value of \var{ALT_CHAR} itself followed by the
  character pressed.  For example, if \exmp{Alt-X} is pressed and
  \var{ALT_CHAR} has a value of 27, the characters \exmp{ESC X} will be
  generated.
\notes
  This variable may not be available on all platforms.
\seealso{META_CHAR, FN_CHAR}
\done

\variable{CURRENT_KBD_COMMAND}
\synopsis{The currently executing keyboard command}
\usage{String_Type CURRENT_KBD_COMMAND}
\description
  The value of the \var{CURRENT_KBD_COMMAND} function represents the
  name of the currently executing procedure bound to the currently
  executing key sequence.
\seealso{LASTKEY, LAST_KBD_COMMAND, _function_name}
\done

\variable{DEC_8BIT_HACK}
\synopsis{Set the input mode for 8 bit control characters}
\usage{Int_Type DEC_8BIT_HACK}
\description
 If set to a non-zero value, a input character between 128 and 160
 will be converted into a two character sequence: \var{ESC} and the
 character itself stripped of the high bit + 64.  The motivation
 behind this variable is to enable the editor to work with VTxxx
 terminals that are in eight bit mode.
\seealso{META_CHAR}
\done

\variable{DEFINING_MACRO}
\synopsis{Non-zero if defining a macro}
\usage{Int_Type DEFINING_MACRO}
\description
  The \var{DEFINING_MACRO} variable will be non-zero is a keyboard
  macro definition is in progress.
\seealso{EXECUTING_MACRO}
\done

\variable{EXECUTING_MACRO}
\synopsis{Non-zero if a keyboard macro is currently executing}
\usage{Int_Type EXECUTING_MACRO}
\description
  The \var{EXECUTING_MACRO} variable will be non-zero is a keyboard
  macro is currently being executed.
\seealso{}
\done

\variable{FN_CHAR}
\synopsis{Set the function key prefix}
\usage{Int_Type FN_CHAR}
\description
  If this variable is non-zero, function keys presses will
  generate a two character sequence: the first character is the
  value of the \var{FN_CHAR} itself followed by the character pressed.
\notes
   This variable is available only for Microsoft window systems.
\seealso{ALT_CHAR, META_CHAR}
\done

\variable{IGNORE_USER_ABORT}
\synopsis{Control keyboard interrupt processing}
\usage{Int_Type IGNORE_USER_ABORT}
\description
 If set to a non-zero value, the keyboard interrupt character, e.g., 
 \exmp{Ctrl-G} will not trigger a S-Lang error.  When JED starts up,
 this value is set to \1 so that the user cannot interrupt the loading 
 of site.sl.  Later, it is set to 0.
\seealso{set_abort_char}
\done

\variable{KILL_LINE_FEATURE}
\synopsis{Configure the kill_line function}
\usage{Int_Type KILL_LINE_FEATURE}
\description
  If non-zero, kill_line will kill through end of line character if the
  cursor is at the beginning of a line.  Otherwise, it will kill only to
  the end of the line.
\seealso{bolp}
\done

\variable{LASTKEY}
\synopsis{The value of the current key sequence}
\usage{String_Type LASTKEY}
\description
  The value of the \var{LASTKEY} variable represents the currently
  executing key sequence.
\notes
  Key sequences involving the null character may not be accurately
  recorded.
\seealso{LAST_KBD_COMMAND}
\done

\variable{LAST_CHAR}
\synopsis{The Last Character read from the keyboard}
\usage{Int_Type LAST_CHAR}
\description
   The value of \var{LAST_CHAR} will be the last character read from
   the keyboard buffer.
\seealso{}
\done

\variable{META_CHAR}
\synopsis{Specify the meta-character}
\usage{Int_Type META_CHAR}
\description
 This variable determines how input characters with the high bit set
 are to be treated.  If \var{META_CHAR} is less than zero, the character
 is passed through un-processed.  However, if \var{META_CHAR} is greater
 than or equal to zero, an input character with the high bit set is
 mapped to a two character sequence.  The first character of the
 sequence is the character whose ascii value is \var{META_CHAR} and the
 second character is the input with its high bit stripped off.
\seealso{DISPLAY_EIGHT_BIT, DEC_8BIT_HACK}
\done

\variable{X_LAST_KEYSYM}
\synopsis{Keysym associated with the last key}
\usage{Int_Type X_LAST_KEYSYM}
\description
  The value of the \var{X_LAST_KEYSYM} variable represents the keysym
  of the most previously processed key.
\notes
  This variable is availible only in the XWindows version of \jed.
\seealso{LASTKEY}
\done

\function{buffer_keystring}
\synopsis{Append string "str" to the end of the input stream}
\usage{Void buffer_keystring (String str);}
\description
  Append string \var{str} to the end of the input stream to be read by JED's
  getkey routines.
\seealso{ungetkey, getkey}
\done

\function{copy_keymap}
\synopsis{Create a new keymap by copying another}
\usage{copy_keymap (String_Type new_map, String_Type old_map)}
\description
  The \var{copy_keymap} creates a new keymap whose name is given by
  \var{new_map} by copying an existing keymap specified by \var{old_map}.
\seealso{make_keymap, keymap_p, use_keymap}
\done

\function{definekey}
\synopsis{Bind keys to a function in a specific keymap}
\usage{Void definekey(String f, String key, String kmap);}
\description
  Unlike \var{setkey} which operates on the global keymap, this function is
  used for binding keys to functions in a specific keymap.  Here \var{f} is
  the function to be bound, \var{key} is a string of characters that make up
  the key sequence and \var{kmap} is the name of the keymap to be used.  See
  \var{setkey} for more information about the arguments.
\seealso{setkey, undefinekey, make_keymap, use_keymap}
\done

\function{dump_bindings}
\synopsis{Insert a list of keybindings for "map" into the buffer}
\usage{Void dump_bindings(String map);}
\description
  This functions inserts a formatted list of keybindings for the keymap
  specified by \var{map} into the buffer at the current point.
\seealso{get_key_binding}
\done

\function{enable_flow_control}
\synopsis{Turn on XON/XOFF flow control}
\usage{Void enable_flow_control (Integer flag);}
\description
  This Unix specific function may be used to turn XON/XOFF flow control
  on or off.  If \var{flag} is non-zero, flow control is turned on; otherwise,
  it is turned off.
\done

\function{flush_input}
\synopsis{Process all forms of queued input}
\usage{Void flush_input ();}
\description
  This function may be used to remove all forms of queued input.
\seealso{input_pending, getkey}
\done

\function{get_key_binding}
\synopsis{Return binding information about a key sequence}
\usage{(type, funct) = get_key_binding ([ keyseq ])}
#v+
   Int_Type type;
   String_Type funct;
   String_Type keyseq;
#v-
\description
  \var{get_key_binding} returns binding information about a specified
  key sequence.  If the optional parameter \var{keyseq} is not
  present, then \var{get_key_binding} will wait for the user to enter
  a key sequence.  If \var{keyseq} is present, then it denotes the key
  sequence.

  This function returns two values: a \dtype{String_Type} or
  \dtype{Ref_Type} representing the key
  sequence binding (\var{funct}), and an integer that indicates the
  key binding type:
#v+
    type   description
    -------------------------------------
      -1   funct is NULL, which indicates that the key has no binding
       0   funct is the name of a S-Lang function
       1   funct is the name of an internal function
       2   funct represents a macro ("@macro")
       3   funct represents a string to be inserted (" STRING")
       4   funct is a reference (Ref_Type) to the actual function
#v-
\seealso{getkey, input_pending}
\done

\function{_getkey}
\synopsis{Read an input byte from the keyboard}
\usage{Int_Type _getkey ()}
\description
  The \ifun{_getkey} function may be used to read a byte character from the
  keyboard.  It should be used instead of \ifun{getkey} when
  byte-semantics are required.
\seealso{input_pending, _ungetkey, getkey}
\done

\function{getkey}
\synopsis{Read an input character from the keyboard}
\usage{Long_Type getkey ()}
\description
  The \var{getkey} function may be used to read an input character from the
  keyboard.  If UTF-8 mode is in effect, the value returned can be
  negative if the key-sequence corresponds to an invalid UTF-8 encoded
  sequence.  In such a case, the value returned will correspond to the
  first byte of the sequence, and will be equal in magnitude to the
  value of byte.
\seealso{input_pending, ungetkey, _getkey}
\done

\function{input_pending}
\synopsis{Test whether there is pending keyboard input}
\usage{Integer input_pending (Integer tsecs);}
\description
  This function is used to see if keyboard input is available to be read
  or not. The paramter \var{tsecs} is the amount of time to wait for input
  before returning if input is not available.  The time unit for \var{tsecs}
  is one-tenth of a second.  That is, to wait up to one second, pass a
  value of ten to this routine.  It returns zero if no input is
  available, otherwise it returns non-zero.  As an example,
#v+
        define peek_key ()
        {
          variable ch;
          ifnot (input_pending (0)) return -1;
          ch = getkey ();
          ungetkey (ch);
          return ch;
        }
#v-
  returns the value of the next character to be read if one is
  available; otherwise, it returns -1.
\seealso{getkey, ungetkey}
\done

\function{keymap_p}
\synopsis{Test if a keymap "kmap" exists}
\usage{Integer keymap_p (String kmap);}
\description
  The \var{keymap_p} function may be used to determine whether or not a
  keymap with name \var{kmap} exists.  If the keymap specified by \var{kmap}
  exists, the function returns non-zero.  It returns zero if the keymap
  does not exist.
\seealso{make_keymap, definekey}
\done

\function{make_keymap}
\synopsis{Create a keymap with name "km"}
\usage{Void make_keymap (String km);}
\description
  The \var{make_keymap} function creates a keymap with a name specified by
  the \var{km} parameter.  The new keymap is an exact copy of the
  pre-defined \exmp{"global"} keymap.
\seealso{use_keymap, copy_keymap, keymap_p, definekey, setkey}
\done

\function{map_input}
\synopsis{Remap an input character "x" to "y".}
\usage{Void map_input (Integer x, Integer y);}
\description
  The \var{map_input} function may be used to remap an input character with
  ascii value \var{x} from the keyboard to a different character with ascii
  value \var{y}.  This mapping can be quite useful because it takes place
  before the editor interprets the character. One simply use of this
  function is to swap the backspace and delete characters.  Since the
  backspace character has an ascii value of \var{8} and the delete character
  has ascii value \var{127}, the statement
#v+
        map_input (8, 127);
#v-
  maps the backspace character to a delete character and
#v+
        map_input (127, 8);
#v-
  maps the delete character to a backspace character.  Used together,
  these two statement effectively swap the delete and backspace keys.
\seealso{getkey}
\done

\function{prefix_argument}
\synopsis{Test if the user has entered a prefix argument}
\usage{Int_Type prefix_argument ()}
\description
  This function may be used to determine whether or not the user has
  entered a prefix argument from the keyboard.  If a prefix argument
  is present, its value is returned, otherwise \NULL will be returned.

  Calling this function cancels the prefix-argument.
\example
  This example displays the prefix argument in the message area:
#v+
     arg = prefix_argument ();
     if (arg == NULL)
       message ("No Prefix Argument");
     else
       vmessage ("Prefix argument: %d", arg);
#v-
\notes
  The old semantics, which are still supported but deprecated allows
  an integer argument to be passed to the function.  This argument
  will be returned instead of \NULL if no prefix-argument is present.
  Using the old semantics, the above example could be written as
#v+
     arg = prefix_argument (-9999);
     if (arg == -9999)
       message ("No Prefix Argument");
     else
       vmessage ("Prefix argument: %d", arg);
#v-
\seealso{set_prefix_argument}
\done

\function{set_abort_char}
\synopsis{change the keyboard character that generates an S-Lang interrupt}
\usage{Void set_abort_char (Integer ch);}
\description
  This function may be used to change the keyboard character that
  generates an S-Lang interrupt.  The parameter \var{ch} is the ASCII value
  of the character that will become the new abort character. The
  default abort character \exmp{Ctrl-G} corresponds to \exmp{ch=7}.
\done

\function{set_current_kbd_command}
\synopsis{Do as if "s" were entered from the keybord}
\usage{Void set_current_kbd_command (String s);}
\description
  Undocumented
\done

\function{set_prefix_argument}
\synopsis{Set the prefix argument}
\usage{Void set_prefix_argument (Int_Type n)}
\description
  This function may be used to set the prefix argument to the value
  specified by \var{n}.  If \var{n} is less than zero, then the prefix
  argument is cancelled.
\seealso{prefix_argument}
\done

\function{setkey}
\synopsis{Bind a key sequence "key" to the function "fun"}
\usage{Void setkey(String fun, String key);}
\description
  This function may be used to define a key sequence specified by the
  string \var{key} to the function \var{fun}.  \var{key} can contain the \exmp{^}
  character which denotes that the following character is to be
  interpreted as a control character, e.g.,
#v+
        setkey("bob", "^Kt");
#v-
  sets the key sequence \exmp{Ctrl-K t} to the function \var{bob}.

  The \var{fun} argument is usually the name of an internal or a user
  defined S-Lang function.  However, if may also be a sequence of
  functions or even another keysequence (a keyboard macro).  For
  example,
#v+
        setkey ("bol;insert(string(whatline()))", "^Kw");
#v-
  assigns the key sequence \exmp{Ctrl-K w} to move to the beginning of a line
  and insert the current line number.  For more information about this
  important function, see the JED User Manual.

  Note that \var{setkey} works on the "global" keymap.
\seealso{unsetkey, definekey}
\done

\function{undefinekey}
\synopsis{Remove a keybinding from "kmap"}
\usage{Void undefinekey (String key, String kmap);}
\description
  This function may be used to remove a keybinding from a specified
  keymap.  The key sequence is given by the parameter \var{key} and the
  keymap is specified by the second parameter \var{kmap}.
\seealso{unsetkey, definekey, what_keymap}
\done

\function{_ungetkey}
\synopsis{Push a byte onto the input stream}
\usage{Void _ungetkey (Int_Type c)}
\description
  This function may be used to push a byte \exmp{c} onto the input
  stream. This means that the next keyboard byte to be read will be
  \exmp{c}.
\seealso{buffer_keystring, _getkey, get_key_binding}
\done

\function{ungetkey}
\synopsis{Push a character onto the input stream}
\usage{Void ungetkey (Integer c);}
\description
  This function may be used to push a character \exmp{c} represented by its
  character code onto the input stream.  This means that the next
  keyboard character to be read will be \exmp{c}.
\seealso{buffer_keystring, getkey, get_key_binding}
\done

\function{unsetkey}
\synopsis{Remove the definition of "key" from the "global" keymap}
\usage{Void unsetkey(String key);}
\description
  This function is used to remove the definition of the key sequence
  \var{key} from the "global" keymap.  This is sometimes necessary to bind
  new key sequences which conflict with other ones.  For example, the
  "global" keymap binds the keys \exmp{"^[[A"}, \exmp{"^[[B"}, \exmp{"^[[C"}, and
  \exmp{"^[[D"} to the character movement functions.  Using
  \exmp{unsetkey("^[[A")} will remove the binding of \exmp{"^[[A"} from the global
  keymap but the other three will remain.  However, \exmp{unsetkey("^[[")}
  will remove the definition of all the above keys.  This might be
  necessary to bind, say, \exmp{"^[["} to some function.
\seealso{setkey, undefinekey}
\done

\function{use_keymap}
\synopsis{Set the keymap for the current buffer}
\usage{Void use_keymap (String km);}
\description
  This function may be used to dictate which keymap will be used by the
  current buffer.  \var{km} is a string value that corresponds to the name
  of a keymap.
\seealso{make_keymap, copy_keymap, keymap_p, what_keymap}
\done

\function{what_keymap}
\synopsis{Return the name of the current buffer's keymap}
\usage{String what_keymap ();}
\description
  This function returns the name of the keymap associated with the
  current buffer.
\seealso{create_keymap, keymap_p}
\done

\function{which_key}
\synopsis{Return the keys that are bound to the function "f"}
\usage{Integer which_key (String f);}
\description
  The \var{which_key} function returns the the number of keys that are
  bound to the function \var{f} in the current keymap.  It also returns
  that number of key sequences with control characters expanded as the
  two character sequence \exmp{^} and the the whose ascii value is the
  control character + 64. For example,
#v+
        define insert_key_bindings (f)
        {
           variable n, key;
           n = which_key (f);
           loop (n)
             {
                 str = ();
                 insert (str);
                 insert ("\n");
             }
        }
#v-
  inserts into the buffer all the key sequences that are bound to the
  function \var{f}.
\seealso{get_key_binding, setkey, what_keymap}
\done

