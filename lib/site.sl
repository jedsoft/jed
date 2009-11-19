% -*- mode: slang; mode: fold; -*-
% Note: This file has been folded.
%_traceback = 1;
%_boseos_info = 3;
%{{{ Description of site.sl file
%
% This file must be present in $JED_ROOT/lib.  JED loads it first--- even
% before reading command line arguments.  The command line arguments are then
% passed to a hook declared in this file for further processing.
%
% In addition to some hooks, this file declares some autoloads for various
% functions and defines utility functions.  Any user specific stuff should be
% placed in the jed.rc (.jedrc) user startup file.  Only put here what you
% believe EVERY user on your system should get!
%
% The best way to make changes in this file is to put all your changes in a
% separate file, defaults.sl.  defaults.sl is NOT distributed with JED.  Code
% at the end of this file checks for the existence of `defaults.sl' and loads
% it if found. Functions occuring in this file (site.sl) may be overloaded in
% defaults.sl. Making changes this way also makes it easier to upgrade to
% future JED versions.
%

%}}}
%{{{ Special note on syntax of some functions
% --------------------------------------------------------------------------
% Note: Some of the small routines here have been written in such a way that
% the stack based nature of the language is exploited.  That is, instead of
% writing:
%        define sum (a, b) { return a + b; }
% I use:
%        define sum () { () + (); }
% The former parses to the bytecode:  =b =a a b + return
% where as the latter parses to:      +
% which is 6 times faster and 6 times more memory efficient!
% --------------------------------------------------------------------------

%}}}
%{{{ Global Variables

public variable _Jed_Emulation = NULL;
public variable _Jed_Default_Emulation = "emacs";
public variable Default_Jedrc_Startup_File = "jed.rc";
public variable _Jed_Color_Scheme = NULL;
public variable _Jed_Default_Color_Scheme = "black3";

%!%+
%\variable{Tab_Always_Inserts_Tab}
%\synopsis{Configure the tab key}
%\description
% If this variable is non-zero, then the tab key will insert tab characters
% into the buffer.  It is possible to override this setting via a mode hook.
%\seealso{local_setkey}
%!%-
public variable Tab_Always_Inserts_Tab = 0;

variable _C_Indentation_Style = NULL;
% This function will get overloaded by cmode.sl
define c_set_style ()
{
   _C_Indentation_Style = ();
}

variable Null_String = "";

%!%+
%\variable{Info_Directory}
%\synopsis{Info_Directory}
%\description
% A comma-separated list of info directories to search.
%!%-
variable Info_Directory;
variable Jed_Bin_Dir;

%!%+
%\variable{Jed_Highlight_Cache_Path}
%\synopsis{Search path for DFA syntax tables}
% A comma-separated list of directories to search for cached DFA syntax
% highlighting tables.  If a table is not found, it will be created on the
% fly and then cached in the directory specified by the
% \var{Jed_Highlight_Cache_Dir} variable.
%\seealso{Jed_Highlight_Cache_Dir, use_dfa_syntax}
%!%-
variable Jed_Highlight_Cache_Path;     %  search paths for EXISTING files

%!%+
%\variable{Jed_Highlight_Cache_Dir}
%\synopsis{Directory where newly created DFA syntax tables are placed}
% If the caching of DFA syntax tables is enabled, the newly created tables
% will be saved in the directory specified by this variable.
%\seealso{Jed_Highlight_Cache_Path, use_dfa_syntax}
%!%-
variable Jed_Highlight_Cache_Dir;      %  dir where NEW files kept

%!%+
%\variable{C_CONTINUED_OFFSET}
%\synopsis{C_CONTINUED_OFFSET}
%\usage{Integer C_CONTINUED_OFFSET = 2;}
%\description
% This variable controls the indentation of statements that are continued
% onto the next line as in the following example:
%#v+
%  if (something)
%    continued_statement ();
%  else
%    another_continued_statement ();
%#v-
%\seealso{C_BRA_NEWLINE, C_BRACE, C_INDENT, C_Colon_Offset}
%!%-
variable C_CONTINUED_OFFSET = 2;

%!%+
%\variable{C_Colon_Offset}
%\synopsis{C_Colon_Offset}
%\description
% Integer C_Colon_Offset = 1;
% This variable may be changed to adjust the indentation of \var{case} statements
% in C-Mode.
%\seealso{c_mode}
%\seealso{C_BRA_NEWLINE, C_BRACE, C_INDENT, C_Colon_Offset}
%!%-
variable C_Colon_Offset = 1;

%!%+
%\variable{C_Preprocess_Indent}
%\synopsis{C_Preprocess_Indent}
%\usage{Integer C_Preprocess_Indent = 1;}
%\description
% This variable controls the indentation of preprocessor directives in
% C-mode.
%\seealso{c_mode}
%\seealso{C_BRA_NEWLINE, C_BRACE, C_INDENT, C_Colon_Offset}
%!%-
variable C_Preprocess_Indent = 1;

%!%+
%\variable{C_Comment_Column}
%\synopsis{C_Comment_Column}
%\description
% Column to begin a C comment--- used by c_make_comment
%!%-
variable C_Comment_Column = 40;

%!%+
%\variable{C_INDENT}
%\synopsis{C_INDENT}
%\usage{Integer C_INDENT = 3;}
%\description
% This value determines the number of columns the current line is indented
% past the previous line containing an opening \exmp{'\{'} character.
%\seealso{C_BRACE, C_BRA_NEWLINE.}
%!%-
variable C_INDENT = 3;

%!%+
%\variable{C_BRACE}
%\synopsis{C_BRACE}
%\usage{Integer C_BRACE = 2;}
%\description
% This is a C-mode variable that specifies how much an opening brace
% should be indented compared its surrounding block.
%\seealso{C_INDENT, C_BRA_NEWLINE}
%!%-
variable C_BRACE = 2;

%!%+
%\variable{C_BRA_NEWLINE}
%\synopsis{C_BRA_NEWLINE}
%\usage{Integer C_BRA_NEWLINE = 1;}
%\description
% This variable is used by the indentation routines for the C langauge.
% If it is non-zero, the \exmp{'\{'} character will be placed on a line by
% itself when one presses the \exmp{'\{'} character.  For K&R indentation style,
% set this variable to zero.
%\seealso{C_INDENT, C_BRACE}
%!%-
variable C_BRA_NEWLINE = 1;

variable compile_parse_error_function = "gcc";

% These are for compatibility

variable REPLACE_PRESERVE_CASE = 0;
variable LAST_SEARCH = Null_String;

%!%+
%\function{custom_variable}
%\synopsis{custom_variable}
%\usage{Integer_Type custom_variable (String_Type name, Any_Type value)}
%\description
% This function is used to create a new public global variable called
% \var{name}, initialized to \var{value}, unless it exists. If the variable
% already exists but is not initialized, then it is initialized to
% \var{value}. This is quite useful for slang files with user configurable
% variables. These variables can be defined and initialized by users
% before evaluating the file, or can be defined and initialized to
% a default value in a file using this function.
%
% This function returns 2 if the variable \var{name} has been defined and
% initialized, or 1 if it has only been intialized. If the variable
% \var{name} has already been defined and initialized, this function does
% nothing and returns 0. If \var{name} is an invalid variable name, this
% function does nothing and returns -1.
%
% This function should be only used by slang programmers, not users.
%\seealso{custom_color}
%!%-
public define custom_variable (name, value)
{
   variable t;
   variable r;

   r = __get_reference (name);
   if (r == NULL)
     {
	eval (sprintf (".[%s]", name));
	r = __get_reference (name);
	@r = value;
	return;
     }

   if (is_defined (name) != -2)
     return;

   if (__is_initialized (r))
     return;

   @r = value;
}

%}}}

%{{{ Some useful functions that are independent of jed intrinsics

%{{{ str_replace_all (str, old, new)
%!%+
%\function{str_replace_all}
%\synopsis{str_replace_all}
%\usage{String str_replace_all (str, old, new);}
%\description
% Replace all occurances of \var{old} in \var{str} with \var{new} and return the
% result.
%\seealso{str_replace, replace_cmd}
%!%-
define str_replace_all (str, old, new)
{
   (str,) = strreplace (str, old, new, strlen (str));
   return str;
}

%}}}

%}}}


%{{{ Compatibility functions

#ifnexists strbytelen
define strbytelen (s)
{
   return strlen (s);
}
define substrbytes (s, n, len)
{
   return substr (s, n, len);
}
#endif

#ifnexists any
define any (x)
{
   return length (where (x));
}
#endif

define define_keywords ()
{
   define_keywords_n (0);
}

define save_search_string ()
{
   LAST_SEARCH = ();
}

% define this now so lib files can refer to it.
define compile_parse_errors ();

%}}}
%{{{ Utility functions required below (dircat, etc)
%{{{ vinsert

%!%+
%\function{vinsert}
%\synopsis{vinsert}
%\usage{Void vinsert (String, fmt,...);}
%\description
% This function is like \var{insert} except that it takes a variable number
% of arguments and a format string.
%\seealso{insert, sprintf, insert_char}
%!%-
define vinsert ()
{
   _NARGS-1; Sprintf; insert;
}

%}}}

%{{{ dircat

%!%+
%\function{dircat}
%\synopsis{Merge a directory name and file name}
%\usage{String_Type = dircat (String_Type a, String_Type b);}
%\description
%  The \var{dircat} function may be used to obtain the path name of a file with
%  filename \var{b} in directory \var{a}.  It performs this function in an
%  operating system dependent manner.
%!%-
define dircat(dir, file)
{
   % Many functions assume dir = NULL is ok, e.g., dircat (getenv (...));
   if (dir == NULL) dir = "";
   if (file == NULL) file = "";

   variable n = strlen(dir);

   if (n)
     {
#ifdef IBMPC_SYSTEM
	variable slash = "\\";
	if (dir[-1] != '\\') dir += slash;
#endif
#ifdef UNIX
	variable slash = "/";
	if (dir[-1] != '/') dir += slash;
	%if (strcmp(substr(dir, n, 1), slash)) dir = strcat(dir, slash);
#endif
#ifdef VMS
	% assume dir = d:[dir]a.dir;1
	% convert a.dir;1 to [.a] first
	variable f1, d1;
	dir = extract_element(dir, 0, ';');   % dir = d:[dir]a.dir
	d1 = extract_element(dir, 0, ']');   %  d1 = d:[dir
	f1 = extract_element(dir, 1, ']');   %  f1 = a.dir

	if (f1 != NULL)
	  {
	     if (strlen (f1))
	       d1 += "." + extract_element(f1, 0, '.');  % d1 = d:[dir.a
	  }

	n = strlen (dir);
	if (n)
	  {
	     if (dir [-1] != ':') d1 += "]";
	  }
	% if (':' != int(substr(dir, strlen(dir), 1))) d1 += "]";
	dir = d1;
#endif
     }
   return expand_filename (dir + file);
}

%}}}

%{{{ bol_skip_white ()
%!%+
%\function{bol_skip_white}
%\synopsis{bol_skip_white}
%\usage{Void bol_skip_white ();}
%\description
% This function combines the two functions \var{bol} and \var{skip_white} into a
% single operation.  That is, it moves the point to the beginning of the
% line and then skips over whitespace to the first non-whitespace character.
%\seealso{bol, skip_white, skip_chars}
%!%-
define bol_skip_white ()
{
   bol (); skip_white ();
}

%}}}

%{{{ bskip_white ()
%!%+
%\function{bskip_white}
%\synopsis{bskip_white}
%\usage{Void bskip_white ();}
%\description
% This function skips backward over whitespace.
% Note: it does not cross lines.
%\seealso{skip_white, bskip_chars}
%!%-
define bskip_white ()
{
   bskip_chars ("\t ");
}

%}}}

%{{{ buffer_filename ()

%!%+
%\function{buffer_filename}
%\synopsis{buffer_filename}
%\usage{String_Type buffer_filename ([String_Type bufname])}
%\description
% When called with no arguments, this function returns the name of the
% file associated with the current buffer.  If called with a string
% argument representing the name of a buffer, it will return the name
% of the file associated with that buffer.  If no file is associated
% with the specified buffer, the empty string will be returned.
%\seealso{getbuf_info}
%!%-
define buffer_filename ()
{
   variable args = __pop_args (_NARGS);
   variable file, dir;
   (file, dir, , ) = getbuf_info(__push_args (args));
   !if (strlen (file)) dir = "";
   return dir + file;
}

%}}}

%{{{ path2list(path)
%% Convert Unix- or OS/2- style path to comma-delimited list
define path2list ()
{
   % path is on stack
#ifndef VMS
   strtrans ( (),
# ifdef UNIX
	      ":",
# else
	      ";",
# endif
	      ",");
#endif
}

%}}}

%{{{ file_type(file)
%!%+
%\function{file_type}
%\synopsis{file_type}
%\description
% returns type of file.  e.g., /usr/a.b/file.c --> c
%\seealso{path_extname}
%!%-
define file_type(file)
{
   file = path_extname (file);
   if (strlen (file))
     file = file [[1:]];
   file;
}

%}}}

%!%+
%\function{search_path_for_file}
%\synopsis{search_path_for_file}
%\usage{String_Type search_path_for_file (path, file [,delim])}
%\description
% The \var{search_path_for_file} function searches the directories
% specified by the delimiter-separated set of directories \var{path}
% for the filename \var{file}. If the file exists, it returns the
% expanded filename, otherwise it returns \NULL.  The optional
% parameter may be used to specify the path delimiter.  The default
% delimiter is system-dependent and is the same as that returned by
% the \ifun{path_get_delimiter} function.
%!%-
define search_path_for_file ()
{
   variable path, f, delim = path_get_delimiter ();
   if (_NARGS == 3)
     delim = ();
   (path, f) = ();

   if (path == NULL)
     return NULL;
   foreach (strtok (path, char(delim)))
     {
        variable dir = ();
        variable file = dircat(dir, f);

        if (file_status(file) == 1)
          return file;
     }

   return NULL;
}

%{{{ expand_jedlib_file (f)
%!%+
%\function{expand_jedlib_file}
%\synopsis{expand_jedlib_file}
%\description
% Search for FILE in jed lib search directories and return
% expanded pathname if found or the Null string otherwise.
%!%-
define expand_jedlib_file (f)
{
   f = search_path_for_file (get_slang_load_path (), f);
   if (f == NULL)
     return "";
   f;
}
%}}}

%{{{ find_jedlib_file(file)
%!%+
%\function{find_jedlib_file}
%\synopsis{find_jedlib_file}
%\description
% find a file from JED_LIBRARY, returns number of lines read or 0 if not
% found.
%!%-
define find_jedlib_file(file)
{
   file = expand_jedlib_file(file);
   !if (strlen(file)) return(0);
   find_file(file);
}

%}}}

%{{{ parse_filename(fn)
%!%+
%\function{parse_filename}
%\synopsis{parse_filename}
%\usage{(dir, file) = parse_filename(fn)}
%\description
% breaks a filespec into dir filename---
% this routine returns dir and filename such that a simple strcat will
% suffice to put them together again.  For example, on unix, /a/b/c
% returns /a/b/ and c
%!%-
define parse_filename(fn)
{
   return (path_dirname (fn), path_basename (fn));
}

%}}}

%}}}
%{{{ Jed library path, info, and bin directories

#ifndef VMS
% Add the current directory to the search path.
%set_jed_library_path (strcat (get_jed_library_path (), ",."));
#endif

#ifdef VMS
Info_Directory = JED_ROOT + "[info]";
Jed_Bin_Dir = JED_ROOT + "[bin]";
#else
Info_Directory = dircat (JED_ROOT, "info");
Jed_Bin_Dir = dircat (JED_ROOT, "bin");
#endif

Jed_Highlight_Cache_Path = get_slang_load_path ();
Jed_Highlight_Cache_Dir = extract_element (Jed_Highlight_Cache_Path, 0, path_get_delimiter());

private define dir_exists (dir)
{
   variable s = stat_file (dir);
   if (s == NULL) return 0;
   return stat_is ("dir", s.st_mode);
}

%!%+
%\function{prepend_to_slang_load_path}
%\synopsis{Prepend a directory to the load-path}
%\usage{prepend_to_slang_load_path (String_Type dir)}
%\description
% This function adds a directory to the beginning of the interpreter's
% load-path.
%\seealso{append_to_slang_load_path, set_slang_load_path}
%!%-
public define prepend_to_slang_load_path (p)
{
   if (dir_exists (p))
     set_slang_load_path (sprintf ("%s%c%s", p, path_get_delimiter (), get_slang_load_path ()));
}

%!%+
%\function{append_to_slang_load_path}
%\synopsis{Append a directory to the load-path}
%\usage{append_to_slang_load_path (String_Type dir)}
%\description
% This function adds a directory to the end of the interpreter's
% load-path.
%\seealso{prepend_to_slang_load_path, set_slang_load_path}
%!%-
public define append_to_slang_load_path (p)
{
   if (dir_exists (p))
     set_slang_load_path (sprintf ("%s%c%s", get_slang_load_path (), path_get_delimiter (), p));
}

variable Jed_Doc_Files = "";
define jed_append_doc_file (file)
{
   if (Jed_Doc_Files == "")
     Jed_Doc_Files = file;
   else
     Jed_Doc_Files = strcat (Jed_Doc_Files, ",", file);

   variable cur_files = get_doc_files();
   set_doc_files([ cur_files[where(cur_files != file)], file ]);
}
define jed_insert_doc_file (file)
{
   if (Jed_Doc_Files == "")
     Jed_Doc_Files = file;
   else
     Jed_Doc_Files = strcat (file, ",", Jed_Doc_Files);
   variable cur_files = get_doc_files();
   set_doc_files( [file, cur_files[ where(cur_files != file) ]] );
}

#ifdef VMS
$1 = JED_ROOT;
#else
$1 = dircat (JED_ROOT, "doc/hlp");
#endif
foreach (["jedfuns.hlp", "libfuns.hlp"])
{
   $2 = ();
#ifdef VMS
   $2 = "[doc.hlp]" + $2;
#endif
   jed_append_doc_file (dircat ($1, $2));
}

#ifexists _slang_doc_dir
if (strlen(_slang_doc_dir) > 0)
  $1 = _slang_doc_dir;
# ifdef VMS
else
  $1 = path_concat ($1, "[doc.txt]");
# endif
jed_append_doc_file (path_concat ($1, "slangfun.txt"));
#endif

__uninitialize (&$1);
__uninitialize (&$2);

#ifdef UNIX
Info_Directory += ",/usr/info,/usr/share/info,/usr/local/info";
#endif

$1 = getenv("INFOPATH");
if ($1 != NULL) Info_Directory = path2list($1);

%}}}
%{{{ Some key definitions

define unset_ctrl_keys ()
{
   foreach ("ABCDEFGJKLNOPQRSTUVWXYZ")%  does not include ^I, ^H, ^M
     {
	variable ch = ();
	unsetkey (char (ch - '@'));
     }
}

setkey("skip_word",		"\e\e[C");  %escape right arrow.
setkey("bskip_word",		"\e\e[D");  %escape left arrow
setkey("upcase_word",		"\eu");
setkey("downcase_word", 	"\el");
setkey("capitalize_word", 	"\ec");
setkey("emacs_escape_x",	"\ex");
setkey("help_prefix",		"\e?");
if (_Backspace_Key != "\x08")
  setkey ("help_prefix", 	"^H");
setkey("do_shell_cmd",		"\e!");
setkey("ctags_popup_tag",	"\e.");
setkey("dabbrev",		"\e/");

#ifdef UNIX OS2
setkey("ispell",		"\e$");
#endif

#ifdef IBMPC_SYSTEM
setkey(" /", "\eOQ");
setkey(" *", "\eOR");
setkey(" +", "\eOm");
setkey(" -", "\eOS");
setkey("toggle_overwrite", "\xE0R");     %/* insert key */
setkey("toggle_overwrite", "\eOp");     %/* insert key */
#endif

%}}}
%{{{ Autoloads
$0 = _stkdepth ();
_autoload("mode_get_mode_info",		"modeinfo",
	  "mode_set_mode_info",		"modeinfo",
	  "text_mode",			"textmode",
	  "c_mode",			"cmode",
	  "slang_mode",			"slmode",
	  "java_mode",			"javamode",
	  "find_binary_file",		"binary",
	  "jed_easy_help",		"jedhelp",
	  "query_replace_match",	"regexp",
	  "re_search_forward",		"regexp",
	  "re_search_backward",		"regexp",
	  "dired",			"dired",
	  "calendar",			"cal",
	  %	    "menu_main_cmds",		"menu",
	  "trim_buffer",		"util",
	  "occur",			"occur",
	  "info_reader",		"info",
	  "info_find_node",		"info",
	  "list_buffers",		"bufed",
	  "append_region",		"buf",
	  "write_region",		"buf",
	  "save_buffers",		"buf",
	  "recover_file",		"buf",
	  "next_buffer",		"buf",
	  "save_buffer_as",		"buf",
	  "most_mode",			"most",
	  "run_most",			"most",
	  "compile",			"compile",
	  "compile_select_compiler",	"compile",
	  "compile_add_compiler",	"compile",
	  "sort",			"sort",
	  "sort_using_function",	"sort",
	  "untab",			"untab",
	  "fortran_mode",		"fortran",
	  "sh_mode", 			"shmode",
	  "ps_mode", 			"pscript",
	  "python_mode",		"pymode",
	  "rot13",			"rot13",
	  "search_forward",		"search",
	  "search_backward",		"search",
	  "replace_cmd",		"search",
	  "replace_across_buffer_files","replace",
	  "isearch_forward",		"isearch",
	  "isearch_backward",		"isearch",
	  "shell",			"shell",
	  "mute_set_mute_keys",		"mutekeys",
	  "html_mode",			"html",
	  "do_shell_cmd",		"shell",
	  "shell_perform_cmd",		"shell",
	  "ctags_backward",		"ctags",
	  "ctags_forward",		"ctags",
	  "ctags_popup_tag",		"ctags",
	  "ctags_find",			"ctags",
	  "find_tag",			"ctags",
	  "apropos",			"help",
	  "expand_keystring",		"help",
	  "describe_bindings",		"help",
	  "describe_function",		"help",
	  "describe_variable",		"help",
	  "help_for_function",		"help",
	  "where_is",			"help",
	  "showkey",			"help",
	  "describe_mode",		"help",
	  "format_paragraph_hook",	"tmisc",
	  "dabbrev",			"dabbrev",
	  "tex_mode",			"tex",
	  "bibtex_mode",		"bibtex",
	  "latex_mode",		"latex",
	  "bkmrk_goto_mark",          "bookmark",
	  "bkmrk_set_mark",           "bookmark",
	  "add_keyword",              "syntax",
	  "lisp_mode",		"lisp",
	  "perl_mode",		"perl",
	  "vhdl_mode",		"vhdlmode",
	  "spice_mode",		"spicemod",
	  "verilog_mode",		"verilog",
	  "tcl_mode",			"tclmode",
	  "lua_mode",			"lua",
	  "hook_add_hook",		"hooks",   %  obsolete
	  "changelog_add_change",	"chglog",

	  %%
	  %% By default, tabs are every TAB columns (default 8).  Calling this function
	  %% will allow the user to set the tabs arbitrarily and bind the TAB key
	  %% appropriately.
	  "edit_tab_stops",		"tabs",
	  "tab_to_tab_stop",		"tabs",
	  "append_string_to_file",	"misc",
	  "write_string_to_file",	"misc",
	  "make_tmp_buffer_name",	"misc",
	  "open_unique_filename",	"tmpfile",
	  "make_tmp_file",		"tmpfile",
#ifnexists glob_to_regexp
	  "glob_to_regexp",		"misc",
#endif
	  "list_directory",		"misc",
	  "directory",			"misc",
#ifexists abbrev_table_p
	  "abbrev_mode",		"abbrev",
	  "set_abbrev_mode",		"abbrev",
	  "save_abbrevs",		"abbrmisc",
	  "define_abbreviation",	"abbrmisc",
#endif
#ifdef VMS
	  "mail",			"mail",  % See also sendmail.sl
	  "mail_format_buffer",	"mail",
	  "dcl_mode",			"dcl",
	  "vms_help",			"vmshelp",
#endif

#ifdef UNIX OS2
	  "unix_man",			"man",
	  "ispell",			"ispell",
#endif
#ifdef UNIX
	  "rmail",			"rmail",
	  "mail",			"sendmail",
	  "mail_format_buffer",		"sendmail",
	  %	    "gdb_mode",			"gdb",
#endif
	  "mailedit_mode",		"mailedit",
#ifdef VMS UNIX
	  "f90_mode",			"f90",
#endif
	  "idl_mode",			"idl",
	  "nroff_mode",		"nroff",
	  "modeline_hook2",		"modehook",
	  "digraph_cmd",		"digraph",
	  "bufed",			"bufed",
	  "push_mode",		"pushmode",
	  "set_selective_display",	"seldisp",

	  "sgml_mode",		"sgml",
	  "docbook_mode",		"docbook",
	  "matlab_mode",		"matlab",
#ifndef VMS
	  "backups_on",		"backups",
	  "backups_off",		"backups",
#endif
	  % Compatibility functions
	  "create_array",		"compat",
	  "strncat",			"compat",
	  "info_mode",			"compat",
	  "get_jed_library_path",	"compat",
	  "set_jed_library_path",	"compat",

	  "tiasm_mode",		"tiasm",

	  "set_comment_info",		"comments",
	  "comment_region",		"comments",
	  "uncomment_region",		"comments",
	  "comment_line",		"comments",
	  "uncomment_line",		"comments",
	  "uncomment_region_or_line",	"comments",
	  "comment_region_or_line",	"comments",

	  "yp_copy_region_as_kill",	"yankpop",
	  "yp_kill_region",		"yankpop",
	  "yp_kill_line",		"yankpop",
	  "yp_yank",			"yankpop",
	  "yp_yank_pop",		"yankpop",
	  "yp_bkill_word",		"yankpop",
	  "yp_kill_word",		"yankpop",
#ifdef UNIX
	  "rcs_check_in_and_out",	"rcs",
	  "rcs_open_file",		"rcs",
	  "auto_compression_mode",	"compress",
#endif
	  "history_load",		"history",

	  (_stkdepth () - $0) / 2);	       %  matches start of _autoload

$0 = _stkdepth ();
_autoload("reg_insert_register",	"register",
	  "reg_copy_to_register",	"register",
	  "register_mode",		"register",
	  "php_mode",			"php",
	  "tm_mode",			"tmmode",
	  "vrun_program",		"runpgm",
	  "paste",			"paste",
	  "toggle_case_search",		"srchmisc",
	  "xform_region",		"xformreg",
	  "require",			"require",
	  "provide",			"require",

	  (_stkdepth () - $0) / 2);	       %  matches start of _autoload

%}}}
%{{{ More Utility functions

%{{{ Simple editing and movement functions

%!%+
%\function{go_up}
%\synopsis{go_up}
%\usage{Void go_up (Integer n);}
%\description
% Move up 'n' lines.
%\seealso{up, go_down}
%!%-
define go_up() { () = up(); }

%!%+
%\function{up_1}
%\synopsis{up_1}
%\usage{Void up_1 ();}
%\description
% Move up 1 line.  If successful, returns 1 otherwise it returns 0.
%\seealso{up, go_down, go_up, go_up_1}
%!%-
define up_1() { up(1); }

%!%+
%\function{go_up_1}
%\synopsis{go_up_1}
%\usage{Void go_up_1 ();}
%\description
% Move up exactly 1 line if possible.
%\seealso{up, go_down}
%!%-
define go_up_1 () { () = up_1(); }

%!%+
%\function{go_down}
%\synopsis{go_down}
%\usage{Void go_down (Integer n);}
%\description
% Move down 'n' lines.
%\seealso{go_up, down}
%!%-
define go_down() { () = down(); }

%!%+
%\function{down_1}
%\synopsis{down_1}
%\usage{Int_Type down_1 ();}
%\description
% Move down exactly one line.  If sucessful, 1 is returned otherwise
% zero is returned.
%\seealso{go_up, down, go_down_1}
%!%-
define down_1 () {  down (1); }

%!%+
%\function{go_down_1}
%\synopsis{go_down_1}
%\usage{Void go_down_1 ();}
%\description
% Move down one lines.
%\seealso{go_up, down}
%!%-
define go_down_1 () { () = down_1(); }

%!%+
%\function{go_left}
%\synopsis{go_left}
%\usage{Void go_left (Integer n);}
%\description
% Move backward 'n' characters.
%\seealso{left, go_right}
%!%-
define go_left() { () = left();}

%!%+
%\function{go_right}
%\synopsis{go_right}
%\usage{Void go_right (Integer n);}
%\description
% Move forward 'n' characters.
%\seealso{right, go_left}
%!%-
define go_right() { () = right();}

%!%+
%\function{go_right_1}
%\synopsis{go_right_1}
%\usage{Void go_right_1 ();}
%\description
% Move forward 1 characters.
%\seealso{right, go_left}
%!%-
define go_right_1() { go_right (1); }

%!%+
%\function{go_left_1}
%\synopsis{go_left_1}
%\usage{Void go_left_1 ();}
%\description
% Move forward 1 characters.
%\seealso{left, go_left}
%!%-
define go_left_1() { go_left (1); }

%!%+
%\function{newline}
%\synopsis{newline}
%\usage{Void newline (Void);}
%\description
% insert a newline in the buffer at point.
%\seealso{insert, insert_char}
%!%-
define newline ()
{
   insert_char('\n');
}

%!%+
%\function{insert_single_space}
%\synopsis{insert_single_space}
%\description
% insert a single space into the buffer.
%!%-
define insert_single_space ()
{
   insert_char(' ');
}

%!%+
%\function{looking_at_char}
%\synopsis{looking_at_char}
%\usage{Integer looking_at_char (Integer ch);}
%\description
% This function returns non-zero if the character at the current editing
% point is 'ch' otherwise it retuns zero.  This function performs a case
% sensitive comparison.
%!%-
define looking_at_char ()
{
   what_char () == ();
}

%}}}

%!%+
%\function{local_setkey}
%\synopsis{local_setkey}
%\usage{Void local_setkey (String fun, String key);}
%\description
% This function is like 'setkey' but unlike 'setkey' which operates on the
% global keymap, 'local_setkey' operates on the current keymap which may or
% may not be the global one.
%\seealso{setkey, definekey, local_unsetkey}
%!%-
define local_setkey ()
{
   definekey((), (), what_keymap());
}

%!%+
%\function{local_unsetkey}
%\synopsis{local_unsetkey}
%\usage{Void local_unsetkey (String key);}
%\description
% This function is like 'unsetkey' but unlike 'unsetkey' which unsets a key
% from the global keymap, 'local_unsetkey' operates on the current keymap
% which may or may not be the global one.
%\seealso{unsetkey, undefinekey, local_setkey}
%!%-
define local_unsetkey ()
{
   undefinekey( (), what_keymap());
}

variable _Reserved_Key_Prefix = NULL;
private define make_reserved_key (key)
{
   if (_Reserved_Key_Prefix == NULL)
     return "";
   strcat (_Reserved_Key_Prefix, key);
}

define definekey_reserved (fun, key, kmap)
{
   definekey (fun, make_reserved_key(key), kmap);
}
define undefinekey_reserved (key, kmap)
{
   undefinekey (make_reserved_key (key), kmap);
}
define local_setkey_reserved (fun, key)
{
   local_setkey (fun, make_reserved_key (key));
}
define local_unsetkey_reserved (key)
{
   local_unsetkey (make_reserved_key (key));
}
define setkey_reserved (fun, key)
{
   setkey (fun, make_reserved_key (key));
}
define unsetkey_reserved (key)
{
   unsetkey (make_reserved_key (key));
}

define get_mode_name ()
{
   what_mode (); pop ();
}

define global_mode_hook (hook)
{
}

%!%+
%\function{call_function}
%\synopsis{Call a function with arguments}
%\usage{call_function (String_Type f, [optional args...])}
%!%-
define call_function ()
{
   variable args = __pop_args (_NARGS - 1);
   variable func = ();
   if (typeof (func) != Ref_Type)
     func = __get_reference (func);
   if (func != NULL)
     (@func) (__push_args (args));
}

%!%+
%\function{runhooks}
%\synopsis{runhooks}
%\usage{Void runhooks (String_Type hook, [optional args...]);}
%!%-
define runhooks ()
{
   variable args = __pop_args (_NARGS);
   call_function (__push_args (args));
}

%!%+
%\function{run_mode_hooks}
%\synopsis{Run the user's mode hooks for the specified mode}
%\usage{run_mode_hooks (mode_hook_name)}
%\description
% This function should be called at the end of the mode setting
% function to allow the user to hook into the function.  It takes a
% single parameter: the name of the mode hook.  Prior to call the
% specified user-hook, this function calls \sfun{global_mode_hook}.
%\seealso{runhooks, global_mode_hook}
%!%-
define run_mode_hooks (hook)
{
   if (Tab_Always_Inserts_Tab)
     {
	local_unsetkey ("\t");
	local_setkey ("self_insert_cmd", "\t");
     }
   global_mode_hook (hook);
   runhooks (hook);
   % This is called after the hook to give the hook a chance to load the
   % abbrev table.
#ifexists abbrev_table_p
   if (abbrev_table_p (get_mode_name ()))
     use_abbrev_table (get_mode_name ());
#endif
}

% This is for backwards compatibility in case the user has exit_hook
private define run_user_exit_hook ()
{
   runhooks ("exit_hook");
   return 1;
}
add_to_hook ("_jed_exit_hooks", &run_user_exit_hook);

%!%+
%\variable{Jed_Tmp_Directory}
%\synopsis{Directory used to hold temporary files}
%\usage{Jed_Tmp_Directory = "/tmp";}
%\description
% This variable is used by the \sfun{make_tmp_file} function to create 
% temporary filenames.
%\seealso{make_tmp_file, make_tmp_buffer_name, open_unique_filename}
%!%-
variable Jed_Tmp_Directory = NULL;
#ifdef UNIX
Jed_Tmp_Directory = "/tmp";
#endif

%{{{ More functions


%!%+
%\function{pop_mark_0}
%\synopsis{pop_mark_0}
%\usage{Void pop_mark_0 ();}
%\description
% Since \var{pop_mark} is used so often with an argument of \var{0}, this function
% is simply equivalent to \var{pop_mark(0)}.
%\seealso{pop_mark, pop_mark_1}
%!%-
define pop_mark_0 ()
{
   pop_mark (0);
}

%!%+
%\function{pop_mark_1}
%\synopsis{pop_mark_1}
%\usage{Void pop_mark_1 ();}
%\description
% Since \var{pop_mark} is used so often with an argument of \var{1}, this function
% is simply equivalent to \var{pop_mark(1)}.
%\seealso{pop_mark, pop_mark_0}
%!%-
define pop_mark_1 ()
{
   pop_mark (1);
}

%!%+
%\function{goto_spot}
%\synopsis{goto_spot}
%\usage{Void goto_spot ();}
%\description
% This function returns to the position of the last pushed spot.  The spot
% is not popped.
%\seealso{push_spot, pop_spot, create_user_mark}
%!%-
define goto_spot ()
{
   pop_spot ();
   push_spot ();
}

%!%+
%\function{push_spot_bob}
%\synopsis{push_spot_bob}
%\usage{Void push_spot_bob ();}
%\description
% The function sequence \var{push_spot (); bob ();} occurs so often that
% it makes sense to have a single function that performs this task.
%\seealso{push_spot, bob, pop_spot, push_spot_bol}
%!%-
define push_spot_bob ()
{
   push_spot ();
   bob ();
}

%!%+
%\function{push_spot_bol}
%\synopsis{push_spot_bol}
%\usage{Void push_spot_bol ();}
%\description
% The function sequence \var{push_spot (); bol ();} occurs so often that
% it makes sense to have a single function that performs this task.
%\seealso{push_spot, bol, pop_spot, push_spot_bob}
%!%-
define push_spot_bol ()
{
   push_spot ();
   bol ();
}

%!%+
%\function{push_mark_eol}
%\synopsis{push_mark_eol}
%\usage{Void push_mark_eol ();}
%\description
% The function sequence \var{push_mark (); eol ();} occurs so often that
% it makes sense to have a single function that performs this task.
%\seealso{push_mark, eol, pop_mark, push_mark_eob}
%!%-
define push_mark_eol ()
{
   push_mark ();
   eol ();
}

%!%+
%\function{push_mark_eob}
%\synopsis{push_mark_eob}
%\usage{Void push_mark_eob ();}
%\description
% The function sequence \var{push_mark (); eob ();} occurs so often that
% it makes sense to have a single function that performs this task.
%\seealso{push_mark, eob, pop_mark, push_mark_eob}
%!%-
define push_mark_eob ()
{
   push_mark ();
   eob ();
}

%!%+
%\function{mark_buffer}
%\synopsis{mark_buffer}
%\usage{mark_buffer ();}
%\description
% This function marks the whole buffer leaving the point at the end
% of the buffer.
%\seealso{push_mark, pop_mark, bob, eob}
%!%-
define mark_buffer ()
{
   bob ();
   push_mark_eob ();
}

%!%+
%\function{bufsubstr_delete}
%\synopsis{bufsubstr_delete}
%\usage{String bufsubstr_delete ()}
%\description
% This functions returns the contents of a region defined my the mark
% and the current point.  The region will be deleted.
%\seealso{bufsubstr}
%!%-
define bufsubstr_delete ()
{
   () = dupmark ();
   bufsubstr ();		       %  on stack
   del_region ();
}

%!%+
%\function{del_eol}
%\synopsis{del_eol}
%\usage{Void del_eol ();}
%\description
% This function deletes from the current position to the end of the line.
%\seealso{del, delete_line, del_through_eol}
%!%-
define del_eol ()
{
   push_mark_eol ();
   del_region ();
}

%!%+
%\function{del_through_eol}
%\synopsis{del_through_eol}
%\usage{del_through_eol ();}
%\description
% This function deletes all text from the current point through the end of
% the line.
%\seealso{del, del_eol, del_region}
%!%-
define del_through_eol ()
{
   del_eol ();
   !if (eobp ()) del ();
}

%!%+
%\function{line_as_string}
%\synopsis{line_as_string}
%\usage{String line_as_string ()}
%\description
% This function returns the current line as a string.  This does not include
% the newline character at the end of the line.  The editing point is left
% at the end of the line.  That is, this function does not preserve the point.
%\seealso{bufsubstr}
%!%-
define line_as_string ()
{
   bol (); push_mark_eol (); bufsubstr ();
}

%!%+
%\function{double_line}
%\synopsis{Duplicate the current line}
%\description
% This function inserts a line into the buffer at the position of the
% current line that is a copy of the current line.  If the position of
% the editing point was originally one line N column C, then the
% editing point will be left on line (N+1) column C.
%\seealso{line_as_string}
%!%-
define double_line ()
{
   _get_point ();
   line_as_string ();		       %  on stack
   newline();
   insert(());
   _set_point (());
}

%!%+
%\function{bol_trim}
%\synopsis{bol_trim}
%\usage{Void bol_trim ();}
%\description
% Move to beginning of line and remove whitespace.
%\seealso{bol, trim}
%!%-
define bol_trim ()
{
   bol (); trim ();
}

%!%+
%\function{eol_trim}
%\synopsis{eol_trim}
%\usage{Void eol_trim ();}
%\description
% Move to end of line and remove whitespace.
%\seealso{eol, trim}
%!%-
define eol_trim ()
{
   eol ();
   trim ();
}

define re_looking_at (re)
{
   push_spot ();
   push_mark_eol ();
   go_right_1 ();		       %  make sure newline is included
   1 == string_match (bufsubstr (), re, 1);   %  on stack
   pop_spot ();
}

define enable_xmouse ()
{
#ifndef IBMPC_SYSTEM
   if (BATCH or is_defined ("X_LAST_KEYSYM"))   %  Xjed
     return;

   variable term = getenv ("TERM");
   if (term == NULL)
     return;
   if (strncmp (term, "xterm", 5))
     return;

   () = evalfile ("mousex");
#endif
}

#ifdef HAS_BLOCAL_VAR
%!%+
%\function{get_blocal_var}
%\synopsis{Return the value of a buffer-local variable}
%\usage{value = get_blocal_var (String name, [default])}
%\description
%  This function returns the value of the buffer-local variable
%  specified by \exmp{name}.  If the the optional \exmp{default}
%  argument is given, it will be returned if no local variable of the
%  specified name exists. Otherwise an error will be thrown.
%\example
%#v+
%    if (get_blocal_var("foo", 0))
%      message("this buffer is fooish");
%#v-
%  will print the message if \exmp{foo} is a buffer-local variable
%  with a nonzero value.
%\seealso{define_blocal_var, blocal_var_exists}
%!%-
define get_blocal_var ()
{
   variable name, value;
   if (_NARGS == 2)
     {
	(name, value) = ();
	!if (blocal_var_exists (name))
	  return value;
     }
   else name = ();
   return _get_blocal_var (name);
}

%!%+
%\function{define_blocal_var}
%\synopsis{Create and initialize a buffer local variable}
%\usage{define_blocal_var (name, value)}
%\description
%  This function may be used to create a buffer-local variable named
%  \exmp{name} and set it to \exmp{value}.  A buffer-local variable is a 
%  variable whose value is local to the current buffer. 
%\notes
%  The order of the \var{name} and \var{value} arguments to this
%  function are the reverse from that of the \ifun{set_blocal_var}
%  function.
%\seealso{get_blocal_var, create_blocal_var, set_blocal_var}
%!%-
define define_blocal_var (name, value)
{
   create_blocal_var (name);
   set_blocal_var (value, name);
}
#endif
%}}}

%{{{ Backup and autosave functions

#ifdef MSDOS OS2 WIN32 IBMPC_SYSTEM
# ifdef MSDOS WIN32
variable MSDOS_Has_Long_File_Names = 0;
# endif
define pc_system_support_long_filenames (dir)
{
# ifdef OS2
   return IsHPFSFileSystem(dir);
# else
   MSDOS_Has_Long_File_Names;
# endif
}
#endif

variable No_Backups = 0;

% returns backup filename.  Arguments to function are dir and file.
define make_backup_filename(dir, file)
{
#ifdef VMS
   return "";
#elifdef UNIX
   if (dir == "/tmp/") return "";
   if (path_extname (file) == ".tmp")
     return "";
#elifdef IBMPC_SYSTEM
   variable type;
   !if (pc_system_support_long_filenames (dir))
     {
	% There are several things to worry about.  Here just break up the
	% filename and truncate type to 2 chars and paste it back.
	% note that this takes a name like file.c and produces file.c~
	% Also, note that if the type is empty as in 'file', it produces
	% 'file.~'

	type = path_extname (file);
	!if (strlen (type))
	  type = ".";
	type = substr (type, 1, 3);
	file = strcat (path_sans_extname (file), type);
     }
#endif
   strcat (dir, file, "~");
}

% returns autosave filename.  Arguments to function are dir and file.
define make_autosave_filename(dir, file)
{
#ifdef VMS
   sprintf ("%s_$%s;1", dir, file);
#elifdef UNIX
   file = expand_symlink (path_concat (dir, file));
   return path_concat (path_dirname (file), 
		       sprintf ("#%s#", path_basename (file)));
#else
# ifdef IBMPC_SYSTEM
   if (pc_system_support_long_filenames (dir))
     file += "#";
   else
     file = strcat (substr(path_sans_extname (file), 1, 7),
		    path_extname (file));
# endif
   dir + "#" + file;
#endif
}

%}}}
%{{{ Some interactive functions (goto_line, column, M-x)

%{{{ emacs_escape_x()
define emacs_escape_x()
{
   variable f = Null_String, i = 0;
   variable mx_prompt;
   variable pa, exec_fun;

   if (MINIBUFFER_ACTIVE)
     {
	call("evaluate_cmd");
	return;
     }

   mx_prompt = "M-x";
   pa = prefix_argument (-1);
   if (pa != -1)
     mx_prompt = "ESC-" + string(pa) + " M-x";

   EXIT_BLOCK
     {
	set_prefix_argument (pa);
	(@exec_fun)(f);

	% If prefix argument still set, then use it as a repeat factor
	if (pa == prefix_argument (-1))
	  loop (pa - 1) (@exec_fun) (f);
	set_prefix_argument (-1);
     }
	
   forever
     {
	% Look for a namespace signature
	if (is_substr (f, "->"))
	  {
	     if (is_defined(f))
	       {
		  exec_fun = &eval;
		  return;
	       }
	  }

	f = strtrans (f, "-", "_");
	if (is_internal(f))
	  {
	     exec_fun = &call;
	     return;
	  }

	if (is_defined(f))
	  {
	     exec_fun = &eval;
	     return;
	  }

	!if (EXECUTING_MACRO or DEFINING_MACRO)
	  {
	     if (i == 1) ungetkey(13);
	     ungetkey(' ');
	     ++i;
	  }
	f = read_with_completion(mx_prompt, "", f, 'F');
     }
}

%}}}

define goto_line_cmd()
{
   read_mini("Goto line:", Null_String, Null_String);
   goto_line(integer(()));
}

define goto_column_cmd()
{
   read_mini("Goto Column:", Null_String, Null_String);
   goto_column(integer(()));
}

%;; scroll other window macros-- bind them yourself
define next_wind_up()
{
   otherwindow();  call("page_up");
   loop (nwindows() - 1) otherwindow();
}

define next_wind_dn()
{
   otherwindow();  call("page_down");
   loop (nwindows() - 1) otherwindow();
}

%!%+
%\function{whatpos}
%\synopsis{whatpos}
%\description
% display row and column information in minibuffer
%!%-
define whatpos ()
{
   variable max_lines;
   push_spot (); eob (); max_lines = what_line (); pop_spot ();
   vmessage ("%s, Line %d of %d lines, Column %d",
	     count_chars (), what_line(), max_lines, what_column ());
}

define goto_top_of_window ()
{
   loop (window_line()-1)
     skip_hidden_lines_backward (1);
   bol ();
}

define goto_bottom_of_window ()
{
   loop (window_info ('r') - window_line ())
     skip_hidden_lines_forward (1);
}

%!%+
%\function{redo}
%\synopsis{Undo the last undo}
%\usage{redo()}
%\description
% Undo the last undo. This works only one step, however
% as any undo is appended to the end of the undo buffer, you can
% actually roll the whole history back.
%\seealso{undo}
%!%-
public define redo ()
{
   try call("kbd_quit");
   catch UserBreakError:
     {
	call("undo");
	message ("Undo will now perform the action of redo");
     };
}

%}}}
%{{{ Mode functions and settings

%!%+
%\function{no_mode}
%\synopsis{no_mode}
%\description
%  Generic mode not designed for anything in particular.
%  Related Functions: \var{text_mode}, \var{c_mode}
%!%-
define no_mode ()
{
   use_syntax_table (Null_String);
   set_mode(Null_String, 0);
   use_keymap("global");
   unset_buffer_hook ("");
   run_mode_hooks ("no_mode_hook");
}

% Function prototypes
% These 'functions' are only here to initialize function pointers.
define _function_pop_0 (x) {0;}
define _function_return_1 () {1;}

%!%+
%\variable{Mode_Hook_Pointer}
%\synopsis{Mode_Hook_Pointer}
%\description
% called from mode_hook.  Returns 0 if it is desired that control return
% to mode_hook or 1 if mode hook should exit after calling mode_hook_ptr
%!%-
variable Mode_Hook_Pointer = &_function_pop_0;

variable Default_Mode = &text_mode;

% Emacs allows a mode definition on the first line of a file
% -*- mode: MODENAME; VAR: VALUE; ... -*-
% which can also include values of local variables

%!%+
%\function{modeline_hook}
%\synopsis{modeline_hook}
%\description
% check first line for the simplest Emacs mode statement
% -*- modename -*-
%!%-
define modeline_hook()
{
   variable mode = Null_String, extra_hook;
   push_spot_bob ();
   go_down (4);
#iffalse
   () = bsearch ("-*- END -*-");
#endif
   push_mark (); bob ();
   narrow ();

   % #!/bin/sh, #!/usr/local/bin/perl, #!/bin/csh -f ...
#ifdef 0
   if (looking_at("#!")) mode = "sh";
#endif

   if (re_fsearch("^\\(#! ?/[^ ]+/\\([^ \t]+\\)\\)"))
     {
	mode = regexp_nth_match (2);

	%  Check for #! /usr/bin/env PGM args...
	if (mode == "env")
	  {
	     go_right (strlen (regexp_nth_match (1)));
	     skip_white ();
	     push_mark ();
	     skip_chars ("^ \t\n");
	     mode = bufsubstr ();
	  }

	!if (is_defined (mode + "_mode"))
	  {
	     if (is_list_element ("bash,ksh,ash,zsh,csh", mode, ','))
	       mode = "sh";
	     else if (is_list_element ("slsh,jdl,jed-script,jdl-script", mode, ','))
	       mode = "slang";
	  }
     }

   if (re_fsearch ("-\\*- *\\([-A-Za-z_+0-9]+\\) *-\\*-"))
     mode = strlow (regexp_nth_match (1));

   bob ();
   % -*- mode: VALUE -*- or -*- eval: VALUE -*-
   extra_hook = re_fsearch ("-\\*- *.+:.+ *-\\*-");

   widen ();

   EXIT_BLOCK
     {
	mode = ();
	if (extra_hook) (mode + modeline_hook2 ()); else mode;
	pop_spot ();		% restore place
     }

   if ( strlen(mode) )
     {
	variable mstr = "_mode";
	mode = strtrans (mode, "-", "_");
	!if (is_substr (mode, mstr)) mode += "_mode"; %mode = strcat (mode, "_mode" );

	if (mode == "c++_mode")
	  mode = "c_mode";

	if (is_defined(mode))
	  {
	     eval (mode);
	     1;			       %  mode was defined
	     return;
	  }
     }
   0;
}

variable Mode_List_Exts = "h,cc,cpp,hpp,hh,sl,txt,doc,f,for,pro,1,pl,pm,v,verilog,vhd,vhdl,vt,sp,cir,py,cxx,m,bib";
variable Mode_List_Modes = "c,c,c,c,c,slang,text,text,fortran,fortran,idl,nroff,perl,perl,verilog,verilog,vhdl,vhdl,vhdl,spice,spice,python,c,matlab,bibtex";

#ifdef MSDOS OS2 IBMPC_SYSTEM
Mode_List_Exts += ",rc,bat,htm";     %  resource file
Mode_List_Modes += ",c,no,html";
#endif

#ifdef VMS UNIX
Mode_List_Exts += ",com,htm,shtml,sgml";     %  resource file
Mode_List_Modes += ",dcl,html,html,docbook";
#endif

#ifdef UNIX
Mode_List_Exts += ",cshrc,tcshrc,login,profile,conf";
Mode_List_Modes += ",sh,sh,sh,sh,sh";
Mode_List_Exts += ",letter,article,followup,jedrc";
Mode_List_Modes += ",text,text,text,slang";
#endif

%!%+
%\function{add_mode_for_extension}
%\synopsis{add_mode_for_extension}
%\usage{Void add_mode_for_extension (String mode, String ext);}
%\description
% This function modifies Mode_List in such a way that when a file with
% filename extension `ext' is read in, function strcat (mode, "_mode")
% will be called to set the mode.   That is, the first parameter 'mode'
% is the name of a mode without the '_mode' added to the end of it.
%!%-
define add_mode_for_extension (mode, ext)
{
   Mode_List_Modes = __tmp(mode) + "," + Mode_List_Modes;
   Mode_List_Exts = __tmp(ext) + "," + Mode_List_Exts;
}

%!%+
%\function{mode_hook}
%\synopsis{mode_hook}
%\description
% This is a hook called by find_file routines to set the mode
% for the buffer. This function takes one parameter, the filename extension
% and returns nothing.
%!%-

define mode_hook (ext)
{
   variable n, mode;
#ifdef VMS
   ext = extract_element(ext, 0, ';');
#endif

#ifndef UNIX
   ext = strlow (ext);
#endif

#ifdef UNIX
   % Strip off final ~
   if (ext[-1] == '~')
     {
	if (strlen (ext) > 1)
	  ext = ext[[:strlen(ext)-2]];
     }
#endif

   if (@Mode_Hook_Pointer(ext)) return;

   if (modeline_hook ()) return;

   n = is_list_element (Mode_List_Exts, ext, ',');

   if (n)
     {
	n--;
	mode = extract_element (Mode_List_Modes, n, ',') + "_mode";
	if (is_defined(mode) > 0)
	  {
	     eval (mode);
	     return;
	  }
     }

   mode = strcat (strlow (ext), "_mode");
   if (is_defined (mode) > 0)
     {
	eval (mode);
	return;
     }

   !if (strncmp (strup (extract_filename (buffer_filename ())), "READ", 4))
     {
	text_mode ();
	return;
     }

   @Default_Mode ();
}

%}}}
%{{{ Buffer flags and related functions

define _test_buffer_flag (x)
{
   variable flags;

   (,,,flags) = getbuf_info ();
   flags & x;
}

define _set_buffer_flag (x)
{
   getbuf_info ();
   () | x;
   setbuf_info (());
}

define _unset_buffer_flag (x)
{
   getbuf_info ();
   () & ~x;
   setbuf_info (());
}

% Usage: set_or_unset_buffer_flag (set, flag)
define _set_or_unset_buffer_flag ()
{
   exch ();			       %  (set,flag) ===> (flag,set)
   if (())
     _set_buffer_flag (());
   else
     _unset_buffer_flag (());
}

define _toggle_buffer_flag (f)
{
   setbuf_info(getbuf_info() xor f);
}

%!%+
%\function{set_buffer_modified_flag}
%\synopsis{set_buffer_modified_flag}
%\description
% sets buf modified flag. If argument is 1, mark
% buffer as modified.  If argument is 0, mark buffer as unchanged.
%!%-
define set_buffer_modified_flag ()
{
   _set_or_unset_buffer_flag ((), 0x1);
}

%!%+
%\function{buffer_modified}
%\synopsis{buffer_modified}
%\usage{Int_Type buffer_modified ()}
%\description
%  returns non-zero if the buffer modified flag is set.  It returns zero
%  if the buffer modified flag is not been set.  This works on the
%  current buffer.  See also 'set_buffer_modified_flag'.
%!%-
define buffer_modified ()
{
   _test_buffer_flag (0x01);
}

%!%+
%\function{set_buffer_undo}
%\synopsis{set_buffer_undo}
%\description
% set undo mode for buffer.  If argument is 1, undo is on.  0 turns it off
%!%-
define set_buffer_undo ()
{
   _set_or_unset_buffer_flag ((), 0x20);
}

%!%+
%\function{set_readonly}
%\synopsis{set_readonly}
%\description
% Takes 1 parameter: 0 turn off readonly
%                    1 turn on readonly
%!%-
define set_readonly ()
{
   _set_or_unset_buffer_flag ((), 0x08);
}

%!%+
%\function{is_readonly}
%\synopsis{Test whether or not the buffer is in read-only mode}
%\usage{Int_Type is_readonly ()}
%\description
% This function returns a non-zero value if the buffer is read-only;
% otherwise it returns 0.
%\seealso{set_readonly, getbuf_info, setbuf_info}
%!%-
define is_readonly ()
{
   _test_buffer_flag (0x08);
}

%!%+
%\function{is_overwrite_mode}
%\synopsis{Checks whether or not the buffer is in overwrite mode}
%\usage{Int_Type is_overwrite_mode ()}
%\description
% This function returns a non-zero value if the buffer is in overwrite-mode;
% otherwise it returns 0.
%\seealso{toggle_overwrite, getbuf_info, setbuf_info}
%!%-
define is_overwrite_mode ()
{
   _test_buffer_flag (0x10);
}

%!%+
%\function{set_overwrite}
%\synopsis{set_overwrite}
%\usage{set_overwrite (Int_Type x)}
%\description
% If the parameter \var{x} is non-zero, the buffer will be put in overwrite
% mode; otherwise it will be ut in insert mode.
%\seealso{toggle_overwrite, is_overwrite_mode, getbuf_info, setbuf_info}
%!%-
define set_overwrite ()
{
   _set_or_unset_buffer_flag ((), 0x10);
}

%!%+
%\function{toggle_crmode}
%\synopsis{Toggle the buffer line endings between CRLF and LF}
%\usage{toggle_crmode ()}
%\description
% The \var{toggle_crmode} function causes the line endings of the buffer to
% alternate between CRLF and LF characters.
%\seealso{getbuf_info, setbuf_info}
%!%-
define toggle_crmode ()
{
   _toggle_buffer_flag (0x400);
   set_buffer_modified_flag (1);
}

%!%+
%\function{toggle_readonly}
%\synopsis{Toggle the readonly status of the buffer}
%\usage{toggle_readonly ()}
%\description
% The \var{toggle_readonly} function toggles the read-only status of the
% current buffer.
%\seealso{set_readonly, is_readonly, getbuf_info, setbuf_info}
%!%-
define toggle_readonly()
{
   _toggle_buffer_flag (0x08);
}

%!%+
%\function{toggle_overwrite}
%\synopsis{Toggle the overwrite mode of the buffer}
%\usage{toggle_overwrite ()}
%\description
% The \var{toggle_overwrite} function toggles the overwrite mode of the
% current buffer.
%\seealso{set_overwrite, is_overwrite_mode, getbuf_info, setbuf_info}
%!%-
define toggle_overwrite()
{
   _toggle_buffer_flag (0x10);
}

%!%+
%\function{toggle_undo}
%\synopsis{Toggle the undo mode of the buffer}
%\usage{toggle_undo ()}
%\description
% The \var{toggle_undo} function toggles the undo mode of the
% current buffer.
%\seealso{getbuf_info, setbuf_info}
%!%-
define toggle_undo()
{
   _toggle_buffer_flag (0x20);
}

%!%+
%\function{set_buffer_no_backup}
%\synopsis{set_buffer_no_backup}
%\usage{Void set_buffer_no_backup ();}
%\description
%
%!%-
define set_buffer_no_backup ()
{
   _set_buffer_flag (0x100);
}

%!%+
%\function{set_buffer_no_autosave}
%\synopsis{set_buffer_no_autosave}
%\usage{Void set_buffer_no_autosave ();}
%\description
%
%!%-
define set_buffer_no_autosave ()
{
   _unset_buffer_flag (0x02);
}

%}}}

