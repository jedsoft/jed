% -*- SLang -*-		dired.sl
%
%  Simple dired mode for JED.
%
%
% To invoke Dired, do `C-x d' or `M-x dired'.
%
%  Moving around
%  =============
%
%  All the usual motion commands plus some extras:
%
%  `C-n' `n' SPC
%       move point to the next line (at the beginning of the file name)
%
%  `C-p' `p'
%       move point to the previous line (at the beginning of the file name)
%
%  DEL
%       move up and unflag
%
%  `^K'
%	dired_kill_line - removes a line from the dired buffer 
%       (This must be set from the dired hook)
%
%  ==============
%
%     The primary use of Dired is to "flag" files for deletion and then
%  delete the previously flagged files.
%
%  `d'
%       Flag this file for deletion.
%
%  `u'
%       Remove deletion flag on this line.
%
%  `DEL'
%       Move point to previous line and remove the deletion flag on that
%       line.
%
%  `~'
%       Flag all backup files (files whose names end with `~') for deletion
%
%  `g'
%       Update the entire contents of the Dired buffer
%
%  Deleting Files
%  ==============
%
%  `x'
%       expunge all flagged files.  Displays a list of all the file names
%       flagged for deletion, and requests confirmation with `yes'.
%       After confirmation, all the flagged files are deleted and then
%       their lines are deleted from the Dired buffer.
%
%  File Manipulations
%  ==================
%
%  `f'
%       Visit the file described on the current line, like typing `C-x C-f'
%       and supplying that file name
%
%  `v'
%       View the file described on the current line
%
%  `r'
%       rename - prompts for a new name for the file on the current line
%
%  'm'
%       move - move a group of flagged files to an new directory
%
% `M-x dired_search'
%       use fsearch with the contents of LAST_SEARCH to perform a search
%       through the files listed in the dired buffer from the current point
%       forward.  Since it stops in the file where the search string is
%       encountered, it is structured so that `M-x dired_search' from the
%       visited file will revert back to the dired buffer and continue the
%       dired_search from the next file in the list.
%

variable Dired_Buffer = "*dired*";
variable Dired_Current_Directory;

!if (keymap_p (Dired_Buffer)) make_keymap (Dired_Buffer);


definekey ("dired_find",	"\r",	Dired_Buffer);
definekey ("dired_find",	"f",	Dired_Buffer);
definekey ("dired_view",	"v",	Dired_Buffer);
definekey ("dired_tag",		"d",	Dired_Buffer);
definekey (".1 dired_untag",	"u",	Dired_Buffer);
definekey ("dired_move",	"m",	Dired_Buffer);
definekey ("dired_delete",	"x",	Dired_Buffer);
definekey (".1 dired_point",	"^N",	Dired_Buffer);
definekey (".1 dired_point",	"n",	Dired_Buffer);
definekey (".1 dired_point",	" ",	Dired_Buffer);
definekey (".1 chs dired_point",	"^P",	Dired_Buffer);
definekey (".1 chs dired_point",	"p",	Dired_Buffer);
#ifdef UNIX
definekey (".1 chs dired_untag",	"^?",	Dired_Buffer); % DEL key
#elifdef IBMPC_SYSTEM
definekey (".1 chs dired_untag",	"\xE0S",Dired_Buffer);   %  DELETE
definekey (".1 chs dired_untag",	"\eOn",	Dired_Buffer);   %  DELETE
#endif
definekey ("dired_flag_backup",	"~",	Dired_Buffer);
definekey ("dired_rename",	"r",	Dired_Buffer);
definekey ("dired_reread_dir",	"g",	Dired_Buffer);
definekey ("describe_mode",	"h",	Dired_Buffer);
definekey ("dired_quick_help",	"?",	Dired_Buffer);
definekey ("dired_quit",	"q",	Dired_Buffer);

define dired_quit ()
{
   delbuf (Dired_Buffer);
}

#ifdef VMS
% takes somthing like dev:[a.b.c] and converts it to dev:[a.b]c.dir
define dired_vms_dir (dir)
{
   variable cdir = Null_String, last = Null_String,  this;
   variable n, file, ch;
   
   n = strlen (dir);
   if (int (substr (dir, n, 1)) != ']') return dir;
   
   forever 
     {
	n--;
	!if (n) break;
	
	ch = int (substr (dir, n, 1));
	if ((ch == '.') or (ch == '[')) break;
     }
   
   if (ch == '[')
     substr (dir, 1, n) +  "000000]";
   else
     substr (dir, 1, n - 1) + "]";

   cdir = ();

   file = substr (dir, n + 1, strlen (dir));
   file = substr (file, 1, strlen (file) - 1) + ".dir";
   cdir + file;
}
#endif

define dired_read_dir (dir)
{
   variable file, flags, spaces = "  ";

#ifdef VMS
   dir = dired_vms_dir (dir);
#elifdef IBMPC_SYSTEM
   dir = expand_filename (dir);
# ifdef MSDOS MSWINDOWS
   dir = msdos_fixup_dirspec (dir);
# endif
#endif
   
   if ( file_status (dir) == 2 ) 
     {	
	(Dired_Current_Directory,) = parse_filename (dircat (dir, Dired_Buffer));
#ifdef VMS
	dir = Dired_Current_Directory;
#endif
     } 
   else 
     {
	(Dired_Current_Directory,dir) = parse_filename (dir);
     }
   if ( change_default_dir (Dired_Current_Directory) )
      error ("Failed to chdir.");
   sw2buf (Dired_Buffer);
   (file,,,flags) = getbuf_info ();
   setbuf_info (file, Dired_Current_Directory, Dired_Buffer, flags);
   set_status_line (" DIRED: %b   (%m%n)  (%p)  |  press '?' for help.", 0);
   set_readonly (0); 
   erase_buffer ();
   use_keymap (Dired_Buffer);
   set_mode ("dired", 0);
#ifdef UNIX
   shell_cmd (sprintf ("ls -al %s 2>/dev/null", dir));
#elifdef MSDOS WIN16
   shell_cmd (sprintf ("dir %s /l/ogne", dir));
#elifdef WIN32
   shell_cmd (sprintf ("dir %s /ogne", dir));
%  shell_cmd (Sprintf ("(dir %s /l/ogne) 2>&1", dir, 1));
#elifdef OS2
   shell_cmd ("dir /n " + dir);
#elifdef VMS
   shell_cmd ("directory/size/date/prot/notrail " + dir);
#endif
   bob ();
#ifdef OS2
   % kill 4 lines of header junk and 2 lines of trailer junk
   push_mark (); go_down (4);
   if ( eolp () ) 
     go_down_1 ();
   del_region ();
   eob (); push_mark ();
   go_up (3);			% implicit eol();
   del_region ();
   bob ();
#elifdef MSDOS MSWINDOWS
   push_mark (); go_down (5); del_region ();	% header junk
   eob (); push_mark ();	% trailer junk
   if ( bsearch ("bytes free") ) 
     {
	go_down_1 (); 
	del_region ();
     } 
   else 
     {
	pop_mark_0 ();
     }
   bob ();
#elifdef VMS
   push_mark (); go_down (3); del_region ();
#endif

   do 
     {
	insert (spaces);
     }
   while (down_1 ());

   bob ();
#ifdef UNIX
   if ( looking_at ("  total "))
     delete_line ();
#endif
   insert ("== ");
   insert (Dired_Current_Directory);
   newline ();
   set_buffer_modified_flag (0); set_readonly(1);
   flush ("");
}

variable Dired_Quick_Help;
Dired_Quick_Help = "d:tag file, u:untag, x:delete tagged files, r:rename, h:more help, ?:this help";

define dired_quick_help ()
{
   message (Dired_Quick_Help);
}

define dired_reread_dir ()
{
   dired_read_dir (Dired_Current_Directory);
}

% set the point at the start of the file name
define dired_point (dirn)
{
   if (dirn > 0) go_down_1 (); else if (dirn < 0) go_up_1 ();

#ifdef UNIX
   bol_skip_white ();
   if (looking_at_char ('l'))
     {
	() = ffind ("->");
	bskip_white ();
     }
   else eol ();
   bskip_chars ("^ \n");

#elifdef OS2
   eol (); 
   if (bfind_char (' ')) go_right_1 ();
#elifdef VMS MSDOS WIN16
   bol (); go_right (2);
#elifdef WIN32
   eol ();
   bskip_chars ("^ \n");
#endif
}

#ifndef VMS
define dired_kill_line ()
{
   bol ();
   if ( bobp () )  return;		% do not remove top line
   set_readonly (0);
   push_mark (); go_down_1 ();
   del_region ();
   set_buffer_modified_flag (0);
   set_readonly (1);
   dired_point (0);
}
#endif

% (name, type) = dired_getfile ()
%
% name = name of file or directory
%
% type = 0 : nothing
% type = 1 : file
% type = 2 : directory
% type = 3 : link
define dired_getfile ()
{
   variable name, type, ext, stat_buf;

   bol ();
   type = not (bobp ());		% assume it will be a file
#ifdef UNIX
   if (type)
     {
	go_right (2);
	if (looking_at_char ('l'))           type = 3;
	else if (looking_at_char ('d'))	     type = 2;
	else
	  {
	     skip_white ();
	     if ( what_column () > 3 )       type = 0;
	  }
     }

   !if ( type )
     {
	bol ();
	return (Null_String, type);
     }

   dired_point (0);
   if (type != 3)
     {
	push_mark_eol ();			% extract the name
	name = bufsubstr ();			% the name
     }
   else
     {
	go_right (ffind (" ->"));
	skip_white ();
	push_mark_eol ();
	name = bufsubstr ();
	stat_buf = stat_file (name);
	if (stat_buf != NULL)
	  {
	     if (stat_is ("dir", stat_buf.st_mode)) type = 2;
	     else if (stat_is ("reg", stat_buf.st_mode)) type = 1;
	  }
     }
#elifdef VMS
   if (type)
     {
	go_right (2);
	if ( ffind ( ".DIR" ) )
	  {
	     type = 2;
	  }
	else
	  {
	     skip_white ();
	     if ( what_column () > 3 )
	       type = 0;
	  }
     }

   !if ( type )
     {
	bol ();
	return (Null_String, type);
     }

   dired_point (0);
   push_mark ();
   () = ffind_char (';');
   skip_chars (";0-9");
   name = bufsubstr ();			% leave on the stack
#elifdef IBMPC_SYSTEM
   if ( type )
     {
	go_right (2);
	if ( ffind ("<DIR>") )
	  {
	     type = 2;
	  }
	else
	  {
	     % Following used to just be what was in the #ifndef OS2..#endif.  Prolly
	     % can be changed to just whats in the #ifdef OS2..#endif.  This is a fix
	     % for OS/2 handling  MDJ 06/11/95
# ifdef OS2
	      if ( eolp () or what_column () > 4 )
	       type = 0;
# else
	      if ( eolp () or what_column () > 3 )
	       type = 0;
# endif
	  }
     }

   !if ( type )
     {
	bol ();
	return (Null_String, type);
     }

# ifdef OS2
   eol (); bskip_white ();
   push_mark ();
   () = bfind_char (' ');
   go_right_1 ();	% Added to fix space at start of filename MDJ 06/11/95
   name = bufsubstr ();
# elifdef WIN32
   dired_point (0);
   push_mark_eol ();
   name = bufsubstr ();
# else
   dired_point (0);
   push_mark ();			% extract the name
   skip_chars ("^ \n");			% skip past name
   name = bufsubstr ();			% the name
   skip_white ();
   if ( what_column () <= 12 )
     {
	push_mark ();
	skip_chars ("^ \n");		% skip past ext
	ext = bufsubstr ();
	if (strlen (ext) or (type != 2))
	  name = sprintf ("%s.%s", name, ext);
     }
# endif
#endif
   dired_point (0);
   return (name, type);
}

define dired_tag ()
{
   variable type;
   EXIT_BLOCK { dired_point (1); }
   
   (, type) = dired_getfile ();
   if ( type != 1 ) return;		% only files!
   
   set_readonly (0);
   bol ();
   insert_char ('D'); del ();
   set_buffer_modified_flag (0);
   set_readonly (1);
}

define dired_untag (dirn)
{
   if ( dirn < 0 )
     {
	!if ( up_1 () ) error ("Top of Buffer.");
     }
   bol ();
   if ( looking_at_char ('D') )
     {
	set_readonly (0);
	insert_char (32); del ();
	set_buffer_modified_flag (0);
	set_readonly (1);
     }
   if ( dirn > 0 )
     dired_point (1);
}

% perform operation on tagged files--- 4 parameters
define dired_xop_tagged_files (prompt, msg, op_function)
{
   variable lbuf = " *Deletions*";
   variable stack, n, fails = Null_String;
   variable file;

   setbuf (Dired_Buffer);
   push_spot_bob ();

   stack = _stkdepth;			% save stack depth
   ERROR_BLOCK
     {
	_pop_n ( _stkdepth - stack );
	sw2buf (Dired_Buffer);
	set_readonly (0);
	bob ();
	while ( bol_fsearch_char ('%') )
	  {
	     insert_char ('D'); del ();
	  }
	pop_spot ();
	set_buffer_modified_flag (0); set_readonly (1);
     }

   set_readonly (0);

   while ( bol_fsearch_char ('D') )
     {
	insert_char ('%'); del ();
	dired_getfile ();
	pop ();			% pop type, leave name on stack
     }

   n = _stkdepth - stack;
   !if (n) error ("No tags!");

   sw2buf (lbuf);
   erase_buffer ();

   loop (n)
     {
	insert ();				% tagged files on stack
	newline ();
     }
   bob ();
   buffer_format_in_columns ();
   if ( get_yes_no (prompt) == 1)
     {
	sw2buf (Dired_Buffer);
	bob ();
	while ( bol_fsearch_char ('%') )
	  {
	     (file,) = dired_getfile ();
	     bol ();
	     push_spot ();
	     file = dircat (Dired_Current_Directory, file);
	     if (@op_function (file) )
	       {
		  pop_spot ();
		  flush (msg + file);
		  push_mark ();
		  go_down_1 ();
		  del_region (); go_left_1 ();
	       }
	     else
	       {
		  pop_spot ();
		  fails += " " + file;
		  insert_char (32); del ();
	       }
	  }
     }

   EXECUTE_ERROR_BLOCK;

   if ( strlen (fails) )
     message ("Operation Failed:" + fails);
}

define dired_delete ()
{
   dired_xop_tagged_files ("Delete these files", "Deleted ", &delete_file);
}

variable Dired_Move_Target_Dir;

define dired_do_move (file)
{
   variable name;

   (name,) = dired_getfile ();
   name = dircat (Dired_Move_Target_Dir, name);

   not (rename_file (file, name));
}

define dired_move ()
{
   Dired_Move_Target_Dir = read_file_from_mini ("Move to dir");
   if ( file_status (Dired_Move_Target_Dir) != 2 )
     error ("Expecting directory name");

   "Move these to " + Dired_Move_Target_Dir;
   dired_xop_tagged_files ((), "Moved ", &dired_do_move);
}

#ifndef VMS
define dired_flag_backup ()
{
   variable name, type;

   push_spot_bob (); set_readonly (0);
   while ( fsearch_char ( '~' ) )
     {
	(name, type) = dired_getfile ();
	if ( (type == 1) and (string_match (name, "~", strlen(name))) ) 
	  {
	     bol ();
	     insert_char ('D'); del ();	% is a backup file
	  }
	eol ();
     }
   pop_spot ();
   set_buffer_modified_flag (0);
   set_readonly (1);
}
#endif

define dired_rename ()
{
   variable oldf, type, len, f, n, nf, nd, od, status;

   (oldf, type)  = dired_getfile ();
   !if ( type ) return;
   sprintf ("Rename %s to", oldf);
   n = read_file_from_mini (());
   %
   %  If new name is a dir, move it to the dir with oldname.
   %  If file is not a directory and exists, signal error.
   %
   status = file_status (n);
   if ( status == 1 )  error ("File exists. Not renamed.");
   else if ( status == 2 ) n = dircat (n, oldf);

   %
   %  Check to see if rename to new directory
   %
   (nd,nf) = parse_filename (n);
   f = dircat (Dired_Current_Directory, oldf);
   (od,) = parse_filename (f);
   if ( rename_file (f, n) ) error ("Operation Failed!");
   set_readonly (0);
#ifdef UNIX
   if ( strcmp (od, nd) )
#elifdef VMS IBMPC_SYSTEM
   if ( strcmp (strup (od), strup (nd)) )
#endif
     {
	delete_line ();
     }
   else
     {
        dired_point (0);
	push_mark ();
#ifdef MSDOS MSWINDOWS
	go_right (12);  del_region ();
	variable nf1 = extract_element (nf, 1, '.');
	if (nf1 == NULL) nf1 = "";
	vinsert ("%-8s %-3s",
		 extract_element (nf, 0, '.'),
		 nf1);
#else
	go_right (strlen (oldf)); del_region ();
	insert (nf);
#endif
     }
   dired_point (1);
   set_buffer_modified_flag (0);
   set_readonly (1);
}

%!%+
%\function{dired}
%\synopsis{dired}
%\description
% Mode designed for maintaining and editing a directory.
% 
% To invoke Dired, do \var{M-x dired} or \var{C-x d} (emacs)
% 
% Dired will prompt for a directory name and get a listing of files in the
% requested directory.
% 
% The primary use of Dired is to "flag" files for deletion and then delete
% the previously flagged files.
% 
% \var{d}	Flag this file for deletion.
% \var{u}	Remove deletion flag on this line.
% DEL	Move point to previous line and remove deletion flag.
% \var{~}	Flag all backup files for deletion.
% 
% \var{x}	eXpunge all flagged files.  Dired will show a list of the
% 	files tagged for deletion and ask for confirmation before actually
% 	deleting the files.
% 
% \var{r}	Rename file on the current line; prompts for a newname
% \var{m}	Move tagged files to a new dir; prompts for dir name
% 
% \var{g}	Update the entire contents of the Dired buffer
% 
% \var{f}	Visit the file described on the current line, like typing
% 	\var{M-x find_file} and supplying that file name.  If current line is a
% 	directory, runs dired on the directory and the old buffer is killed.
% 
% \var{v}	View the file described on the current line in MOST mode.
% 
% \var{q}	Quit dired mode.
% 
% \var{M-x dired_search}
% 	use fsearch to perform a search through the files listed in the
% 	dired buffer from the current point forward.  \var{M-x dired_search}
% 	from the visited file will revert to the dired buffer and continue
% 	the search from the next file in the list.
% 
% all the usual motion commands plus some extras:
% 
% \var{C-n} \var{n} SPC
% 	move point to the next line (at the beginning of the file name)
% 
% \var{C-p} \var{p}
% 	move point to the previous line (at the beginning of the file name)
% 
% \var{M-x dired_kill_line}	\var{^K} (emacs)
% 	removes a line from the dired buffer
%!%-
define dired ()
{
   dired_read_dir (read_file_from_mini ("Directory:"));
   dired_quick_help ();
   run_mode_hooks ("dired_hook");
}

define dired_find ()
{
   variable name, type;

   (name, type) = dired_getfile ();

   name = dircat (Dired_Current_Directory, name);

   if ( type == 1 ) 
     {
	!if ( read_file (name) ) error ("Unable to read file.");
	pop2buf (whatbuf ());
     } 
   else if ( type == 2 ) 
     {
	dired_read_dir (name);
     }
}

define dired_view ()
{
   variable name, type;

   (name, type) = dired_getfile ();

   name = dircat (Dired_Current_Directory, name);
   if ( type == 1 ) 
     {
	!if ( read_file (name) ) error ("Unable to read file.");
	pop2buf (whatbuf ());
	most_mode ();
     }
}

#ifndef VMS
define dired_search_files ()
{
   go_right_1 ();		% start after this one
   if ( fsearch (LAST_SEARCH) )
     return (1);		% found - stop search
   
   if ( buffer_modified () ) error ("buffer has been modified");
   delbuf (whatbuf ());
   pop2buf (Dired_Buffer);
   return 0;
}

define dired_search ()
{
   variable str, name, type;

   !if ( bufferp (Dired_Buffer) ) error ( "*dired* not available.");
   
   if ( strcmp (Dired_Buffer, whatbuf () ) ) % continue last search
     {      
	!if ( strlen (LAST_SEARCH) ) error ("No specified search string");
	if ( dired_search_files () ) return;
	go_down_1 (); 			% do the next file!
     }
   else 
     {
	str = read_mini ("dired_search:", Null_String, LAST_SEARCH);
	!if ( strlen (str) ) error ("Specify search string");
	save_search_string (str);
     }
   
   do
     {
	(name, type) = dired_getfile ();
	if ( type == 1 ) 			% only search files
	  {	     
	     name = dircat (Dired_Current_Directory, name);
	     !if ( read_file (name) ) error ("Unable to read file.");
	     if ( dired_search_files () )
	       {
		  pop2buf (whatbuf ());
		  return;
	       }
	  }
     }
   while ( down_1 () );
}
#endif
