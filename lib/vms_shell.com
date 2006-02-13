$!
$!  This is used by JED to execute a command in the shell.  I do
$!  it this way because not every command takes a /output qualifier
$!
$ define/user/nolog sys$error sys$output
$ define/user/nolog sys$input nl:
$ 'p1' 'p2' 'p3' 'p4' 'p5' 'p6' 'p7' 'p8' 'p9' 
$ exit