%!%+
%\function{toggle_line_number_mode}
%\synopsis{toggle_line_number_mode}
%\usage{Void toggle_line_number_mode ();}
%\description
% This function toggles the line number display state on or off.
%\seealso{set_line_number_mode}
%!%-
define toggle_line_number_mode ()
{
   set_line_number_mode (-1);
}
add_completion ("toggle_line_number_mode");

% Make this a wrapper around _set_color to allow the user to give it a more
% sophisticated definition.
define set_color (){_set_color;}

% Comma separated list of directories
public variable Color_Scheme_Path = "";
foreach (strtok (get_slang_load_path (), char(path_get_delimiter())))
{
   $1 = ();
   Color_Scheme_Path = dircat ($1, "colors");
   if (2 == file_status (Color_Scheme_Path))
     break;
}

define set_color_scheme (scheme)
{
   variable file;
   if (scheme == NULL)
     return;
   scheme = string (scheme);       %  for back-compatability, file may be an integer

   file = search_path_for_file (Color_Scheme_Path, scheme + ".sl", ',');
   if (file == NULL)
     {
	% Try .slc file
	file = search_path_for_file (Color_Scheme_Path, scheme + ".slc", ',');
	if (file == NULL)
	  {
	     vmessage ("Color scheme %S is not supported", scheme);
	     return;
	  }
     }
   %  strip .sl[c] to get pre-parsed version
   _Jed_Color_Scheme = path_sans_extname (file);
   () = evalfile (_Jed_Color_Scheme);
}

