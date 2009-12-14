% Functions and variables for (un-)commenting lines and regions
% taken from ide.sl
% modified by GM <g.milde web.de>
% modified by JED

custom_variable ("JED_COMMENT_COLUMN", 40);

private variable Comment_Data = Assoc_Type[Struct_Type];

private define extract_major_mode (mode)
{
   % mode can be: "major-mode (minor-mode)"
   return extract_element (mode, 0, ' ');
}


%!%+
%\function{get_comment_info}
%\synopsis{Get comment information according to mode}
%\usage{Struct_Type = get_comment_info ( [mode] ); }
%\description
% Retrieves the comment information according to the optional \exmp{mode}
% argument, or for the present mode if \exmp{mode} is not present.
% Every mode that wants to use this function should provide comment information
% using the \sfun{set_comment_info} function.
%
% The returned value is a structure with the following fields:
%#v+
%     cbeg       % begin comment string
%     cend       % end comment string
%     flags      % flags
%     column     % preferred column for comments
%#v-
% If comment information does not exist for the mode, then \ivar{NULL} will
% be returned.
%\seealso{set_comment_info, comment_region, comment_line, uncomment_region}
%!%-
public define get_comment_info ()
{
   !if (_NARGS)
     get_mode_name ();

   variable modename = ();
   
   modename = extract_major_mode (modename);

   loop (2)
     {
	if (assoc_key_exists (Comment_Data, modename))
	  return Comment_Data[modename];
	
	modename = strlow (modename);
     }
   return NULL;
}

%!%+
%\function{set_comment_info}
%\synopsis{Set comment information for a mode}
%\usage{set_comment_info ([mode,] cbeg, cend, flags)}
%\description
% This function sets comment information for a specified mode. If the
% optional mode argument is not present, the current mode will be used.  The
% other 3 required arguments represent the comment start string (\exmp{cbeg}), 
% the comment end string (\exmp{cend}), and an integer flags argument that 
% indications how these strings are to be used by the \sfun{comment_region} 
% function.  In particular, \exmp{flags} is a bitmapped integer whose bits
% have the following meaning:
%#v+
%     0x01  :  Comments will start at column defined by the region start,
%               otherwise comments will be indented to the level of the
%               first line in the region.
%     0x02  :  Lines in the region will be padded out to the same column.
%     0x04  :  Blank lines will be commented.
%#v-
%\seealso{set_comment_info, comment_region, comment_line, uncomment_region}
%!%-
public define set_comment_info (cbeg, cend, flags)
{
   if (_NARGS == 3)
     get_mode_name ();
   variable mode = ();
   
   variable s = struct 
     {
	cbeg, cend, flags, column
     };
   s.cbeg = cbeg;
   s.cend = cend;
   s.flags = flags;
   s.column = JED_COMMENT_COLUMN;

   Comment_Data[mode] = s;
}

public define set_comment_column (column)
{
   if (_NARGS == 1)
     get_mode_name ();
   variable mode = ();
   variable s = get_comment_info (mode);
   if (s != NULL)
     s.column = column;
}

private define _get_comment_info ()
{
   variable m = get_mode_name ();
   m = extract_major_mode (m);
   variable s = get_comment_info (m);
   if (s == NULL)
     verror("No comment strings defined for %s mode", m);
   return s;
}

private define compute_max_column (beg_mark, end_mark)
{
   variable max_column;
   
   goto_user_mark (end_mark);
   max_column = what_column ();
   goto_user_mark (beg_mark);

   forever
     {
	eol ();
	if (create_user_mark () >= end_mark)
	  break;
	bskip_white ();
	if (what_column () > max_column)
	  max_column = what_column ();
	go_down_1 ();
     }
   return max_column;
}


