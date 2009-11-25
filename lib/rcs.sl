% File:          rcs.sl      -*- SLang -*-
%
% Author:        Guido Gonzato, <ggonza@tin.it>. Contributions by JED.
% 
% Version:       1.0.1. This file provides an interface to RCS a la
%                Emacs (sort of).
%
% Installation:  unless rcs.sl is already loaded from site.sl (check), 
%                insert this line in your .jedrc:
%  
%                  () = evalfile ("rcs.sl");
%
%                or, better, insert autoload lines in your defaults.sl
%                like this:
%  
%                  autoload ("rcs_open_file", "rcs.sl");
%                  autoload ("rcs_check_in_and_out", "rcs.sl");
%                  autoload ("rcs_read_log", "rcs.sl");
% 
% Usage:         rcs_open_file ()         -- open an RCS file
%                rcs_check_in_and_out ()  -- check in/out an RCS file
%                rcs_read_log ()          -- read the change history
% 
%                you might want to set these key bindings in your .jedrc:
% 
%                  setkey_reserved ("rcs_open_file",        "vf");
%                  setkey_reserved ("rcs_check_in_and_out", "vv");
%                  setkey_reserved ("rcs_read_log",         "vl");
%
%
% Last updated:	23 February 2001

%_debug_info = 1;
private variable Last_Comment = "";

% Build a file name like "/home/guido/RCS/file.txt,v"
private define build_rcs_filename (file)
{
  variable dir;
  (dir, file) = parse_filename (file);
  return dircat (dircat (dir, "RCS"), strcat (file, ",v"));
}

private define checkout (file)
{
  variable cmd, dir, name;
  
  flush (sprintf ("Checking out %s...", file));

  (dir, name) = parse_filename (file);

  cmd = sprintf ("cd %s; co -l %s > /dev/null 2>&1", dir, name);
  if (0 != system (cmd))
    verror ("Error checking out %s!", file);

  flush (sprintf ("Checking out %s...done.", file));
}

private define checkin (file, msg)
{
  variable dir, name, cmd;

  () = write_buffer (file);
  (dir, name) = parse_filename (file);
  Last_Comment = read_mini (msg, "", Last_Comment);
  cmd = sprintf ("cd %s; echo \"%s\" | ci %s > /dev/null 2>&1",
                  dir, Last_Comment, name);
  if (0 != system (cmd))
    verror ("Error checking in %s!", file);

  set_readonly (1);
  setbuf_info (getbuf_info () | 0x8);
  flush ("Note: file is write protected.");
}

define rcs_open_file ()         % Emacs uses ^X-v-f
{
  variable rcs_file, dir, file;

  file = read_file_from_mini ("RCS open file:");
  
  file = file [[:-3]]; % remove ",v"
  (dir, file) = parse_filename (file);
  file = dircat (dir [[:-5]], file); % remove "RCS/"
  checkout (file);
  () = find_file (file);
}

define rcs_check_in_and_out ()  % Emacs uses ^X-v-v
{
  variable file, dir, flags;

  % check if the current buffer is attached to an RCS file.
  (file, dir,, flags) = getbuf_info();
  file = dircat (dir, file);

  % if it doesn't exist, then create the RCS dir, check in, and exit
  if (0 == file_status (build_rcs_filename (file))) {
    dir = dircat (dir, "RCS");
    if (0 == file_status (dir)) {
      if (0 != mkdir (dir, 0777))
        verror ("Error creating RCS directory %s!", dir);
    }
    checkin (file, "RCS file description:");
    return;
  }
  
  % the RCS file exists; if the buffer is read only, then check it out
  if (flags & (1 shl 3)) { %  readonly
    checkout (file);
    delbuf (whatbuf());
    () = find_file (file);
    return;
  }
  
  % Otherwise, check it in
  checkin (file, "Enter a change comment:");
}

private variable rlog_buf = "*rlog*";

define close_rlog_buffer ()
{
  variable cbuf = whatbuf ();
  setbuf (rlog_buf);
  set_buffer_modified_flag (0);
  setbuf (cbuf);
  delbuf (rlog_buf);
}

define rcs_read_log ()
{
  variable rcs_file, dir, file, cmd, tmp_file;
  variable rlog_map= "rlog_map";

  file = read_file_from_mini ("rlog of RCS file:");
  file = file [[:-3]]; % remove ",v"
  (dir, file) = parse_filename (file);
  file = dircat (dir [[:-5]], file); % remove "RCS/"

  tmp_file = make_tmp_file ("/tmp/jedrlog");
  cmd = sprintf ("rlog %s > %s", file, tmp_file); % exec rlog
  if (0 != system (cmd)) 
    verror ("Error rlogging %s!", file);

  sw2buf (rlog_buf);
  insert_file (tmp_file);
  delete_file (tmp_file);
  most_mode ();
  !if (keymap_p (rlog_map)) {
    make_keymap (rlog_map);
    definekey ("close_rlog_buffer", "q", rlog_map);
  }
  use_keymap (rlog_map);
  set_readonly (1);
}

provide ("rcs");
% --- End of file rcs.sl ---