%!%+
%\function{custom_color}
%\synopsis{Create a color object to be customized by the user}
%\usage{custom_color (color, fg, bg)}
%\description
% This function may be used to create a specified color object.  If the object
% does not already exist, it will be given the specified foreground and
% background colors.
%\seealso{custom_variable}
%!%-
define custom_color (color, fg, bg)
{
   if (-1 != color_number (color))
     return;

   add_color_object (color);
   set_color (color, fg, bg);
}

private variable Email_Address = NULL;
define get_emailaddress ()
{
   if (Email_Address != NULL)
     return Email_Address;

   return strcat (get_username (), "@", get_hostname ());
}
define set_emailaddress (s)
{
   Email_Address = s;
}

%{{{ Help stuff

%!%+
%\variable{help_for_help_string}
%\synopsis{help_for_help_string}
%\description
% string to display at bottom of screen upon JED startup and when
% user executes the help function.
%!%-
variable help_for_help_string;

help_for_help_string =
#ifdef VMS
  "-> Help:H  Menu:?  Info:I  Apropos:A  Key:K  Where:W  Fnct:F  VMSHELP:M  Var:V";
#elifdef IBMPC_SYSTEM
"-> Help:H  Menu:?  Info:I  Apropos:A  Key:K  Where:W  Fnct:F  Var:V  Mem:M";
#else
"-> Help:H  Menu:?  Info:I  Apropos:A  Key:K  Where:W  Fnct:F  Var:V  Man:M";
#endif

%%
%% help function
%%

%!%+
%\variable{Help_File}
%\synopsis{Help_File}
%\description
% name of the file to load when the help function is called.
%!%-
variable Help_File = "jed.hlp";   %% other modes will override this.

%{{{ help()

%!%+
%\function{help}
%\synopsis{help}
%\usage{Void help ([String_Type help_file])}
%\description
% This function pops up a window containing the specified help file.  If the
% function was called with no arguments, the the file given by the \var{Help_File}
% variable will be used.
%!%-
define help ()
{
   variable hlp = "*help*", buf, rows;

   % optional argument with default
   variable help_file=Help_File;
   if (_NARGS)
     help_file = ();

   if (help_file == NULL) help_file = "";

   !if (strlen(help_file)) help_file = "generic.hlp";
   help_file = expand_jedlib_file(help_file);

  !if (buffer_visible (hlp))
     {
	buf = whatbuf();
	onewindow();
	rows = window_info('r');
	setbuf(hlp);
	set_readonly(0);
	erase_buffer();

	() = insert_file(help_file);
	pop2buf(hlp);
	eob(); bskip_chars("\n");
	rows = rows / 2 - (what_line() + 1);
	bob();
	set_buffer_modified_flag(0);
	set_readonly(1);
	pop2buf(buf);
	loop (rows) enlargewin();
     }

   update_sans_update_hook (1);
   message(help_for_help_string);
}

%}}}

variable Global_Top_Status_Line = " *** To activate menus, press `ESC ? ?'.  For help, press `ESC ? h'. ***";
() = set_top_status_line (Global_Top_Status_Line);

%{{{ help_prefix()
define help_prefix()
{
   variable c;

   !if (input_pending(7)) flush (help_for_help_string);
   c = toupper (getkey());
   switch (c)
     { case  8 or case 'H': help (); }
     { case  'A' : apropos (); }
     { case  'B' : describe_bindings (); }
     { case  'I' : info_reader (); }
     { case  '?' : call ("select_menubar");}
     { case  'F' : describe_function ();}
     { case  'V' : describe_variable ();}
     { case  'W' : where_is ();}
     { case  'C' or case 'K': showkey ();}
     { case  'M' :
#ifdef UNIX OS2
	unix_man();
#else
# ifdef VMS
	vms_help ();
# endif
#endif
#ifdef MSDOS MSWINDOWS
	call("coreleft");
#endif
     }
     { beep(); clear_message ();}
}

%}}}

%}}}
%{{{ Mini-Buffer related stuff

% Load minibuffer routines now before any files are loaded.
% This will reduce fragmentation on PC.

% Make sure this is defined even in batch mode.
public define mini_init_minibuffer ();
!if (BATCH)
  () = evalfile("mini");

%{{{ Reading from Mini-Buffer functions
%for compatability with older versions
%!%+
%\function{read_file_from_mini}
%\synopsis{read_file_from_mini}
%\usage{String read_file_from_mini (String p);}
%\description
% This function prompts the user for a file name using \var{p} as a prompt.
% It reads a filename with completion from the mini-buffer and returns
% it.
%\seealso{read_with_completion, read_mini}
%!%-
define read_file_from_mini ()
{
   read_with_completion( () , "", "", 'f');
}

%!%+
%\function{read_string_with_completion}
%\synopsis{read_string_with_completion}
%\usage{String read_string_with_completion (prompt, dflt, list)}
%\description
% This function takes 3 String parameters and returns a String.  The
% first parameter is used as the prompt, the second parameter is the
% default value to be returned and the third parameter is a list to be used
% for completions.  This list is simply a comma separated list of strings.
%!%-
define read_string_with_completion (prompt, dflt, list)
{
   read_with_completion (list, prompt, dflt, Null_String, 's');
}

%}}}

%}}}
%{{{ Startup hook

%!%+
%\variable{Startup_With_File}
%\synopsis{Startup_With_File}
%\description
% If non-zero, startup by asking user for a filename if one was
% not specified on the command line.
%!%-
variable Startup_With_File = 0;

%% startup hook
%!%+
%\function{jed_startup_hook}
%\synopsis{jed_startup_hook}
%\description
% Function that gets executed right before JED enters its main editing
% loop.  This is for last minute modifications of data structures that
% did not exist when startup files were loaded.
%!%-
define jed_startup_hook()
{
   variable n, hlp, ok = 0, file;
   variable scratch = "*scratch*";

   % turn on Abort character processing
   IGNORE_USER_ABORT = 0;

   runhooks ("startup_hook");

   try
     {
	ifnot ((whatbuf != scratch) || buffer_modified())
	  {
	     try
	       {
		  () = insert_file (expand_jedlib_file("cpright.hlp"));
		  set_buffer_modified_flag (0);
		  bob();
		  file = "";
		  message ("");
		  if (Startup_With_File > 0)
		    {
		       forever
			 {
			    file = read_file_from_mini ("Enter Filename:");
			    if (strlen(extract_filename(file))) break;
			 }
		    }
		  else ifnot (Startup_With_File)
		    {
		       do
			 {
			    update_sans_update_hook (1);
			 }
		       while (not (input_pending(600)));   %  1 minute
		    }
	       }
	     finally
	       {
		  setbuf (scratch);
		  erase_buffer ();
		  set_buffer_modified_flag (0);
	       }
	     if (file != "") () = find_file(file);
	  }
     }
   finally
     eval (".()jed_startup_hook");
}

add_to_hook ("_jed_startup_hooks", &jed_startup_hook);

%}}}

