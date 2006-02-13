public variable Menu_Popups_Loaded = 0;

$1 = "Global";
menu_create_menu_bar ($1);

menu_append_popup ($1, "&File");
menu_append_popup ($1, "&Edit");
menu_append_popup ($1, "M&ode");       %  mode-specific
menu_append_popup ($1, "&Search");
menu_append_popup ($1, "&Buffers");
menu_append_popup ($1, "W&indows");    %  Emacs uses ESC-w for yank.  Keep it
!if (_jed_secure_mode)
  menu_append_popup ($1, "S&ystem");
menu_append_popup ($1, "&Help");

menu_set_menu_bar_prefix ($1, "F10 key ==> ");


#ifndef IBMPC_SYSTEM
setkey ("select_menubar", "\e[21~");    %  F10
# ifdef UNIX
_for (0, 9, 1)
{
   % Bind keys F0-F9 to select_menubar.
   $1 = ();
   setkey ("select_menubar", sprintf ("^(k%d)", $1));
}
#endif
#endif

private define unset_setkey (fun, key)
{
   unsetkey (key);
   setkey (fun, key);
}

define enable_menu_keys ()
{
#ifdef UNIX
   unset_setkey ("select_menubar", "^(k;)");
#else
# ifdef IBMPC_SYSTEM
   unset_setkey ("select_menubar", "^@D");
# endif
#endif

   unset_setkey ("select_menubar", "\em");
   unset_setkey ("@\emF", "\ef");
   unset_setkey ("@\emE", "\ee");
   unset_setkey ("@\emo", "\eo");      %  Mode menu
   unset_setkey ("@\emS", "\es");
   unset_setkey ("@\emB", "\eb");
   unset_setkey ("@\emi", "\ei");
   unset_setkey ("@\emH", "\eh");
   unset_setkey ("@\emy", "\ey");
#ifdef IBMPC_SYSTEM
   unset_setkey ("@\emF", "^@!");      %  Alt-F
   unset_setkey ("@\emE", "^@^R");     %  Alt-E
   unset_setkey ("@\emo", "^@^X");     %  Alt-O
   unset_setkey ("@\emS", "^@^_");     %  Alt-S
   unset_setkey ("@\emB", "^@0");      %  Alt-B
   unset_setkey ("@\emi", "^@^W");     %  Alt-I
   unset_setkey ("@\emH", "^@#");      %  Alt-H
   unset_setkey ("@\emy", "^@^U");     %  Alt-Y
#endif
}

enable_menu_keys ();

private variable Active_Mode = NULL;
private define init_mode_callback (menu)
{
   variable fun;
   variable mode;
   mode = get_mode_name ();
   if (mode == Active_Mode)
     return;
   Active_Mode = mode;

   menu += ".M&ode";
   menu_delete_items (menu);
   fun = mode_get_mode_info ("init_mode_menu");
   if (fun == NULL)
     {
	menu_set_object_available (menu, 0);
	return;
     }
   menu_set_object_available (menu, 1);
   @fun (menu);
}

autoload ("menu_load_popups", "popups");

menu_set_init_menubar_callback ("Global", &init_mode_callback);
menu_set_select_menubar_callback ("Global", &menu_load_popups);

