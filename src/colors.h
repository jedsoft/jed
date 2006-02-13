#ifndef _JED_COLORS_H_
#define _JED_COLORS_H_

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
#define JKEY_COLOR	 		28 /* JKEY_COLOR must be last */

#if SLANG_VERSION < 20000
# define JMAX_COLORS	128	       /* Do NOT increase this number */
#else
# define JMAX_COLORS	256	       /* Do NOT increase this number */
#endif

extern int jed_init_color_intrinsics (void);
extern int jed_get_color_obj (char *name);
extern int jed_set_color (int, char *, char *);

#endif				       /* _JED_COLORS_H_ */
