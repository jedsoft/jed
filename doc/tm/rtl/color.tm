\function{add_color_object}
\synopsis{Return the object number associated with "obj"}
\usage{add_color_object (String_Type name)}
\description
  This function creates a new color object with a specified name.  See
  the \var{set_color} documentation for pre-defined color objects.
\seealso{set_color, get_color, color_number}
\done

\function{color_number}
\synopsis{Return the object number associated with "obj"}
\usage{Integer color_number (String obj)}
\description
  This function returns the object number associated with the
  string \var{obj}.  Valid names for \var{obj} are as per \var{set_color}.
\seealso{set_color, set_column_colors}
\seealso{WANT_SYNTAX_HIGHLIGHT, USE_ANSI_COLORS}
\done

\function{get_color}
\synopsis{Return the foreground and background color of an object}
\usage{(String_Type fg, String_Type bg) =  get_color (String_Type name)}
\description
  This function returns the foreground and background colors of the
  specified color object.
\seealso{set_color, color_number, add_color_object}
\done

\function{set_color}
\synopsis{Set fore- and background colors of an object}
\usage{set_color (String_Type obj, String_Type fg, String_Type bg)}
\description
 This function sets the foreground and background colors of an object
 specified by the string \var{obj} to \var{fg} and \var{bg}.  The exact values of
 the strings \var{fg} and \var{bg} are system dependent.  For the X-Window
 system, they can be any string that the server understands, e.g.,
 \var{"SteelBlue"}.  For other systems, the color must be one of the
 following:
#v+
   "black"            "gray"
   "red"              "brightred"
   "green"            "brightgreen"
   "brown"            "yellow"
   "blue"             "brightblue"
   "magenta"          "brightmagenta"
   "cyan"             "brightcyan"
   "lightgray"        "white"
   "default"
#v-
 On most terminals, the values in the second column have no affect
 when used as the background color.

 Using "default" as a foreground or background color instructs the
 editor to tell the terminal to use the default foreground and
 background colors of the window.  If you use a terminal that uses a
 transparant background, then you need to specify "default" as the
 background color.  Not all terminals support the notion of the
 "default" color.

 The valid names for \var{obj} are:
#v+
   "normal"      Default foreground/background
   "status"      The status window line
   "region"      Highlighted Regions
   "cursor"      Text Cursor (X-Windows)
   "cursorovr"   Text Cursor in overwrite mode (X-Windows)
   "menu"        The menu bar
   "error"       Error messages
   "message"     Other messages
   "dollar"      Color of the indicator that text extends beyond the
                 boundary of the window.
   "linenum"     Line number field
#v-
 If color syntax highlighting is enabled, the following object names
 are also meaningful:
#v+
   "number"      Numbers in C-mode and Equations in TeX-mode
   "delimiter"   Commas, semi-colons, etc...
   "keyword"     Language dependent
   "keyword1"    Language dependent
   "keyword2"    Language dependent
   "keyword3"    Language dependent
   "keyword4"    Language dependent
   "keyword5"    Language dependent
   "keyword6"    Language dependent
   "keyword7"    Language dependent
   "keyword8"    Language dependent
   "keyword9"    Language dependent
   "string"      Literal strings
   "comment"     Comments
   "operator"    Such as +, -, etc...
   "preprocess"  Preprocessor lines
   "tab"
   "trailing_whitespace"
   "html"        <html> and '<' syntax objects.
#v-
 If line attributes are available, then you may also specifiy the color
 of the hidden line indicator:
#v+
   "..."         Hidden line indicator
#v-
 The color of the menu objects may be specified via
#v+
   "menu_char"              Menu item key-shortcut color
   "menu_shadow"            Color of the shadow
   "menu_selection"         Selected menu-item color
   "menu_popup"             Color of the popup box
   "menu_selection_char"    Selected menu item key-shortcut color
#v-
 Xjed defines the following objects:
#v+
   "mouse"                  Mouse cursor color
   "border"                 Window borde color
#v-
\seealso{define_syntax, set_color_esc, set_column_colors, set_color_object}
\seealso{WANT_SYNTAX_HIGHLIGHT, USE_ANSI_COLORS}
\done

\function{set_color_esc}
\synopsis{associate an escape sequence with an object}
\usage{Void set_color_esc (String object, String esc_seq)}
\description
  This function may be used to associate an escape sequence with an
  object.  The escape sequence will be sent to the terminal prior to
  sending updating the object.  It may be used on mono terminals to
  underline objects, etc...  The object names are the same names used by
  the \var{set_color} function.
  Note: Care should be exercised when using
  this function.  Also, one may need to experiment around a little to
  get escape sequences that work together.
\seealso{set_color}
\done

\function{set_color_object}
\synopsis{Associate colors "fg" and "bg" with object "obj"}
\usage{Void set_color_object (Integer obj, String fg, String bg)}
\description
  Associate colors \var{fg} and \var{bg} with object \var{obj}.  Valid values for \var{obj}
  are in the range 30 to 128.  All other values are reserved.  Values for
  the strings \var{fg} and \var{bg} are as given by the description for \var{set_color}.
\seealso{set_column_colors, set_color}
\done

\function{set_column_colors}
\synopsis{Associate a color with columns \var{c0} through \var{c1}}
\usage{Void set_column_colors (Integer color, Integer c0, Integer c1)}
\description
  This function associates a color with columns \exmp{c0} through \var{c1} in the
  current buffer.  That is, if there is no syntax highlighting already
  defined for the current buffer, when the current buffer is displayed,
  columns \exmp{c0} through \var{c1} will be displayed with the attributes of the
  \var{color} object.  The parameters \exmp{c0} and \var{c1} are restricted to the range
  1 through \var{SCREEN_WIDTH}.  Use the function \var{set_color_object} to assign
  attributes to the \var{color} object.
\seealso{set_color_object}
\done

