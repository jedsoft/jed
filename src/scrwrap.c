/* Copyright (c) 2025 John E. Davis
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

#include "buffer.h"
#include "screen.h"
#include "misc.h"
#include "colors.h"

static char *Visual_Wrap_Indicator = "\\";

void scrwrap_set_visual_wrap_indicator (SLwchar_Type wch)
{
   static SLuchar_Type buffer[12];

   Visual_Wrap_Indicator = "\\";
   if (SLwchar_wcwidth (wch) == 1)
     {
	SLuchar_Type *u = SLutf8_encode (wch, buffer, sizeof(buffer));
	if (u != NULL)
	  {
	     *u = 0;
	     Visual_Wrap_Indicator = (char *)buffer;
	  }
     }
}

void scrwrap_init (Scrwrap_Type *wt, Window_Type *w, int wrap_mode)
{
   memset ((char *)wt, 0, sizeof (Scrwrap_Type));
   wt->wrap_mode = wrap_mode;
   wt->rmin = w->sy;
   wt->rmax = w->sy + w->rows;
   wt->cmax = w->width;
   wt->cmax -= CBuf->line_num_display_size;
   wt->column = 0;
   wt->num_wraps = 0;
}

void scrwrap_calculate_rel_position (Scrwrap_Type *wt,
				     Line *line, int point,
				     int *rowp, int *colp)
{
   SLuchar_Type *p0, *p1;
   int row, col;
   int is_mini = IN_MINI_WINDOW;
   int width;

   p0 = line->data;
   p1 = p0 + point;

   /* In this mode, a long line will be displayed as:
    *    line.......\
    *    continued..\  <--- The \ is added and not part of the string
    *    to here
    */
   if ((wt->wrap_mode == 0) || is_mini)
     {
	if (rowp != NULL) *rowp = 0;
	if (colp != NULL)
	  {
	     col = 1 + jed_compute_effective_length (p0, p1);
	     if (is_mini) col += Mini_Info.effective_prompt_len;
	     *colp = col;
	  }
	return;
     }

   width = wt->cmax;
   col = jed_compute_effective_length (line->data, line->data+point);
   width--;	       /* will wrap, account for \\ continuation */
   row = col/width;
   col = col % width;
   if ((col == 0) && (row > 0)
       && ((point == line->len) || (*p1 == '\n')))
     {
	col = width;
	row--;
     }
   if (rowp != NULL) *rowp = row;
   if (colp != NULL) *colp = 1 + col;		       /* col is 1-based */
}

/* Write a simple ascii string that contains no control chars.  Such a string
 * occupies strlen(str) columns.
 */
static void write_ascii_chars (Scrwrap_Type *wt, SLuchar_Type *str, unsigned int nbytes, int color)
{
   SLuchar_Type *s = str, *smax;
   int r = wt->row, c = wt->col;
   int c_max = wt->cmax - 1;	       /* allow for continuation char */
   int num_wraps;

   smax = s + nbytes;
   num_wraps = 0;
   while (s < smax)
     {
	if (c < c_max)
	  {
	     s++;
	     c++;
	     continue;
	  }
	if (r >= wt->rmin)
	  {
	     SLsmg_write_chars (str, s);
	     SLsmg_set_color (JDOLLAR_COLOR);
	     SLsmg_write_string (Visual_Wrap_Indicator);/* continuation char */
	     SLsmg_set_color (color);
	     SLsmg_erase_eol ();
	  }
	str = s;
	r++;
	c = 0;
	num_wraps++;
	SLsmg_gotorc (r, c);
	if (r >= wt->rmax) break;
     }
   if ((r >= wt->rmin) && (r < wt->rmax)) SLsmg_write_chars (str, s);

   wt->num_wraps += num_wraps;
   wt->row = r;
   wt->col = c;
   wt->column = wt->num_wraps * c_max + c;
}

static void write_as_hex_strings (Scrwrap_Type *wt, SLuchar_Type *p, unsigned int nbytes, int color)
{
   SLuchar_Type *pmax = p + nbytes;
   while (p < pmax)
     {
	char buf[8];

	sprintf (buf, "<%02X>", *p);
	write_ascii_chars (wt, (SLuchar_Type *)buf, 4, color);
	if (wt->row >= wt->rmax) return;
	p++;
     }
}

/* In this routine, *rp is the physical sceen row, and *cp is the logical column where the cursor would be
 * if no wrapping took place.  It is necessary to track this so that tabs can be expanded properly.
 * The physical column is given by dc.
 */
