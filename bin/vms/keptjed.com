$ verify = f$verify (0)
$!
$!  Kept_Jed.com --- basede on kepteditor.com by Joe Kelsey
$!
$!  Defines the PF1 key to reattach instantly instead of going back
$!  through this file again.
$!
$ if p1 .eqs. "-BATCH"
$ then
$   runjed 'p1' 'p2' 'p3' 'p4' 'p5' 'p6' 'p7' 'p8' 'p9'
$   exit
$ endif
$!
$ name            = "JED " + f$trnlnm ( "TT" ) - ":"
$ priv_list       = f$setprv ("NOWORLD, NOGROUP")
$ context         = 0
$ Loop:
$   pid             = f$pid ( context )
$   proc            = f$getjpi (pid, "PRCNAM")
$   if proc .eqs. name then goto attach
$   if context .ne. 0 then goto Loop
$!
$  args = p1 + " " + p2 + " " + p3 + " " -
     + p4 + " " + p5 + " " + p6 + " " + p7 + " " + p8
$  priv_list = f$setprv ( priv_list )
$  write sys$error  "[Spawning a new Kept JED]"
$  define/user sys$input sys$command
$  spawn/nolog/process="''name'" runjed 'args'
$!
$!  define gold key (PF1) to immediately attach
$!
$ context = 0    
$ gold_loop:
$   pid = f$pid ( context )
$   proc = f$getjpi (pid, "PRCNAM")
$   if proc .eqs. name then goto gold_loop_exit
$   if context .ne. 0 then goto gold_loop
$   goto quit             !could not find jed process so we quit
$!
$ gold_loop_exit:
$   define/key/nolog/terminate/noecho pf1  "attach/ident=''pid'"
$   define/nolog/job JED_PID "''pid'"
$   goto quit
$!
$!
$ attach:
$    priv_list       = f$setprv ( priv_list )
$    message_status = f$environment("message")
$    set noon
$    set message /nofacility/noidentification/noseverity/notext
$    set on
$    set message 'message_status
$    if p1 .eqs. "" then goto no_logical
$    temp = f$trnlnm("SYS$DISK") + f$directory() + p1
$    temp = f$edit(temp,"lowercase")
$    define/nolog/job jed_file_name "''temp'"
$    no_logical:
$    write sys$error "[Attaching to process ''NAME']"
$    define/user sys$input sys$command
$    attach "''NAME'"
$!
$ quit:
$!
$    message_status = f$environment("message")
$    set noon
$    set message /nofacility/noidentification/noseverity/notext
$    deassign/job jed_file_name
$    set on
$    set message 'message_status
$    write sys$error -
       "[Attached to DCL in directory ''F$TRNLNM("SYS$DISK")'''F$DIRECTORY()']"
$    if verify then set verify
$    exit
    
