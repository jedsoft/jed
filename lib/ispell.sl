%%
%%  Ispell interface
%%

%!%+
%\variable{Ispell_Program_Name}
%\synopsis{spell-check program name}
%\usage{variable Ispell_Program_Name = ""}
%\description
% The spell check command used by the \sfun{ispell} function. It must
% be ispell-compatible (one of "ispell", "aspell" or "hunspell").  If
% unset, the ispell program will be auto-detected by searching the
% path for one of the above programs.
%\seealso{ispell, search_path_for_file}
%!%-
custom_variable("Ispell_Program_Name", NULL);

% Search for candidates:
if ((Ispell_Program_Name == NULL) || (Ispell_Program_Name == ""))
{
   Ispell_Program_Name = "ispell";

   foreach $1 (["aspell", "hunspell", "ispell"])
     {
	if (NULL != search_path_for_file(getenv("PATH"), $1))
	  {
	     Ispell_Program_Name = $1;
	     break;
	  }
     }
   Ispell_Program_Name += " -a";
}

define ispell ()
{
   variable ibuf, buf, file, letters, num_win, old_buf;
   variable word, cmd, p, num, n, new_word;
   
#ifdef OS2   
   file = make_tmp_file("jedt");
#else   
   file = make_tmp_file("/tmp/jed_ispell");
#endif   
   letters = "\a"R;
   
   ibuf = " *ispell*";
   buf = whatbuf();
   
   skip_chars(letters); bskip_chars(letters); push_mark(); % push_mark();

   n = _get_point ();
   skip_chars(letters);
   if (_get_point () == n)
     {
	pop_mark_0 (); %pop_mark_0 ();
	return;
     }
   
   %word = bufsubstr();
#ifdef MSDOS MSWINDOWS WIN32
   () = system(sprintf("echo %s | %s > %s", 
		       bufsubstr(), Ispell_Program_Name, file));
#else
   if (pipe_region(sprintf ("%s > '%s'", Ispell_Program_Name, file)))
       error ("ispell process returned a non-zero exit status.");
#endif
   
   setbuf(ibuf); erase_buffer();
   () = insert_file(file);
   () = delete_file(file);
   
   %%
   %% parse output
   %%
   bob();
   if (looking_at_char('@'))   % ispell header
     {
	del_through_eol ();
     }
   
   if (looking_at_char('*') or looking_at_char('+'))
     {
	message ("Correct");   % '+' ==> is derived from
	bury_buffer (ibuf);
	return;
     }
   
   if (looking_at_char('#')) 
     {
      	bury_buffer (ibuf);
	return (message("No clue."));
     }

   del(); trim(); eol_trim(); bol();
   if (ffind_char (':'))
     {
	skip_chars(":\t ");
	push_mark();
	bol();
	del_region();
     }

   insert ("(0) ");
   n = 1;
   while (ffind_char (','))
     {
	del ();
	trim(); newline();
	vinsert ("(%d) ", n);
	++n;
     } 
   
   bob();
   num_win = nwindows();
   pop2buf(buf);
   old_buf = pop2buf_whatbuf(ibuf);   

   set_buffer_modified_flag(0);
   variable ok = 0;
   try
     {
	num = read_mini("Enter choice. (^G to abort)", "0", "");
	if (0 == fsearch(sprintf ("(%s)", num)))
	  throw RunTimeError, "$num is an invalid choice"$;

	() = ffind_char (' '); trim();
	push_mark_eol(); trim(); new_word = bufsubstr();
	set_buffer_modified_flag(0);
	sw2buf(old_buf);
	pop2buf(buf);
	ok = 1;
	bskip_chars(letters); push_mark();
	skip_chars(letters); del_region();
	insert(new_word);
     }
   finally
     {
	if (ok == 0)
	  {
	     sw2buf(old_buf);
	     pop2buf(buf);
	  }
	if (num_win == 1) onewindow();
	bury_buffer(ibuf);
     }
}

