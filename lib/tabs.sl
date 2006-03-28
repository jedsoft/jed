%%
%%  Tab routines for JED
%%


%% The following defines the tab stops to be 8 column:
custom_variable ("Tab_Stops", [0:19] * TAB_DEFAULT + 1);

define tab_to_tab_stop ()
{
   variable c, goal, i;
   c = what_column ();
  
   foreach (Tab_Stops)
     {
	goal = ();
	if (goal > c) break;
     }
   
   insert_spaces (goal - c);
}

setkey ("tab_to_tab_stop", "^I");

private variable _Tabs_Buffer;

$1 = "*TabsEdit*";
!if (keymap_p($1))
{
   make_keymap ($1);
   undefinekey ("^C", $1);
   definekey ("tabs_install_tab_stops", "^C^C", $1);
}

%% emacs like edit tab stops
define edit_tab_stops ()
{
   variable tbuf, i;
   _Tabs_Buffer = whatbuf();
   tbuf = "*TabsEdit*";
 
   sw2buf(tbuf); erase_buffer();
   TAB = 0;
      
   use_keymap(tbuf);
   foreach (Tab_Stops)
     {
	i = ();
	goto_column (i);
	insert_char ('T');
     }
   newline ();
   _for (1, 13, 1)
     {
	i = ();
	goto_column (10 * i);
	insert (string(i));
     }
   newline ();
   
   loop (13) insert ("1234567890");
   
   insert ("\nTo install changes, type control-c control-c.");
   bob ();
   set_buffer_modified_flag (0);
   set_overwrite (1);
}


define tabs_install_tab_stops ()
{
   variable i;
   bob ();

   _for (0, length (Tab_Stops) - 1, 1)
     { 
	i = ();
	skip_white ();
	if (eolp()) break;
	Tab_Stops[i] = what_column ();
	go_right_1 ();
     }
   
   Tab_Stops[[i:]] = 0;

   set_buffer_modified_flag (0);
   delbuf (whatbuf ());
   sw2buf ( _Tabs_Buffer);
}
