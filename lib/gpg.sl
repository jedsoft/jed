autoload ("open_filter_process", "syncproc");
autoload ("close_filter_process", "syncproc");

custom_variable ("GPG_Encrypt_Program", "gpg -c --batch --quiet -o - --passphrase-fd 0");
custom_variable ("GPG_Decrypt_Program", "gpg --decrypt --batch -o - --passphrase-fd 0");

private variable Passphrases = Assoc_Type[String_Type, ""];

private define check_is_encrypted (file)
{
   variable ext = path_extname (file);

   if ((ext == ".gpg") or (ext == ".gpg#"))
     return 0;
   
   return -1;
}

private define read_mini_hidden (prompt, default)
{

   if (default != NULL)
     {
	prompt += sprintf (" [** buffer default **]", default);
     }

   prompt += ":";

   flush (prompt);

   variable s = "";
   forever
     {
	variable ch = getkey ();
	if (ch == '\r')
	  break;
	s += char (ch);
     }
   
   if ((s == "") and (default != NULL))
     s = default;
   
   return s;
}

private define get_pass_phrase (path, use_blocal_phrase, set_blocal_phrase, confirm_phrase)
{
   variable default_pass_phrase = NULL;
   variable file = path_basename (path);

   % if it's an autosave file, try to get the passphrase from the Passphrases assoc
   if (file[-1] == '#')
     {
	variable f  = path_concat(path_dirname(path), strtrim(file, "#"));
	if (assoc_key_exists (Passphrases, f))
	  return Passphrases[f];
     }

   if (use_blocal_phrase or set_blocal_phrase
       and (0 == blocal_var_exists ("_gpg_pass_phrase")))
     create_blocal_var ("_gpg_pass_phrase");

   if (use_blocal_phrase)
     default_pass_phrase = get_blocal_var ("_gpg_pass_phrase");

   forever
     {
	variable p = read_mini_hidden (sprintf ("Passphrase for %s", file), 
				       default_pass_phrase);
	if (p == "")
	  return NULL;
   
	
	if ((default_pass_phrase == NULL) and confirm_phrase)
	  {
	     if (p != read_mini_hidden ("Confirm Passphrase", NULL))
	       {
		  flush ("Confirmation failed.  Try again");
		  sleep (1);
		  continue;
	       }
	  }
	
	if (set_blocal_phrase and (p != default_pass_phrase))
	  {
	     if (1 == get_y_or_n ("Save passphrase as buffer-default"))
	       set_blocal_var (p, "_gpg_pass_phrase");
	  }

	if (file[-1] != '#')
	  Passphrases[path] = p;

	return p;
     }
}

private define _write_encrypted_region (file, append)
{
   if (append)
     return 0;

   variable i = check_is_encrypted (file);
   if (i == -1) return 0;
   
   variable p = get_pass_phrase (file, 1, 1, 1);
   if (p == "")
     return 0;

   variable txt = bufsubstr ();

   variable cmd = sprintf ("%s > %s", GPG_Encrypt_Program, file);
   variable fp = popen (cmd, "w");
   if (fp == NULL)
     verror ("%s failed", cmd);
   
   if (orelse 
       {(-1 == fputs (p + "\n", fp))}
       {(-1 == fputs (txt, fp))}
	 {(0 != pclose (fp))})
     verror ("write to %s failed", cmd);
   
   return 1;
}

private define write_encrypted_region (file)
{
   return _write_encrypted_region (file, 0);
}

private define parse_gpg_errors (file)
{
   variable st = stat_file (file);
   if (st == NULL)
     return;
   if (0 == st.st_size)
     return;
   
   % For now, just insert the contents into a separate buffer.
   variable cbuf = whatbuf ();
   setbuf ("*gpg-errors*");
   erase_buffer ();
   () = insert_file (file);
   % TODO: warnings such as these can probably be ignored.
   % gpg: CAST5 encrypted data
   % gpg: encrypted with 1 passphrase
   % gpg: WARNING: message was not integrity protected

   setbuf (cbuf);
}
   
private define _insert_encrypted_file (file, use_blocal_phrase, set_blocal_phrase, confirm_phrase)
{
   variable i = check_is_encrypted (file);
   if (i == -1)
     return 0;

   if (1 != file_status (file))
     return 0;

   variable stderr_filename = make_tmp_file ("gpgerr");
   () = fopen (stderr_filename, "w");
   () = chmod (stderr_filename, 0600);

   variable cmd = sprintf ("%s %s 2>%s", GPG_Decrypt_Program, file, stderr_filename);
   variable pid = open_filter_process (["/bin/sh", "-c", cmd], ".");
   send_process (pid, get_pass_phrase (file, use_blocal_phrase, set_blocal_phrase, confirm_phrase));
   send_process (pid, "\n");

   () = close_filter_process (pid);
   parse_gpg_errors (stderr_filename);
   () = delete_file (stderr_filename);

   return 1;
}

private define insert_encrypted_file (file)
{
   return _insert_encrypted_file (file, 0, 0, 0);
}


private define read_encrypted_file (file)
{
   if (_insert_encrypted_file (file, 0, 1, 0))
     {
	%create_blocal_var ("Auto_Compression_Mode");
	return 1;
     }
   return 0;
}


add_to_hook ("_jed_insert_file_hooks", &insert_encrypted_file);
add_to_hook ("_jed_read_file_hooks", &read_encrypted_file);
append_to_hook ("_jed_write_region_hooks", &write_encrypted_region);
%append_to_hook ("_jed_append_region_hooks", &append_e);

private define encrypted_set_mode_hook (ext)
{
   variable i, file;

   (file,,,) = getbuf_info ();
   i = check_is_encrypted (file);
   if (i != -1)
     {
	file = file[[0:strlen(file)-strlen(ext)-2]];
	mode_hook (file_type (file));
	return 1;
     }
   return 0;
}
add_to_hook ("_jed_set_mode_hooks", &encrypted_set_mode_hook);
