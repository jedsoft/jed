private variable Compressed_File_Exts
  = [".gz", ".Z", ".bz2"];

private variable Compress_File_Pgms
  = ["gzip",
     "uncompress",
     "bzip2"];

private variable Uncompress_File_Pgms
  = ["gzip -dc %s",
     "uncompress -c %s",
     "bzip2 -dc %s"
     ];

private variable Auto_Compression_Mode = 0;

private define check_is_compressed (file)
{
   variable ext = path_extname (file);
   
   variable i = where (ext == Compressed_File_Exts);

   if (length (i))
     return i[0];
   return -1;
}

private define _write_compressed_region (file, append)
{
   !if (blocal_var_exists ("Auto_Compression_Mode")
	or Auto_Compression_Mode)
     return 0;

   variable i = check_is_compressed (file);
     
   if (i == -1) return 0;

   variable cmd = sprintf ("%s > %s", Compress_File_Pgms[i], file);
   if (append)
     cmd = sprintf ("%s >> %s", Compress_File_Pgms[i], file);
   
   variable status = pipe_region (cmd);

   if (status != 0)
     verror ("%s returned %d", cmd, status);

   return 1;
}

private define write_compressed_region (file)
{
   return _write_compressed_region (file, 0);
}

private define append_compressed_region (file)
{
   return _write_compressed_region (file, 1);
}

private define insert_compressed_file (file)
{
   !if (Auto_Compression_Mode)
     return 0;

   variable i = check_is_compressed (file);
   if (i == -1)
     return 0;

   if (1 != file_status (file))
     return 0;

   variable cmd = sprintf (Uncompress_File_Pgms[i], file);

   () = run_shell_cmd (cmd);

   return 1;
}
   
private define read_compressed_file (file)
{
   if (insert_compressed_file (file))
     {
	create_blocal_var ("Auto_Compression_Mode");
	return 1;
     }
   return 0;
}


add_to_hook ("_jed_insert_file_hooks", &insert_compressed_file);
add_to_hook ("_jed_read_file_hooks", &read_compressed_file);
append_to_hook ("_jed_write_region_hooks", &write_compressed_region);
append_to_hook ("_jed_append_region_hooks", &append_compressed_region);

private define compressed_set_mode_hook (ext)
{
   variable i, file;

   !if (Auto_Compression_Mode)
     return 0;

   (file,,,) = getbuf_info ();
   i = check_is_compressed (file);
   if (i != -1)
     {
	file = file[[0:strlen(file)-strlen(ext)-2]];
	mode_hook (file_type (file));
	return 1;
     }
   return 0;
}
add_to_hook ("_jed_set_mode_hooks", &compressed_set_mode_hook);

%!%+
%\function{auto_compression_mode}
%\synopsis{Toggle auto-compression-mode}
%\usage{auto_compression_mode ([Int_Type state [,&prev_state]])}
%\description
% The \var{auto_compression_mode} function toggles the auto-compression-mode
% on or off. When on, files whose names end with \exmp{.gz}, \exmp{.Z}, or
% \exmp{.bz2} will automatically uncompressed when read in, and compressed 
% when written out.
%!%-
   
public define auto_compression_mode ()
{
   if (_NARGS)
     {
	if (_NARGS == 2)
	  {
	     variable prev_statep = ();
	     @prev_statep = Auto_Compression_Mode;
	  }
	Auto_Compression_Mode = ();
	return;
     }

   variable state = "OFF";

   Auto_Compression_Mode = not Auto_Compression_Mode;
   if (Auto_Compression_Mode)
     state = "ON";

   vmessage ("Auto Compression Mode: %s", state);
}
