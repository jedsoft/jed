\function{enable_top_status_line}
\synopsis{Set the top_status_line: 1 enabled, 0 hidden}
\usage{Void enable_top_status_line (Integer x);}
\description
  If x is non-zero, the top status line is enabled.  If x is zero, the
  top status line is disabled and hidden.
\seealso{set_top_status_line}
\done

\function{menu_append_item}
\synopsis{Append an entry to a menu}
\usage{menu_append_item (menu, name, fun [,client_data])}
#v+
    String_Type menu, name;
    String_Type or Ref_Type fun;
    Any_Type client_data
#v-
\description
   The \var{menu_append_item} function appends a menu item called
   \var{name} to the menu \var{menu}.  If called with \3 arguments,
   the third argument must be a string that will get executed or
   called when the menu item is selected.  

   When called with \4 arguments, the \var{fun} argument may be either
   a string or a reference to a function.  When the item is selected,
   the function will be called and \var{client_data} will be passed to
   it.
\seealso{menu_append_popup}
\done

\function{menu_append_popup}
\synopsis{Append a popup menu to a menu}
\usage{menu_append_popup (String_Type parent_menu, String_Type popup_name)}
\description
  The \var{menu_append_popup} function may be used to append a new
  popup menu with name \var{popup_name} to the menu \var{parent_menu},
  which may either be another popup menu or a menu bar.
\seealso{menu_append_item, menu_append_separator}
\done

\function{menu_append_separator}
\synopsis{Append a separator to a menu}
\usage{menu_append_separator (String_Type menu)}
\description
  The \var{menu_append_separator} function appends a menu item
  separator to the menu \var{menu}.
\seealso{menu_append_item, menu_append_popup}
\done

\function{menu_copy_menu}
\synopsis{Copy a menu to another}
\usage{menu_copy_menu (String_Type dest, String_Type src)}
\description
  Then \var{menu_copy_menu} function copies the menu item, which may
  be another popup menu, to another popup menu.
\seealso{menu_append_popup, menu_append_item}
\done

\function{menu_create_menu_bar}
\synopsis{Create a menu bar}
\usage{menu_create_menu_bar (String_Type name)}
\description
  The \var{menu_create_menu_bar} function may be used to create a new
  menu bar called \var{name}.  The new menu bar may be associated with
  a buffer via the \var{menu_use_menu_bar} function.
\seealso{menu_use_menu_bar, menu_append_popup}
\done

\function{menu_delete_item}
\synopsis{Delete a menu item}
\usage{menu_delete_item (String_Type name)}
\description
  The \var{menu_delete_item} function deletes the menu called
  \var{name} and all of its submenus.
\example
  To delete the \exmp{System} menu from the global menu bar, use
#v+
    menu_delete_item ("Global.S&ystem");
#v-
\seealso{menu_delete_items}
\done

\function{menu_delete_items}
\synopsis{Delete the items from a menu}
\usage{menu_delete_items (String_Type menu)}
\description
  The \var{menu_delete_items} function deletes all the menu items
  attached to a specified popup menu.  However, unlike the related
  function \var{menu_delete_item}, the popup menu itself will not be
  removed.
\seealso{menu_delete_item, menu_append_popup}
\done

\function{menu_insert_item}
\synopsis{Insert an entry into a menu at a specified position}
\usage{menu_insert_item (position, menu, name, fun [,client_data])}
#v+
    Int_Type/String_Type position;
    String_Type menu, name;
    String_Type or Ref_Type fun;
    Any_Type client_data;
#v-
\description
   The \var{menu_insert_item} function inserts a menu item called
   \var{name} to the menu \var{menu} at a specified position.
   
   The insertion position may be specified as an integer, or as the
   name of a menu item within \var{parent_menu}.  When specified as an
   integer, the insertion will take place at the corresponding
   position of the menu, where zero denotes the first item.  If the
   position specifier is the name of a menu item, the the insertion
   will take place before that item.

   If called with \4 arguments, the third argument must be a string
   that will get executed or called when the menu item is selected.

   When called with \5 arguments, the \var{fun} argument may be either
   a string or a reference to a function.  When the item is selected,
   the function will be called and \var{client_data} will be passed to
   it.
\seealso{menu_append_item, menu_insert_popup, menu_insert_separator}
\done

\function{menu_insert_popup}
\synopsis{Inserts a popup menu into a menu at a specified position}
\usage{menu_insert_popup (position, parent_menu, popup_name)}
#v+
    Int_Type/String_Type position;
    String_Type parent_menu, popup_name;
