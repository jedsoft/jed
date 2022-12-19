/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

/* vms_get_help.c */

#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#include <slang.h>

#include "jdmacros.h"

#include <string.h>
#include <descrip.h>

/* Not all systems have this (GCC) so I am not using it. */
/* #include <hlpdef.h> */
#define HLP$M_PROMPT 1
#define HLP$M_LIBLIST 16

#include "buffer.h"
#include "ins.h"
#include "cmds.h"

/*	Need to pass strings by descriptor to LBR$OUTPUT_HELP;	*/
/*	string descriptors in SYS$LIBRARY:DESCRIP.H.		*/

/*	Change definition of $DESCRIPTOR macro. In descrip.h,	*/
/*	sizeof(string)-1 used, but sizeof() bites on strings	*/
/*	with spaces, or with '$'. That means that the filename	*/
/*	might be truncated (i.e., 'SYS$HELP:HELPLIB.OLB'	*/
/*	becomes 'SYS'). Ditto the topic (i.e., 'SET HOST /LOG'	*/
/*	becomes 'SET').						*/

#define $STR_DESC(x,y) \
  (x).dsc$w_length = strlen(y);\
  (x).dsc$b_dtype = DSC$K_DTYPE_T;\
  (x).dsc$b_class = DSC$K_CLASS_S;\
  (x).dsc$a_pointer = (y)

/*--------------------------------------------------------------*/
/*								*/
/*	vms_get_help: call the librarian utility to get help	*/
/*								*/
/*	input:							*/
/*	char helpfile  - name of the help file to use		*/
/*	char helptopic - name of topic to use initially		*/
/*			 (Since HLP$M_PROMPT is set, user will	*/
/*			  get interactive prompting once in.)	*/
/*	returns:						*/
/*	int	       - 0 for failure, 1 for success		*/
/*								*/
/*	Translated from a FORTRAN subroutine. That subroutine	*/
/*	was gutted from several of J.W. Pflugrath's routines	*/
/*	for I/O, et cetera.					*/
/*								*/
/*	Jim Clifton	Brandeis University			*/
/*	J.W. Pflugrath	Cold Spring Harbor Laboratory		*/
/*								*/
/*--------------------------------------------------------------*/

int vms_get_help(char *helpfile, char *helptopic) /*{{{*/
{
   int LBR$OUTPUT_HELP(), output_help_to_buf(), input_new_helptopic();
   int istat;
   unsigned int flags;
   struct dsc$descriptor_s topnam, filnam;

   $STR_DESC(topnam,helptopic);
   $STR_DESC(filnam,helpfile);

   flags = HLP$M_PROMPT;

   istat = LBR$OUTPUT_HELP(&output_help_to_buf,
			   0,  /* output width 0 is 80 */
			   &topnam,  /* topic */
			   &filnam, /* lib file */
			   &flags,
			   /* Also try:
						   M_LIBLIST
						   M_HELP */
			   &input_new_helptopic);
   if (istat != 1) return 0;
   return 1;
}

/*}}}*/

/*----------------------------------------------------------------------*/
/*									*/
/*	Here we mimic the syntax of LIB$PUT_OUTPUT			*/
/*									*/
/* VMS system procedures (including the RTL) pass strings by		*/
/* descriptor. That's what LBR$OUTPUT_HELP expects (demands!). This 	*/
/* routine (and the next) just function to integrate LBR$OUTPUT_HELP	*/
/* I/O into jed I/O.							*/
/*									*/
/*	Jim Clifton	Brandeis University				*/
/*									*/
/*----------------------------------------------------------------------*/

int output_help_to_buf(struct dsc$descriptor_s *string) /*{{{*/
{
   (void) jed_insert_nbytes ((unsigned char *) string->dsc$a_pointer, string->dsc$w_length);
   (void) jed_insert_newline ();
   return(1);	/* should add codes that LIB$PUT_OUTPUT can return */
}

/*}}}*/

/*----------------------------------------------------------------------*/
/*									*/
/*	Mimic the syntax of LIB$GET_INPUT()				*/
/*									*/
/* Note that the first 2 characters of the LBR$OUTPUT_HELP prompt are	*/
/* "^M^J", so we send read_from_minibuffer the address of the 3rd char.	*/
/*									*/
/*	Jim Clifton	Brandeis University				*/
/*									*/
/*----------------------------------------------------------------------*/

int input_new_helptopic(struct dsc$descriptor_s *newtopic, /*{{{*/
			struct dsc$descriptor_s *prompt,
			unsigned int *len_newtopic)
{
   char *newtop, prompt_buf[256];
   int dofree;

   strncpy(prompt_buf,prompt->dsc$a_pointer,prompt->dsc$w_length);
   prompt_buf[prompt->dsc$w_length] = 0;

   newtop = "";
   dofree = 0;

   if ((SLang_run_hooks("vms_help_newtopic", 1, prompt_buf + 2) > 0)
       && (0 == SLpop_string (&newtop)))
     dofree = 1;

   strcpy (newtopic->dsc$a_pointer, newtop);
   *len_newtopic = newtopic->dsc$w_length = strlen (newtop);
   newtopic->dsc$b_dtype = DSC$K_DTYPE_T;
   newtopic->dsc$b_class = DSC$K_CLASS_S;

   if (dofree) SLfree(newtop);

   return(1);	/* should add codes that LIB$GET_INPUT can return */
}

/*}}}*/

