/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#include <ssdef.h>
#include <string.h>

#include "buffer.h"
#include "vmsmail.h"

extern int mail$send_add_bodypart ();
extern int mail$send_begin ();
extern int mail$send_add_attribute ();
extern int mail$send_add_address ();
extern int mail$send_message ();
extern int mail$send_end ();

static void fill_struct(Mail_Type *m, int act, char *s) /*{{{*/
{
   m->code = act;
   m->buflen = strlen(s);
   m->addr = (long) s;
   m->junk = m->ret = 0;
}

/*}}}*/

static int vms_send_buffer(int *context, Mail_Type *mt0, Buffer *b) /*{{{*/
{
   Mail_Type m;
   Line *l = b->beg;
   int n = 0, len;
   unsigned char *p;

   while (l != NULL)
     {
	m.code = MAIL$_SEND_RECORD;
	len = l->len;
	p = l->data;
	if (len && ('\n' == *(p + (int)(len - 1)))) len--;
	m.buflen = len;
	m.addr = (long) p;
	m.junk = m.ret = 0;
	if (SS$_NORMAL != mail$send_add_bodypart(context, &m, mt0)) return(0);
	l = l->next;
	n++;
     }
   return(n);
}

/*}}}*/

/* to might be a comma separated list--- parse it too */
int vms_send_mail(char *to, char *subj) /*{{{*/
{
   Mail_Type mt0, mt;
   int context = 0;
   char *p;

   mt0.code = mt0.buflen = mt0.addr = mt0.ret = mt0.junk = 0;

   if (SS$_NORMAL != mail$send_begin(&context, &mt0, &mt0))
     {
	return(0);
     }
#if 0
   fill_struct(&mt, MAIL$_SEND_TO_LINE, to);
   if (SS$_NORMAL != mail$send_add_attribute(&context, &mt, &mt0))
     {
	return(0);
     }

   fill_struct(&mt, MAIL$_SEND_USERNAME, to);
   if (SS$_NORMAL != mail$send_add_address(&context, &mt, &mt0))
     {
	return(0);
     }
#endif
   while (1)
     {
	while (*to && ((*to <= ' ') || (*to == ','))) to++;
	if (*to == 0) break;
	p = to;
	while ((*p > ' ') && (*p != ',')) p++;

        mt.code = MAIL$_SEND_TO_LINE;
	mt.buflen = p - to;
	mt.ret = mt.junk = 0;
	mt.addr = (long) to;

	if (SS$_NORMAL != mail$send_add_attribute(&context, &mt, &mt0))
	  {
	     return(0);
	  }

	mt.code = MAIL$_SEND_USERNAME;
	mt.buflen = p - to;
	mt.ret = mt.junk = 0;
	mt.addr = (long) to;

	if (SS$_NORMAL != mail$send_add_address(&context, &mt, &mt0))
	  {
	     return(0);
	  }
	to = p;
     }

   fill_struct(&mt, MAIL$_SEND_SUBJECT, subj);
   if (SS$_NORMAL != mail$send_add_attribute(&context, &mt, &mt0))
     {
	return(0);
     }

   if (!vms_send_buffer(&context, &mt0, CBuf))
     {
	return(0);
     }

   if (SS$_NORMAL != mail$send_message(&context, &mt0, &mt0))
     {
	return(0);
     }

   if (SS$_NORMAL != mail$send_end(&context, &mt0, &mt0))
     {
	return(0);
     }
   return(1);
}

/*}}}*/

