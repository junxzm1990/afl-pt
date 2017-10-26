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
 * Module name:    main.h
 * Author name:    Zachary Smith
 * Create date:    1 Sept 2000
 * Purpose:        Externs for main.c.
 *----------------------------------------------------------------------
 * Changes:
 * 15 Oct 00, tuorfa@yahoo.com: removed echo_mode extern
 * 19 Sep 01, tuorfa@yahoo.com: added output personality
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 * 09 Nov 08, arkadiusz.firus@gmail.com: define CONFIG_DIR
 * 17 Jan 10, daved@physiol.usyd.edu.au: change CONFIG_DIR to drop outputs/
 * 31 Dec 20, willi@wm1.at: CONFIG_DIR now determined at configure time
 *--------------------------------------------------------------------*/


extern int lineno;
extern int debug_mode;
extern int simple_mode;
extern int inline_mode;
extern int no_remap_mode;


#ifndef _OUTPUT
#include "output.h"
#endif

#define CONFIG_DIR PKGDATADIR "/"
#define DEFAULT_OUTPUT "html"

extern OutputPersonality *op;


