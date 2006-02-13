private define p(x)
{
   () = fprintf (stdout, "%s\n", x);
}

p("Jed usage forms:");
p("");
p("0.  jed [--version | --help]");
p("1.  jed [--batch] [-n] [-e emulation] [-a alt-jedrc-file] \\");
p("        [file ...] \\            % edit files");
p("        [-g linenumber] \\       % goto line");
p("        [-s search-string] \\    % performa search");
p("        [-l file-to-load] \\     % load slang file");
p("        [-f function] \\         % execute function");
p("        [-i file-to-insert] \\   % insert file");
p("        [-2] \\                  % split window");
p("        [-tmp] \\                % do not backup buffer");
p("        [-hook funct (argv)] \\  % exec funct with rest of argv parameters");
p("        [--ANYTHING]            % execute ANYTHING as a function");
p("");
p("2.  jed -script FILE [arg ...]");
p("");
p("3.  jed-script FILE [arg ...]");
p("        This form sets __argv[0] to FILE, __argv[1] to arg, ...");
p("");
quit_jed ();
