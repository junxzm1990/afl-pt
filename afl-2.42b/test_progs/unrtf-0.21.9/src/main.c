/*=============================================================================
   GNU UnRTF, a command-line program to convert RTF documents to other formats.
   Copyright (C) 2000, 2001, 2004 by Zachary Smith

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

   The maintainer is reachable by electronic mail at daved@physiol.usyd.edu.au
=============================================================================*/


/*----------------------------------------------------------------------
 * Module name:    main.c
 * Author name:    Zachary Smith
 * Create date:    01 Sep 00
 * Purpose:        main() routine with file open/close.
 *----------------------------------------------------------------------
 * Changes:
 * 14 Oct 00, tuorfa@yahoo.com: added -nopict option
 * 15 Oct 00, tuorfa@yahoo.com: added verify_file_type() 
 * 08 Apr 01, tuorfa@yahoo.com: more GNU-like switches implemented
 * 24 Jul 01, tuorfa@yahoo.com: removed verify_file_type()
 * 03 Aug 01, tuorfa@yahoo.com: added --inline switch
 * 08 Sep 01, tuorfa@yahoo.com: added use of UnRTF
 * 19 Sep 01, tuorfa@yahoo.com: addition of output personalities
 * 22 Sep 01, tuorfa@yahoo.com: added function-level comment blocks 
 * 23 Sep 01, tuorfa@yahoo.com: added wpml switch
 * 08 Oct 03, daved@physiol.usyd.edu.au: added stdlib.h for linux
 * 07 Jan 04, tuorfa@yahoo.com: removed broken PS support
 * 25 Sep 04, st001906@hrz1.hrz.tu-darmstadt.de: added stdlib.h for djgpp
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 06 Jan 06, marcossamaral@terra.com.br: includes verbose mode 
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 * 17 Dec 07, daved@physiol.usyd.edu.au: support for --noremap from
 *		David Santinoli
 * 09 Nov 08, arkadiusz.firus@gmail.com: support for -t <tag_file>
 		and read stdin if no input file provided
 * 13 Dec 08, daved@physiol.usyd.edu.au: search path code
 * 17 Jan 10, daved@physiol.usyd.edu.au: change search path to directory
 *		containing output conf and font charmap files
 * 14 Dec 11, jf@dockes.org: cleaned up get_config
 * 14 Dec 11, daved@physiol.usyd.edu.au: cleaned up get_config added --quiet
 *--------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>

#include "defs.h"
#include "error.h"
#include "word.h"
#include "convert.h"
#include "parse.h"
#include "hash.h"
#include "malloc.h"
#include "path.h"

#include "output.h"
#include "user.h"
#include "main.h"
#include "util.h"

int nopict_mode; /* TRUE => Do not write \pict's to files */
int dump_mode;   /* TRUE => Output a dump of the RTF word tree */
int debug_mode;  /* TRUE => Output debug comments within HTML */
int lineno;      /* Used for error reporting and final line count. */
int simple_mode; /* TRUE => Output HTML without SPAN/DIV tags -- This would
					probably be more useful if we could pull out <font> tags
					as well. */
int inline_mode; /* TRUE => Output HTML without HTML/BODY/HEAD -- This is
					buggy. I've seen it output pages of </font> tags. */
/* marcossamaral - 0.19.9 */
int verbose_mode;  /* TRUE => Output additional informations about unrtf */
int no_remap_mode; /* don't remap codepoints */
int quiet;	   /* TRUE => don't output header comments */


OutputPersonality *op = NULL;

/*========================================================================
 * Name:	get_config
 * Purpose:	Updates output acording to information found in file path.
 * Args:	Configuration file base name, OutputPersonality
 * Returns:	Updated OutputPersonality.
 *=======================================================================*/

OutputPersonality *
get_config(char *name, OutputPersonality *op)
{
    char *configfile = 0;
    if(!path_checked && check_dirs() == 0)
    {
	fprintf(stderr,"No config directories. Searched: %s\n", search_path);
	exit(1);
    }
    configfile = search_in_path(name, "conf");
    if (configfile == NULL)
    {
	fprintf(stderr, "failed to find %s.conf in search path dirs\n", name);
	exit(1);
    }
    op = user_init(op, configfile);
    free(configfile);
    return op;
}

/*========================================================================
 * Name:	main
 * Purpose:	Main control function.
 * Args:	Args.
 * Returns:	Exit code.
 *=======================================================================*/

