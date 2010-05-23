/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#include <stdio.h>
#include <string.h>
#include <slang.h>

#include "vterm.h"
#include "misc.h"

/* This is a virtual terminal used by the window system displays. */

int VTerm_Num_Rows, VTerm_Num_Cols;
SLsmg_Char_Type **VTerm_Display;
int VTerm_Suspend_Update;

static int Scroll_R1, Scroll_R2;
static SLsmg_Color_Type Current_Color;
static int Current_Row;
static int Current_Col;

/* Note: In my opinion, area clearing operations should use the current color.
 * Unfortunately, window systems clear using the default color.  So, that must
 * be mimicked here.  This means that the color argument to this function 
 * should be 0.  Sigh.
 */
static void blank_line (SLsmg_Char_Type *s, int len, int color)
{
   SLsmg_Char_Type *smax = s + len;
   memset ((char *)s, 0, len * sizeof (SLsmg_Char_Type));
   while (s < smax)
     {
	s->nchars = 1;
	s->color = color;
	s->wchars[0] = ' ';
	s++;
     }
}

     
int vterm_reset_display (void)
{
   int r;

   if (VTerm_Suspend_Update) return 0;

   Current_Col = Current_Row = 0;
   Current_Color = 0;
   Scroll_R2 = Scroll_R1 = 0;

   if (VTerm_Display == NULL)
     return 0;
   
   for (r = 0; r < VTerm_Num_Rows; r++)
     SLfree ((char *) VTerm_Display [r]);

   SLfree ((char *)VTerm_Display);
   VTerm_Display = NULL;
   
   return 0;
}

int vterm_init_display (int rows, int cols)
{
   int r;

   if (VTerm_Suspend_Update) return 0;

   vterm_reset_display ();

   if ((rows == 0) || (cols == 0))
     {
	rows = 24;
	cols = 80;
     }
   
   VTerm_Num_Cols = cols;
   VTerm_Num_Rows = rows;

   VTerm_Display = (SLsmg_Char_Type **)jed_malloc0 (rows * sizeof (SLsmg_Char_Type *));
   if (VTerm_Display == NULL)
     return -1;

   for (r = 0; r < rows; r++)
     {
	SLsmg_Char_Type *s;

	if (NULL == (s = (SLsmg_Char_Type *) SLmalloc (cols * sizeof (SLsmg_Char_Type))))
	  {
	     vterm_reset_display ();
	     return -1;
	  }

	VTerm_Display[r] = s;
     }

   vterm_set_scroll_region (0, VTerm_Num_Rows - 1);
   vterm_reverse_video (0);
   vterm_goto_rc (0, 0);
   vterm_cls ();

   return 0;
}

void vterm_set_scroll_region (int r1, int r2)
{
   if (VTerm_Suspend_Update) return;

   if (r1 >= VTerm_Num_Rows) r1 = VTerm_Num_Rows - 1;
   if (r1 < 0) r1 = 0;

   if (r2 >= VTerm_Num_Rows) r2 = VTerm_Num_Rows - 1;
   if (r2 < 0) r2 = 0;
   
   Scroll_R1 = r1;
   Scroll_R2 = r2;
}

void vterm_delete_nlines (int n)
{
   if (VTerm_Suspend_Update) return;

   if (VTerm_Display == NULL)
     return;

   if (n > Scroll_R2 - Scroll_R1) n = 1 + Scroll_R2 - Scroll_R1;

   while (n > 0)
     {
	SLsmg_Char_Type *s;
	int r;

	s = VTerm_Display[Scroll_R1];

	for (r = Scroll_R1; r < Scroll_R2; r++)
	  VTerm_Display[r] = VTerm_Display[r + 1];
	     
	VTerm_Display[Scroll_R2] = s;
        blank_line (s, VTerm_Num_Cols, 0);
	n--;
     }
}

