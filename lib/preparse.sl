variable Preprocess_Only = 1;
() = evalfile ("bytecomp.sl");

#ifdef HAS_DFA_SYNTAX
define preparse_enable_highlight_cache (file, name)
{
   create_syntax_table (name);

   % Make sure existing cached files are removed
   forever
     {
	variable dirfile = search_path_for_file (Jed_Highlight_Cache_Path, file, ',');
	if (dirfile == NULL)
	  break;

	if (-1 == remove (dirfile))
	  break;
     }
   dirfile = dircat (Jed_Highlight_Cache_Dir, file);
   () = remove (dirfile);

   _dfa_enable_highlight_cache (dirfile, name);
}

private define create_dfa_cache (file)
{
   file = expand_jedlib_file (file);
   !if (strlen (file))
     return;
   setbuf ("*dfa-cache*");
   erase_buffer ();
   if (-1 == insert_file_region (file, 
				 "%%% DFA_CACHE_BEGIN %%%",
				 "%%% DFA_CACHE_END %%%"))
     return;
   
   bob ();
   !if (fsearch ("dfa_enable_highlight_cache"))
     return;
   
   replace ("dfa_enable_highlight_cache", "preparse_enable_highlight_cache");
   eob ();
   !if (re_bsearch ("[ \t]*dfa_set_init_callback[ \t]*([ \t]*&[ \t]*\\([^,]+\\),[ \t]*\"\\([^\"]+\\)\""))
     return;
   
   variable fun = regexp_nth_match (1);
   variable mode = regexp_nth_match (2);
   delete_line ();
   % Note that $1 has been unitialized because the use of $1 here is not
   % supported.
   vinsert ("()=__tmp($1); %s(\"%s\");", fun, mode);

   evalbuffer ();
}

flush ("creating DFA syntax tables...");

% List of modes for which dfa cache tables should be constructed
foreach ([
	  "cmode.sl",
	  "html.sl",
	  "javamode.sl",
	  "perl.sl",
	  "php.sl",
	  "preparse.sl",
	  "pscript.sl",
	  "pymode.sl",
	  "shmode.sl",
	  "slmode.sl",
	  "tclmode.sl",
	  "texcom.sl",
	  "tpascal.sl"
	  ])
{
   create_dfa_cache ();
}
#endif

exit_jed();
