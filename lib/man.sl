%%
%%  execute man then remove backspaces
%%

%!%+
%\function{man_clean_manpage}
%\synopsis{man_clean_manpage}
%\description
% remove _^H and ^H combinations and multiple blank lines (man page)
%!%-
define man_clean_manpage ()
{
   variable clean = "Cleaning man page...";
   push_spot_bob ();

   flush (clean);
   % I do not know what man on debian etch or lenny is doing, but it
   % is producing this combination: \xE2\x88\x{2212} for a -.  Note that
   % \xE2\x88 are the leading bytes of \u{2212}.  For some reason,
   % these are getting doubled.  So replace the entire combination
   replace ("\xE2\x88\x{2212}", "-");
   replace ("\xE2\x80\x{2010}", "-");

   % Convert other unicode versions of - to ascii -.
   replace ("\x{2212}", "-");
   replace ("\x{2010}", "-");
   replace ("\x{00AD}", "-");

   replace ("_\010", "");	% remove _^H underscores
   while ( fsearch ("\010") )
     {
	del ();
	del ();
     }				% remove overstrike
   
   % Now removed SGR sequences such as ESC[1m, ESC[m, etc....
   bob ();
   while (re_fsearch ("\e\\[\\d*m"))
     () = replace_match ("", 0);

   trim_buffer ();		% remove multiple blank lines
   pop_spot ();

   flush (strcat (clean, "done"));
}

%!%+
%\function{unix_man}
%\synopsis{unix_man}
%\description
% retrieve a man page entry and use clean_manpage to clean it up
%!%-
define unix_man ()
{
   variable subj, buf = "*manual-entry*", msg = "Getting man page...";

   subj = read_mini ("man", Null_String, Null_String);
   !if ( strlen (subj) ) return;

   pop2buf (buf);
   set_readonly (0);
   erase_buffer ();
   flush (msg);
#ifdef OS2
   () = run_shell_cmd (sprintf ("man %s 2> nul", subj));
#else
   () = run_shell_cmd (sprintf ("man %s 2> /dev/null", subj));
#endif
   man_clean_manpage ();
   bob ();
   set_buffer_modified_flag (0);
   most_mode ();
   set_readonly (1);
}

