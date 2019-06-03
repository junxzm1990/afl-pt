#ifndef _CONVERT_H_INCLUDED_
#define _CONVERT_H_INCLUDED_


/*===========================================================================
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
===========================================================================*/


/*----------------------------------------------------------------------
 * Module name:    convert
 * Author name:    Zachary Smith
 * Create date:    19 Sep 2001
 * Purpose:        Definitions for the conversion module
 *----------------------------------------------------------------------
 * Changes:
 * 31 Mar 05, by daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 * 09 Nov 08, arkadiusz.firus@gmail.com: codepage improvements
 * 07 Oct 11, jf@dockes.org: added _CONVERT_H_INCLUDED_
 *--------------------------------------------------------------------*/

#include "word.h"

enum {
	CHARSET_ANSI=1,
	CHARSET_MAC,
	CHARSET_CP437,
	CHARSET_CP850,
};

extern void word_print (Word*);
#define FONT_GREEK  "cp1253"
#define FONT_SYMBOL "SYMBOL"

#endif /* _CONVERT_H_INCLUDED_ */
