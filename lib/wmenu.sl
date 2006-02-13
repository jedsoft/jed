enable_top_status_line (0);

private variable Wmenu_Modes_Popup;
 

private define simple_menu()
{
   variable menubar;
   variable file_popup, edit_popup, search_popup, buffers_popup, 
     modes_popup, help_popup, windows_popup;

   w32_destroy_menubar();
   file_popup = w32_create_popup_menu();
   ERROR_BLOCK 
      {
	 w32_destroy_menu(file_popup);
      }
   w32_append_menu_item(file_popup, "Open\t^X^F", 0x1, "find_file");
   w32_append_menu_item(file_popup, "Save\t^X^W", 0x2, "write_buffer");
   w32_append_menu_item(file_popup, "Save Buffers\t^X^s", 0x3, "save_buffers");
   w32_append_menu_item(file_popup, "Insert File\t^Xi", 0x4, "insert_file");
   w32_append_separator(file_popup);
   w32_append_menu_item(file_popup, "Shell Cmd", 0x5, "do_shell_cmd");
   w32_append_separator(file_popup);
   w32_append_menu_item(file_popup, "Exit\t^X^C", 0x6, "exit_jed");

   edit_popup = w32_create_popup_menu();
   ERROR_BLOCK 
      {
	 w32_destroy_menu(file_popup);
	 w32_destroy_menu(edit_popup);
      }
   w32_append_menu_item(edit_popup, "Undo\t^Xu", 10, "undo");
   w32_append_separator(edit_popup);
   w32_append_menu_item(edit_popup, "Cut", 11, "kill_region");
   w32_append_menu_item(edit_popup, "Copy", 12, "copy_region");
   w32_append_menu_item(edit_popup, "Paste", 13, "yank");
   
   search_popup = w32_create_popup_menu();
   ERROR_BLOCK 
      {
	 w32_destroy_menu(file_popup);
	 w32_destroy_menu(edit_popup);
	 w32_destroy_menu(search_popup);
      }
   w32_append_menu_item(search_popup, "Search Forward", 20, "search_forward");
   w32_append_menu_item(search_popup, "Search Backward", 21, "search_backward");
   w32_append_menu_item(search_popup, "Replace", 23, "replace_cmd");
   
   buffers_popup = w32_create_popup_menu();
   ERROR_BLOCK 
      {
	 w32_destroy_menu(file_popup);
	 w32_destroy_menu(edit_popup);
	 w32_destroy_menu(search_popup);
	 w32_destroy_menu(buffers_popup);
      }
   w32_append_menu_item(buffers_popup, "Kill", 30, "kill_buffer");
   w32_append_menu_item(buffers_popup, "Switch To", 31, "switch_to_buffer");
   w32_append_menu_item(buffers_popup, "List Buffers", 32, "list_buffers");
   Wmenu_Modes_Popup = w32_create_popup_menu();
   ERROR_BLOCK 
      {
	 w32_destroy_menu(file_popup);
	 w32_destroy_menu(edit_popup);
	 w32_destroy_menu(search_popup);
	 w32_destroy_menu(buffers_popup);
	 w32_destroy_menu(Wmenu_Modes_Popup);
      }
   w32_append_menu_item(Wmenu_Modes_Popup, "C Mode", 40, "c_mode");
   w32_append_menu_item(Wmenu_Modes_Popup, "Text Mode", 41, "text_mode");
   w32_append_menu_item(Wmenu_Modes_Popup, "No Mode", 42, "no_mode");
   w32_append_menu_item(Wmenu_Modes_Popup, "Fortran Mode", 43, "fortran_mode");
   w32_append_popup_menu(buffers_popup, "Modes", Wmenu_Modes_Popup);
   
   windows_popup = w32_create_popup_menu();
   ERROR_BLOCK 
      {
	 w32_destroy_menu(file_popup);
	 w32_destroy_menu(edit_popup);
	 w32_destroy_menu(search_popup);
	 w32_destroy_menu(buffers_popup);
	 w32_destroy_menu(Wmenu_Modes_Popup);
	 w32_destroy_menu(windows_popup);
      }
   w32_append_menu_item(windows_popup, "One Window", 50, "one_window");
   w32_append_menu_item(windows_popup, "Split Window", 51, "split_window");
   w32_append_menu_item(windows_popup, "Other Window", 52, "other_window");
   w32_append_menu_item(windows_popup, "Delete Window", 53, "delete_window");
   w32_append_separator(windows_popup);
   w32_append_menu_item(windows_popup, "Redraw", 54, "redraw");
   
   help_popup = w32_create_popup_menu();
   ERROR_BLOCK 
      {
	 w32_destroy_menu(file_popup);
	 w32_destroy_menu(edit_popup);
	 w32_destroy_menu(search_popup);
	 w32_destroy_menu(buffers_popup);
	 w32_destroy_menu(Wmenu_Modes_Popup);
	 w32_destroy_menu(windows_popup);
	 w32_destroy_menu(help_popup);
      }
   w32_append_menu_item(help_popup, "Show Key", 60, "showkey");
   w32_append_menu_item(help_popup, "Where Is Command", 61, "where_is");
   
   menubar = w32_get_menubar();
   w32_append_popup_menu(menubar, "&File", file_popup);
   w32_append_popup_menu(menubar, "&Edit", edit_popup);
   w32_append_popup_menu(menubar, "&Search", search_popup);
   w32_append_popup_menu(menubar, "&Buffers", buffers_popup);
   w32_append_popup_menu(menubar, "&Windows", windows_popup);
   w32_append_popup_menu(menubar, "&Help", help_popup);
   
   w32_set_init_popup_callback("_w32_init_popup");
   w32_redraw_menubar();
}

define _w32_init_popup(hpopup)
{
   variable mode;
   
   if (hpopup == Wmenu_Modes_Popup)
      {
	 (, mode) = what_mode();

	 
	 w32_check_menu_item(Wmenu_Modes_Popup, 40, mode == 2);   %  C mode
	 w32_check_menu_item(Wmenu_Modes_Popup, 41, mode == 1);   %  Text mode
	 w32_check_menu_item(Wmenu_Modes_Popup, 42, mode == 0);   %  No mode
	 w32_check_menu_item(Wmenu_Modes_Popup, 43, mode == 16);   %  Fortran mode
      }
}

simple_menu ();
private define simple_menu ();	       %  delete the function