#ifdef VMS
%{{{ resume_hook()
%% This resume hook is need for VMS when returning from spawn.
%% In fact, it is NEEDED for certain JED functions on VMS so declare it.
private define vms_resume_hook ()
{
   variable file = getenv("JED_FILE_NAME");
   if (file != NULL)
     !if (find_file(file)) error("File not found!");
}
%}}}
add_to_hook ("_jed_resume_hooks", &vms_resume_hook);
#endif VMS

%{{{ find_file_hook(file)

% called AFTER a file is read in to a buffer.  FILENAME is on the stack.
private define find_file_hook ()
{
   variable dir, a, f, m;
   (f, dir,,) = getbuf_info ();

#ifndef VMS
   if (file_status(dir) != 2)
     {
	verror ("Directory %s is invalid", dir);
     }
#endif

   if (No_Backups) set_buffer_no_backup ();
   a = make_autosave_filename(dir, f);
   if (file_time_compare(a, dircat (dir, f)) > 0)
     {
	m = sprintf ("Autosave file is newer. Use ESC-X recover_file. (%s)", f);
	flush(m);
        () = input_pending(30);
	message(m);
     }
   runhooks ("user_find_file_hook");
}
%}}}
add_to_hook ("_jed_find_file_after_hooks", &find_file_hook);

