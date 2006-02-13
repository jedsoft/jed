$ ! Command procedure to invoke an editor for MAIL.
$ !
$ ! Inputs:
$ !
$ !	P1 = Input file name.
$ !	P2 = Output file name.
$ !
$ ! If MAIL$EDIT is undefined, MAIL will invoke callable EDT.
$ ! If MAIL$EDITis defined to be a command procedure,
$ ! MAIL will create a subprocess to edit the mail.
$ !
$ ! Note that this procedure is run in the context of a subprocess.
$ ! LOGIN.COM is not executed.  However, all process logical names
$ ! and DCL global symbols are copied.
$ !
$ ! The default directory is the same as the parent process
$ !
$!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
$!
$! jed:
$!
$!
$!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
$!
$!    *** jed ***
$!
$    VERIFY = F$VERIFY (0)
$    define/job/nolog jed_attach_to 'f$getjpi("", "PID")
$    define /user sys$input 'f$trnlnm("SYS$OUTPUT")'
$    if (p1 .EQS. "") THEN GOTO NO_INPUT_jed
$!
$       copy 'p1' 'p2'
$!
$  No_Input_Jed:
$!
$  priv_list       = f$setprv ("NOWORLD, NOGROUP")
$  pid := 'f$trnlnm("JED_PID")'
$  if (pid .eqs. "") then goto no_attach
$  context         = 0
$  Loop:
$     if (pid .eqs. f$pid ( context )) then goto attach
$     if (context .ne. 0) then goto loop
$!
$  no_attach:
$     priv_list       = f$setprv (priv_list)
$     runjed 'p2'
$     goto done
$  attach:
$     priv_list       = f$setprv (priv_list)
$     define/nolog/job jed_file_name "''p2'"
$     attach/id = 'pid'
$     deassign/job jed_file_name
$ done:
$!
$    IF (VERIFY) THEN SET VERIFY
$    EXIT
