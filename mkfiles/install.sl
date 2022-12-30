private variable Script_Version_String = "0.1.2";

require ("cmdopt");
require ("glob");

private define convert_path (path)
{
   return strtrans (path, "/", "\\");
}

private define mkdir_p (dir);
private define mkdir_p (dir)
{
   dir = convert_path (dir);

   if ((-1 == mkdir (dir)) && (errno != EEXIST))
     {
	variable parent = path_dirname (dir);
	if ((-1 == mkdir_p (parent))
	    || (-1 == mkdir (dir)))
	  {
	     () = fprintf (stderr, "Failed to create %s: %s\n",
			   dir, errno_string ());
	     exit (1);
	  }
     }
   return 0;
}

private define run_cmd (cmd)
{
   () = system (cmd);
}

private define install_file (file, dir)
{
   if (stat_file (file) == NULL)
     {
	() = fprintf (stdout, "%s does not exist -- skipping it\n", file);
	return;
     }
   () = fprintf (stdout, "Copying %s to %s\n", file, dir);
   dir = convert_path (dir);
   file = convert_path (file);

   run_cmd ("copy /y $file $dir"$);
}

private define install_files (pat, dir)
{
   foreach (glob (pat))
     {
	variable file = ();
	install_file (file, dir);
     }
}

private define install_binfiles (bindir)
{
   variable objdir = "gw32objs";
   variable dir = bindir;
   () = mkdir_p (dir);
   install_file ("src/$objdir/jed.exe"$, dir);
   install_file ("src/$objdir/wjed.exe"$, dir);

   dir = path_concat (bindir, "w32");
   () = mkdir_p (dir);
   install_files ("bin/w32/*.exe", dir);
   install_files ("bin/w32/README", dir);
}

private define install_libfiles (libdir)
{
   variable dir = libdir;
   () = mkdir_p (dir);
   install_files ("lib/*.sl", dir);
   install_files ("lib/*.dat", dir);
   install_files ("lib/*.hlp", dir);
   install_files ("lib/jed.rc", dir);

   dir = path_concat (libdir, "colors");
   () = mkdir_p (dir);
   install_files ("lib/colors/*.sl", dir);
   install_files ("lib/colors/README", dir);
}

private define install_conffiles (confdir)
{
   variable dir = confdir;
   () = mkdir_p (dir);
   variable file = path_concat (dir, "jed.conf");
   if (NULL == stat_file (file))
     {
	install_file ("lib/jed.conf", file);
     }
}

private define install_docfiles (docdir)
{
   variable dir = docdir;
   () = mkdir_p (dir);
   install_files ("doc/README", dir);

   dir = path_concat (docdir, "txt");
   () = mkdir_p (dir);
   install_files ("doc/txt/*.txt", dir);

   dir = path_concat (docdir, "hlp");
   () = mkdir_p (dir);
   install_files ("doc/txt/*.hlp", dir);
}

private define exit_version ()
{
   () = fprintf (stdout, "Version: %S\n", Script_Version_String);
   exit (0);
}

private define exit_usage ()
{
   variable fp = stderr;
   () = fprintf (fp, "Usage: %s [options] install\n", __argv[0]);
   variable opts =
     [
      "Options:\n",
      " -v|--version               Print version\n",
      " -h|--help                  This message\n",
      " --prefix=/install/prefix   Default is /usr\n",
      " --distdir=/path            Default is blank\n",
     ];
   foreach (opts)
     {
	variable opt = ();
	() = fputs (opt, fp);
     }
   exit (1);
}

% The windows installation of jed (and jed binary) assume that
% everything is installed under the JED_ROOT directory:
%
%   $JED_ROOT/bin/jed.exe
%   $JED_ROOT/lib/site.sl ...
%
% In the following, JED_ROOT will be set to $PREFIX/jed
% 
define slsh_main ()
{
   variable c = cmdopt_new ();
   variable destdir = "";
   variable prefix = "/usr";

   c.add("h|help", &exit_usage);
   c.add("v|version", &exit_version);
   c.add("destdir", &destdir; type="str");
   c.add("prefix", &prefix; type="str");
   variable i = c.process (__argv, 1);

   if ((i + 1 != __argc) || (__argv[i] != "install"))
     exit_usage ();

   % See the comment in makefile.m32 for the motivation behind the
   % sillyness involving X-.
   if (0==strncmp (destdir, "X-", 2))
     destdir = substr (destdir, 3, -1);

   () = fprintf (stdout, "Using destdir=%s, prefix=%s\n", destdir, prefix);

   variable root = strcat (destdir, prefix);
   variable jedroot = path_concat (root, "jed");
   variable bindir = path_concat (jedroot, "bin");
   variable libdir = path_concat (jedroot, "lib");
   variable docdir = path_concat (jedroot, "doc");
   variable confdir = path_concat (jedroot, "etc");

   install_binfiles (bindir);
   install_libfiles (libdir);
   install_docfiles (docdir);
   install_conffiles (confdir);
}
