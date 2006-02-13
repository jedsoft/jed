% This interface is obsolete.  New code should use the add_to_hook function

private variable Hooks = NULL;

private define add_hook_function (name, main_hook)
{
   variable h;
   
   h = struct
     {
	hook_name,
	next,
	main_hook_function,
	list_of_hooks
     };

   h.hook_name = name;
   h.main_hook_function = main_hook;
   
   h.next = Hooks;
   Hooks = h;
}

private define find_hook (name)
{
   variable h;
#iftrue
   foreach (Hooks)
     {
	h = ();
	if (name == h.hook_name)
	  return h;
     }
   return NULL;
#endif
}

define hook_add_hook (hook_name, hook_function)
{
   variable h;
   variable list;

   switch (hook_name)
     {
      case "save_buffer_hook":
	add_to_hook ("_jed_write_buffer_before_hooks", hook_function);
	return;
     }
     {
      case "init_display_hook":
	add_to_hook ("_jed_init_display_hooks", hook_function);
	return;
     }
     {
      case "reset_display_hook":
	add_to_hook ("_jed_reset_display_hooks", hook_function);
	return;
     }
   
   h = find_hook (hook_name);
   if (h == NULL)
     verror ("hook %s unknown to this interface", hook_name);

   list = struct
     {
	hook_function,
	next
     };
   list.hook_function = hook_function;
   list.next = h.list_of_hooks;
   h.list_of_hooks = list;
}

   

% This function just runs the hooks with arguments assuming that the
% hook returns nothing
private define do_simple_hook (name)
{
   variable args, h;

   args = __pop_args (_NARGS - 1);

   h = find_hook (name);
   if (h == NULL)
     return;

   foreach (h.list_of_hooks)
     {
	h = ();
	@h.hook_function (__push_args(args));
     }
}

   
define save_buffer_hook (file, mode)
{
   do_simple_hook (file, mode, _function_name ());
}
add_hook_function ("save_buffer_hook", &save_buffer_hook);

define init_display_hook ()
{
   do_simple_hook (_function_name ());
}
add_hook_function ("init_display_hook", &init_display_hook);

define reset_display_hook ()
{
   do_simple_hook (_function_name ());
}
add_hook_function ("reset_display_hook", &reset_display_hook);
