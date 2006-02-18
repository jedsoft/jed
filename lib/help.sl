%% help.sl

%% apropos function
define apropos ()
{
   variable n, cbuf, s, a;
   if (MINIBUFFER_ACTIVE) return;
   
   s = read_mini("apropos:", "", "");
   a = _apropos("Global", s, 0xF);
   vmessage ("Found %d matches.", length (a));
   a = a[array_sort (a)];
   cbuf = whatbuf();
   pop2buf("*apropos*");
   erase_buffer();
   foreach (__tmp(a))
     {
	insert(());
	newline();
     }
   buffer_format_in_columns();   
   bob();
   set_buffer_modified_flag(0);
   pop2buf(cbuf);
}

private define is_gold_key (str)
{
   variable gold = __get_reference ("Key_Gold");
   if (gold == NULL)
     return 0;

   gold = @gold;
   if (gold[0] == '\e')
     gold = "[" + substr (gold, 2, -1);
   return str == gold;
}


%!%+
%\function{expand_keystring}
%\synopsis{expand_keystring}
%\usage{String expand_keystring (String key)}
%\description
% This function takes a key string that is suitable for use in a 'setkey'
% definition and expands it to a human readable form.  
% For example, it expands ^X to the form "Ctrl-X", ^[ to "ESC", 
% ^[[A to "UP", etc...
%\seealso{setkey}
%!%-
define expand_keystring (key)
{
   variable n = strlen (key);
   variable str, i = 1, ch, key_string = "", the_key;
   
   while (i <= n)
     {
	ch = typecast (key[i - 1], UChar_Type);
	++i;
	if (((ch == '^') and (i <= n)) or (ch < 32))
	  {
	     % control char
	     %% common cases are '^[, ^[[?, ^[O?'

	     if (ch < 32)
	       {
		  str = char (ch + '@') + substr (key, i, 2);
		  i--;
	       }
	     else str = substr (key, i, 3);   %  ^[ form
	     if (str[0] == '[')
	       {
		  i += 3;
		  switch (str)
#ifndef IBMPC_SYSTEM	     
		    { case "[[A" : "UP"; }
		    { case "[[B" : "DOWN"; }
		    { case "[[C" : "RIGHT"; }
		    { case "[[D" : "LEFT"; }
		    { case "[OA" : "UP"; }
		    { case "[OB" : "DOWN"; }
		    { case "[OC" : "RIGHT"; }
		    { case "[OD" : "LEFT"; }
#endif
		    { is_gold_key(str): "GOLD"; }
		    { % default
		       i -= 2; "ESC";
		    }
		  the_key = ();
	       }
	     else
	       {
		  i++;
		  the_key = "Ctrl-" + char(int(str));
	       }
	  }
	else the_key = char (ch);
	
	if (strlen(key_string)) key_string += " ";
	key_string += the_key;
     }
   
   key_string;
}


%% show key
public define showkey ()
{
   variable f, type;

   flush("Show Key: ");

   (type, f) = get_key_binding ();

   if (f == NULL)
     {
	vmessage ("Key \"%s\" is undefined.",
		  expand_keystring (LASTKEY));
	return;
     }
   
   variable ks = expand_keystring (LASTKEY);

   switch (type)
     {
      case 0:
	if (1 == is_defined (f))
	  vmessage ("Key \"%s\" runs the intrinsic function \"%s\".", ks, f);
	else
	  vmessage ("Key \"%s\" runs the S-Lang function \"%s\".", ks, f);
     }
     {
      case 1:
	vmessage ("Key \"%s\" runs the internal function \"%s\".", ks, f);
     }
     {
      case 2:
	vmessage ("Key \"%s\" runs the keyboard macro \"%s\".", ks, f);
     }
     {
      case 3:
	vmessage ("Key \"%s\" inserts \"%s\".", ks, f);
     }
     {
      case 4:
	vmessage ("Key \"%s\" is a reference %S", ks, f);
     }
}

define where_is ()
{
   variable n, cmd;
   if (MINIBUFFER_ACTIVE) return;
   
   cmd = read_with_completion ("Where is command:", "", "", 'F');
   !if (strlen (cmd)) return;
   n = which_key (cmd);
   !if (n) return message ("Not on any keys.");
   
   message (expand_keystring ());
   --n; loop (n) pop ();
}


define help_get_doc_string (f)
{
   variable file;
   variable n, str;
   
   n = 0;
   str = NULL;
   do
     {
	file = extract_element (Jed_Doc_Files, n, ',');
	if (file == NULL)
	  break;
	
	str = get_doc_string_from_file (file, f);
	n++;
     }
   while (str == NULL);
   
   return (file, str);
}

define help_for_function (f)
{
   variable cbuf = whatbuf ();
   variable tmp = " *jed-help*";
   variable help = "*function-help*";
   variable doc_str, file;
   variable value;
   variable str = "";

   % For variables such as TAB, whose value depends upon the buffer, 
   % evaluate the variable in the current buffer.
   if (is_defined (f) < 0)
     {
	% __get_reference cannot return NULL since f is defined
	value = __get_reference (f);
	if (__is_initialized (value))
	  str = sprintf ("%S %s: value = %S\n", typeof (@value), f, @value);
	else
	  str = ("%s: <Uninitialized Variable>\n", f);
     }
   else if (is_internal (f))
     str = (f + ": internal function\n");
   
   pop2buf (help); set_readonly (0); erase_buffer ();
   vinsert (str);

   (file, doc_str) = help_get_doc_string (f);
   if (doc_str != NULL)
     vinsert ("%s[Obtained from file %s]", doc_str, file);     
   else	if (is_internal (f)) % this block can be removed
     {                  % once  internal funs are documented
	vinsert ("\nUse  call (\"%s\")  to access from slang\n\n", f);
	insert (strcat ("You might bind an internal function to a key ",
			"using setkey() or definekey()\n"));
     }
   else 
     {
	vinsert ("%s: Undocumented ", f);
	switch (is_defined (f))
	  {
	   case 1: 
	     insert ("intrinsic function");
	  }
	  {
	   case 2:
	     insert ("slang function");
	  }
	  {
	     insert (" and unknown");
	  }
     }

   insert ("\n-----------------------------------\n");
   
   bob ();
   set_buffer_modified_flag (0);
   pop2buf (cbuf);
}

define help_do_help_for_object (prompt, flags)
{
   variable n, objs;
   
   if (MINIBUFFER_ACTIVE) return;

#ifntrue
   n = _apropos ("", flags);
   
   objs = "";
   loop (n)
     {
	objs = () + "," + objs;
     }
#else
   objs = _apropos ("Global", "", flags);
   objs = objs[array_sort (objs)];
   objs = strjoin (objs, ",");
#endif
   help_for_function (read_string_with_completion (prompt, "", objs));
}

   
define describe_function ()
{
   help_do_help_for_object ("Describe Function:", 0x3);
}

define describe_variable ()
{
   help_do_help_for_object ("Describe Variable:", 0xC);
}


define describe_mode ()
{
   variable flags, modstr;
   (modstr, flags) = what_mode ();
   
   modstr = extract_element (modstr, 0, ' ');
   !if (strlen (modstr)) modstr = "no";
   !if (is_defined (modstr))
     {
	modstr = strlow (modstr);
	!if (is_defined (modstr))
	  {
	     modstr += "_mode";
	     !if (is_defined (modstr))
	       error ("Mode is not defined: " +  modstr);
	  }
     }
   help_for_function (modstr);
}


define describe_bindings ()
{
   flush("Building bindings..");
   variable map = what_keymap ();
   variable buf = whatbuf ();
   variable cse = CASE_SEARCH;  CASE_SEARCH = 1;
   pop2buf("*KeyBindings*");
   erase_buffer ();
   dump_bindings (map);
   bob(); replace ("ESC [ A", "UP");
   bob(); replace ("ESC [ B", "DOWN");
   bob(); replace ("ESC [ C", "RIGHT");
   bob(); replace ("ESC [ D", "LEFT");
   bob(); replace ("ESC O P", "GOLD");
   bob();
   CASE_SEARCH = cse;
   set_buffer_modified_flag(0);
   pop2buf (buf);
   message ("done");
}

	    
   
