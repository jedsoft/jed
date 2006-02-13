% Register facility.
%
% CHANGELOG
% =========
%
% v2.0 2002/09/04
%       o Re-implemented to use an associative array so that registers can
%         be given meaningful names.   Added reg_get/set_registers functions 
%         for use by Francesc's register load/save functions.
%
% v1.0 2000/12/29
%
%	o Added register management from the menu bar.
%
%	o 'register' evolved from no-mode to 'quasimode'. That is,
%	  it behaves like 'minued' but without edition/update capabilities.
%	  (Francesc Rocher <f.rocher@computer.org>)
%

private variable Register_Buffer_Arrays = Assoc_Type[String_Type,""];

public define reg_get_registers ()
{
   return Register_Buffer_Arrays;
}

public define reg_set_registers (reg)
{
   Register_Buffer_Arrays = reg;
}

private define get_register_name ()
{
   variable keys = assoc_get_keys (Register_Buffer_Arrays);
   keys = strjoin (keys, ",");
   variable name = read_with_completion (keys, "Register Name:", "", "", 's');
   name = strtrim (name);
   if (name == "") name = NULL;
   return name;
}

public define reg_copy_to_register ()
{
   !if (markp ())
     error ("No region defined.");
   
   variable name = get_register_name ();
   if (name != NULL)
     {
	Register_Buffer_Arrays [name] = bufsubstr ();
     }
}

public define reg_insert_register ()
{
   variable name = get_register_name ();
   if (name == NULL)
     return;

   !if (assoc_key_exists (Register_Buffer_Arrays, name))
     {
	vmessage ("Register '%s' does not exist", name);
	return;
     }

   insert (Register_Buffer_Arrays[name]);
   vmessage ("Register '%s' inserted.", name);
}

private variable Reg_Mark;
private variable Reg_Cline = color_number ("menu_selection");
private variable Reg_Line = 1;
private variable Reg_Nwindows;
private variable Reg_Cbuf;

private define reg_prev ()
{
   return re_bsearch ("^[^ \t]");
}

private define reg_next ()
{
   return re_fsearch ("^[^ \t]");
}

private define reg_update_hook ()
{
   bol ();
   if (looking_at ("   "))
     {
        if (what_line () > Reg_Line)
          {
             !if (reg_next ())
                () = reg_prev ();
          }
        else
          {
             !if (reg_prev ())
                () = reg_next ();
          }
     }
   bol ();
   Reg_Line = what_line ();
   Reg_Mark = create_line_mark (Reg_Cline);
}

public define reg_quit ()
{
   setbuf ("*registers*");
   set_buffer_modified_flag (0);

   sw2buf (Reg_Cbuf);

   delbuf ("*registers*");
   %menu_set_object_available ("Global.&Edit.Re&gisters.&Insert", 0);
   %menu_set_object_available ("Global.&Edit.Re&gisters.Ca&ncel", 0);
   if (Reg_Nwindows == 1)
      onewindow ();
   else
      otherwindow ();
}

public define reg_insert ()
{
   push_mark ();
   eol (); 
   bskip_white ();
   variable name = bufsubstr ();
   reg_quit ();
   insert (Register_Buffer_Arrays [name]);
   recenter (0);
}

public define reg_help ()
{
   message ("?: this help, q: quit mode, RET: insert register");
}


$1 = "register";
!if (keymap_p ($1))
{
   make_keymap ($1);
   definekey ("reg_help", "?", $1);
   definekey ("reg_quit", "q", $1);
   definekey ("reg_insert", "\r", $1);
}


public define register_mode ()
{
   variable mode = "register";

   if (0 == length (Register_Buffer_Arrays))
     error ("There are no registers defined.");

   Reg_Nwindows = nwindows ();
   Reg_Cbuf = pop2buf_whatbuf ("*registers*");
   pop2buf ("*registers*");
   set_readonly (0);
   erase_buffer ();

   foreach (Register_Buffer_Arrays) using ("keys", "values")
     {
	variable key, value;
	
	(key, value) = ();
	variable l = what_line ();

	l++;
	
	insert (key); 
	newline ();
	insert (value);
	newline ();

	while (what_line () > l)
	  {
	     go_up_1 ();
	     bol ();
	     whitespace (3);
	  }
	eob ();
     }

   call ("backward_delete_char_untabify");
   bob ();
   set_buffer_modified_flag (0);
   set_buffer_hook ("update_hook", &reg_update_hook);
   %set_column_colors (Colorin_Left, 1, 3);
   toggle_readonly ();

   %menu_set_object_available ("Global.&Edit.Re&gisters.&Insert", 1);
   %menu_set_object_available ("Global.&Edit.Re&gisters.Ca&ncel", 1);

   use_keymap (mode);
   set_mode (mode, 0);
   run_mode_hooks ("register_mode_hook");
   reg_help ();
}
