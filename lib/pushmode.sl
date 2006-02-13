% The functions push_mode and pop_mode are designed to allow one to temporarily
% switch buffer modes and then restore the original mode.
% It requires a version of jed with buffer local variables.
% To use it, add:
%
%    autoload ("push_mode", "pushmode");
%
% to you .jedrc file.

define push_mode ()
{
   variable mode, old_mode, keymap;
   variable var_name, try_mode;

   var_name = "push-mode-stack";
   !if (blocal_var_exists (var_name))
     define_blocal_var (var_name, "");

   if (_NARGS)
     mode = ();
   else
     mode = strtrim (read_mini ("Push to mode:", Null_String, Null_String));
   
   !if (strlen (mode))
     return;
   
   if (is_defined (mode) <= 0)
     {
	mode += "_mode";
	if (is_defined (mode) <= 0)
	  error ("Mode is not defined.");
     }
   
   (old_mode,) = what_mode ();
   !if (strlen (old_mode))
     old_mode = "no";
   
   old_mode = strtrans (old_mode, "-", "_");
   % Some modes may set modeline to two words.
   try_mode = strlow (strtrans (old_mode, " ", "_") + "_mode");
   if (2 != is_defined (try_mode))
     {
	try_mode = strlow (extract_element (old_mode, 0, ' ') + "_mode");
	if (2 != is_defined (try_mode))
	  verror ("Unable to get mode.  Tried %s.", try_mode);
     }
   
   keymap = what_keymap ();
   
   no_mode ();
   eval ("." + mode);		       %  RPN form

   set_blocal_var (sprintf (".%s|%s,%s", try_mode, keymap,
			    get_blocal_var (var_name)),
		   var_name);
}

define pop_mode ()
{
   variable var_name = "push-mode-stack";
   variable modes, keymap, mode;
   
   modes = get_blocal_var (var_name);
   !if (strlen (modes))
     error ("mode stack is empty.");
   
   mode = extract_element (modes, 0, ',');
   keymap = extract_element (mode, 1, '|');
   mode = extract_element (mode, 0, '|');
   
   no_mode ();
   eval (mode);
   use_keymap (keymap);
   set_blocal_var (extract_element (modes, 1, ','), var_name);
}

   
   
   
   
   
   
   
   
