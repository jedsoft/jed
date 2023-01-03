/* -*- default window hooks for non-GUI jed */
/* Copyright (c) 2009-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */

#include "config.h"
#include "jed-feat.h"
#include "window.h"

int (*jed_new_window_cb) (Window_Type *) = NULL;
void (*jed_free_window_cb) (Window_Type *) = NULL;
int (*jed_create_mini_window_cb) (Window_Type *) = NULL;
int (*jed_leave_window_cb) (Window_Type *) = NULL;
int (*jed_enter_window_cb) (Window_Type *) = NULL;
int (*jed_split_window_cb) (Window_Type *, Window_Type *) = NULL;
int (*jed_window_geom_change_cb) (void) = NULL;

