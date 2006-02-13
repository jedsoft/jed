%%
%%  Ispell interface
%%


define ispell()
{
   variable ibuf, buf, file, letters, num_win, old_buf;
   variable word, cmd, p, num, n, new_word;
   
#ifdef OS2   
   file = make_tmp_file("jedt");
#else   
   file = make_tmp_file("/tmp/jed_ispell");
#endif   
   letters = "a-zA-Z";
   
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
   () = system(sprintf("echo %s | ispell -a > %s", bufsubstr(), file));
#else
   if (pipe_region(strcat("ispell -a > ", file)))
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
   while (ffind_char (' '))
     {
	go_left_1 ();
	if (looking_at_char(',')) del(); else go_right_1 ();
	trim(); newline();
	vinsert ("(%d) ", n);
	++n;
     } 
   
   bob();
   num_win = nwindows();
   pop2buf(buf);
   old_buf = pop2buf_whatbuf(ibuf);   
   
   ERROR_BLOCK 
     {
	sw2buf(old_buf);
	pop2buf(buf);
	if (num_win == 1) onewindow();
	bury_buffer(ibuf);
     }

   set_buffer_modified_flag(0);
   num = read_mini("Enter choice. (^G to abort)", "0", Null_String);
   num = sprintf ("(%s)", num);
   
   if (fsearch(num))
     {
	() = ffind_char (' '); trim();
	push_mark_eol(); trim(); new_word = bufsubstr();
	set_buffer_modified_flag(0);
	sw2buf(old_buf);
	pop2buf(buf);
	bskip_chars(letters); push_mark();
	skip_chars(letters); del_region();
	insert(new_word);
     }
   else 
     {
	sw2buf(old_buf);
	pop2buf(buf);
     }
   if (num_win == 1) onewindow();
   bury_buffer(ibuf);
}

