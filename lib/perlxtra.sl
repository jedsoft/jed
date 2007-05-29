% this -*- SLang -*- file defines extra routines that can also be used to
% run or format snippets of Perl code that exist outside of 'perl_mode'
%
% ------------------------------------------------------------------------
% Changes:
% 2002-11-19  Mark Olesen <mark dot olesen at gmx dot net>
% - perl_info and perldoc simplified
%
% 2002-08-29  Mark Olesen <mark dot olesen at gmx dot net>
% - split off from perl.sl
% - perl_exec / perl_check / perltidy now respect narrowed buffers
%
% 2006-08-27  Mark Olesen <mark dot olesen at gmx dot net>
% - cosmetics
%
% 2006-11-11 JED
% - Added provide statement.
%
% 2007-03-21 Mark Olesen
% - save modified file before running perltidy/perl_exec
%
% ------------------------------------------------------------------------
%{{{ default values for Perl custom variables
% override these default values in ~/.jedrc

%!%+
%\variable{Perl_Flags}
%\synopsis{Perl_Flags}
%\usage{String Perl_Flags = "-w";}
%\description
% Extra (or 'expert') command-line options (switches) for running Perl.
% eg, \var{'-I'} or \var{'-M'}.
% You only need these if you know why you need these.
%
% Warnings are *always* enabled, regardless of what you try here.
% If your code doesn't run with \var{'-w'}, re-write it so it does
% or you're an expert and know which sections of code should have
% warnings disabled anyhow.
%!%-
custom_variable("Perl_Flags", Null_String);

%!%+
%\variable{Perl_Indent}
%\synopsis{Perl_Indent}
%\usage{Integer Perl_Indent = 4;}
%\description
% This value determines the number of columns the current line is indented
% past the previous line containing an opening \exmp{'\{'} character.
% eg.,
%#v+
%  if (test) {
%      statement();
%  }
%  else {
%      statement();
%  }
%#v-
%
% The default value (4) corresponds to the default for \var{perltidy}
%
%\seealso{C_INDENT, Perl_Continued_Offset}
%!%-
custom_variable("Perl_Indent", 4);

%}}}

static variable
  tmp_input    = "_tmp_jedperl_",
  shell_output = "*shell-output*",
  help_buf     = "*help-perl*";         % interact with perldoc information

