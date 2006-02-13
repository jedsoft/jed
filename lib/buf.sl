%%   Buffer routines for Jed.  Functions included here are:
%%    
%%     save_buffers  : saves buffers that are associated with a file
%%                     with no user intervention
%%     recover_file  : restore buffer from autosave file.
%%
%%



%!%+
%\function{save_buffers}
%\synopsis{save_buffers}
%\usage{Void save_buffers ();}
%\description
% Save all modified buffers that are associated with a file without
% user intervention.
%!%-
define save_buffers ()
{
   variable file, dir, flags, buf, ch;
   
   loop (buffer_list())
     { 
	buf = ();
	ch = int(buf);
	if ((ch == 32) or (ch == '*')) continue;  %% internal buffer or special
      
	(file, dir,, flags) = getbuf_info (buf);
      
	!if (strlen(file)) continue;        %% no file assciated with it
	if (flags & 1)
	  {
	     setbuf(buf);
	     () = write_buffer(dircat (dir, file));
	  }
     }  
}

      
     
%% write region to file
define write_region()
{
   variable file;
   !if (markp) error("Set Mark first!");
   file = read_file_from_mini("File:");
   write_region_to_file(file);
}


define append_region ()
{
   variable file;
   !if (markp) error("Set Mark first!");
   file = read_file_from_mini("Append to File:");
   if (-1 == append_region_to_file(file)) error ("Append failed.");
}

;%% restores buffer from autosave file.
define recover_file ()
{
   variable flags, file, dir, as, buf;
   
   (file, dir,, flags) = getbuf_info();
   !if (strlen(file)) error("Buffer not associated with a file.");
   as = make_autosave_filename (dir, file);
   if (file_status(as) != 1)
    {
       error (as + " not readable.");
    }
    
   buf = whatbuf();
   as;
   if (file_time_compare(as, dircat (dir, file)))
     {
        " more recent. Use it";
     }
   else " not recent. Use it";
   
   if (get_yes_no(() + ()) > 0)
     {
	what_line();
	setbuf(buf);
	erase_buffer();
	() = insert_file(as);
	goto_line();
     } 
}
	   
  
%!%+
%\function{next_buffer}
%\synopsis{Cycle through the list of buffers}
%\usage{Void next_buffer ()}
%\description
%   Switches to the next in the list of buffers.
%\notes
%   (This is the same function as mouse_next_buffer in mouse.sl)
%\seealso{buffer_list, list_buffers}
%!%-
public define next_buffer ()
{
   variable n, buf, cbuf = whatbuf ();

   n = buffer_list ();		       %/* buffers on stack */
   loop (n)
     {
	buf = ();
	n--;
	if (buf[0] == ' ') % hidden buffers like " <mini>"
	  continue;
	sw2buf (buf);
	_pop_n (n);
	return;
     }
}

%{{{ save_buffer_as(force_overwrite = 0)
%!%+
%\function{save_buffer_as}
%\synopsis{Save the buffer to a different file/directory}
%\usage{Void save_buffer_as(force_overwrite=0)}
%\description
%   Asks for a new filename and saves the buffer under this name.
%   Asks before overwriting an existing file, if not called with
%   force_overwrite=1. 
%   Sets readonly flag to 0, becouse if we are able to write, 
%   we can also modify.
%\seealso{save_buffer, write_buffer}
%!%-
define save_buffer_as ()
{
   variable force_overwrite = 0;
   if (_NARGS)
     force_overwrite = ();
   
   variable file = read_file_from_mini(sprintf("Save %s to:", whatbuf()));
   !if (strlen(file))
     return;

   if (file_status(file) == 2) % directory
     file = path_concat (file, extract_element(whatbuf(), 0, ' '));
   
   if ((force_overwrite == 0)
       and (1 == file_status (file)))
     {
	if (1 != get_y_or_n (sprintf ("File \"%s\" exists, overwrite?", file)))
	  return;
     }
   () = write_buffer(file);
   set_readonly(0);       % if we are able to write, we can also modify
}
%}}}