%{{{ Completions

%
% completions  -- everything here must be predefined
% I just push the strings onto the stack and loop 'add_completion' over them
%
$0 = _stkdepth();
_add_completion ("toggle_undo", "calendar", "trim_buffer",
#ifexists abbrev_table_p
		 "abbrev_mode", "define_abbreviation", "save_abbrevs",
#endif
		 "occur", "append_region", "write_region",
		 "replace_across_buffer_files",
		 "recover_file", "compile", "sort", "untab", "fortran_mode",
		 "save_buffers",
		 "isearch_forward", "isearch_backward", "shell",
		 "edit_tab_stops", "c_mode", "toggle_crmode",
		 "text_mode", "no_mode", "goto_line_cmd", "goto_column_cmd",
		 "describe_mode",
		 "evalbuffer", "open_rect", "kill_rect", "insert_rect",
		 "copy_rect", "blank_rect",
		 "dired", "re_search_forward", "re_search_backward",
		 "query_replace_match", "bufed",
		 "describe_bindings", "search_backward", "search_forward",
		 "replace_cmd", "find_binary_file", "latex_mode", "sh_mode",
#ifdef UNIX VMS
		 "mail",
#endif
#ifdef UNIX OS2
		 "ispell",
#endif
#ifdef UNIX
		 "auto_compression_mode",
		 %		 "gdb_mode",
#endif
		 "slang_mode",
		 "python_mode",
		 _stkdepth - $0);      %  matches _add_completion

%}}}

