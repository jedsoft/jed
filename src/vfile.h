#ifndef _DAVIS_VFILE_H_
#define _DAVIS_VFILE_H_
/* Copyright (c) 1992-2010 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#define VFILE_TEXT  1
#define VFILE_BINARY  2
extern unsigned int VFile_Mode;

typedef struct
{
   char *buf;			       /* buffer for stream */
   char *bmax;			       /* pointer to end buffer */
   char *bp;			       /* current pointer in stream */
   char *eof;			       /* pointer to EOF if non NULL */
   int fd;			       /* file descrip for stream */
   unsigned int size;		       /* default buffer size */
   unsigned int mode;
   unsigned int cr_flag;	       /* true if lines end in cr */
} VFILE;

extern char *vgets(VFILE *, unsigned int *);
extern VFILE *vopen(SLFUTURE_CONST char *, unsigned int, unsigned int);
extern void vclose(VFILE *);
extern VFILE *vstream(int, unsigned int, unsigned int);

#endif