void scrwrap_write_bytes (Scrwrap_Type *wt, SLuchar_Type *u, SLuchar_Type *umax, int color)
{
   unsigned char *p, *pmax;
   int utf8_mode;
   int max_dc, tab, r_min, r_max;
   unsigned char display_8bit;

   if (wt->row >= wt->rmax) return;

   if (color == -1) color = 0;
   SLsmg_set_color (color);

   if (u == NULL) return;
   if (wt->wrap_mode == 0)
     {
	wt->num_wraps = 0;
	wt->col = wt->column = SLsmg_get_column ();   /* The row does not change */
	SLsmg_write_chars (u, umax);
	return;
     }

   utf8_mode = Jed_UTF8_Mode;
   if (utf8_mode)
     display_8bit = 0xA0;
   else
     display_8bit = (unsigned char) SLsmg_Display_Eight_Bit;

   tab = SLsmg_Tab_Width;

   r_min = wt->rmin;
   r_max = wt->rmax;
   max_dc = wt->cmax-1;	       /* allow for \ continuation char */

   p = u;
   pmax = umax;

   while (1)
     {
	SLwchar_Type wc;
	SLstrlen_Type nconsumed;
	unsigned char ch;
	unsigned int ddc;

	if (p == pmax)
	  {
	     /* Output buffered chars */
	     write_ascii_chars (wt, u, p-u, color);
	     break;
	  }

	ch = *p;
	if (ch < 0x80)
	  {
	     if ((ch >= 0x20) && (ch != 0x7F))
	       {
		  /* ascii */
		  p++;
		  continue;
	       }

	     /* Write out any buffered ascii characters */
	     if (u != p)
	       {
		  write_ascii_chars (wt, u, p-u, color);
		  u = p;
	       }

	     p++;
	     u = p;	       /* absorb the control char */

	     if ((ch == '\t') && (tab > 0))
	       {
		  ddc = tab - ((wt->column + tab) % tab);
		  while (ddc != 0)
		    {
		       write_ascii_chars (wt, (SLuchar_Type *)" ", 1, color);
		       ddc--;
		    }
	       }
	     else
	       {
		  /* non-tab control char -- display as ^X */
		  SLuchar_Type buf[2];
		  if (ch == 127) ch = '?'; else ch = ch + '@';
		  buf[0] = '^'; buf[1] = ch;
		  write_ascii_chars (wt, buf, 2, color);
	       }
	     if (wt->row >= wt->rmax) break;

	     continue;
	  }

	/* Not ascii -- write any buffered chars */
	if (u != p)
	  {
	     write_ascii_chars (wt, u, p-u, color);
	  }

	nconsumed = 1;

	if ((utf8_mode == 0)
	    || (NULL == SLutf8_decode (p, pmax, &wc, &nconsumed)))
	  {
	     /* Failed to decode as UTF-8 */
	     if ((utf8_mode == 0)
		 && (display_8bit && (*p >= display_8bit)))
	       {
		  /* *p displays as a single width char, treat is as ascii */
		  p++;
		  continue;
	       }

	     /* Otherwise display in <XX> form */
	     write_as_hex_strings (wt, p, nconsumed, color);
	  }
	else if (wc < (SLwchar_Type)display_8bit)
	  {
	     /* display the wc as <XX> */
	     ch = (unsigned char) wc;
	     write_as_hex_strings (wt, &ch, 1, color);
	  }
	else
	  {
	     int dc;
	     ddc = SLwchar_wcwidth (wc);
	     wt->col = dc = wt->col + ddc;
	     wt->column += ddc;
	     if (dc > max_dc)
	       {
		  if ((wt->row >= r_min) && (wt->row < r_max))
		    {
		       SLsmg_set_color (JDOLLAR_COLOR);
		       SLsmg_write_string (Visual_Wrap_Indicator);
		       SLsmg_erase_eol ();
		       SLsmg_set_color (color);
		    }
		  wt->col = 0;
		  wt->row++;
		  wt->num_wraps++;
		  SLsmg_gotorc (wt->row, 0);
		  u = p;
	       }
	     if ((wt->row >= wt->rmin) && (wt->row < wt->rmax))
	       {
		  SLsmg_write_chars (p, p + nconsumed);
	       }
	  }

	p += nconsumed;
	u = p;
	if (wt->row >= wt->rmax) break;
     }
}

