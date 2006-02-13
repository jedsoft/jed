

!if (is_defined ("Abbrev_File"))
{
   variable Abbrev_File;
   $1 = getenv ("JED_HOME");
   if ($1 == NULL)
     {
	$1 = getenv ("HOME");
	if ($1 == NULL) $1 = "";
     }
   
#ifdef VMS
   if (0 == strlen ($1))
     {
	Abbrev_File = "SYS$LOGIN:abbrevs.sl";
     }
   else Abbrev_File = dircat ($1, Abbrev_File);
#else
   
#ifdef UNIX
   Abbrev_File = ".abbrevs.sl";
#else
   Abbrev_File = "abbrevs.sl";
#endif
   Abbrev_File = dircat ($1, Abbrev_File);
#endif % VMS 
}

if (file_status (Abbrev_File) > 0) pop (evalfile (Abbrev_File));

define set_abbrev_mode (val)
{
   if (val)
     _set_buffer_flag (0x800);
   else
     _unset_buffer_flag (0x800);
}

define abbrev_mode ()
{
   _toggle_buffer_flag (0x800);

   if (_test_buffer_flag (0x800))
     message ("Abbrev mode ON");
   else
     message ("Abbrev mode OFF");
}


provide ("abbrev");
