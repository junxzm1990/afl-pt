#ifndef _MY_ICONV_H_INCLUDED_
#define _MY_ICONV_H_INCLUDED_

/*=============================================================================
   GNU UnRTF, a command-line program to convert RTF documents to other formats.
   Copyright (C) 2000,2001,2004 by Zachary Smith

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

   The maintainer is reachable by electronic mail at daved@physiol.usyd.edu.au
=============================================================================*/


/*----------------------------------------------------------------------
 * Module name:  my_iconv.h
 * Author name:  Arkadiusz Firus
 * Create date:  01 Jul 08
 * Purpose:    my_conv definitions
 *----------------------------------------------------------------------
 * Changes:
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 * 31 Dec 12, daved@physiol.usyd.edu.au: added _MY_ICONV_H_INCLUDED_
 * 31 Dec 12, willi@wm1.at: dropped CHARMAP_DIR - no longer used
 *--------------------------------------------------------------------*/

#ifndef HAVE_ICONV_H
#include <iconv.h>
#define HAVE_ICONV_H
#endif

#define char_table_size 256

typedef struct
{
	iconv_t desc;
	char **char_table;
} my_iconv_t;

#define MY_ICONV_T_CLEAR {(iconv_t) -1, NULL}

my_iconv_t my_iconv_open(const char *tocode, const char *fromcode);

size_t my_iconv(my_iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);

my_iconv_t my_iconv_close(my_iconv_t cd);

int my_iconv_is_valid(my_iconv_t cd);

void my_iconv_t_make_invalid(my_iconv_t *cd);

#endif /* _MY_ICONV_H_INCLUDED_ */
