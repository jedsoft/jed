% Complete the current word looking for similar word-beginnings
%
% Versions
%   1 May 1994       Adrian Savage (afs@jumper.mcc.ac.uk)
%              	     Extensively modified by JED
%   2.0 2003-05-01   rewrite by G.Milde <g.milde@web.de>
%        	     added support for scanning in a list of buffers
%   2.1 	     added customizability
%   2.2      	     look at last finding first
%   		     (as emacs does, tip P. Boekholt)
%   2.2.1    	     bugfix: invalid mark when buffer of last
%                    expansion killed (P. Boekholt)
%   2.3   2003-12-01 prevent flooding the undo-buffer (using getkey for
%                	   subsequent invocations)
%   2.3.1 2003-12-05 replaced unget_keystring with buffer_keystring
%   2.4   2004-03-15 dabbrev() takes a prefix argument for the
%                    buflist-scope (this is checked in dab_reset())
%                    clearer documentation (open_buffers -> all buffers)
%                    (hints by J. E. Davis)
%   2.4.1 2004-03-30 new custom var Dabbrev_Case_Search
%   	  	     added documentation for custom vars and get_buflist
%   3.0   2004-04-03 Changed much of the code to permit to allow for greater
%                    extensibility.
%   3.0.1 2004-06-07 Minor bug fixes (P. Boekholt)
%
%
% USAGE:
% Put in path und bind to a key, e.g.
% setkey("dabbrev", "^A");          % expand from Dabbrev_Default_Buflist
% setkey("dabbrev(get_buflist(1))", "\ea"); % expand from visible buffers
%
% You can use any function that returns a list of buffers as argument,
% make sure it is declared, e.g. with autoload("get_buflist", "dabbrev");
%
% You could even define your own metafunction that does something usefull
% (e.g. open a buffer) and then calls dabbrev("buf1\nbuf2\n ...") to expand
% from listed buffers.
%
% CUSTOMIZATION
%
% Some custom variables can be used to tune the behaviour of dabbrev:
% (The defaults are set to make dabbrev work as version 1)
%
% "Dabbrev_delete_tail", 0      % replace the existing completion
% "Dabbrev_Default_Buflist", 0  % default to whatbuf()
% "Dabbrev_Look_in_Folds", 1    % open folds when scanning for completions

% ---------------------------------------------------------------------------

% debug info, uncomment to trace down bugs
% _traceback = 1;
% _debug_info = 1;

% --- Variables

%
%!%+
%\variable{Dabbrev_delete_tail}
%\synopsis{Let completion replace word tail?}
%\usage{Int_Type Dabbrev_delete_tail = 0}
%\description
%  Should the completion replace the part of the word behind the cursor?
%\seealso{dabbrev}
%!%-
custom_variable("Dabbrev_delete_tail", 0);

%!%+
%\variable{Dabbrev_Default_Buflist}
%\synopsis{Which buffers should dabbrev expand from?}
%\usage{Int_Type Dabbrev_Default_Buflist = 0}
%\description
% The buffer-list when dabbrev is called without argument
%     0 = current buffer,
%     1 = visible buffers (including the current),
%     2 = all buffers of same mode,
%     3 = all buffers,
%     4 = other visible buffers (excluding the current),
%     5 = all other buffers of same mode  (excluding the current),
%     6 = all other buffers  (excluding the current)
%\seealso{dabbrev}
%!%-
custom_variable("Dabbrev_Default_Buflist", 0);

%!%+
%\variable{Dabbrev_Look_in_Folds}
%\synopsis{Scan folds for expansions}
%\usage{Int_Type Dabbrev_Look_in_Folds = 1}
%\description
% Should dabbrev scan folded parts of the source buffer(s)
% for expansions too?
%\seealso{dabbrev}
%!%-
custom_variable("Dabbrev_Look_in_Folds", 1);

