#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>

#ifdef __MINGW32__
# ifdef NO_GLOBBING
int _CRT_glob = 0;
# endif
#endif

/* The more I learn about this OS, the less I care for it.  This program
 * is necessary to run pipes.  Yuk!
 */
static char *build_cmd (char **args, unsigned int num)
{
   unsigned int len;
   unsigned int i;
   char *buf;
   
   len = 0;
   for (i = 0; i < num; i++)
     len += strlen (args[i]) + 1;

   if (NULL == (buf = malloc (len + 1)))
     {
	fprintf (stderr, "Out of memory\n");
	return NULL;
     }
   
   len = 0;
   for (i = 0; i < num; i++)
     {
	strcpy (buf + len, args[i]);
	len += strlen (args[i]);
	buf[len] = ' ';
	len++;
     }
   buf[len] = 0;
   return buf;
}

int main (int argc, char **argv)
{
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   char *cmd;

   if (argc == 1)
     {
	fprintf (stderr, "Usage: %s CMD", argv[0]);
	return 1;
     }
   
   if (NULL == (cmd = build_cmd (argv + 1, argc-1)))
     return 1;

   memset ((char *) &si, 0, sizeof (STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = STARTF_USESTDHANDLES;
   si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
   si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
   si.hStdError = GetStdHandle (STD_ERROR_HANDLE);

   if (FALSE == CreateProcess (NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL,
			       &si, &pi))
     {
	fprintf (stderr, "%s: Command '%s' failed to run\n", argv[0], cmd);
	free (cmd);
	return 1;
     }

   WaitForSingleObject (pi.hProcess, INFINITE);
   CloseHandle (pi.hProcess);
   CloseHandle (pi.hThread);
   free (cmd);
   return 0;
}
