$!------------------------make.com for Jed--------------------------------------
$! Modified VMSMAKE for JED - Andy Harper, Kings College London
$!                            Martin Zinser (zinser@decus.decus.de)
$! AH	- Use logicals to define SLANG location, if defined	[16-MAY-1996]
$! AH	- A few bug fixes!					[16-MAY-1996]
$! MPJZ - Extensive re-write to check
$!         Platform, Compiler and MMS/MMK                       [02-MAR-1998]   
$!
$ ver = f$verify(0)
$!
$ Make = ""
$!
$! Check for MMK/MMS
$!
$ If F$Search ("Sys$System:MMS.EXE") .nes. "" Then Make = "MMS"
$ If F$Type (MMK) .eqs. "STRING" Then Make = "MMK"
$!
$! Look for the compiler used
$!
$ its_decc = (f$search("SYS$SYSTEM:DECC$COMPILER.EXE") .nes. "")
$ its_vaxc = .not. its_decc .and. (f$search("SYS$SYSTEM:VAXC.EXE") .nes. "")
$ its_gnuc = .not.(its_decc .or. its_vaxc) .and. (f$trnlnm("gnu_cc").nes."")
$!
$! Exit if no compiler available
$!
$ if (.not. (its_decc .or. its_vaxc .or. its_gnuc))
$  then
$   write sys$output "C compiler required to build Jed"
$   exit
$  endif
$!
$! Override if requested from the commandline
$!
$ if (p1 .eqs. "DECC") 
$  then 
$   its_decc = .true.
$   its_gnuc = .false.
$   its_vaxc = .false.
$ endif
$ if (p1 .eqs. "VAXC") 
$  then 
$   its_decc = .false.
$   its_gnuc = .false.
$   its_vaxc = .true.
$ endif
$ if (p1 .eqs. "GCC") 
$  then 
$   its_decc = .false.
$   its_gnuc = .true.
$   its_vaxc = .false.
$ endif
$ defs = ""
$!
$! If you do not want floating point, comment out next line
$    defs = "FLOAT_TYPE,"
$!
$! S-Lang Include directory (where slang.olb is located)
$    slanglib = "[-.-.slang.src]"
$    SLANGOBJ = SLANGLIB + "SLANG.OLB"			! AH 16-MAY-1996
$
$!
$! If logical names defining the location of the slang libraries and header
$! files exist, use them in preference to the above definition of SLANGLIB
$!	-  SLANG_ROOT:[SRC] = location of header files (slang.h)
$!	-  SLANG_LIBRARY    = (if defined) location of SLANG.OLB
$!			      (if not defined) SLANG_ROOT:[SRC]SLANG.OLB used
$
$ if f$trnlnm("SLANG_ROOT") .nes. ""			! AH 16-MAY-1996
$   then						! AH 16-MAY-1996
$     SLANGLIB := "SLANG_ROOT:[SRC]"			! AH 16-MAY-1996
$     if f$trnlnm("SLANG_LIBRARY") .nes. ""		! AH 16-MAY-1996
$       then						! AH 16-MAY-1996
$         SLANGOBJ = "SLANG_LIBRARY"			! AH 16-MAY-1996
$       else						! AH 16-MAY-1996
$         SLANGOBJ = SLANGLIB  +  "SLANG.OLB"		! AH 16-MAY-1996
$     endif						! AH 16-MAY-1996
$ endif							! AH 16-MAY-1996
$!
$! Build the option files
$!
$ open/write optf jed.opt
$ open/write xoptf xjed.opt
$ write optf "Identification=""Jed B0.99-8"""
$ write xoptf "Identification=""XJed B0.99-8"""
$ write optf "''slangobj'/lib"
$ write xoptf "''slangobj'/lib"
$!
$ ccopt = "/define=(''defs'JED)/include=(''slanglib')"
$ if its_decc 
$  then 
$   C_C = "CC"
$   if f$getsyi("HW_MODEL").ge.1024
$    then
$     ccopt = "/float=ieee/name=(as_is,short)/prefix=all" + "''ccopt'"
$    else
$    ccopt = "/DECC/prefix=all" + "''ccopt'"
$ endif
$ if its_vaxc 
$  then 
$   C_C = "CC"
$ endif
$ if its_gnuc
$  then
$   C_C = "GCC"
$   ccopt = "/nocase_hack/warnings" + "''ccopt'"
$   write optf "gnu_cc:[000000]gcclib/lib"
$   write xoptf "gnu_cc:[000000]gcclib/lib"
$ endif
$!
$!Close the option files
$!
$! Note: to include decw$dwtlibshr and vaxcrtl shouldn't be necessary
$!
$ write xoptf "sys$share:decw$xlibshr/share"
$ close optf
$ close xoptf
$ if (p1 .eqs. "INSTALL") then goto install
$ bfiles = "abbrev," -
	+ "buffer," -
	+ "cmds," -
	+ "file," -
	+ "indent," -
	+ "ins," -
	+ "intrin," -
	+ "keymap," -
	+ "ledit," -
	+ "line," -
	+ "lineattr," -
	+ "main," -
	+ "misc," -
	+ "paste," -
	+ "replace"
