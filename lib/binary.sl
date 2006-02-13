% It is used to read a file in binary mode.

define find_binary_file ()
{   
   variable file, bytes, len, pos;
   
   file = read_file_from_mini ("Find Binary File:");
   ERROR_BLOCK
     {
	set_file_translation (0);
     }
   set_file_translation (1);
   () = find_file (file);
   EXECUTE_ERROR_BLOCK;
   no_mode();
   set_overwrite(1);
   %
   %  set the binary file flags
   getbuf_info();
   setbuf_info(() | 0x200);
   
   bytes = count_chars();
   if (string_match(bytes, ".*of \\([0-9]+\\)$", 1))
     {
	(pos, len) = string_match_nth(1);
	bytes = substr(bytes, pos + 1, len);
	message (bytes + " bytes.");
     }
}