%{{{ save_buffer()
%!%+
%\function{save_buffer}
%\synopsis{save_buffer}
%\usage{Void save_buffer ();}
%\description
% Save current buffer.
%!%-
define save_buffer()
{
   variable file;

   !if (buffer_modified ())
     {
	message("Buffer not modified.");
	return;
     }

   file = buffer_filename ();
   !if (strlen(file))
     file = read_file_from_mini ("Save to file:");

   !if (strlen(file))
     error ("File name not specified");

   () = write_buffer (file);

}
add_completion("save_buffer");

%}}}
%{{{ insert_buffer()
define insert_buffer()
{
   variable buf = read_with_completion("Insert Buffer:", "", "", 'b');
   push_spot();
   try
     insbuf(buf);
   finally
     pop_spot ();
}
add_completion("insert_buffer");

%}}}

%{{{ Word movement and processing functions

%%
%%  word movement definitions.  Since these vary according to editors,
%%  they are S-Lang routines.
%%

define skip_word ()
{
   while (skip_non_word_chars(), eolp())
     {
	if (1 != right(1)) break;
     }
   skip_word_chars();
}

define bskip_word()
{
   while (bskip_non_word_chars(), bolp())
     {
	!if (left(1)) break;
     }
   bskip_word_chars();
}

define delete_word()
{
   push_mark(); skip_word(); del_region();
}

define bdelete_word ()
{
   push_mark(); bskip_word(); del_region();
}

define xform_word ()		       %  parameter on stack
{
   while (skip_non_word_chars(), eolp())
     {
	if (1 != right(1)) break;
     }
   push_mark(); skip_word();
   xform_region(());
}

define capitalize_word()
{
   xform_word('c');
}

define upcase_word()
{
   xform_word('u');
}

define downcase_word()
{
   xform_word('d');
}

%}}}

