$! -*-dcl-*-
$! This file defines the appropriate symbols for JED.  It requires
$! two parameters.
$! -----
$! Set up or overwrite the jed_root logical.
$ this_dev= f$parse(f$env("procedure"),,,"device","no_conceal")
$ this_dir= f$parse(f$env("procedure"),,,"directory","no_conceal")
$ this_clp= f$extract(f$length(this_dir)-1,1,this_dir)
$ this_opp[0,7]= f$cvsi(0,8,this_clp)- 2
$ this_dir= this_dir- "''this_clp'''this_opp'"
$ the_2up_dir= this_dir- this_clp+ ".--"+ this_clp
$ the_root_dir= f$parse("''this_dev'''the_2up_dir'",,,"directory") -
        - this_clp+ "."+ this_clp
$ define/job /trans=conc jed_root 'this_dev''the_root_dir'
$!
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