%!%+
%\variable{Dabbrev_Case_Search}
%\synopsis{Let dabbrev stick to case}
%\usage{Int_Type Dabbrev_Case_Search = 1}
%\description
%  Should dabbrev consider the case of words when looking for expansions?
%  Will be overridden by a blocal variable "Dabbrev_Case_Search" or by the
%  mode-info variable "dabbrev_case_search".
%\seealso{dabbrev}
%!%-
custom_variable("Dabbrev_Case_Search", 0);

% --- Functions

private define get_buffer_mode_name (buf)
{
   setbuf (buf);
   return get_mode_name ();
}
   
private define get_buflist(scope)
{
   variable cbuf = whatbuf ();
   !if(scope)
     return cbuf;

   variable buffers = [buffer_list (), pop ()];

   if (scope > 3)
     {
	buffers = buffers [where (buffers != cbuf)];
	scope -= 3;
     }
   variable i;
   
   % prune hidden buffers, unless editing one
   if (cbuf[0] != ' ')
     {
	i = where (array_map (Int_Type, &strncmp, buffers, " ", 1));
	buffers = buffers[i];
     }

   switch (scope)
     {
      case 1:
	i = array_map (Int_Type, &buffer_visible, buffers);
     }
     {
      case 2:
	variable mode = get_mode_name ();
	i = (mode == array_map (String_Type, &get_buffer_mode_name, buffers));
	setbuf (cbuf);
     }
     {
      case 3:
	i = [1:length(buffers)];
     }
   buffers = buffers[where (i)];
   return strjoin (buffers, "\n");
}

% get the word tail
private define dab_get_word_tail(word_chars, kill)
{
   push_mark;
   skip_chars(word_chars);
   exchange_point_and_mark();
   if (kill)
     return bufsubstr_delete();
   else
     return bufsubstr();
}

private variable Dab_Context_Type = struct
{
   scan_mark, completion_list,
     patterns, pattern_index,
     buffer_list, buffer_list_index,
     match_methods, match_methods_index, 
     word_chars, search_dir,
     start_mark, completion, 
     prefix_mark,
     start_buffer,     % buffer being edited and where completion is to take place
};

% Switch to buf, mark position, widen if narrowed
% TODO: How about hidden lines?
private define enter_buffer (c, buf)
{
   c.start_mark = create_user_mark ();
   setbuf (buf);
   push_spot();
   if (count_narrows() and Dabbrev_Look_in_Folds)
     {
	push_narrow ();
	widen_buffer ();
     }
   if (c.scan_mark != NULL)
     goto_user_mark (c.scan_mark);
   else if (buf != c.start_buffer)
     %  start search at EOB, otherwise a completion at the current point in this buffer may
     %  be missed.
     eob ();			       
   
}

private define leave_buffer (c)
{
   pop_narrow ();
   pop_spot ();
   setbuf (user_mark_buffer (c.start_mark));
   goto_user_mark (c.start_mark);
}
private define dab_exact_match (prefix, word)
{
   if (strncmp (prefix, word, strlen (prefix)))
     return NULL;
   return word;
}

private define dab_uppercase_match (prefix, word)
{
   return strup (word);
}

private define dab_lowercase_match (prefix, word)
{
   return strlow (word);
}


