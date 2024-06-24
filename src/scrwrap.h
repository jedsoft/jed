/* Copyright (c) 2025 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef JED_SCRWRAP_H_
#define JED_SCRWRAP_H_

/* 0 based row */
typedef struct
{
   int wrap_mode;
   int row, col;		       /* current physical screen row/col */
   int rmin, rmax;		       /* physical window where writes take place: rmin<=r<rmax */
   int cmax;			       /* maximum physical screen column, min assumed to be 0 */
   int num_wraps;
   int column;			       /* absolute column, in the absence of wrapping */
}
Scrwrap_Type;

extern void scrwrap_init (Scrwrap_Type *wt, Window_Type *w, int wrap_mode);
extern void scrwrap_calculate_rel_position (Scrwrap_Type *wt,
					    Line *l, int point,
					    int *rowp, int *colp);
extern void scrwrap_write_bytes (Scrwrap_Type *wt, SLuchar_Type *u, SLuchar_Type *umax, int color);
extern void scrwrap_set_visual_wrap_indicator (SLwchar_Type wch);

#endif				       /* JED_SCRWRAP_H_ */