#v-
\description
   The \var{menu_insert_popup} function will insert a popup menu with
   name \var{popup_name} into a pre-existing popup menu or menu bar 
   with name given by {parent_menu}.  

   The insertion position may be specified as an integer, or as the
   name of a menu item within \var{parent_menu}.  When specified as an
   integer, the insertion will take place at the corresponding
   position of the menu, where zero denotes the first item.  If the
   position specifier is the name of a menu item, the the insertion
   will take place before that item.
\seealso{menu_append_popup, menu_insert_item, menu_insert_separator}
\done

\function{menu_insert_separator}
\synopsis{Inserts a separator into a menu at a specified position}
\usage{menu_insert_separator (position, parent_menu)}
#v+
   Int_Type/String_Type position;
   String_Type parent_menu;
#v-
\description
   The \var{menu_insert_separator} function inserts a separator
   into a pre-existing popup menu or menu bar with name given 
   by \var{parent_menu}.
   
   The insertion position may be specified as an integer, or as the
   name of a menu item within \var{parent_menu}.  When specified as an
   integer, the insertion will take place at the corresponding
   position of the menu, where zero denotes the first item.  If the
   position specifier is the name of a menu item, the the insertion
   will take place before that item.
\seealso{menu_append_separator, menu_insert_item, menu_insert_popup}
\done   

\function{menu_select_menu}
\synopsis{Select a menu item}
\usage{menu_select_menu (String_Type menu)}
\description
  This function allows interpreter access to a specified menu it.  If
  the parameter specifies a popup menu, then the corresponding menu
  will be invoked.  Otherwise the function bound to the specified menu
  will be called.
\example
    menu_select_menu ("Global.&File");
    menu_select_menu ("Global.&File.Save &Buffers");
\seealso{menu_insert_item}
\done   

\function{menu_set_init_menubar_callback}
\synopsis{Set the initialize menu bar callback}
\usage{menu_set_init_menubar_callback (Ref_Type cb)}
\description
  The \var{menu_set_init_menubar_callback} may be used to specify the
  function that is to be called whenever a menu bar may need to be
  updated.  This may be necessary when the user switches buffers or
  modes.  The callback function must accept a single argument which is
  the name of the menubar.
\seealso{menu_set_select_menubar_callback, menu_create_menu_bar}
\done

\function{menu_set_menu_bar_prefix}
\synopsis{Set the prefix string to be displayed on the menu bar}
\usage{menu_set_menu_bar_prefix (String_Type menubar, String_Type prefix)}
\description
  The \var{menu_set_menu_bar_prefix} specifies the string that is to
  be displayed on the specified menu bar.  The default prefix is
  \exmp{"F10 key ==> "}.
\seealso{menu_create_menu_bar}
\done

\function{menu_set_object_available}
\synopsis{Set the availability of a menu item}
\usage{menu_set_object_available (String_Type menuitem, Int_Type flag)}
\description
  The \var{menu_set_object_available} function may be used to activate
  or inactivate the specified menu item, depending upon whether
  \var{flag} is non-zero or zero, respectively.
\seealso{menu_append_item}
\done

\function{menu_set_select_menubar_callback}
\synopsis{Set the function to be called when the menu bar is activated}
\usage{menu_set_select_menubar_callback (String_Type menubar, Ref_Type f)}
\description
  The \var{menu_set_select_menubar_callback} function is used to
  indicate that the function whose reference is \var{f} should be
  called whenever the menu bar is selected.  The callback function is
  called with one argument: the name of the menu bar.
\seealso{menu_set_init_menubar_callback, menu_set_select_popup_callback}
\done

\function{menu_set_select_popup_callback}
\synopsis{Specify the function to be called prior to a popup}
\usage{menu_set_select_popup_callback (String_Type popup, Ref_Type f}
\description
  The \var{menu_set_select_popup_callback} function may be used to
  specify a function that should be called just before a popup menu is
  displayed.  The callback function must be defined to take a single
  argument, namely the name of the popup menu.
  
  The basic purpose of this function is to allow the creation of a
  dynamic popup menu.  For this reason, the popup menu will have its
  items deleted before the callback function is executed.
\seealso{menu_set_select_menubar_callback, menu_append_item}
\done

\function{menu_use_menu_bar}
\synopsis{Associate a menu bar with the current buffer}
\usage{menu_use_menu_bar (String_Type menubar)}
\description
  The \var{menu_use_menu_bar} function may be used to associate a
  specified menu bar with the current buffer. If no menu bar has been
  associated with a buffer, the \exmp{"Global"} menu bar will be used.
\seealso{menu_create_menu_bar}
\done

\function{set_top_status_line}
\synopsis{Set the string to be displayed at the top of the display}
\usage{String set_top_status_line (String str);}
\description
  This functions sets the string to be displayed at the top of the
  display. It returns the value of the line that was previously
  displayed.
\seealso{enable_top_status_line}
\done