int
main (int argc, char **argv)
{
	FILE *f;
	Word * word;
	char *path = NULL;
	char *env_path_p = '\0';

	int i;
	nopict_mode = debug_mode = dump_mode = inline_mode = no_remap_mode = FALSE;
	/* initialize search path to compiled-in value */
	search_path = DEFAULT_UNRTF_SEARCH_PATH;

	if((env_path_p = getenv("UNRTF_SEARCH_PATH")) != NULL)
	{
		if(verbose_mode)
			fprintf(stderr, "got environment path: %s\n", env_path_p);
		search_path=env_path_p;
	}

	/* Handle arguments */

	for (i = 1; i < argc; i++) {
		if (!strcmp("--dump", argv[i])) dump_mode = TRUE;
		else if (!strcmp("-d", argv[i])) dump_mode = TRUE;
		else if (!strcmp("--debug", argv[i])) debug_mode = TRUE;
		else if (!strcmp("--verbose", argv[i])) verbose_mode = TRUE;
		else if (!strcmp("--quiet", argv[i])) quiet = TRUE;
		else if (!strcmp("--simple", argv[i])) simple_mode = TRUE;
		else if (!strcmp("--noremap", argv[i])) no_remap_mode = TRUE;
		else if (!strcmp("-t", argv[i]))
		{
			if ((i + 1) < argc && *argv[i + 1] != '-')
			{
				i++;
				op = get_config(argv[i], op);
			}
		} 
		else if (!strcmp("-P",  argv[i]))
		{
			if(i+1 > argc)
			{
				fprintf(stderr,"-P needs a path argument\n");
				exit(1);
			}
			search_path=argv[++i];
		}
		else if (!strcmp("--inline", argv[i])) inline_mode = TRUE;
		else if (!strcmp("--help", argv[i])) {
			usage();
		}
		else if (!strcmp("--version", argv[i])) {
			fprintf(stderr, "%s\n", PACKAGE_VERSION);
			fprintf(stderr, "search path is: %s\n", search_path);
			exit(0);
		}
		else if (!strcmp("--nopict", argv[i])) nopict_mode = TRUE;
		else if (!strcmp("-n", argv[i])) nopict_mode = TRUE;
		else if (!strncmp("--", argv[i], 2))
			op = get_config(&argv[i][2], op);
		else
		{
			if (*argv[i] == '-') usage();

			if (path) 
				usage();
			else 	
				path = argv[i];
		}
	}



	if (op == NULL)
		op = get_config(DEFAULT_OUTPUT, op);
	if(!path_checked && check_dirs() == 0)
	{
		fprintf(stderr,"no config directories\n");
		exit(1);
	}


	
	/* Program information */
	if (verbose_mode || debug_mode) {
		fprintf(stderr, "This is UnRTF ");
		fprintf(stderr, "version %s\n", PACKAGE_VERSION);
		fprintf(stderr, "By Dave Davey, Marcos Serrou do Amaral and Arkadiusz Firus\n");
		fprintf(stderr, "Original Author: Zachary Smith\n");
		show_dirs();
	}

	if (debug_mode) fprintf(stderr, "Debug mode.\n");
	if (dump_mode) fprintf(stderr, "Dump mode.\n");

	/* Open file for reading. Append ".rtf" to file name if not supplied. */
	if (path == NULL)
		f = stdin;
	else
	{
		f = fopen(path, "r");
		if (!f) {
			char path2[200];
			strcpy(path2, path);
			strcat(path2, ".rtf");
			f = fopen(path2, "r");
			if (!f)
				error_handler("Cannot open input file");
		}
	}

	if (verbose_mode || debug_mode) fprintf(stderr, "Processing %s...\n", path); 

	/* Keep track of lines processed. This is arbitrary to the user as
	 *   RTF ignores newlines. May be helpful in error tracking. */
	lineno = 0;

	/* All the work starts here. word_read() should keep reading words until
	 *   the end of the file. */
	word = word_read(f);

	if (dump_mode) {
		word_dump(word);
		printf("\n");
	} else {
/* Should we also optimize word before dump? - AF */
		word = optimize_word(word, 1);
		word_print(word);
	}

	fclose(f);

	/* marcossamaral - 0.19.9 */
	if(verbose_mode || debug_mode) { 
		unsigned long total=0;
		total = hash_stats();
		fprintf(stderr, "Done.\n");
        	fprintf(stderr, "%lu words were hashed.\n", total);
	}

	if (debug_mode) {
		fprintf(stderr, "Total memory allocated %ld bytes.\n", 
			total_malloced());
	}

	/* May as well */
	word_free(word);

	return 0;
}

