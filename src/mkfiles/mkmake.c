#include <stdio.h>
#include <slang.h>
#include <stdlib.h>

static void usage (void)
{
   fprintf (stderr, "Usage: mkmake [DEF1 [DEF2 ...]]\n");
   exit (1);
}

   
int main (int argc, char **argv)
{
   char buf[1024];
   int i;
   SLPreprocess_Type pt;
   
   if (isatty (0))
     usage ();
   
   SLprep_open_prep (&pt);

   pt.preprocess_char = '!';
   pt.comment_char = '#';
   pt.flags = SLPREP_BLANK_LINES_OK | SLPREP_COMMENT_LINES_OK;
   
   for (i = 1; i < argc; i++)
     SLdefine_for_ifdef (argv[i]);
   
   while (NULL != fgets (buf, sizeof (buf) - 1, stdin))
     {
	if (SLprep_line_ok (buf, &pt))
	  {
	     fputs (buf, stdout);
	  }
     }
   
   SLprep_close_prep (&pt);
   return 0;
}
   