%{{{ smart_set_mark_cmd ()

%!%+
%\function{push_visible_mark}
%\synopsis{push_visible_mark}
%\usage{Void push_visible_mark ();}
%\description
% This function is performs the same task as \var{push_mark} except that the
% region between this mark and the cursor position will be highlighted.
% Such a mark is said to be a visible mark.
%\seealso{push_mark, pop_mark, set_mark_cmd}
%!%-
define push_visible_mark ()
{
   push_mark ();
   call ("set_mark_cmd");
}

%!%+
%\function{set_mark_cmd}
%\synopsis{set_mark_cmd}
%\usage{Void set_mark_cmd ();}
%\description
% If a mark is already set, and that mark is a visible mark, then this
% function will remove that mark.  It will then push a visible mark onto
% the mark stack.
%\seealso{push_visible_mark, pop_mark, smart_set_mark_cmd}
%!%-
define set_mark_cmd ()
{
   if (is_visible_mark ())
     pop_mark_0 ();

   push_visible_mark ();
}

%!%+
%\function{smart_set_mark_cmd}
%\synopsis{smart_set_mark_cmd}
%\usage{Void smart_set_mark_cmd ();}
%\description
% If the top mark is a visible mark, this function will remove that mark;
% otherwise it will push a visible mark onto the mark stack.  Use of
% this function has the effect of toggling a highlighted region.
%\seealso{set_mark_cmd, push_mark, push_visible_mark}
%!%-
define smart_set_mark_cmd ()
{
   if (is_visible_mark ())
     {
	pop_mark_0 ();
	return;
     }
   set_mark_cmd ();
}

%}}}

%{{{ buffer_format_in_columns()
%!%+
%\function{buffer_format_in_columns}
%\synopsis{buffer_format_in_columns}
%\description
% Prototype Void buffer_format_in_columns();
% takes a buffer consisting of a sigle column of items and converts the
% buffer to a multi-column format.
%!%-
define buffer_format_in_columns()
{
   push_spot_bob ();
   forever
     {
	_for (0,4,1)
	  {
	     goto_column(() * 14 + 1);
	     if (eolp())
	       {
		  if (eobp())
		    {
		       pop_spot();
		       return;
		    }
		  insert_single_space;
		  del();
	       }
	  }
	!if (down_1 ()) break;
	% bol (); % this is a side effect of going down
     }
   pop_spot();
}

%}}}

%{{{ delete_line()
define delete_line()
{
   bol(); push_mark_eol (); go_right_1 (); del_region();
}

%}}}

%{{{ set_fill_column ()
define set_fill_column ()
{
   push_spot();
   eol();
   WRAP = what_column ();
   pop_spot();
   vmessage ("WRAP column at %d.", WRAP);
}

%}}}

%{{{ rename_buffer(name)
%!%+
%\function{rename_buffer}
%\synopsis{Rename the current buffer}
%\usage{rename_buffer (String_Type new_name)}
%\description
% This function may be used to change the name of the current buffer to the
% one specified by the \var{new_name} parameter.
%\seealso{setbuf_info, whatbuf}
%!%-
define rename_buffer (name)
{
   variable flags = getbuf_info(); pop(); setbuf_info(name, flags);
}

%}}}

%{{{ deln (n)
%!%+
%\function{deln}
%\synopsis{deln}
%\usage{Void deln (Integer n);}
%\description
% delete the next 'n' characters.
%!%-
define deln (n)
{
   push_mark (); go_right(n); del_region ();
}

%}}}

%{{{ insert_spaces (n)
define insert_spaces (n)
{
   loop (n) insert_single_space ();
}

%}}}

%{{{ blooking_at (str)
define blooking_at (str)
{
   variable n = strlen (str);

   EXIT_BLOCK
     {
	pop_spot ();
     }

   push_spot ();

   if (n != left(n)) return 0;
   return looking_at (__tmp(str));
}

%}}}

%{{{ exchange_point_and_mark ()
define exchange_point_and_mark ()
{
   call ("exchange");
}

%}}}

%{{{ str_split (str, n)
% This ought to be a slang intrinsic!!!
define str_split (str, n)
{
   substr (str, 1, n - 1);
   substr (str, n, -1);
}

%}}}

#ifndef VMS
%{{{ expand_file_hook (file)
define expand_file_hook (file)
{
   variable changed = 0;
   variable envvar;
   variable pos, len, name, dir;
   variable file0, file1, file2;

   file2 = file;
   file = Null_String;
   % Check for environment variable of form $(variable)
   while (
#if (_slang_version >= 20100)
	  strlen (file2) && string_match (file2, "\\$[^/$]+", 1)
#else
	  andelse {strlen (file2)}{string_match (file2, "\\$[^/$]+", 1)}
#endif
	  )
     {
	changed++;
	(pos, len) = string_match_nth (0);
	pos++;
	(file0, file1) = str_split (file2, pos);
	(file1, file2) = str_split (file1, len + 1);

	envvar = getenv (substr (file1, 2, len - 1));
	if (envvar == NULL) envvar = "";
	file += file0 + envvar;
     }

   file += file2;

# ifdef UNIX
   % Now look for things like: /~name/...
   pos = string_match (file, "^~", 1);
   !if (pos)
     pos = -string_match (file, "/~", 1);

   if (pos)
     {
	if (pos < 0)
	  {
	     pos = -pos;
	     pos++;
	  }
#  iffalse
	pos++;
	file = substr (file, pos, strlen (file));
#  else
	file = file[[pos:]];
#  endif
	pos = is_substr (file, "/");
	if (pos)
	  {
	     (name, file) = str_split (file, pos);
	  }
	else
	  {
	     name = file;
	     file = Null_String;
	  }

	!if (strlen (name))
	  return 0;

	if (file[0] == '/') (, file) = str_split (file, 2);
	(dir,,,,) = get_passwd_info (name);
	file = dircat (dir, file);
	changed++;
     }
# endif

   if (changed)
     {
	file;
     }
   changed;
}

set_expansion_hook ("expand_file_hook");

%}}}
#endif VMS

