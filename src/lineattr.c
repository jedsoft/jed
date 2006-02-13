/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1998, 2000, 2002, 2003, 2004, 2005, 2006 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include <stdio.h>

#include "config.h"
#include "jed-feat.h"

#if JED_HAS_LINE_ATTRIBUTES

/*{{{ Include Files */

#include <slang.h>

#include "jdmacros.h"
#include "buffer.h"
#include "lineattr.h"
#include "screen.h"
#include "paste.h"
#include "ins.h"

/*}}}*/

char *Line_Read_Only_Error = "Line is read only!";

static void set_line_readonly (int *ro) /*{{{*/
{
   if (*ro) CLine->flags |= JED_LINE_IS_READONLY;
   else CLine->flags &= ~JED_LINE_IS_READONLY;
}

/*}}}*/

void jed_skip_hidden_lines_forward (int *flagp) /*{{{*/
{
   unsigned int flag;
   
   if (*flagp) flag = JED_LINE_HIDDEN;
   else flag = 0;
   
   while (jed_down (1))
     {
	if ((CLine->flags & JED_LINE_HIDDEN) != flag)
	  break;
     }
   bol ();
}

/*}}}*/

void jed_skip_hidden_lines_backward (int *flagp) /*{{{*/
{
   unsigned int flag;
   
   if (*flagp) flag = JED_LINE_HIDDEN;
   else flag = 0;
   
   while (jed_up(1))
     {
	if ((CLine->flags & JED_LINE_HIDDEN) != flag)
	  {
	     eol ();
	     return;
	  }
     }
   bol ();
}

/*}}}*/

static void set_line_hidden (int *hide) /*{{{*/
{
   if (*hide)
     {
	CLine->flags |= JED_LINE_HIDDEN;
     }
   else
     CLine->flags &= ~JED_LINE_HIDDEN;
   
   /* register_change (0); */
   Suspend_Screen_Update = 1;
}

/*}}}*/

static void set_region_hidden (int *hidep) /*{{{*/
{
   int hide = *hidep;
   Line *l;
   
   if (0 == narrow_to_lines ())
     return;
   
   l = CBuf->beg;
   while (l != NULL)
     {
	if (hide)
	  {
	     l->flags |= JED_LINE_HIDDEN;
	  }
	else
	  l->flags &= ~JED_LINE_HIDDEN;
	
	l = l->next;
     }
   
   widen ();
   touch_screen ();
}

/*}}}*/

static int is_line_hidden (void) /*{{{*/
{
   return (0 != (CLine->flags & JED_LINE_HIDDEN));
}

/*}}}*/

SLang_Intrin_Fun_Type JedLine_Intrinsics[] = /*{{{*/
{
   MAKE_INTRINSIC_I("set_line_readonly", set_line_readonly, VOID_TYPE),
   /* Prototype: Void set_line_readonly (Integer flag);
    * This function may be used to turn on or off the read-only state of the
    * current line.  If the integer parameter @flag@ is non-zero, the line
    * will be made read-only.  If the paramter is zero, the read-only state
    * will be turned off.
    * Related Functions: @getbuf_info@
    */
   MAKE_INTRINSIC_I("set_line_hidden", set_line_hidden, VOID_TYPE),
   /* Prototype: Void set_line_hidden (Integer flag);
    * If the parameter @flag@ is non-zero, the current line will be given
    * the hidden attribute.  This means that it will not be displayed.  If the
    * parameter is zero, the hidden attribute will be turned off.
    * Related Functions: @set_region_hidden@, @is_line_hidden@
    */
   MAKE_INTRINSIC_I("set_region_hidden", set_region_hidden, VOID_TYPE),
   /* Prototype: Void set_region_hidden (Integer flag);
    * This function may be used to hide the lines in a region.  If @flag@ is 
    * non-zero, all lines in the region will be hidden.  If it is zero, the
    * lines in the region will be made visible.
    * Related Functions: @set_line_hidden@, @is_line_hidden@, @skip_hidden_lines_forward@
    */
   MAKE_INTRINSIC("is_line_hidden", is_line_hidden, INT_TYPE, 0),
   /* Prototype: Integer is_line_hidden ();
    * This function returns a non-zero value if the current line is hidden.  It
    * will return zero if the current line is visible.
    * Related Functions: @set_line_hidden@
    */
   MAKE_INTRINSIC_I("skip_hidden_lines_backward", jed_skip_hidden_lines_backward, VOID_TYPE),
   /* Prototype: Void skip_hidden_lines_backward (Integer type);
    * This function may be used to move backward across either hidden or non-hidden
    * lines depending upon whether the parameter @type@ is non-zero or zero.
    * If @type@ is non-zero, the Point is moved backward across hidden lines 
    * until a visible line is reached.  If @type@ is zero, visible lines will
    * be skipped instead.  If the top of the buffer is reached before the
    * appropriate line is reached, the Point will be left there.
    * 
    * Note: The functions @up@ and @down@ are insensitive to whether or not
    * a line is hidden.
    * Related Functions: @skip_hidden_lines_forward@, @is_line_hidden@
    */
   MAKE_INTRINSIC_I("skip_hidden_lines_forward", jed_skip_hidden_lines_forward, VOID_TYPE),
   /* Prototype: Void skip_hidden_lines_forward (Integer type);
    * This function may be used to move forward across either hidden or non-hidden
    * lines depending upon whether the parameter @type@ is non-zero or zero.
    * If @type@ is non-zero, the Point is moved forward across hidden lines 
    * until a visible line is reached.  If @type@ is zero, visible lines will
    * be skipped instead.  If the end of the buffer is reached before the
    * appropriate line is reached, the Point will be left there.
    * 
    * Note: The functions @up@ and @down@ are insensitive to whether or not
    * a line is hidden.
    * Related Functions: @skip_hidden_lines_backward@, @is_line_hidden@
    */
   MAKE_INTRINSIC(NULL,NULL,0,0)
};

/*}}}*/


#endif  			       /* JED_HAS_LINE_ATTRIBUTES */
