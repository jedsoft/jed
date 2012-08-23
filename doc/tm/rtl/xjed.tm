\function{x_copy_region_to_cutbuffer}
\synopsis{Copy the region to the X cutbuffer}
\usage{x_copy_region_to_cutbuffer ()}
\description
    places a copy of the region to the X cutbuffer for insertion in other
    X-window programs. In wjed the region is copies to the clipboard.
\seealso{x_copy_region_to_selection}
\done

\function{x_copy_region_to_selection}
\synopsis{Copy the region to the X selection}
\usage{x_copy_region_to_selection ()}
\description
    places a copy of the region to the X selection for insertion in other
    X-window programs. This function is only available in xjed.
\seealso{x_copy_region_to_cutbuffer}
\done

\function{x_insert_cutbuffer}
\synopsis{Insert the content of the X cutbuffer}
\usage{Int_Type x_insert_cutbuffer ()}
\description
 Inserts cutbuffer (in wjed the clipboard) into the current buffer and 
 returns the number of characters inserted. 
\seealso{}
\done

\function{x_insert_selection}
\synopsis{Insert data from the X selection owner}
\usage{Int_Type x_insert_selection ()}
\description
 This function only requests selection data from the selection owner.
 If Xjed received EVENT, Xjed inserts selection data into the current buffer
 and returns the number of characters inserted.
\seealso{}
\done

\function{x_server_vendor}
\synopsis{Return the vendor name of the X server}
\usage{String_Type x_server_vendor ()}
\description
   This function returns the vendor name of the X server.
\seealso{}
\done

\function{x_set_icon_name}
\synopsis{Set the icon do display in X windows (xjed only)}
\usage{x_set_icon_name (String_Type name)}
\description
\seealso{}
\done

\function{x_set_keysym}
\synopsis{Associate a string with a key}
\usage{x_set_keysym (Int_Type keysym, Int_Type shift, String_Type str)}
\description
 This function may be used to assocate a string \exmp{str} with a key
 \exmp{keysym} modified by mask \exmp{shift}. Pressing the key
 associated with \exmp{keysym} will then generate the keysequence
 given by \exmp{str}. The function keys are mapped to integers in the
 range \exmp{0xFF00} to \exmp{0xFFFF}. On most systems, the keys that
 these mappings refer to are located in the file
 \exmp{/usr/include/X11/keysymdef.h}. For example, on my system, the
 keysyms for the function keys \exmp{XK_F1} to \exmp{XK_F35} fall in
 the range \exmp{0xFFBE} to \exmp{0xFFE0}. So to make the \exmp{F1}
 key correspond to the string given by the two characters
 \exmp{Ctrl-X} \exmp{Ctrl-C}, simply use:
#v+
    x_set_keysym (0xFFBE, 0, "^X^C"); 
#v-
 The \exmp{shift} argument is an integer with the
 following meanings:
#v+
    0   : unmodified key
    '$' : shifted
    '^' : control
#v-
 Any other value for shift will default to 0 (unshifted).
\seealso{x_set_meta_keys}
\done

\function{x_set_meta_keys}
\synopsis{Specify which modifier keys are to be interpreteted as meta keys}
\usage{x_set_meta_keys (Int_Type mod_mask)}
\description
  The \var{x_set_meta_keys} function allows the user to specify which
  modifier keys are to be interpreteted as meta keys.  The parameter
  \var{mod_mask} is a bitmapped value whose bits correspond to a
  modifier mask:
#v+
      0: Mod1Mask
      1: Mod2Mask
      2: Mod3Mask
      3: Mod4Mask
      4: Mod5Mask
#v-
\example
#v+
   x_set_meta_keys ((1<<0) | (1<<3));
#v-
  specifies that meta keys are to be associated with Mod1Mask and
  Mod4Mask.
\seealso{x_set_keysym}
\done

\function{x_toggle_visibility}
\synopsis{Shows or hides the window}
\usage{x_toggle_visibility([Integer_Type hide])}
\description
 Makes the window visible or invisible. If the argument \exmp{hide} is
 not given, the visility of the windows is toggled, e.g. if the window
 is invisible it becomes visible and vice versa. If the optional
 argument \exmp{hide} is given the state to the window becomes visible
 if \exmp{hide} is non-zero or invisible if \exmp{hide} is zero,
 independent of the current state.
\notes
 You can also hide the window by calling \ifun{suspend}, but you can not show
 it, if the window doesn't have the focus.
\seealso{suspend}
\done

\function{x_set_window_name}
\synopsis{Set the title of the xjed window}
\usage{x_set_window_name (String_Type name)}
\done

\function{x_warp_pointer}
\synopsis{Move the mouse cursor to the current editing position}
\usage{x_warp_pointer ()}
\description
  This function may be used to move the mouse cursor to the editing
  position.
\done