$!
$ bfiles = bfiles -
	+ ",screen," -
	+ "search," -
	+ "sig," -
	+ "syntax," -
	+ "sysdep," -
	+ "text," -
	+ "undo," -
	+ "vfile," -
	+ "vmshelp," -
	+ "vmsmail," -
	+ "vterm," -
	+ "blocal," -
	+ "mouse," -
	+ "menu," -
	+ "jedwin," -
	+ "window"
$ bfiles = bfiles -
	+ ",colors" -
	+ ",lock" -
	+ ",hooks" -
	+ ",version" -
	+ ",userinfo" -
	+ ""
$!
$ jfiles = bfiles + ",display"
$ xfiles = bfiles + ",xterm"
$!
$ copy jedconf.h config.h
$ purge config.h
$!
$ if (p2 .eqs. "XJED") then goto make_xjed_label
$!
$ call simple_make 'jfiles
$ write sys$output "Linking jed"
$ link/exec=jed.exe 'jfiles',jed.opt/opt
$!
$ make_xjed_label:
$  call simple_make 'xfiles
$  write sys$output "Linking Xjed"
$  link/exec=xjed.exe 'xfiles,xjed.opt/opt
$!
$  if (ver .eq. 1) then set verify
$  exit
$!
$!
$!  Installation
$!  
$ Install:
$   write sys$output "Sorry, no installation implemented yet."
$!-----------------------------------------------------------------------
$!  Note: The installation requires the following director structure:
$!    JED_ROOT:[lib]
$!    JED_ROOT:[lib.tm]
$!    JED_ROOT:[lib.colors]
$!    JED_ROOT:[info]
$!    JED_ROOT:[doc]
$!    JED_ROOT:[bin]
$!  See Makefile.in for what should be copied to these directories.
$!-----------------------------------------------------------------------
$!   destdef = f$trnlnm("JED_ROOT")
$!   again:
$!   def = ""
$!   if (destdef .nes. "") then def = "(default ''destdef')"
$!   write sys$output "Installation Directory"
$!   inquire dest "''def'"
$!   if (dest .eqs. "") then dest = destdef
$!   if (dest .eqs. "") then goto again
$!   if (f$parse(dest) .nes. "") then goto do_copy
$!      write sys$output "Directory does not exist.   Creating it."
$!      create/dir/prot=(w:re) 'dest'
$!  do_copy:
$!   copy/log/prot=(w:re) *.sl, *.hlp, *.rc, *.info, jed.*in, jed.exe, *.com  -
 !     'dest'
$!
$!  simple make
$!
$  simple_make: Subroutine !Subroutine to build from file-list
$!
$! P1 = List of files to compile
$!
$  files = p1
$  count = 0
$  next_file:
$    f = f$element(count, ",", files)
$    count = count + 1
$    if (f .eqs. ",") then goto exit_sub
$    objf = f$search("''f'.obj")
$    if (objf .eqs. "") then goto compile_it
$    tobj = f$file_attr(objf, "RDT")
$    tc  = f$file_attr("''f'.c", "RDT")
$    if (f .eqs. "sysdep")
$    then
$      if ( f$cvtime(tobj) .lts. f$cvtime(f$file_attr("vms.c","RDT"))) -
	 then goto compile_it
$    endif
$    if (f$cvtime(tc) .lts. f$cvtime(tobj)) then goto next_file
$  compile_it:  
$    write sys$output "''C_C' ''ccopt' ''f'.c"
$    'C_C''ccopt' 'f'.c
$    goto next_file
$  exit_sub:
$ENDSUBROUTINE
