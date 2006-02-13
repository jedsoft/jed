%  backups.sl                   -*- SLang -*-
%
%  This file provides numbered backups whenever a file is saved,
%  in a fashion similar to Emacs. For Jed 0.99.12 upwards.
%  Written by Guido Gonzato  <ggonza@tin.it>
%
%  Last updated: 5 December 2000
%
%  Usage:
%  Let's suppose that you edit a new file, foo.bar. If you save it, the
%  numbered backup copy foo.bar.~1~ will be created; saving again will
%  create foo.bar.~2~, and so on. The standard backup copy is unaffected.
%  Win32 users need to set the variable LFN=y to use backups.
%
%  To enable numbered backups, put this line in your .jedrc:
%  backups_on ();

private variable DO_BACKUPS = 1;

private define numbered_backups (buf)
{
   variable status, version;
   variable file;

   if (0 == DO_BACKUPS)
     return;

   % check whether an old copy or backup files exist

   version = 0;
   do {
      version++;
      file = sprintf ("%s.~%d~", buf, version);
   }
   while (1 == file_status (file));

   % mark the whole buffer and write it to file - don't use write_buffer ()

   push_spot ();
   mark_buffer ();
   () = write_region_to_file (sprintf ("%s.~%d~", buf, version));
   pop_spot();
}
%
define backups_off ()
{
  DO_BACKUPS = 0;
  flush ("Numbered backups disabled.");
}
%

define backups_on ()
{
  DO_BACKUPS = 1;
  flush ("Numbered backups enabled.");
}
%

add_to_hook ("_jed_save_buffer_before_hooks", &numbered_backups);

%
% --- End of file backups.sl ---

