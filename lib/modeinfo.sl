% These routines facilitate the manipulation of mode dependent data.

private variable Mode_Info_Type = struct
{
     fold_info,			       %  folding.sl: beg\rend\r\beg1\r\end1
     dabbrev_word_chars,		       %  word chars for dabbrev mode
     init_mode_menu,
     chglog_get_item,
     use_dfa_syntax,
};

private variable Mode_Info_List = Assoc_Type [Struct_Type];

private define find_mode_info (mode_name)
{
   !if (assoc_key_exists (Mode_Info_List, mode_name))
     return NULL;
   
   return Mode_Info_List[mode_name];
}

define mode_set_mode_info (field_name, field_value)
{
   variable mode_info;
   variable mode_name = get_mode_name ();
   
   if (_NARGS == 3)
     mode_name = ();
     
   mode_info = find_mode_info (mode_name);
   if (mode_info == NULL)
     {
	mode_info = @Mode_Info_Type;
	Mode_Info_List [mode_name] = mode_info;
     }

   variable fields = get_struct_field_names (mode_info);
   !if (any (fields == field_name))
     {
	variable new_mode_info = @Struct_Type ([fields, field_name]);
	
	foreach (fields)
	  {
	     variable f = ();
	     set_struct_field (new_mode_info, f, get_struct_field (mode_info, f));
	  }
	mode_info = new_mode_info;
	Mode_Info_List [mode_name] = mode_info;
     }
   set_struct_field (mode_info, field_name, field_value);
}
   
define mode_get_mode_info (field_name)
{
   variable mode_info;
   
   if (_NARGS == 1)
     get_mode_name ();

   variable mode_name = ();
     
   mode_info = find_mode_info (mode_name);
   if (mode_info == NULL)
     return NULL;
   
   !if (any (field_name == get_struct_field_names (mode_info)))
     return NULL;

   return get_struct_field (mode_info, field_name);
}
