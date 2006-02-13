%  sccs.sl			-*- slang -*-
%  [stolen from rcs.sl]
%  [sccs.sl rev 0.6, by Phil Brown]
%
%  This file provides an interface to SCCS
%   Unfortunately, not as complete as the emacs version, but
%   its a start
%  I would really like to know how to show the in/out status of the
%  file on the status line, and also show it is controlled by SCCS,
%  like emacs does
%
%  RCS version Written by Guido Gonzato  <guido@ibogeo.df.unibo.it>
%  Modified by JED on 20 Nov 1999.
%  Modified by Phil Brown, phil@bolthole.com on Jun 1 2001.
%
%  The interface provides two functions that you care about:
%    sccs_open_file: -- open an SCCS file
%    sccs_check_in_and_out: -- check in or out an SCCS file
%
%  To use this facility, put this in your .jedrc:
%    require ("sccs.sl");
%
%  You may bind these functions in your .jedrc file similar to the following:
%
%    setkey ("sccs_check_in_and_out", "^S^C");
%    setkey ("sccs_open_file", "^S^F");
%

private variable Last_Comment = "";

% Build a file name like "/home/guido/SCCS/s.file"
private define build_sccs_filename (file)
{
   variable dir;
   (dir, file) = parse_filename (file);
   return dircat (dircat (dir, "SCCS"), strcat ("s.",file));
}

% probably .... cause I dont understand what it is doing
private define execute_sccs_cmd (cmd)
{
   variable cbuf, buf;
   
   buf = "*SCCS Message*";
   cbuf = whatbuf ();
   setbuf (buf);
   erase_buffer ();
   if (0 != run_shell_cmd (cmd))
     {
	pop2buf (buf);
	return -1;
     }
   bury_buffer (buf);
   setbuf (cbuf);
   return 0;
}


private define checkout (file)
{
   variable cmd;
   variable dir;
   variable name;
   
   flush (sprintf ("Checking out %s...", file));

   (dir, name) = parse_filename (file);

   cmd = sprintf ("cd %s; sccs edit %s 2>&1", dir, name);
   if (0 != execute_sccs_cmd (cmd))
     verror ("Error checking out %s!", file);

   flush (sprintf ("Checking out %s...done.", file));
}

private define checkin (file)
{
   variable dir, name, cmd;

   () = write_buffer (file);
   
   (dir, name) = parse_filename (file);
   Last_Comment = read_mini ("Enter a change comment:", "", Last_Comment);

   cmd = sprintf ("cd %s; echo \"%s\" | sccs delget %s > /dev/null 2>&1",
		  dir, Last_Comment, name);
   if (0 != execute_sccs_cmd (cmd))
     verror ("Error checking in %s!", file);

   set_readonly (1);
   flush ("Note: file is write protected.");
}

% calls "sccs create" on file. Note that this normally
% leaves a dropping in the form of ",filename"
private define sccs_create (file) 
{
   variable dir, name, cmd;

   () = write_buffer (file);

   flush (sprintf ("Creating %s...",file));

   (dir, name) = parse_filename (file);

   cmd = sprintf ("cd %s; sccs create %s > /dev/null 2>&1",
		  dir, name);
   if (0 != execute_sccs_cmd (cmd)) {
     verror ("Error doing initial sccs-create for %s!", file);
     return;
   }

   set_readonly (1);
   flush ("Note: file is write protected.");
}

% If it is checked out, check it it. 
% If it is not checked out, check it out
define sccs_check_in_and_out ()	%^S^C
{
   variable dir, file;

   % check if the current buffer is attached to an SCCS file.
   (file, dir,,) = getbuf_info();
   file = dircat (dir, file);

   % If "p-file" doesn't exist, then create the SCCS file
   % Otherwise check in/out normally.
   if (0 == file_status (build_sccs_filename (file)))
     {
	sccs_create (file);
	return;
     }

   % the SCCS file for current buffer exists;
   % if the buffer is read only, then check it out
   if (is_readonly ())
     {
	checkout (file);
	delbuf (whatbuf());
	() = find_file (file);
	return;
     }

   % Otherwise, check it in
   checkin (file);
}

% This checks out a file that we dont have loaded, but
% is under SCCS control. File must already exist, and be controlled
define sccs_open_file ()		% ^X^F
{
   variable sccs_file, file;

   file = read_file_from_mini ("SCCS open file:");

  % check whether the file exists; load it if it does
  % if not, try the SCCS version
   
   if (1 == file_status (file)) 
     {
	() = find_file (file);
	if(is_readonly()){
		sccs_check_in_and_out ();
	}
	return;
     }

   sccs_file = build_sccs_filename (file);

   % now check if this file exists
   if (0 == file_status (sccs_file)) {
     verror ("SCCS file %s not found.", sccs_file);
     return;
   }

   checkout (file);
   () = find_file (file);
}

% This function works on a file/buffer you already have loaded.
% --- End of file sccs.sl ---
