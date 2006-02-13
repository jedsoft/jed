#ifndef _JED_WIN32_H_
#define _JED_WIN32_H_
extern FILE *w32_popen(char *, char *);
extern int w32_pclose(FILE *);
extern char *w32_build_command (char **, unsigned int);
extern int w32_init_subprocess_support (int);
extern int jed_init_w32_support (void);
#endif
