/* Copyright (c) 2007 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef _JED_COLORS_H_
#define _JED_COLORS_H_

/* These numbers should not be changed.  They are hard-coded in the
 * dfa cache files.
 */
#define JNORMAL_COLOR	 		0
#define JCURSOR_COLOR			1
#define JSTATUS_COLOR	 		2
#define JREGION_COLOR	 		3
#define JMENU_COLOR	 		4
#define JOP_COLOR	 		5
#define JNUM_COLOR	 		6
#define JSTR_COLOR			7
#define JCOM_COLOR			8
#define JDELIM_COLOR			9
#define JPREPROC_COLOR			10
#define JMESSAGE_COLOR			11
#define JERROR_COLOR			12
#define JDOLLAR_COLOR			13
#define JDOTS_COLOR			14
#define JMENU_CHAR_COLOR		15
#define JMENU_SHADOW_COLOR		16
#define JMENU_SELECTION_COLOR		17
#define JMENU_POPUP_COLOR		18
#define JMENU_SELECTED_CHAR_COLOR	19
#define JCURSOROVR_COLOR		20
#define JLINENUM_COLOR			21
#define JTWS_COLOR			22
#define JTAB_COLOR			23
#define JURL_COLOR			24
#define JITALIC_COLOR			25
#define JUNDERLINE_COLOR		26
#define JBOLD_COLOR			27
#define JHTML_KEY_COLOR			28
#define JKEY_COLOR	 		29
#define JKEY1_COLOR	 		(JKEY_COLOR+1)
#define JKEY2_COLOR	 		(JKEY_COLOR+2)
#define JKEY3_COLOR	 		(JKEY_COLOR+3)
#define JKEY4_COLOR	 		(JKEY_COLOR+4)
#define JKEY5_COLOR	 		(JKEY_COLOR+5)
#define JKEY6_COLOR	 		(JKEY_COLOR+6)
#define JKEY7_COLOR	 		(JKEY_COLOR+7)
#define JKEY8_COLOR	 		(JKEY_COLOR+8)
#define JKEY9_COLOR	 		(JKEY_COLOR+9)   /* 38 */

#define FIRST_USER_COLOR		64

#define JMAX_COLORS	256	       /* Do NOT increase this number */

extern int jed_init_color_intrinsics (void);
extern int jed_get_color_obj (char *name);
extern int jed_set_color (int, char *, char *);

#endif				       /* _JED_COLORS_H_ */