private define create_completion_context () % (buflist = whatbuf())
{
   % List of buffers to scan for completions
   variable c = @Dab_Context_Type;
   variable buffer_list;
   
   c.start_buffer = whatbuf();

   if (_NARGS)
     {
	if (_NARGS > 1)
	  error ("Incorrect usage of dabbrev-- one argument expected");

	buffer_list = ();
	if (typeof (buffer_list) != String_Type)
	  buffer_list = get_buflist (buffer_list);
     }
   else
     {
	variable buflist_scope = prefix_argument(-1);
	if (buflist_scope == -1)
	  buflist_scope = Dabbrev_Default_Buflist;
	% buflist_scope = get_blocal("Dabbrev_Default_Buflist",
	% 		  	      Dabbrev_Default_Buflist;
	buffer_list = get_buflist(buflist_scope);
     }
   
   if (strlen (buffer_list) == 0)
     c.buffer_list = String_Type[0];
   else
     c.buffer_list = strchop (buffer_list, '\n', 0);

   c.buffer_list_index = 0;

   % get word_chars from: 1. mode_info, 2. blocal_var, 3. get_word_chars
   variable word_chars = mode_get_mode_info("dabbrev_word_chars");
   if (word_chars == NULL)
     {
	if (blocal_var_exists("Word_Chars"))
	  word_chars = get_blocal_var("Word_Chars");
	else
	  word_chars = "_" + get_word_chars();
     }
   c.word_chars = word_chars;

   % Get patterns to expand from (keep cursor position)
   push_mark ();
   bskip_chars ("^" + word_chars);
   variable tmp = create_user_mark ();
   bskip_chars (word_chars);
   c.prefix_mark = create_user_mark ();
   exchange_point_and_mark();
   variable pattern = bufsubstr ();
   if (tmp == c.prefix_mark)
     error("nothing to expand");
   c.patterns = [pattern];
   c.pattern_index = 0;

   c.completion_list = Assoc_Type[Int_Type];
   c.completion_list[""] = 1;

   c.match_methods = [&dab_exact_match];
   
   variable cs = NULL;
   if (blocal_var_exists("Dabbrev_Case_Search"))
     cs = get_blocal_var("Dabbrev_Case_Search");
   else
     cs = mode_get_mode_info ("dabbrev_case_search");
   
   if (cs == NULL)
     cs = Dabbrev_Case_Search;
   
   if (cs == 0)
     {
	if (strlow (pattern) == pattern)
	  c.match_methods = [c.match_methods, &dab_lowercase_match];
	if (strup (pattern) == pattern)
	  c.match_methods = [c.match_methods, &dab_uppercase_match];
     }

   c.match_methods_index = 0;
   c.search_dir = 0;
   return c;
}

  

private define dab_search (c, search_dir, match_method, pattern)
{
   variable cs = CASE_SEARCH;
   EXIT_BLOCK
     {
	CASE_SEARCH = cs;
     }
   CASE_SEARCH = 0;
   variable found, word_chars = c.word_chars, prefix_mark = c.prefix_mark;
   
   forever 
     {
	do
	  {
	     if (search_dir == 1)
	       {
		  go_right(1);
		  found = fsearch (pattern);
	       }
	     else 
	       found = bsearch (pattern);

	     !if (found)
	       return NULL;
	     
	     % test whether at begin of a word
	     push_spot();
	     bskip_chars(word_chars);

	     variable m = create_user_mark ();
	     pop_spot();
	  }
	while ((m != create_user_mark ()) or (m == prefix_mark));
	
	variable len = strlen (pattern);
	push_spot ();
	push_mark ();
	go_right (len);
	%skip_chars ("^" + word_chars);
	skip_chars (word_chars);
	variable word = bufsubstr ();
	pop_spot ();
	
	word = (@match_method) (pattern, word);

	if (word != NULL)
	  return substr (word, len+1, -1);
     }
}

private define is_completion_ok (c, completion)
{
   if (assoc_key_exists (c.completion_list, completion))
     return 0;

   c.completion_list[completion] = 1;
   return 1;
}

private define dab_process_buffer (c, buf, pattern)
{
   enter_buffer (c, buf);	       %  spot pushed
   EXIT_BLOCK
     {
	leave_buffer (c);
     }
   
   variable dir = c.search_dir;
   while (dir < 2)
     {
	variable match_methods_index = c.match_methods_index;
	variable match_methods_index_max = length (c.match_methods);
	
	while (match_methods_index < match_methods_index_max)
	  {
	     variable match_method = c.match_methods[match_methods_index];
	     variable completion = dab_search (c, dir, match_method, pattern);

	     if (completion == NULL)
	       {
		  goto_spot ();   %  go back and start the search over
		  match_methods_index++;
		  continue;
	       }

	     if (is_completion_ok (c, completion))
	       {
		  c.match_methods_index = match_methods_index;
		  c.scan_mark = create_user_mark ();
		  c.search_dir = dir;
		  return completion;
	       }
	  }
	c.match_methods_index = 0;
	dir++;
     }
   c.search_dir = 0;
   c.scan_mark = NULL;
   return NULL;
}


private define dab_expand (c)
{
   variable completion;
   while (c.pattern_index < length (c.patterns))
     {
	variable pattern = c.patterns[c.pattern_index];
	variable buffer_list_index = c.buffer_list_index;
	variable buffer_list_index_max = length (c.buffer_list);
	while (buffer_list_index < buffer_list_index_max)
	  {
	     completion = dab_process_buffer (c, c.buffer_list[buffer_list_index], pattern);
	     if (completion != NULL)
	       {
		  c.buffer_list_index = buffer_list_index;
		  return completion;
	       }
	     buffer_list_index++;
	  }
	c.buffer_list_index = 0;
	c.pattern_index++;
     }
   c.pattern_index = 0;

   vmessage("No more completions for \"%s\" in [%s]",
	    strjoin(c.patterns, ","), strjoin(c.buffer_list, ","));
   return NULL;
}

% ----- main function --------------------------------------------------

%!%+
%\function{dabbrev}
%\synopsis{Complete the current word looking for similar words}
%\usage{dabbrev([optional_argument])}
%\description
% Takes the current stem (part of word before the cursor)
% and scans the current buffer for words that begin with this stem.
% The current word is expanded by the non-stem part of the finding. 
% Subsequent calls to dabbrev replace the last completion with the next
% guess.
%
% The search for completions takes place over a list of buffers specified 
% by the \var{Dabbrev_Default_Buflist} variable unless \var{dabbrev} has
% been called with an argument.  The optional argument may either be an 
% integer whose value is interpreted as for \var{Dabbrev_Default_Buflist}, 
% or a string containing a newline separated list of buffer names to search.
%
% The scan proceeds as follows:
%#v+
%     foreach buffer in buflist
%       from cursor backwards to the beginning of the buffer
%       from cursor forwards to the end of the buffer
%#v-
%\example
% The current buffer contains the line
%#v+
%   foo is better than foobar, foobase or foo
%#v-
% with the cursor at the end of the line.
% dabbrev completes foo with foobase.
% If called again (immediately) foobase is changed to foobar
% If called once again, foobase is changed to foo and a message is
% given: No more completions.
%
%\notes
%  You can use the optional argument to have keybindings to different
%  "flavours" of dabbrev.
%#v+
% setkey("dabbrev", "^A");                 % expand from Dabbrev_Default_Buflist
% setkey("dabbrev(1)", "\ea");             % expand from visible buffers
% setkey("dabbrev(\"wordlist\")","\e^A");  % expand from the buffer "wordlist"
%#v-
%
%\seealso{Dabbrev_Default_Buflist, Dabbrev_Look_in_Folds}
%!%-

private variable Completion_Context = NULL;
private variable Completion = NULL;
private define before_key_hook ();
private define before_key_hook (fun)
{
   if (typeof (fun) == Ref_Type)
     fun = "&";

   if ((0 == is_substr (fun, "dabbrev"))
       or (Completion == NULL))
     {
	remove_from_hook ("_jed_before_key_hooks", &before_key_hook);
	Completion_Context = NULL;
	return;
     }
   push_mark();
   go_left(strlen (Completion));
   del_region();
}

   
public define dabbrev()  %(buflist=whatbuf())
{
   variable type, fun, key, args = __pop_args(_NARGS);

   add_to_hook ("_jed_before_key_hooks", &before_key_hook);

   if (Completion_Context == NULL)
     Completion_Context = create_completion_context (__push_args (args));
   
   Completion = dab_expand (Completion_Context);

   if (Completion != NULL)
     insert (Completion);
}
