/* Copyright (c) 1992-2010 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#ifndef _JED_FILE_H_
#define _JED_FILE_H_
#include <stdio.h>
#ifdef VMS
# include <fcntl.h>
#endif

#include "buffer.h"
#include "vfile.h"

extern int Require_Final_Newline;
extern int read_file(char *);
extern int insert_file(char *);
extern void set_file_modes(void);
extern void auto_save_buffer(Buffer *);
extern void auto_save_all(void);
extern void auto_save(void);
extern int write_region_to_fp(int);
extern int write_region(char *);
extern int read_file_pointer(int);
extern int insert_file_pointer(VFILE *);
extern int append_to_file(char *);
extern int write_file_with_backup(char *);
extern void visit_file(char *, char *);
extern void fixup_dir(char *);
extern char *dir_file_merge(char *, char *);
extern int file_status(SLFUTURE_CONST char *);
extern int file_changed_on_disk (Buffer *, char *);
extern int file_time_cmp(char *, char *);
extern char *file_type(SLFUTURE_CONST char *);
extern void check_buffer(Buffer *);
extern void set_file_trans(int *);

extern char *jed_expand_link(char *);

#ifdef REAL_UNIX_SYSTEM
extern int jed_get_inode_info (char *, dev_t *, ino_t *);
extern int Jed_Backup_By_Copying;
#endif
#ifndef VMS
extern int jed_copy_file (char *, char *);
#endif
extern void jed_set_umask (int);
extern void jed_set_buffer_ctime (Buffer *);

extern int jed_unlock_buffer_files (void);
extern int jed_unlock_buffer_file (Buffer *);
extern int jed_lock_buffer_file (Buffer *);
extern int jed_lock_file (char *);
extern int jed_unlock_file (char *);
extern int jed_dirfile_to_dir_file (char *dirfile, char **dirp, char **filep);
extern char *jed_standardize_filename (SLFUTURE_CONST char *file);
extern char *jed_dir_file_merge (SLFUTURE_CONST char *dir, SLFUTURE_CONST char *file);
extern char *jed_get_canonical_pathname (char *);
extern int jed_buffer_file_is_readonly (Buffer *);
extern int jed_file_is_readonly (char *file, int respect_perms);
#endif				       /* _JED_FILE_H_ */
