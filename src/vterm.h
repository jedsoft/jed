/* Copyright (c) 1992, 1998, 2000, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
extern int VTerm_Num_Rows, VTerm_Num_Cols;
#if SLANG_VERSION < 10310
typedef unsigned short SLsmg_Char_Type;
# define SLSMG_EXTRACT_CHAR(x) ((x) & 0xFF)
# define SLSMG_EXTRACT_COLOR(x) (((x)>>8)&0xFF)
# define SLSMG_BUILD_CHAR(ch,color) (((SLsmg_Char_Type)(unsigned char)(ch))|((color)<<8))
#endif
extern SLsmg_Char_Type **VTerm_Display;
extern int VTerm_Suspend_Update;

extern int vterm_init_display (int, int);
extern int vterm_reset_display (void);
extern void vterm_set_scroll_region (int, int);
extern void vterm_delete_nlines (int);
extern void vterm_reverse_index (int);
extern void vterm_del_eol (void);
extern void vterm_goto_rc (int, int);
extern void vterm_reverse_video (int);
extern void vterm_cls (void);
extern void vterm_write_nchars (char *, unsigned int);
extern void vterm_forward_cursor (int);
/* extern void vterm_write (unsigned short *, unsigned int); */
