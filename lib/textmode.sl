%!%+
%\function{text_indent_relative}
%\synopsis{Indent to next indentation point}
%\description
% The \var{text_indent_relative} function inserts enough whitespace to move 
% the editing point to the next indentation level defined by the whitespace
% pattern of the previous non-blank line.  If the current point is beyond
% the last indentation level of the reference line, then a literal TAB will 
% be inserted into the buffer.
%\seealso{set_buffer_hook, newline_and_indent}
%!%-
public define text_indent_relative ()
{
   variable c0, c;
   
   push_spot ();
   c0 = what_column ();
   bol ();
   bskip_chars ("\n\t ");
   if (bobp ())
     {
	pop_spot ();
	insert ("\t");
	return;
     }

   c = goto_column_best_try (c0);
   skip_white ();

   if (c == what_column ())
     {
	skip_chars ("^ \t\n");
	skip_white ();
     }

   c = what_column ();
   pop_spot ();
   if (c <= c0)
     {
	insert ("\t");
	return;
     }
   whitespace (c - c0);
}

define text_newline_and_indent_relative ()
{
   push_spot ();
   bol_skip_white ();
   variable skip_indent = bolp ();
   pop_spot ();
   newline ();
   if (skip_indent)
     return;
   indent_line ();
}

$1 = "Text";
!if (keymap_p ($1)) make_keymap ($1);
definekey ("indent_line", "\t", $1);

%!%+
%\function{text_mode}
%\synopsis{text_mode}
%\description
% Mode for indenting and wrapping text
% Functions that affect this mode include:
% 
%#v+
%     Function:                 Default Binding:
%       text_indent_relative        TAB
%       newline_and_indent          RETURN
%       format_paragraph            ESC Q
%       narrow_paragraph            ESC N
%#v-
% 
%\seealso{no_mode, c_mode, set_buffer_hook}
%\seealso{WRAP_INDENTS, WRAP, TAB, TAB_DEFAULT}
%!%-
public define text_mode()
{
   variable mode = "Text";
   no_mode ();
   set_mode(mode, 1);
   use_keymap (mode);
   %set_buffer_hook ("indent_hook", "text_indent_relative");
   %set_buffer_hook ("newline_indent_hook", "text_newline_and_indent_relative");
   unset_buffer_hook ("indent_hook");
   unset_buffer_hook ("newline_indent_hook");
   run_mode_hooks ("text_mode_hook");
}
