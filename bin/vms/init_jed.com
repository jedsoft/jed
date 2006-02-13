$! This file defines the appropriate symbols for JED.  It requires
$! two parameters:
$ define/job jed_library jed_root:[lib]
$ jedexe = "jed_root:[bin.vms]jed." + f$getsyi("ARCH_NAME") + "_EXE"
$ if (p1 .nes "0") .and. (p1 .nes. "1") then goto USAGE
$ if (p2 .nes "0") .and. (p2 .nes. "1") then goto USAGE
$ if (p1 .eqs. "1") then goto KEPT
$!
$!   simple editor
$!
$ runjed :== $'jexexe'
$ jed :== $'jedexe'
$ goto do_mail
$!
$ kept:
$  runjed :== $'jedexe'
$  jed :== @jed_root:[bin.vms]keptjed.com
$ do_mail:
$  if (p2 .nes. "1") then goto done
$  define/job mail$edit "@jed_root:[bin.vms]jed_mail.com"
$  done:
$   exit
$!
$ usage:  type sys$input

    This command procedure requires two parameters with the value of
    either 0 or 1.  If first parameter is 1, it indicates that JED is
    to run as a kepteditor otherwise JED is run as a normal executable.
    If the second parameter has a value of 1, JED is used as the editor
    for mail.  So, the following are valid:

       @jed_root:[bin.vms]init_jed 0 0   !normal/no mail
       @jed_root:[bin.vms]init_jed 0 1   !normal/mail
       @jed_root:[bin.vms]init_jed 1 0   !kept/nomail  <-- most systems
       @jed_root:[bin.vms]init_jed 1 1   !kept/mail

$ exit
