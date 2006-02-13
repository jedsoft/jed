#ifdef XWINDOWS
custom_variable ("XTerm_Pgm", "xterm");
define _jed_run_program_hook (s)
{
   s = strtrim_end (s, " \t&");
   return system (sprintf ("%s -e %s &", XTerm_Pgm, s));
}
#endif

#ifdef WIN32
custom_variable("W32shell_Perform_Globbing", 0);
define _win32_get_helper_app_name ()
{
   variable s, h;

   s = getenv ("COMSPEC");
   if (s == NULL)
     s = "cmd.exe";
   if (W32shell_Perform_Globbing)
     h = dircat (Jed_Bin_Dir, "w32/g32shell.exe");
   else
     h = dircat (Jed_Bin_Dir, "w32/w32shell.exe");

   sprintf ("%s %s /c", h, s);
}
#endif

public define vrun_program ()
{
   variable args = __pop_args (_NARGS);
   return run_program (sprintf (__push_args (args)));
}


