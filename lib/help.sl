%% help.sl
require ("keydefs");
autoload ("glob", "glob");


%!%+
%\variable{Help_Describe_Bindings_Show_Synopsis}
%\synopsis{Used to control the searching of synopsis strings}
%\usage{variable Help_Describe_Bindings_Show_Synopsis = 0;}
%\description
% If the value of this variable is non-zero, the
% \sfun{describe_bindings} function will search through the
% documentation for synopsis strings and display the resulting strings
% along with the key bindings.  Since this can be a time consuming
% process for slow computers or slow filesystems, this feature is
% turned off by default.
%\example
%  variable Help_Describe_Bindings_Show_Synopsis = 1;
%!%-
custom_variable ("Help_Describe_Bindings_Show_Synopsis", 0);

% Convert all controls chars in key and return ^ form.  (\e --> ^[)
private define convert_keystring (key)
{
   variable new_key = "";
   variable i = 1, n = strlen (key);
   while (i <= n)
     {
	variable the_key = substr (key, i, 1); i++;
	if (the_key[0] < ' ')
	  the_key = strcat ("^", char (the_key[0] + '@'));
	new_key = strcat (new_key, the_key);
     }
   return new_key;
}	     

private define make_key_name_table ()
{
   variable key_vars = _apropos ("Global", "\\c^Key_", 8);
   variable a = Assoc_Type[String_Type];
   foreach (key_vars)
     {
	variable key_name = ();
	variable vref = __get_reference (key_name);
	if (vref == NULL)
	  continue;
	if (0 == __is_initialized (vref))
	  continue;
	variable value = @vref;
	if (typeof (value) != String_Type)
	  continue;
	if (value == "")
	  continue;
	key_name = strtrans (substr (key_name, 5, -1), "_", "-");
	value = convert_keystring (value);
	a[value] = key_name;
     }
   return a;
}

private variable Key_Name_Table = make_key_name_table ();
private define make_key_name_table (); %  nolonger needed

% a definition for a lonely Alt character causes problems, because no
% Alt-XY is recognized
$1 = __get_reference ("ALT_CHAR");
if ($1 != NULL)
{
   $1 = @$1;
   if ($1 != 0)
     assoc_delete_key (Key_Name_Table, convert_keystring(char($1)));
}

% the definition of Key_Space is missing, but it is useful
!if (assoc_key_exists(Key_Name_Table, " "))
  Key_Name_Table[" "] = "Space";

private define keyeqs (seq, key)
{
   variable n = strbytelen (key);
   if (strnbytecmp (seq, key, n))
     return 0;
   
   return n;
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
define expand_keystring (seq)
{
   variable alt_char = 0;

   if (is_defined ("ALT_CHAR"))
     alt_char = @__get_reference ("ALT_CHAR");

   seq = convert_keystring (seq);
   
   if (assoc_key_exists (Key_Name_Table, seq))
     return Key_Name_Table[seq];
   
   variable key_seqs = assoc_get_keys (Key_Name_Table);
   variable key_name, expanded_key = "";
   
   forever 
     {
	variable n = strbytelen (seq);
	if (n == 0)
	  break;
	
	variable dn = 0;
	foreach (key_seqs)
	  {
	     variable key_seq = ();
	     dn = keyeqs (seq, key_seq);
	     if (dn)
	       {
		  key_name = Key_Name_Table[key_seq];
		  break;
	       }
	  }
	variable append_space = 1;
	if (dn == 0)
	  {
	     if ((seq[0] == '^') and (n > 1))
	       {
		  variable ch = seq[1];
		  switch (ch)
		    { case 'I': "TAB";}
		    { case 'M': "RET";}
		    { case '[':
		       if ((alt_char == 27) and (seq[2] != '\0'))
			 {
			    append_space = 0;
			    "Alt-";
			 }
		       else
			 "ESC";
		    }
		    {
		       % default
		       "Ctrl-" + char (seq[1]);
		    }
		  key_name = ();
		  dn = 2;
	       }
	     else
	       {
		  key_name = substr (seq, 1, 1);
		  dn = strbytelen (key_name);
	       }
	  }
	expanded_key = strcat (expanded_key, key_name);
	if (append_space)
	  expanded_key = strcat (expanded_key, " ");
	seq = substrbytes (seq, dn+1, -1);
     }
   if (strlen (expanded_key))
     return substr (expanded_key, 1, strlen(expanded_key) - 1);
   return expanded_key;
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
	
	if (2 == file_status (file))
	  {
	     foreach (glob (path_concat (file, "*.hlp")))
	       {
		  file = ();
		  str = get_doc_string_from_file (file, f);
		  if (str != NULL)
		    break;
	       }
	  }
	else
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
   pop2buf("*KeyBindings*");
   variable cse = CASE_SEARCH;  CASE_SEARCH = 1;
   erase_buffer ();
   dump_bindings (map);
   
   if (map != "global")
     {
	variable dump_end_mark = create_user_mark();
	insert("\nInherited from the global keymap:\n");
	push_spot();
	dump_bindings("global");
	pop_spot();

	variable global_map = Assoc_Type[String_Type, ""];
	while ( not eobp() )
	  {
	     push_mark();
	     () = ffind("\t\t\t");
	     variable key = bufsubstr();
	     go_right (3);
	     push_mark();
	     % Could have a newline here
	     if (not fsearch("\t\t\t")) eob ();
	     go_up_1();
	     () = dupmark();
	     global_map[key] = bufsubstr();
	     del_region ();
	     delete_line ();
	  }
	
	bob();
	forever
	  {
	     push_mark();
	     () = ffind("\t\t\t");
	     key = bufsubstr();
	     if (key == "")
	       break;
	     
	     variable global_map_key = global_map[key];
	     go_right (3);
	     if (global_map_key != "")
	       {
		  push_mark();
		  () = fsearch("\t\t\t");
		  if (create_user_mark() > dump_end_mark)
		    goto_user_mark(dump_end_mark);
		  go_up_1();
		  () = dupmark();

		  if (bufsubstr() == global_map_key)
		    {
		       del_region ();
		       delete_line ();
		       push_spot();
		       eob();
		       (global_map_key,) = strreplace (global_map_key, "\n", "\\n", strlen(global_map_key));
		       vinsert ("%s\t\t\t%s\n", key, global_map_key);
		       pop_spot();
		    }
		  else
		    {
		       pop_mark_0();
		       go_down_1 ();
		    }
	       }
	     else
	       {
		  () = fsearch ("\t\t\t");
		  bol ();
	       }
	  }
     }
   else
     {
	bob();
	while ( not eobp() )
	  {
	     if (not ffind("\t\t\t"))
	       {
		  go_up_1();
		  insert("\\n");
		  del();
	       }
	     if (0 == down_1()) break;
	  }
     }
   
   bob(); 

   do
     {
	push_mark ();
	if (not (ffind("\t\t\t")))
	  {
            pop_mark(0);
            continue;
         }

        key = bufsubstr();
        variable old_len = strlen(key);
        (key,) = strreplace(key, "ESC", "\e", strlen(key));
        key = str_delete_chars(key, " ");
        (key,) = strreplace(key, "SPACE", " ", strlen(key));
        (key,) = strreplace(key, "DEL", "\x7F", strlen(key));
        (key,) = strreplace(key, "TAB", "\t", strlen(key));
        bol();
        () = replace_chars(old_len, expand_keystring(key));

        if (what_column () <= TAB)
	  insert_char('\t');
        else
         {
            if ( what_column() <= TAB*4 )
              deln( (what_column()-1)/TAB - 1 );
	    else
	      deln(3);
	 }
     }
   while (down_1 ());

   bob();

   EXIT_BLOCK
     {
	bob();
	CASE_SEARCH = cse;
	set_buffer_modified_flag(0);
	pop2buf (buf);
	message ("done");
     }
   
   if (Help_Describe_Bindings_Show_Synopsis == 0)
     return;

   variable synopsis = Assoc_Type[String_Type];
   flush ("Looking up key descriptions...");
   while (fsearch ("\t\t\t"))
     {
	go_right (3);
	if ( looking_at_char('@') or looking_at_char(' ') )
	  continue;
        if ( looking_at_char('.') )
	  {
             eol();
             bskip_chars("a-zA-Z_");
	  }

	push_mark();
	!if ( ffind_char('(') )
	  eol();
	variable fun = bufsubstr();
	variable dsc;
	if (assoc_key_exists(synopsis, fun))
	  dsc = synopsis[fun];
	else
	  {
	     (,dsc) = help_get_doc_string(fun);
	     if (dsc == NULL) dsc = "";
	     synopsis[fun] = dsc;
	  }

	if (dsc != "")
          {
	     eol();
	     dsc = substr(dsc, is_substr(dsc, "\n SYNOPSIS\n ")+12, strlen(dsc));
	     whitespace( 48 - what_column() );
	     insert(substr(dsc, 1, is_substr(dsc, "\n")-1) );
          }
	eol ();
     }
}