define find_file_read_only ()
{
   call ("find_file");
   set_readonly (1);
}

%!%+
%\function{enable_dfa_syntax_for_mode}
%\synopsis{Use DFA syntax highlighting for one or more modes}
%\usage{enable_dfa_syntax_for_mode (String_Type mode, ...)}
%\description
%  This function may be used to enable the use of DFA syntax highlighting
%  for one or more specified modes.  Each of the String_Type arguments must
%  be the name of a mode.  The name of a buffer's mode is usually displayed on
%  the status line.
%\example
%  To enable DFA syntax highlighting for perl and postscript modes, use
%#v+
%     enable_dfa_syntax_for_mode ("perl", "PostScript");
%#v-
%\seealso{disable_dfa_syntax_for_mode, use_dfa_syntax}
%!%-
define enable_dfa_syntax_for_mode ()
{
   loop (_NARGS)
     {
	variable mode = ();
	mode_set_mode_info (mode, "use_dfa_syntax", 1);
     }
}

%!%+
%\function{disable_dfa_syntax_for_mode}
%\synopsis{Use DFA syntax highlighting for one or more modes}
%\usage{disable_dfa_syntax_for_mode (String_Type mode, ...)}
%\description
%  This function may be used to disable the use of DFA syntax highlighting
%  for one or more specified modes.  Each of the String_Type arguments must
%  be the name of a mode.  The name of a buffer's mode is usually displayed on
%  the status line.
%\example
%  To disable DFA syntax highlighting for C and S-Lang modes, use
%#v+
%     disable_dfa_syntax_for_mode ("C", "SLang");
%#v-
%\seealso{enable_dfa_syntax_for_mode, use_dfa_syntax}
%!%-
define disable_dfa_syntax_for_mode ()
{
   loop (_NARGS)
     mode_set_mode_info ((), "use_dfa_syntax", 0);
}

% This fixes some bug in OS2 dealing with 'dir' issued non-interactively.
#ifdef OS2
if (NULL != getenv("DIRCMD")) putenv("DIRCMD=/ogn");
#endif

() = evalfile ("os.sl");
%}}}

%---------------------------------------------------------------------------

%!%+
%\variable{Jed_Home_Directory}
%\synopsis{User's jed home directory}
%\description
%  The value of this variable specifies the user's so-called home directory
%  where personal jed-related files are assumed to be found.  Normally, this
%  corresponds to the user's home directory unless the user has specified
%  an alternate directory via the \var{JED_HOME} environment variable.
%!%-
public variable Jed_Home_Directory;
#ifdef VMS
Jed_Home_Directory = "SYS$LOGIN:";
if (NULL != getenv("JED_HOME"))
  Jed_Home_Directory = "JED_HOME:";
#else
Jed_Home_Directory = getenv ("JED_HOME");
if (Jed_Home_Directory == NULL)
  Jed_Home_Directory = getenv("HOME");
#endif
if (Jed_Home_Directory == NULL)
{
#ifdef IBMPC_SYSTEM
   % on ms systems, try USERPROFILE (win32), if not set, use C:\.
   Jed_Home_Directory = getenv("USERPROFILE");
   if (Jed_Home_Directory == NULL)
     Jed_Home_Directory = "C:\\";
#else
   Jed_Home_Directory = "";
#endif
}

private define patch_cmdline_file (file)
{
#ifdef UNIX
   variable ch = file[0];
   file = strcompress (file, "/");
   if (ch == '/')
     file = strcat ("/", file);
   return file;
#else
   file;
#endif
}

% This is the command line hook function that is called from main
define command_line_hook () %{{{
{
   variable n, i, file, depth, next_file, tmp;
   variable init_file;

   n = __argc; --n; i = 1;	       %  skip argv[0]
   if (BATCH)
     {
	if ((__argv[i] == "--help") or (__argv[i] == "-help"))
	  () = evalfile ("jedusage");

	--n; ++i; 	% -batch - 1st arg is not used
     }

   init_file = dircat (Jed_Home_Directory,
#ifdef UNIX
		       ".jedrc"
#else
		       "jed.rc"
#endif
		       );

   if (1 != file_status (init_file))
     init_file = Default_Jedrc_Startup_File;

   % if first argument is -n then do NOT load init file
   while (n)
     {
	file = __argv[i];
	if ((file == "-a") and (n > 1))
	  {
	     i++; n--;
	     init_file = __argv[i];
	     if (1 != file_status (init_file))
	       {
		  flush (strcat ("File does not exist: ", init_file));
		  usleep (2000);
	       }
	     i++; n--;
	     break;
	  }

	if ((file == "-e") and (n > 1))
	  {
	     i++; n--;
	     _Jed_Default_Emulation = __argv[i];
	     i++; n--;
	     continue;
	  }

	if (file == "-n")
	  {
	     init_file = NULL;
	     i++;
	     n--;
	     continue;
	  }

	break;
     }

   if (init_file != NULL)
     {
	if (file_status (init_file) != 1)
	  {
	     init_file = Default_Jedrc_Startup_File;
	     if (file_status (init_file) != 1)
	       init_file = "jed.rc";   %  pick up the one distributed with jed
	  }

	depth = _stkdepth ();
	() = evalfile (init_file);
	depth = _stkdepth () - depth;

	if (depth)
	  {
	     flush ("Excess junk left on stack by " + init_file);
	     usleep (1000);
	     _pop_n (depth);
	  }
     }

   % Set up defaults in case user did not do it.
   !if (BATCH)
     {
	if (_Jed_Emulation == NULL)
	  {
	     () = evalfile (_Jed_Default_Emulation);
	     enable_menu_keys ();
	  }
	if (_Jed_Color_Scheme == NULL)
	  set_color_scheme (_Jed_Default_Color_Scheme);
     }

   mini_init_minibuffer ();

#ifdef UNIX

   if (strchop (__argv[0], '/', 0)[-1] == "info")
     {
	info_reader (__argv[[1:]]);
	return;
     }
#endif
   !if (n) return;

   %
   % Is JED to emulate most?
   %
   if ("-most" == __argv[i])
     {
	run_most (i + 1);
	return;
     }

   if ("-info" == __argv[i])
     {
	info_reader (__argv[[i+1:]]);
	return;
     }

   while (n > 0)
     {
	file = __argv[i];

	--n; ++i;
	if (n)
	  {
	     next_file = __argv[i];
	     variable next_file_arg = patch_cmdline_file (next_file);
	  }

	switch (file)
	  {case "-f" and n : eval(next_file_arg);}
	  {case "-g" and n : goto_line(integer(next_file_arg));}
	  {case "-s" and n :
	     () = fsearch(next_file);
	     save_search_string(next_file);
	  }
	  {case "-l" and n : () = evalfile(next_file_arg); }
	  {case "-i" and n : () = insert_file(next_file_arg);}
	  {case "-2" : splitwindow(); ++n; --i;}
	  {case "-tmp":
	     set_buffer_no_backup ();
	     set_buffer_no_autosave ();
	     ++n; --i;
	  }
#iftrue
	  {case "-hook" and n:		% run user hook
	     variable hookfun = __get_reference (next_file);
	     if (hookfun != NULL)
	       {
		  i++;		% skip next_file
		  (@hookfun)(__argv[[i:]]);
		  return;
	       }
	  }
#endif
	  {
	     tmp = strtrans (substr (file, 3, -1), "-", "_");
	     (not (strncmp (file, "--", 2))
	      && is_defined (tmp))
	       :
	     eval (tmp);
	     ++n; --i;
	  }
	  {
	     (n && (file[0] == '+') 
	      && (Int_Type == _slang_guess_type (file))
	      && (atoi (file) >= 0)) :

	     () = find_file (next_file_arg);
	     goto_line (atoi(file));
	  }
	  {
	     flush ("Reading " + file);
	     () = find_file(patch_cmdline_file (file));  ++n; --i;
	  }

	--n; ++i;
     }
}

%}}}

#ifdef UNIX
if (getenv ("COLORTERM") == "rxvt")
  TERM_BLINK_MODE = 1;		       %  allow high-intensity bg colors
#endif

%---------------------------------------------------------------------------

#ifndef VMS			       %  FIXME for VMS
define get_executable_path (pgm)
{
   variable dir = path_dirname (pgm);
   if (path_is_absolute (dir))
     return dir;

# ifdef IBMPC_SYSTEM
   if (path_extname (pgm) == "")
     pgm += ".exe";
# endif

   if ((0 == is_substr (pgm, "/"))
# ifdef IBMPC_SYSTEM
       && (0 == is_substr (pgm, "\\"))
# endif
      )
     {
	pgm = search_path_for_file (getenv ("PATH"), pgm);
	if (pgm == NULL)
	  return NULL;
	dir = path_dirname (pgm);
	% The PATH could contain "."
	if (path_is_absolute (dir))
	  return dir;
     }

   % Relative to the CWD
   variable cwd = getcwd ();
   if (cwd == NULL)
     return NULL;
   return path_concat (cwd, dir);
}

% If jed is located in /some/install/prefix/bin/, return /some/install/prefix
private variable Jed_Install_Prefix;
private define guess_jed_install_prefix ()
{
   if (0 == __is_initialized (&Jed_Install_Prefix))
     {
	Jed_Install_Prefix = get_executable_path (__argv[0]);
	if (NULL != Jed_Install_Prefix)
	  Jed_Install_Prefix = path_dirname (Jed_Install_Prefix);
     }
   return Jed_Install_Prefix;
}

foreach ([
# ifexists _slang_install_prefix
	  _slang_install_prefix,
# endif
	  guess_jed_install_prefix ()
	 ])
{
   $1 = ();
   if ($1 == NULL)
     continue;
   $2 = path_concat ($1, "share/slsh");
   if (2 != file_status ($2))
     continue;
   append_to_slang_load_path ($2);

   $2 = path_concat ($2, "help");
   if (2 == file_status ($2))
     jed_append_doc_file ($2);

   $2 = path_concat ($1, "share/slsh/local-packages");
   if (2 == file_status ($2))
     {
	append_to_slang_load_path ($2);

	$2 = path_concat ($2, "help");
	if (2 == file_status ($2))
	  jed_append_doc_file ($2);
     }

   break;
}
#endif				       %  !VMS

%
%  This code fragment looks for the existence of "defaults.sl" and loads
%  it.  This file IS NOT distributed with JED.
%
if (strlen(expand_jedlib_file("defaults.sl")))
  () = evalfile("defaults");
#ifdef UNIX
else 
  {
     % Map /install/prefix/bin/jed to /install/prefix/etc/
     $1 = getenv ("JED_CONF_DIR");
     if ($1 == NULL)
       {
	  $1 = guess_jed_install_prefix ();
	  if ($1 != NULL)
	    {
	       $1 = path_concat ($1, "etc");
	       if (($1 == "/usr/etc") and (0 == file_status ($1)))
		 $1 = "/etc";
	    }
       }
     if ($1 != NULL)
       {
	  $1 = path_concat ($1, "jed.conf");
	  if (1 == file_status ($1))
	    () = evalfile ($1);
       }
  }
#endif

%require ("profile");
%_boseos_info = 0;
%enable_profiling ();
