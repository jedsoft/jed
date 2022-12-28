#include <stdio.h>
#include <slang.h>
#include <stdlib.h>
#include <string.h>

char *SLmalloc (SLstrlen_Type len)
{
   char *m = malloc (len);
   if (m == NULL)
     (void) fprintf (stderr, "Out of memory\n");
   return m;
}

SLFUTURE_VOID *SLcalloc (SLstrlen_Type n, SLstrlen_Type len)
{
   SLFUTURE_VOID *p = SLmalloc (n*len);
   if (p != NULL)
     memset (p, 0, n*len);
   return p;
}

void SLfree (SLFUTURE_VOID *s)
{
   if (s != NULL) free (s);
}

void SLang_free_slstring (SLCONST SLstr_Type *s)
{
   if (s != NULL) free ((void *)s);
}

char *SLang_create_slstring (char *s)
{
   char *t;

   t = SLmalloc (strlen (s) + 1);
   if (t == NULL)
     return t;

   strcpy (t, s);
   return t;
}


static void usage (void)
{
   fprintf (stderr, "Usage: mkmake [DEF1 [DEF2 ...]]\n");
   exit (1);
}

int main (int argc, char **argv)
{
   char buf[1024];
   int i;
   SLprep_Type *pt;

   if (isatty (0))
     usage ();

   if (NULL == (pt = SLprep_new ()))
     return 1;

   if ((-1 == SLprep_set_prefix (pt, "!"))
       || (-1 == SLprep_set_comment (pt, "#", ""))
       || (-1 == SLprep_set_flags (pt, SLPREP_BLANK_LINES_OK | SLPREP_COMMENT_LINES_OK)))
     {
	SLprep_delete (pt);
	return 1;
     }

   for (i = 1; i < argc; i++)
     SLdefine_for_ifdef (argv[i]);

   while (NULL != fgets (buf, sizeof (buf) - 1, stdin))
     {
	if (SLprep_line_ok (buf, pt))
	  {
	     fputs (buf, stdout);
	  }
     }

   SLprep_delete (pt);
   return 0;
}

