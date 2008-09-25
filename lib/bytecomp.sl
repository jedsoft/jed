% bytecomp.sl	-*- SLang -*-
%
% !! WARNING: It is not a good idea to invoke this file directly.
%             Instead load it via preparse.sl.  This may be performed
%             in batch mode as:
%
%      jed -batch -n -l preparse
%
%

!if (is_defined ("Preprocess_Only"))
{
   variable Preprocess_Only = 0;
}

define jed_byte_compile_file (f)
{
   variable file;
   
   file = expand_jedlib_file(f);
   if (strlen (file))
     {
	% flush (strcat ("Processing ", file));
	byte_compile_file (file, Preprocess_Only);
     }
   else flush (strcat (f, " not found"));
}

#ifnexists __load__bytecomp__only__
$0 = _stkdepth ();

% list of file to byte compile:
# ifdef UNIX VMS
"mail.sl"; 
"iso-latin.sl";
%   "dispesc.sl";
# endif
# ifdef UNIX
"rmail.sl";
"sendmail.sl";
"mailalias.sl";
"mime.sl";
"perl.sl";
"perlxtra.sl";
"compress.sl";
# endif
# ifdef IBMPC_SYSTEM
"dos437.sl"; "dos850.sl";  "dos852.sl"; "brief.sl";
"win1250.sl";
# endif
# ifdef XWINDOWS MOUSE
"mouse.sl";
# endif

# ifdef HAS_LINE_ATTR
"folding.sl";
# endif

#ifdef VMS
"vmshelp.sl";
#endif
if (is_defined ("KILL_ARRAY_SIZE")) 
{
   "yankpop.sl";
   "register.sl";
}
"sccs.sl";
"rcs.sl";
"minued.sl";
"history.sl";
"chglog.sl";
"wmark.sl";
"occur.sl";
"javamode.sl";
"modehook.sl";
"hooks.sl";
"nroff.sl";
"html.sl";
"docbook.sl";
"idl.sl";
"shmode.sl";
"mousex.sl";
"abbrev.sl";
"abbrmisc.sl";
"dabbrev.sl";
"mutekeys.sl";
"bookmark.sl";
"replace.sl";
"srchmisc.sl";
"texcom.sl";
"tex.sl";
"bibtex.sl";
"latex.sl";
"latex209.sl";
"ltx-math.sl";
"binary.sl";
"isearch.sl";
"rot13.sl";
"tabs.sl";
"untab.sl";
"jedhelp.sl";
"ctags.sl";
"compile.sl";
"menu.sl";
"dired.sl";
"util.sl";
"syntax.sl";
"tmisc.sl";
"cmisc.sl";
"misc.sl";
"help.sl";
"cal.sl";
"man.sl";
"fortran.sl";
"dcl.sl";
"shell.sl";
"most.sl";
"info.sl";
"ispell.sl";
"sort.sl";
"regexp.sl";
"wordstar.sl";
"buf.sl";
"emacsmsc.sl";
"ide.sl";
"cmode.sl";
"php.sl";
"slmode.sl";
"search.sl";
%"linux.sl";
"textmode.sl";
"modeinfo.sl";
"comments.sl";
"keydefs.sl";
"jed.sl";
"jedusage.sl";
if (is_defined ("menu_create_menu_bar") == 1)   %  intrinsic fun
{
   "menus.sl";
   "popups.sl";
}
"mini.sl";
"edt.sl";
"emacs.sl";
"site.sl";

$0 = _stkdepth () - $0;

loop ($0)
{
   jed_byte_compile_file (());
}


$0 = _stkdepth ();
"tclmode.sl";
"ashell.sl";
"bufed.sl";
"macro.sl";
"filter.sl";
%"keycode.sl";
"wmenu.sl";
"lisp.sl";
"pscript.sl";
"acompile.sl";
"digraph.sl";
"cua.sl";
"cuamisc.sl";
"f90.sl";
"ff90.sl";
"iso-lat2.sl";
"iso-lat3.sl";
"iso2xxx.sl";
"krconv.sl";
"maple.sl";
"pipe.sl";
"pushmode.sl";
"pymode.sl";
"seldisp.sl";
"sortmisc.sl";
"verilog.sl";
"spicemod.sl";
"vhdlmode.sl";
"tiasm.sl";
"backups.sl";
"matlab.sl";
"recent.sl";
"tpascal.sl";
"aprocess.sl";
"tmmode.sl";
"xformreg.sl";
"gpg.sl";
"paste.sl";
"syncproc.sl";
"tmpfile.sl";

$0 = _stkdepth () - $0;

loop ($0)
{
   jed_byte_compile_file (());
}


% Now do color schemes
# ifndef VMS
_debug_info = 1;
private define do_color_schemes ()
{
   foreach (strtok (Color_Scheme_Path, ","))
     {
	variable dir = ();
	variable files = listdir (dir);
	variable file;
	variable i;
   
	if (files == NULL)
	  continue;
   
	i = where (array_map (Int_Type, &string_match, files, "\\.sl$", 1));
	!if (length (i))
	  continue;
	
	foreach (files[i])
	  {
	     file = ();
	     byte_compile_file (dircat (dir, file), Preprocess_Only);
	  }
     }
}

do_color_schemes ();
# endif
#endif				       %  __load__bytecomp__only__

