/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1999-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

/*{{{ Include Files */
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <time.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <slang.h>
#include "jdmacros.h"

#ifdef REAL_UNIX_SYSTEM
# define HAS_TCPIP_CODE		1
# include <netdb.h>
# ifndef h_errno
extern int h_errno;
# endif
# define RESOLVE_DOT_CONF	"/etc/resolv.conf"
#else
# define HAS_TCPIP_CODE		0
#endif

#ifdef REAL_UNIX_SYSTEM
# include <pwd.h>
# define HAS_PASSWORD_CODE	1
#else
# define HAS_PASSWORD_CODE	0
#endif

#include "userinfo.h"
#include "misc.h"

/*}}}*/

typedef struct
{
   char *username;
   char *realname;
   char *hostname;
}
User_Info_Type;

User_Info_Type User_Info;

static int set_user_info (char **what, char *value)
{
   if (NULL == (value = SLang_create_slstring (value)))
     return -1;
   SLang_free_slstring (*what);   /* NULL ok */
   *what = value;
   return 0;
}

static int set_hostname (char *name)
{
   if (*name == 0) name = "localhost";
   return set_user_info (&User_Info.hostname, name);
}

static int set_username (char *name)
{
   return set_user_info (&User_Info.username, name);
}

static int set_realname (char *name)
{
   return set_user_info (&User_Info.realname, name);
}

static int is_fqdn (char *h) /*{{{*/
{
   char *p;

   p = strchr (h, '.');
   if ((p == NULL) || (p == h))
     return 0;

   if ((unsigned int)((p - h) + 1) == strlen (h))
     return 0;

   /* We may want something more sophisticated here */
   return 1;
}

/*}}}*/

#ifdef RESOLVE_DOT_CONF
static char *skip_whitespace_chars (char *b)
{
   while ((*b == ' ') || (*b == '\t') || (*b == '\n'))
     b++;
   return b;
}

static char *skip_non_whitespace_chars (char *b)
{
   while (*b && (*b != ' ') && (*b != '\t') && (*b != '\n'))
     b++;
   return b;
}
#endif

static int get_domainname (char *dom, unsigned int domlen)
{
#ifdef RESOLVE_DOT_CONF
   FILE *fp;
   char buf[1024];

   if (NULL == (fp = fopen (RESOLVE_DOT_CONF, "r")))
     return -1;

   while (NULL != fgets (buf, sizeof (buf), fp))
     {
	char *d, *b;

	b = buf;

	if (*b == '#')
	  continue;

	b = skip_whitespace_chars (b);

	if (0 != strncmp ("domain", b, 6))
	  continue;

	b += 6;
	if ((*b != ' ') && (*b != '\t'))
	  continue;

	b = skip_whitespace_chars (b);

	d = b;
	b = skip_non_whitespace_chars (b);

	if (b == d) continue;

	*b = 0;

	strncpy (dom, d, domlen);
	dom[domlen-1] = 0;
	fclose (fp);
	return 0;
     }
   fclose (fp);
   return -1;
#else
   (void) dom;
   (void) domlen;
   return -1;
#endif
}

static char *combine_host_and_domain (char *a, char *b)
{
   unsigned int len;
   char *c, *cc;

   len = strlen (a) + strlen (b) + 2;
   if (NULL == (c = SLmalloc (len)))
     return NULL;

   if (*b == '.') b++;
   sprintf (c, "%s.%s", a, b);

   cc = SLang_create_slstring (c);
   SLfree (c);
   return cc;
}

#ifndef HAVE_GETHOSTNAME
#define gethostname _my_gethostname
static int _my_gethostname (char *buf, unsigned int len)
{
   (void) buf;
   (void) len;
   return -1;
}
#endif

/* Try to get a fully qualified domain name. */
static char *get_hostname (void)
{
#if HAS_TCPIP_CODE
   struct hostent *host_entry = NULL;
#endif
   char buf[256], *b;
   char domain_name[256];

#ifdef JED_HOSTNAME
   if (is_fqdn (JED_HOSTNAME))
     return SLang_create_slstring (JED_HOSTNAME);
#endif

   b = buf;

   if ((-1 == gethostname (buf, sizeof (buf)))
       || (*buf == 0))
     {
	b = getenv ("HOSTNAME");
	if ((b == NULL) || (*b == 0))
	  return NULL;
     }

   /* The machine that jed is running on may not have access to
    * a DNS, e.g., the node on a compute cluster.  So, it the value
    * returned by gethostname looks to be a FQDN, then use it.
    */
   if (is_fqdn (buf))
     return SLang_create_slstring (buf);

#if HAS_TCPIP_CODE
   host_entry = gethostbyname (b);

#if defined(TRY_AGAIN) && !defined(MULTINET)
   if ((host_entry == NULL) && (h_errno == TRY_AGAIN))
     {
	sleep (2);
	host_entry = gethostbyname (b);
     }
#endif
   if ((host_entry != NULL)
       && (host_entry->h_name != NULL)
       && (host_entry->h_name[0] != 0))
     {
	char **aliases;

	if (is_fqdn ((char *)host_entry->h_name))
	  return SLang_create_slstring ((char *)host_entry->h_name);

	if (NULL != (aliases = host_entry->h_aliases))
	  {
	     while (*aliases != NULL)
	       {
		  if (is_fqdn (*aliases))
		    return SLang_create_slstring (*aliases);
		  aliases++;
	       }
	  }

	/* We have no FQDN */
	b = (char *)host_entry->h_name;
     }
#endif				       /* HAS_TCPIP_CODE */

   if (*b == 0)
     return NULL;

   if ((0 == is_fqdn (b))
       && (0 == get_domainname (domain_name, sizeof (domain_name))))
     {
	return combine_host_and_domain (b, domain_name);
     }

   /* Oh well */
   return SLang_create_slstring (b);
}