void vterm_reverse_index (int n)
{
   if (VTerm_Suspend_Update) return;

   if (VTerm_Display == NULL)
     return;
   
   if (n > Scroll_R2 - Scroll_R1) n = 1 + Scroll_R2 - Scroll_R1;

   while (n > 0)
     {
	SLsmg_Char_Type *s;
	int r;

	s = VTerm_Display[Scroll_R2];

	for (r = Scroll_R2; r > Scroll_R1; r--)
	  VTerm_Display[r] = VTerm_Display[r - 1];
	     
	VTerm_Display[Scroll_R1] = s;
        blank_line (s, VTerm_Num_Cols, 0);
	n--;
     }
}


void vterm_del_eol (void)
{
   if (VTerm_Suspend_Update) return;

   if (VTerm_Display == NULL)
     return;

   blank_line (VTerm_Display[Current_Row] + Current_Col,
	       VTerm_Num_Cols - Current_Col, 0);
}

void vterm_goto_rc (int r, int c)
{
   if (VTerm_Suspend_Update) return;

   r += Scroll_R1;

   if (r >= VTerm_Num_Rows) r = VTerm_Num_Rows - 1;
   if (r < 0) r = 0;
   
   if (c >= VTerm_Num_Cols) c = VTerm_Num_Cols - 1;
   if (c < 0) c = 0;
   
   Current_Col = c;
   Current_Row = r;
}

void vterm_forward_cursor (int n)
{
   Current_Col += n;
   if (Current_Col >= VTerm_Num_Cols)
     Current_Col = VTerm_Num_Cols - 1;
}

void vterm_reverse_video (int c)
{
   if (VTerm_Suspend_Update) return;
   Current_Color = c;
}

void vterm_cls (void)
{
   int r;

   if (VTerm_Suspend_Update) return;

   if (VTerm_Display == NULL)
     return;
   
   for (r = 0; r < VTerm_Num_Rows; r++)
     blank_line (VTerm_Display[r], VTerm_Num_Cols, 0);
}

	
void vterm_write_nchars (char *s, unsigned int len)
{
   SLsmg_Char_Type *p, *pmax;
   char *smax;
   SLsmg_Color_Type color;

   if (VTerm_Suspend_Update) return;

   if (VTerm_Display == NULL)
     return;
   
   p = VTerm_Display[Current_Row];
   pmax = p + VTerm_Num_Cols;
   p += Current_Col;
   
   if (p + len >= pmax)
     memset ((char *) p, 0, (pmax - p) * sizeof (SLsmg_Char_Type));
   else
     memset ((char *) p, 0, len * sizeof (SLsmg_Char_Type));

   color = Current_Color;
   smax = s + len;
   while ((p < pmax) && (s < smax))
     {
	p->color = color;
	p->nchars = 1;
	p->wchars[0] = (unsigned char) *s;
	p++;
	s++;
     }

   vterm_forward_cursor (len);
}

/* This is commented out until I can test it.  It was submitted by */
#if 0
/* // return number of wide characters written */
int vterm_write_nbytes (char *s, unsigned int len)
{
   SLsmg_Char_Type *p, *pmin, *pmax;
   char *smax, *s1;
   int ncons;
   SLsmg_Color_Type color;
   SLwchar_Type wch;
      
   if (VTerm_Suspend_Update) return 0;

   if (VTerm_Display == NULL)
      return 0;

   p = VTerm_Display[Current_Row];
   pmax = p + VTerm_Num_Cols;
   p += Current_Col;
   pmin = p;

   memset ((char *) p, 0, (pmax - p) * sizeof (SLsmg_Char_Type));

   color = Current_Color;
   smax = s + len;
   while ((p < pmax) && (s < smax))
   {
      s1 = SLutf8_decode (s, smax, &wch, &ncons);
      if (s1 == NULL)
      {
         vterm_forward_cursor (p - pmin);
         vterm_write_nchars(s, smax - s);
         return (p - pmin) + (smax - s);
      }
      else
      {
         p->color = color;
         p->nchars = 1;
         p->wchars[0] = wch;
         p++;
         s = s1;
      }
   }

   vterm_forward_cursor (p - pmin);
   
   return (p - pmin);
}
#endif
