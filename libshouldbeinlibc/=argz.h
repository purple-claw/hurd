/* Routines for dealing with '\0' separated arg vectors.

   Copyright (C) 1995 Free Software Foundation, Inc.

   Written by Miles Bader <miles@gnu.ai.mit.edu>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef __ARGZ_H__
#define __ARGZ_H__

/* Make a '\0' separated arg vector from a unix argv vector, returning it in
   ARGZ, and the total length in LEN.  If a memory allocation error occurs,
   ENOMEM is returned, otherwise 0.  The result can be destroyed using free. */
error_t argz_create(char **argv, char **argz, int *len);

/* Returns the number of strings in ARGZ.  */
int argz_count (char *argz, int len);

/* Puts pointers to each string in ARGZ into ARGV, which must be large enough
   to hold them all.  */
void argz_extract (char *argz, int len, char **argv);

/* Make '\0' separated arg vector ARGZ printable by converting all the '\0's
   except the last into the character SEP.  */
void argz_stringify(char *argz, int len, int sep);

/* Add BUF, of length BUF_LEN to the argz vector in ARGZ & ARGZ_LEN.  */
error_t
argz_append (char **argz, unsigned *argz_len, char *buf, unsigned buf_len);

/* Add STR to the argz vector in ARGZ & ARGZ_LEN.  This should be moved into
   argz.c in libshouldbelibc.  */
error_t argz_add (char **argz, unsigned *argz_len, char *str);

/* Remove ENTRY from ARGZ & ARGZ_LEN, if any.  */
void argz_remove (char **argz, unsigned *argz_len, char *entry);

/* Insert ENTRY into ARGZ & ARGZ_LEN at position N.  */
error_t argz_insert (char **argz, unsigned *argz_len, unsigned n, char *entry);

#endif /* __ARGZ_H__ */
