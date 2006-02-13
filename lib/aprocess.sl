define aprocess_stringify_status (pid, flags, status)
{
   variable str;

   switch (flags)
     { case 2: "stopped by signal";}
     { case 4: "exit";}
     { case 8: "killed by signal";}
     {
	"??";
     }
   str = ();
   return sprintf ("%s %d", str, status);
}