static int get_hostname_info (void)
{
   char *host;

   host = get_hostname ();
   if (host != NULL)
     {
	if (-1 == set_hostname (host))
	  {
	     SLang_free_slstring (host);
	     return -1;
	  }
	SLang_free_slstring (host);
	return 0;
     }

   if (SLang_get_error ())
     return -1;

   return set_hostname ("localhost");
}

static int get_username_info (void)
{
   char *name = NULL;
#if HAS_PASSWORD_CODE
   struct passwd *pw;
#endif

#if HAS_PASSWORD_CODE
   /* I cannot use getlogin under Unix because some implementations
    * truncate the username to 8 characters.  Besides, I suspect that
    * it is equivalent to the following line.
    */
   /* The man page for getpwuid indicate that the pointer _may_ be to a static
    * area.  So, there is nothing to free here.
    */
   pw = getpwuid (getuid ());
   if (pw != NULL)
     name = pw->pw_name;
#endif

   if (((name == NULL) || (*name == 0))
       && ((name = getenv("USER")) == NULL)
       && ((name = getenv("LOGNAME")) == NULL))
     name = "unknown";

   if (-1 == set_username (name))
     return -1;

   name = getenv ("NAME");
#if HAS_PASSWORD_CODE
   if ((name == NULL)
       && (pw != NULL)
       && (pw->pw_gecos != NULL))
     name = pw->pw_gecos;
#endif
   if (name == NULL)
     name = "";

   if (-1 == set_realname (name))
     return -1;

   return 0;
}

static int get_user_info (void)
{
   if (-1 == get_hostname_info ())
     return -1;

   if (-1 == get_username_info ())
     return -1;

   return 0;
}

char *jed_get_username (void)
{
   return SLang_create_slstring (User_Info.username);
}

char *jed_get_hostname (void)
{
   return SLang_create_slstring (User_Info.hostname);
}

static void get_passwd_cmd (char *name) /*{{{*/
{
   char *password = NULL;
   char *dir = NULL;
   char *shell = NULL;
   int uid = -1, gid = -1;
#if HAS_PASSWORD_CODE
   struct passwd *pwent;

   if (*name == 0)
     pwent = getpwuid (getuid ());
   else
     pwent = getpwnam (name);

   if (pwent != NULL)
     {
	password = pwent->pw_passwd;
	uid = pwent->pw_uid;
	gid = pwent->pw_gid;
	dir = pwent->pw_dir;
	shell = pwent->pw_shell;
     }
#endif
   if (password == NULL) password = "";
   if (dir == NULL) dir = "";
   if (shell == NULL) shell = "";

   (void) SLang_push_string (dir);
   (void) SLang_push_string (shell);
   (void) SLang_push_string (password);
   (void) SLang_push_integer (uid);
   (void) SLang_push_integer (gid);
}

/*}}}*/

static char *get_username_cmd (void)
{
   return User_Info.username;
}
static char *get_realname_cmd (void)
{
   return User_Info.realname;
}
static char *get_hostname_cmd (void)
{
   return User_Info.hostname;
}
static void set_username_cmd (char *s)
{
   (void) set_username (s);
}
static void set_realname_cmd (char *s)
{
   (void) set_realname (s);
}
static void set_hostname_cmd (char *s)
{
   (void) set_hostname (s);
}

static SLang_Intrin_Fun_Type User_Intrinsics [] =
{
   MAKE_INTRINSIC_S("get_passwd_info", get_passwd_cmd, VOID_TYPE),
   MAKE_INTRINSIC_0("get_username", get_username_cmd, SLANG_STRING_TYPE),
   MAKE_INTRINSIC_0("get_hostname", get_hostname_cmd, SLANG_STRING_TYPE),
   MAKE_INTRINSIC_0("get_realname", get_realname_cmd, SLANG_STRING_TYPE),

   MAKE_INTRINSIC_S("set_username", set_username_cmd, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_S("set_hostname", set_hostname_cmd, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_S("set_realname", set_realname_cmd, SLANG_VOID_TYPE),

   MAKE_INTRINSIC_0(NULL, NULL, 0)
};

int jed_init_userinfo (void)
{
   (void) get_user_info ();

   if (-1 == SLadd_intrin_fun_table (User_Intrinsics, NULL))
     return -1;

   return 0;
}
