%% occur function
%%


$1 = "Occur";
!if (keymap_p ($1))
{
   make_keymap ($1);
}

definekey ("occur_goto_line", "g", $1);
variable Occur_Buffer = Null_String;

define occur_goto_line ()
{
   variable line;
   
   !if (bufferp (Occur_Buffer))
     return;
   
   bol ();
   push_mark ();
   !if (ffind (":"))
     {
	pop_mark_0 ();
	return;
     }
   
   line = integer (bufsubstr ());
   
   
   pop2buf (Occur_Buffer);
   goto_line (line);
}


%!%+
%\function{occur}
%\synopsis{occur}
%\usage{Void occur ();}
%\description
% This function may be used to search for all occurances of a string in the
% current buffer.  It creates a separate buffer called \var{*occur*} and 
% associates a keymap called \var{Occur} with the new buffer.  In this
% buffer, the \var{g} key may be used to go to the line described by the
% match.
%!%-
define occur()
{
   variable str, tmp, n;
   
   str = read_mini("Find All (Regexp):", LAST_SEARCH, Null_String);
   !if (strlen (str))
     return;

   tmp = "*occur*";
   Occur_Buffer = whatbuf();
   pop2buf(tmp);
   erase_buffer();
   pop2buf(Occur_Buffer);
   
   push_spot();
   bob ();
   while (re_fsearch(str))
     {
	line_as_string ();  % stack-- at eol too
	n = what_line ();
	
	setbuf(tmp);
	vinsert ("%4d:", n);
	insert(()); 
	newline();
	setbuf(Occur_Buffer);
	!if (down_1 ()) %% so we do not find another occurance on same line
	  break;
     }
   pop_spot();
   setbuf(tmp);
   bob(); set_buffer_modified_flag(0);
   
   use_keymap ("Occur");
   run_mode_hooks ("occur_mode_hook");
}