public define comment_region ()
{
   check_region (1);

   ERROR_BLOCK
     {
	pop_spot ();
	pop_mark_0 ();
     }

   variable info = _get_comment_info ();
   variable cbeg = info.cbeg;
   variable cend = info.cend;
   variable flags = info.flags;
   
   variable end_mark = create_user_mark ();
   exchange_point_and_mark ();
   variable beg_mark = create_user_mark ();
   variable indent_col = 1;

   if (beg_mark == end_mark)
     {
	pop_spot ();
	pop_mark_0 ();
	insert (cbeg);
	insert (cend);
	return;
     }

   if (flags & 0x01)
     {
	indent_col = what_column ();
	bskip_white ();
	!if (bolp())
	  indent_col = 1;
     }
   else
     {
	skip_chars ("\n\t ");
	bskip_white ();
	if (bolp ())
	  {
	     skip_white ();
	     if (create_user_mark () < end_mark)
	       {
		  indent_col = what_column ();
		  move_user_mark (beg_mark);
	       }
	  }
     }

   goto_user_mark (end_mark);
   bskip_chars ("\n\t ");

   variable ok_to_pad_end = 0;
   if (create_user_mark () > beg_mark)
     {
	move_user_mark (end_mark);
	pop_mark_0 ();
	push_mark ();
	skip_white ();
	ok_to_pad_end = eolp ();
     }

   variable max_column = 0;
   if ((flags & 0x02) and strlen (cend))
     max_column = compute_max_column (beg_mark, end_mark) + strlen (cbeg);
	

   goto_user_mark (beg_mark);
   insert (cbeg);
   bol ();
   narrow_to_region ();

   ERROR_BLOCK
     {
	widen_region ();
	pop_spot ();
     }
   eol ();

   if (max_column)
     {
	trim ();
	if (what_column () < max_column) 
	  goto_column (max_column);	
     }
   insert (cend);

   variable comment_blank_lines = flags & 0x04;

   forever 
     {
	if (comment_blank_lines)
	  {
	     !if (down_1 ())
	       break;
	     skip_white ();
	     if (eolp ())
	       goto_column (indent_col);
	  }
	else
	  {
	     skip_chars ("\n\t ");
	     if (eobp ())
	       break;
	  }

	if (what_column () >= indent_col)
	  goto_column (indent_col);

	insert (cbeg);
	eol ();
	if (max_column)
	  {
	     if ((create_user_mark () < end_mark ())
		 or ok_to_pad_end)
	       {
		  trim ();
		  if (what_column () < max_column) 
		    goto_column (max_column);
	       }
	  }
	insert (cend);
     }
   widen_region ();
   pop_spot ();
}

public define uncomment_region ()
{
   check_region (1);
   variable end_mark = create_user_mark ();
   exchange_point_and_mark ();
   narrow_to_region ();

   ERROR_BLOCK
     {
	widen_region ();
	pop_spot ();
     }

   variable info = _get_comment_info (); 
   variable cbeg = info.cbeg;
   variable cend = info.cend;
   variable do_trim = info.flags & 0x02;

   variable len_cbeg = strlen (cbeg);
   variable len_cend = strlen (cend);

   do
     {
	!if (ffind (cbeg))
	  continue;
	
	deln (len_cbeg);
	eol ();
	!if (bfind (cend))
	  continue;

	deln (len_cend);
	if (do_trim)
	  {
	     skip_white ();
	     if (eolp ())
	       trim ();
	  }
     }
   while (down_1 ());
   widen_region ();
   pop_spot ();
}

public define comment_line ()
{
   ERROR_BLOCK { pop_spot (); }
   push_spot_bol ();
   push_mark_eol ();
   comment_region ();
   pop_spot ();
}

public define uncomment_line ()
{
   ERROR_BLOCK { pop_spot (); }
   push_spot_bol ();
   push_mark_eol ();
   uncomment_region ();
   pop_spot ();
}

public define comment_region_or_line ()
{
   if (-1 != prefix_argument (-1))
     {
	uncomment_region_or_line ();
	return;
     }

   if (markp ())
     return comment_region ();

   variable s = _get_comment_info ();
   push_spot ();
   bol_skip_white ();
   if (looking_at (s.cbeg))
     {
	pop_spot ();
	return uncomment_line ();
     }
   pop_spot ();
   comment_line ();
}

public define uncomment_region_or_line ()
{
   if (markp ())
     uncomment_region ();
   else
     uncomment_line ();
}


% for some modes we have comment string definitons here
set_comment_info ("html", "<!-- ", " -->", 0);
set_comment_info ("sgml", "<!-- ", " -->", 0);
set_comment_info ("docbook", "<!-- ", " -->", 0);
set_comment_info ("C", "/* ", " */", 0);
set_comment_info ("SLang", "% ", "", 0);
set_comment_info ("TeX", "% ", "", 0);
set_comment_info ("LaTeX", "% ", "", 0);
set_comment_info ("SH", "# ", "", 0);
set_comment_info ("matlab", "# ", "", 0);
set_comment_info ("perl", "# ", "", 0);
set_comment_info ("Fortran", "C ", "", 0);
set_comment_info ("TPas", "{ ", " }", 0);
set_comment_info ("PHP", "// ", "", 0);
set_comment_info ("java", "/* ", " */", 0);
set_comment_info ("tm", "#% ", "", 0);
set_comment_info ("python", "# ", "", 0);
set_comment_info ("idl", "; ", "", 0);

provide ("comments");

