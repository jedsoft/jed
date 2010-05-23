#ifndef _GTK_JED_H_
#define _GTK_JED_H_

#include <gtk/gtk.h>

/* extern void    jedGtkUpdateEdSize( int, int ); */
extern void    jGtkCheckEdSize( int, int );
extern void    jGtkWidenEd( int, int, int );
extern void    jgtk_updMiniWinWidgetWinXRef( int );
extern void    jgtk_updEdWidgetWinXRef( Window_Type *, int, int );

extern void    jgtk_updOWEdWin( Window_Type *, int, int, int, int );
extern void    jgtk_delEdWin( Window_Type *, Window_Type *, int, int, int, int, int );
extern void    jgtk_splitEdWin( Window_Type *, Window_Type *, int, int, int, int, int );
extern void    jgtk_createTopEdWin( Window_Type *, int, int, int, int, int );
extern int    jgtk_createEditorMiniWin( Window_Type * );
extern void    updateScrollbar( Window_Type * );
extern void    jGtkSetWinSizes(void);

extern void jGtkSetWinSizes(void);

extern void jgtk_initToolbarArray(void);
extern void jgtk_initMenubarStruct(void);

extern int jgtk_createKeyEvents (char *);
extern void jGtkSetFocus(void);
extern void jGtkAttachMenubar( GtkWidget *mb );
extern void jGtkAddToolbar(GtkWidget *tb, int where);

#endif