%-------------------------------------------------------------------------
%!%+
%\function{perltidy}
%\synopsis{Void perltidy (Void)}
%\description
% Runs the \var{perltidy} program on a region, buffer or narrowed buffer.
% The \var{perltidy} program (available from \var{http://perltidy.sourceforge.net})
% must be installed for this to work!
%
% With a numerical prefix argument (eg Ctrl-U), prompts for extra flags
% for \var{perltidy}.
%
% The following style preferences settings in \var{~/.perltidyrc} seem to
% give good results:
%#v+
%   -et=8       # standard tabs
%   -nola       # no outdent labels
%   -wba="."    # break after string concatenation
%   -se         # errors to standard error output
%   -sbt=2      # very tight square brackets
%#v-
%\seealso{perl_indent_region, perl_indent_buffer, perl_mode}
%!%-
define perltidy ()      % <AUTOLOAD> this function
{
    variable cmd  = "perltidy -st -q";  % command plus invariant flags
    variable line = what_line();        % we'll try to return here later
    variable opts = "-nola";            % optional flags

    variable file, dir, thisbuf, flags;
    (file, dir, thisbuf, flags) = getbuf_info();
    if (change_default_dir(dir)) {
        error("cd '" + dir + "' failed");
    }

    % with a prefix argument, we can add extra flags
    if ( -9999 != prefix_argument (-9999) ) {
        opts = read_mini( "perltidy flags:", Null_String, opts );
    }

    % check if we want a tmp file
    % we need a tmp file for a processing
    % 1: a region
    % 2: a narrowed buffer
    % 3: when no file is attached
    variable use_tmp = markp(); % a region
    !if (use_tmp) {     % no region, but a narrowed buffer
        use_tmp = count_narrows();
        if (use_tmp) mark_buffer();
    }

    !if (use_tmp) {     % check if a file is attached
        !if (strlen(file)) use_tmp = 1;
        mark_buffer();
    }

    narrow();
    if (use_tmp) {
        file = tmp_input;       % we need to use a tmp file
        mark_buffer();
        () = write_region_to_file(file);

        % guess the start indentation level
        bob();
        do {
            skip_white();
            if (eolp()) continue;       % ignore blank lines
            % round column number up and use to estimate the indentation level
            cmd += sprintf(" -sil=%d", int((what_column() + 1) / Perl_Indent));
            break;
        } while (down_1());
    }
    else if (flags & 0x01) {     % buffer modified - save the file first
        () = write_buffer (dir + file);
    }

    sw2buf(shell_output);
    erase_buffer ();

    % clean-up function
    % unfortunately run_shell_cmd doesn't always signal an error!!
    ERROR_BLOCK {
        sw2buf(thisbuf);
        delbuf(shell_output);
        if (use_tmp) () = delete_file(file);
        widen();
        goto_line(line);
        bol();
        flush(Null_String);
    }

    % add flags and the file name
    if ( strlen(opts) )
      cmd = strjoin( [ cmd, opts, file ], " " );
    else
      cmd = strjoin( [ cmd, file ], " " );

    flush(cmd);
    variable rc = run_shell_cmd(cmd);
    set_buffer_modified_flag(0);        % mark as unchanged

    % handle errors from 'run_shell_cmd'
    if (rc) error("error running perltidy");

    % the command apparently worked
    % switch back to our original buffer and update everything
    sw2buf(thisbuf);
    mark_buffer();
    del_region();       % use del_region so that undo will work
    insbuf(shell_output);
    EXECUTE_ERROR_BLOCK;
}


% Run perl with some flags on current region if one is defined, otherwise
% on the whole buffer.
%
% Display output in *shell-output* buffer window.
%
% Error messages look like this:
% Missing right curly or square bracket at Foo.pl line 7, at end of line
%
% Thus we'll look for ' at FILENAME line '
static define do_perl (opts, prompt)
{
    variable cmd  = "perl -w";
    variable args = Null_String;
    variable line = 0;                  % line offset

    variable file, dir, thisbuf, flags;
    (file, dir, thisbuf, flags) = getbuf_info();
    if (change_default_dir(dir)) {
        error("cd '" + dir + "' failed");
    }

    if (strlen(Perl_Flags)) opts += " " + Perl_Flags;   % tack on our flags

    % with a prefix argument, we can edit perl flags
    if ( -9999 != prefix_argument (-9999) ) {
        opts = read_mini( "Perl flags:", Null_String, opts );
    }

    % check if we want a tmp file
    % we need a tmp file for a processing
    % 1: a region
    % 2: a narrowed buffer
    % 3: when no file is attached
    variable use_tmp = markp(); % a region
    !if (use_tmp) {     % no region, but a narrowed buffer
        use_tmp = count_narrows();
        if (use_tmp) mark_buffer();
    }

    !if (use_tmp) {     % check if a file is attached
        if (strlen(prompt))
          args = read_mini( prompt, Null_String, Null_String );

        !if (strlen(file)) {            % no file attached
            use_tmp = 1;
            mark_buffer();
        }
    }

    if (use_tmp) {
        file = tmp_input;               % we need to use a tmp file
        check_region(1);                % canonical region & push_spot
        exchange_point_and_mark();      % goto start
        line = what_line();

        % force 'strict';
        % also introduces a line offset of 1 as a nice side-effect
        () = write_string_to_file( "use strict;\n", file );
        () = append_region_to_file(file);
        pop_spot();
    }
    else if (flags & 0x01) {     % buffer modified - save the file first
        () = write_buffer (dir + file);
    }


    variable oldbuf = pop2buf_whatbuf(shell_output);
    erase_buffer ();

    % in case our system command bombs out
    ERROR_BLOCK {
        if (use_tmp) () = delete_file(file);
    }

#ifdef OS2 UNIX
    args += " 2>&1";    % re-direct stderr as well
#endif

    variable rc = run_shell_cmd(strjoin( [cmd, opts, file, args], " "));
    set_buffer_modified_flag(0);        % mark output as unchanged

    % report errors from 'run_shell_cmd'
    if (rc) flush("error running perl");

    EXECUTE_ERROR_BLOCK;

    % try to restore any window that got replaced by the shell-output
    %%     if (strlen(oldbuf)
    %%  and (oldbuf != shell_output)
    %%  and (oldbuf != thisbuf) )
    %%       {
    %%    splitwindow(); sw2buf(oldbuf); pop2buf(shell_output);
    %%       }
    eob();

    % No output - close the shell-window and display message
    if (bobp()) {
        pop2buf(thisbuf);
        onewindow();
        message("No output.");
    }
    else if ( right( bsearch( " at " + file + " line " ) ) ) {
        % Move to the line in source that generated the error
        skip_white();   % for safety's sake
        push_mark();
        skip_chars ("0-9");
        line += integer(bufsubstr());
        %%      flush (sprintf ("goto line %d", line));         % Debug
        pop2buf(thisbuf);
        goto_line(line);
        bol();
    }
}


%!%+
%\function{perl_exec}
%\synopsis{Void perl_exec (Void)}
%\description
% This function runs \var{perl} on a region, buffer or narrowed buffer.
% With a numerical prefix argument (eg Ctrl-U), also prompts for
% extra Perl flags.
% Display output in *shell-output* buffer window.
%\seealso{perl_check, perl_mode}
%!%-
define perl_exec()  {   % <AUTOLOAD>
    do_perl(Null_String, "perl @ARGV:");
}


%!%+
%\function{perl_check}
%\synopsis{Void perl_check (Void)}
%\description
% This function runs a perl \var{-CT} check on a region, buffer or narrowed buffer.
% Display output in *shell-output* buffer window.
%\seealso{perl_exec, perltidy, perl_mode}
%!%-
define perl_check() {
    do_perl("-cT", Null_String);        % check with tainting on
}


% we seem to need this an awful lot, since we currently have no help mode
% and the user may have deleted the buffer in the meantime
% ... with a bit better integration in the main JED distribution,
% we could reduce this overhead
%
static define attach_keymap (name)
{
    !if (keymap_p(name)) {
        make_keymap(name);
        definekey("perl_help",   "?",   name);
        definekey("perl_help",   "\r",  name);
        definekey("perl_help",   "^C?", name);  % for consistency
        definekey("perl_info",   "^Ci", name);
    }

    if (bufferp(name)) {
        variable cbuf = whatbuf();
        setbuf(name);
        use_keymap(name);               % attach keymap here
        setbuf(cbuf);
    }
}

%
% insert the results of a shell command into the help buffer
%
static define perl_get_help (cmd)
{
    variable cbuf = pop2buf_whatbuf(help_buf);
    erase_buffer();
    attach_keymap(help_buf);

    flush(cmd);
#ifdef UNIX
    () = run_shell_cmd(cmd + " 2>/dev/null"); % discard stderr
#else
    () = run_shell_cmd(cmd);
#endif

    bob();
    set_buffer_modified_flag(0);
    pop2buf(cbuf);
    flush(Null_String);
}

static define help_for_perl (what)
{
    % empty string -> translate to 'perl' (like a table-of-contents)
    if (orelse { what == NULL } {not(strlen (what))} ) what = "perl";
    perl_get_help("perldoc -t " + what);
}

%% %!%+
%% %\function{extract_word}
%% %\synopsis{extract_word}
%% %\usage{String extract_word (String Word_Chars)}
%% %\description
%% % extract a word defined by \var{Word_Chars} from the current buffer
%% %!%-
static define extract_word (chars)
{
    !if (markp()) {
        % skip leading non-word chars, including newline
        do {
            skip_chars ("^" + chars);
            !if (eolp()) break;
        } while (down (1));
        bskip_chars (chars);    % in case we started in the middle of a word
        push_mark(); skip_chars (chars);        % mark the word
    }
    return bufsubstr();
}

%!%+
%\function{perl_info}
%\synopsis{Void perl_info (Void)}
%\description
% displays the perl settings \var{perl -V} in the help buffer
%!%-
%\seealso{perldoc, perl_help}
define perl_info () { perl_get_help("perl -V"); }       % <AUTOLOAD>


%!%+
%\function{perl_help}
%\synopsis{Void perl_help (Void)}
%\description
% extract an alphanumeric keyword (a function) and display help
% via perldoc for it
%!%-
%\seealso{perldoc, perl_mode}
define perl_help ()     % <AUTOLOAD>
{
    variable what = extract_word(":0-9A-Z_a-z");
    !if (strlen(what)) {
        flush("Sorry no word extracted");
        return;         % no string - no help
    }

    % all lower-case words treated as function names
    % provided they don't start with 'perl' (mostly manpages)
    if (strncmp(what, "perl", 4)) {
        if (string_match(what, "^[a-z][a-z0-9]+$", 1)) {
            what = strcat("-f ", what);
        }
    }
    help_for_perl(what);
}

%!%+
%\function{perldoc}
%\synopsis{Void perldoc (void)}
%\description
% use perldoc to find information
% The '-f' option is inferred for lowercase strings not starting with 'perl'
%
% perldoc [options] PageName|ModuleName|ProgramName...
% perldoc [options] -f BuiltinFunction
% perldoc [options] -q FAQRegex
%
% Options:
%  -u   Display unformatted pod text
%  -m   Display module's file in its entirety
%  -q   Search the text of questions (not answers) in perlfaq[1-9]
%\seealso{perl_help, perl_mode}
%!%-
define perldoc ()       % <AUTOLOAD> <COMPLETE>
{
    !if (MINIBUFFER_ACTIVE)
      help_for_perl(read_mini("perldoc:", Null_String, Null_String));
}

provide ("perlxtra");
% -------------------------------------------------------- [end of S-Lang]

