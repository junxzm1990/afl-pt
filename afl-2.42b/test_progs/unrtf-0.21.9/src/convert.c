
/*===========================================================================
   GNU UnRTF, a command-line program to convert RTF documents to other formats.
   Copyright (C) 2000,2001,2004 Zachary Thayer Smith

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
===========================================================================*/


/*----------------------------------------------------------------------
 * Module name:    convert
 * Author name:    Zachary Smith
 * Create date:    24 Jul 01
 * Purpose:        Performs conversion from RTF to other formats.
 *----------------------------------------------------------------------
 * Changes:
 * 24 Jul 01, tuorfa@yahoo.com: moved code over from word.c
 * 24 Jul 01, tuorfa@yahoo.com: fixed color table reference numbering.
 * 30 Jul 01, tuorfa@yahoo.com: moved hex convert to util.c
 * 30 Jul 01, tuorfa@yahoo.com: moved special expr tables to special.c
 * 30 Jul 01, tuorfa@yahoo.com: moved attribute stack to attr.c
 * 31 Jul 01, tuorfa@yahoo.com: began addition of hash of rtf commands
 * 01 Aug 01, tuorfa@yahoo.com: finished bulk of rtf command hash
 * 03 Aug 01, tuorfa@yahoo.com: removed no-op hash entries to save space
 * 03 Aug 01, tuorfa@yahoo.com: code to ignore rest of groups for \*, etc
 * 03 Aug 01, tuorfa@yahoo.com: fixed para-alignnot being cleared by \pard
 * 03 Aug 01, tuorfa@yahoo.com: added support for \keywords group
 * 03 Aug 01, tuorfa@yahoo.com: added dummy funcs for header/footer
 * 03 Aug 01, tuorfa@yahoo.com: began addition of hyperlink support
 * 04 Aug 01, tuorfa@yahoo.com: fixed debug string printing
 * 05 Aug 01, tuorfa@yahoo.com: added support for hyperlink data with \field
 * 06 Aug 01, tuorfa@yahoo.com: added support for several font attributes
 * 08 Aug 01, gommer@gmx.net: bugfix for picture storing mechanism
 * 08 Sep 01, tuorfa@yahoo.com: added use of UnRTF
 * 11 Sep 01, tuorfa@yahoo.com: added support for JPEG and PNG pictures
 * 19 Sep 01, tuorfa@yahoo.com: added output personality support
 * 22 Sep 01, tuorfa@yahoo.com: added function-level comment blocks 
 * 23 Sep 01, tuorfa@yahoo.com: fixed translation of \'XX expressions
 * 08 Oct 03, daved@physiol.usyd.edu.au: more special character code
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 29 Mar 05, daved@physiol.usyd.edu.au: more unicode support
 * 31 Mar 05, daved@physiol.usyd.edu.au: strcat security bug fixed
 * 06 Jan 06, marcossamaral@terra.com.br: patch from debian 0.19.3-1.1
 * 03 Mar 06, daved@physiol.usyd.edu.au: fixed creation date spelling
		and added support for accented characters in titles from
		Laurent Monin
 * 09 Mar 06, daved@physiol.usyd.edu.au: don't print null post_trans
 * 18 Jun 06, daved@physiol.usyd.edu.au: fixed some incorrect comment_end
 * 18 Jun 06, frolovs@internet2.ru: codepage support
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 * 17 Dec 07, daved@physiol.usyd.edu.au: Italian month name spelling corrections
 *		from David Santinoli
 * 09 Nov 08, arkadiusz.firus@gmail.com: adopt iconv
 * 04 Jan 10, arkadiusz.firus@gmail.com: deal with (faulty) negative unicodes
 * 04 Jan 10, daved@physiol.usyd.edu.au: suppress <font face=Symbol>
 * 07 Oct 11, jf@dockes.org: major improvement to code pages and to unicode
 * 	related code
 * 09 Jun 13, jf@dockes.org: ignore nonshppict flag to avoid useless output
 * 09 Jun 13, peterli@salk.edu, added code to cope with bold/italic
 			requests that are not properly nested.  Change
			is ifdef'd SUPPORT_UNNESTED.  At present only applied
			to cmd_b, but if generally accepted could/should be
			more widely applied
 * 27 Jun 13, daved@physiol.usyd.edu.au: added a few more cmds to
 		SUPPORT_UNNESTED
 * 09 Dec 14, Michal Zalewski - changes in process_color_table, cmd_cf, cmd_cb
 		and cmd_highlight to protect against malformed RTF input
 * 09 Dec 14, hanno@hboeck.de changes to avoid null pointers
 * 11 Dec 14, daved@physiol.usyd.edu.au: changed handling of "*" command so
 *		that included pictures are not ignored
 * 12 Dec 14, daved@physiol.usyd.edu.au: check that bogus month not in info
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

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_STRING_H
/* For strcasestr() */
#define __USE_GNU 
#include <string.h>
#endif

#include <errno.h>

#include "defs.h"
#include "parse.h"
#include "util.h"
#include "malloc.h"
#include "main.h"
#include "error.h"
#include "word.h"
#include "hash.h"
#include "convert.h"
#include "attr.h"

typedef struct {
	char *name;
	int (*func)(Word*, int, char, int);
	char *debug_print;
} HashItem;
static HashItem *find_command(const char *cmdpp, int *hasparamp, int *paramp);

extern int nopict_mode;

/*
#define BINARY_ATTRS
*/

my_iconv_t desc = MY_ICONV_T_CLEAR;

/* Nested tables aren't supported.
 */
static int coming_pars_that_are_tabular = 0;
static int within_table = FALSE;
static int have_printed_row_begin=FALSE;
static int have_printed_cell_begin=FALSE;
static int have_printed_row_end=FALSE;
static int have_printed_cell_end=FALSE;
static void check_for_table();


/* Previously in word_print_core function 
 */
static int total_chars_this_line=0; /* for simulating \tab */


/* Paragraph alignment (kludge)
 */
enum {
	ALIGN_LEFT=0,
	ALIGN_RIGHT,
	ALIGN_CENTER,
	ALIGN_JUSTIFY
};



/* This value is set by attr_push and attr_pop 
 */
int simulate_smallcaps;
int simulate_allcaps;


/* Most pictures must be written to files. */
enum {
	PICT_UNKNOWN=0,
	PICT_WM,
	PICT_MAC,
	PICT_PM,
	PICT_DI,
	PICT_WB,
	PICT_JPEG,
	PICT_PNG,
	PICT_EMF,
};
static int within_picture=FALSE;
static int within_picture_depth;
static int picture_file_number=1;
static char picture_path[255];
static int picture_width;
static int picture_height;
static int picture_bits_per_pixel=1;
static int picture_type=PICT_UNKNOWN;
static int picture_wmetafile_type;
static char *picture_wmetafile_type_str;

static int EndNoteCitations=FALSE;

static int have_printed_body=FALSE;
static int within_header=TRUE;


static const char *hyperlink_base = NULL;


void starting_body();
void starting_text();
void print_with_special_exprs (const char *s);

static int banner_printed=FALSE;


/*========================================================================
 * Name:	print_banner
 * Purpose:	Writes program-identifying text to the output stream.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void
print_banner () {
	if (!banner_printed) {
		if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
	  	printf(" Translation from RTF performed by ");
		printf("UnRTF, version ");
		printf("%s ", PACKAGE_VERSION);
		if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
	}
	banner_printed=TRUE;
}


/*========================================================================
 * Name:	starting_body
 * Purpose:	Switches output stream for writing document contents.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void
starting_body ()
{
	if (!have_printed_body) {
		if (!inline_mode) {
			if (safe_printf(0, op->header_end)) fprintf(stderr, TOO_MANY_ARGS, "header_end");
			if (safe_printf(0, op->body_begin)) fprintf(stderr, TOO_MANY_ARGS, "body_begin");
		}
		within_header = FALSE;
		have_printed_body = TRUE;
	}
}


/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/


static char *month_strings[12]= {
#ifdef ENGLISH
  "January","February","March","April","May","June","July","August",
  "September","October","November","December"
#endif
#ifdef FRANCAIS
  "Janvier","Fevrier","Mars","Avril","Mai","Juin","Juillet","Aout","Septembre",
  "Octobre","Novembre","Decembre"
#endif
#ifdef ITALIANO
  "Gennaio","Febbraio","Marzo","Aprile","Maggio","Giugno",
  "Luglio","Agosto","Settembre","Ottobre","Novembre","Dicembre"
#endif
#ifdef ESPANOL /* amaral - 0.19.2 */
  "Enero","Febrero","Marzo","Abril","Mayo","Junio","Julio","Agosto",
  "Septiembre","Octubre","Noviembre","Diciembre"
#endif
#ifdef DEUTSCH  /* daved - 0.21.6 */
  "Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August",
  "September", "Oktober", "November", "Dezember"
#endif
#ifdef PORTUGUES /* amaral - 0.19.2 */
  "Janeiro","Fevereiro","Marco","Abril","Maio","Junho","Julho","Agosto",
  "Setembro","Outubro","Novembro","Dezembro"
#endif
};


/*========================================================================
 * Name:	word_dump_date
 * Purpose:	Extracts date from an RTF input stream, writes it to
 *              output stream.
 * Args:	Word*, buffered RTF stream
 * Returns:	None.
 *=======================================================================*/

void 
word_dump_date (Word *w)
{
	int year=0, month=0, day=0, hour=0, minute=0;
	CHECK_PARAM_NOT_NULL(w);

	while (w)
	{
	 	const char *s = word_string (w);
		if (!s)
			return;
		if (*s == '\\') {
			++s;
			if (!strncmp (s, "yr", 2) && isdigit(s[2])) {
				year = atoi (&s[2]);
			}
			else if (!strncmp (s, "mo", 2) && isdigit(s[2]))
			{
				month= atoi (&s[2]);
				if(month > 12)
				{
					warning_handler("bogus month");
					return;
				}
			}
			else if (!strncmp (s, "dy", 2) && isdigit(s[2])) {
				day= atoi (&s[2]);
			}
			else if (!strncmp (s, "min", 3) && isdigit(s[3])) {
				minute= atoi (&s[3]);
			}
			else if (!strncmp (s, "hr", 2) && isdigit(s[2])) {
				hour= atoi (&s[2]);
			}
		}
		w=w->next;
	}
	if (year && month > 0 && month <= 12 && day) {
		printf("%d %s %d ", day, month_strings[month-1], year);
	}
	if (hour && minute) {
		printf("%02d:%02d ", hour, minute);
	}
}



/*-------------------------------------------------------------------*/

typedef struct {
	int num;
	char *name;
        char *encoding;
} FontEntry;

static char *default_encoding = "CP1252";
/* current_encoding not NULL to be safe. "" is detected to avoid
   free() on 1st call */
static char *current_encoding = ""; 
/* Encoding expected by op_translate_buffer. Until we find a way to specify
   transparent UTF-8 passthrough */
static char *output_encoding = "UTF-32BE";
static int default_font_number = 0; // Set by \deffx command
static int had_ansicpg;
#define MAX_FONTS (8192)
static FontEntry font_table[MAX_FONTS];
static int total_fonts=0;

static void flush_iconv_input();
static void accumulate_iconv_input(int ch);

static void 
set_current_encoding(char *encoding)
{
    if (current_encoding && *current_encoding)
        my_free(current_encoding);
    current_encoding = my_strdup(encoding);
}

static void 
maybeopeniconv()
{
    if (!my_iconv_is_valid(desc)) {
        /* This may happen if output begins without a font command */
        char *encoding = attr_get_param(ATTR_ENCODING);
        if (!encoding || !*encoding) 
            encoding = default_encoding;
        desc = my_iconv_open(output_encoding, encoding);
        set_current_encoding(encoding);
    }
}


/*========================================================================
 * Name:	lookup_font
 * Purpose:	Fetches the font entry from the already-read font table.
 * Args:	Font#.
 * Returns:	Font name.
 *=======================================================================*/
FontEntry *
lookup_font (int num) 
{
    int i;
    if (total_fonts) 
	for(i=0;i<total_fonts;i++) {
            if (font_table[i].num==num) 
                return &font_table[i];
	}
    return NULL;
}
char* 
lookup_fontname (int num) {
    FontEntry *e = lookup_font(num);
    if (e == NULL)
        return NULL;
    return e->name;
}

// Extract name and parameter from command. RTF spec says name is max
// 32 chars (inc. param I think), let's be conservative:
#define MAX_CONTROL_LEN 50
// Returns 0 for error, 1 if ok no parm, 0 else
static int controlToNameParm(const char *s, char *name, int maxlen, int *parm)
{
    const char *cp = s;
    int len = 0;

    if (s == 0)
        return 0;
    while (isalpha(*cp) && len < maxlen) {
        *name++ = *cp++;
        len++;
    }
    if (len == maxlen)
        return 0;
    *name = 0;
    if (!*cp)
        return 1;
    if (!(*cp == '-') && !isdigit(*cp))
        return 0;
    *parm=atoi(cp);
    return 2;
}

// translate \fcharset parameter value to code page. See MS RTF doc
static const int fcharsetparmtocp(int parm)
{
    switch (parm) {
    case 0: return 1252;
    case 1: return 0;
    case 2: return 42; 
    case 77: return 10000;
    case 78: return 10001;
    case 79: return 10003;
    case 80: return 10008;
    case 81: return 10002;
    case 83: return 10005;
    case 84: return 10004;
    case 85: return 10006;
    case 86: return 10081;
    case 87: return 10021;
    case 88: return 10029;
    case 89: return 10007;
    case 128: return 932;
    case 129: return 949;
    case 130: return 1361;
    case 134: return 936;
    case 136: return 950;
    case 161: return 1253;
    case 162: return 1254;
    case 163: return 1258;
    case 177: return 1255;
    case 178: return 1256;
    case 186: return 1257;
    case 204: return 1251;
    case 222: return 874;
    case 238: return 1250;
    case 254: return 437;
    default: return 1252;
    }
}

// Translate code page to encoding name hopefully suitable as iconv input
static char *cptoencoding(parm)
{
    // Note that CP0 is supposed to mean current system default, which does
    // not make any sense as a stored value, we don't handle it.

    // It's quite possible that some of the CPxx values had better be
    // replaced in some case by an equivalent better understood by
    // iconv. Need testing
    switch (parm) {
    case 42: return "SYMBOL";
    case 437: return "CP437"; /* United States IBM */
    case 708: return "CP708"; /* Arabic (ASMO 708) */
    case 709: return "CP709"; /* Arabic (ASMO 449+, BCON V4) */
    case 710: return "CP710"; /* Arabic (transparent Arabic) */
    case 711: return "CP711"; /* Arabic (Nafitha Enhanced) */
    case 720: return "CP720"; /* Arabic (transparent ASMO) */
    case 819: return "CP819"; /* Windows 3.1 (United States and Western Europe) */
    case 850: return "CP850"; /* IBM multilingual */
    case 852: return "CP852"; /* Eastern European */
    case 860: return "CP860"; /* Portuguese */
    case 862: return "CP862"; /* Hebrew */
    case 863: return "CP863"; /* French Canadian */
    case 864: return "CP864"; /* Arabic */
    case 865: return "CP865"; /* Norwegian */
    case 866: return "CP866"; /* Soviet Union */
    case 874: return "CP874"; /* Thai */
    case 932: return "CP932"; /* Japanese */
    case 936: return "CP936"; /* Simplified Chinese */
    case 949: return "CP949"; /* Korean */
    case 950: return "CP950"; /* Traditional Chinese */
    case 1250: return "CP1250"; /* Eastern European */
    case 1251: return "CP1251"; /* Cyrillic */
    case 1252: return "CP1252"; /* Western European */
    case 1253: return "CP1253"; /* Greek */
    case 1254: return "CP1254"; /* Turkish */
    case 1255: return "CP1255"; /* Hebrew */
    case 1256: return "CP1256"; /* Arabic */
    case 1257: return "CP1257"; /* Baltic */
    case 1258: return "CP1258"; /* Vietnamese */
    case 1361: return "CP1361"; /* Johab */
    case 10000: return "MAC"; /* MAC Roman */
    case 10001: return "CP10001"; /* MAC Japan ?? Iconv does not know this*/
    case 10004: return "MACARABIC"; /* MAC Arabic */
    case 10005: return "MACHEBREW"; /* MAC Hebrew */
    case 10006: return "MACGREEK"; /* MAC Greek */
    case 10007: return "MACCYRILLIC"; /* MAC Cyrillic */
    case 10029: return "MACCENTRALEUROPE"; /* MAC Latin2 */
    case 10081: return "MACTURKISH"; /* MAC Turkish */
    case 57002: return "CP57002"; /* Devanagari */
    case 57003: return "CP57003"; /* Bengali */
    case 57004: return "CP57004"; /* Tamil */
    case 57005: return "CP57005"; /* Telugu */
    case 57006: return "CP57006"; /* Assamese */
    case 57007: return "CP57007"; /* Oriya */
    default: return "CP1252";
    }
}

/*========================================================================
 * Name:	process_font_table
 * Purpose:	Processes the font table of an RTF file.
 * Args:	Tree of words.
 * Returns:	None.
 *=======================================================================*/

void
process_font_table (Word *w)
{
    Word *w2;
    CHECK_PARAM_NOT_NULL(w);

    if (safe_printf(0, op->fonttable_begin)) fprintf(stderr, TOO_MANY_ARGS, "fonttable_begin");

    while (w) {
        int num;
        char name[BUFSIZ];
        const char *tmp;

        if ((w2 = w->child))
	{
            tmp = word_string(w2);
	    if(!tmp)
	    	return;
            if (!strncmp("\\f", tmp, 2)) {
                num = atoi(&tmp[2]);
                name[0] = 0;
                int cpgcp = -1;
                int fcharsetcp = -1;
                w2 = w2->next;
                while (w2) {
                    tmp = word_string (w2);
                    if (tmp && tmp[0] != '\\') {
                        if (strlen(tmp) + strlen(name) > BUFSIZ - 1) {
                            fprintf(stderr, "Invalid font table entry\n");
                            name[0] = 0;
                        }
                        else
                            strncat(name,tmp,sizeof(name) - strlen(name) - 1);
                    } else if (tmp) {
                        char nm[MAX_CONTROL_LEN+1];
                        int parm;
                        int ret = 
                            controlToNameParm(tmp+1,nm, MAX_CONTROL_LEN, &parm);
                        if (ret == 2) {
                            if (!strcmp(nm, "fcharset")) {
                                fcharsetcp = fcharsetparmtocp(parm);
                            } else if (!strcmp(nm, "cpg")) {
                                cpgcp = parm;
                            }
                        }
                    }
                    w2 = w2->next;
                }

                /* Chop the gall-derned semicolon. */
                { char *t;
                    if ((t = strchr(name, ';')))
                        *t = 0;
                }

                font_table[total_fonts].num=num;
                font_table[total_fonts].name=my_strdup(name);
                // Explicit cpg parameter has priority on fcharset one
                if (cpgcp == -1)
                    cpgcp = fcharsetcp;
                if (cpgcp != -1)
                    font_table[total_fonts].encoding = cptoencoding(cpgcp);
                else {
                    /* If there is "symbol" in the font name, use
                     * symbol encoding, else no local encoding */
                    if (strcasestr(name, "symbol"))
                        font_table[total_fonts].encoding = "SYMBOL";
                    else 
                        font_table[total_fonts].encoding = 0;
                }
		// need to have this outside last block as it must be detected
		// even if cpgcp is set
		if (strcasecmp(name, "symbol") == 0)
			font_table[total_fonts].encoding = "SYMBOL";
                if (safe_printf(0, assemble_string(op->fonttable_fontnr, num))) fprintf(stderr, TOO_MANY_ARGS, "fonttable_fontnr");
                if (safe_printf(1, op->fonttable_fontname, name)) fprintf(stderr, TOO_MANY_ARGS, "fonttable_fontname");
                total_fonts++;
            }
        }
        w=w->next;
    }

    if (safe_printf(0, op->fonttable_end)) fprintf(stderr, TOO_MANY_ARGS, "fonttable_end");

    /*
     * If the default font has an encoding, set it as default
     * encoding. The default font number was set by the \deff command
     * in the header (or not in which case it is o). Don't do it if
     * we had an explicit ansicpg command (no logic in this, just works).
     */
    if (total_fonts > 0 && !had_ansicpg) {
        FontEntry *e = lookup_font(default_font_number);
        if (e && e->encoding && *e->encoding)
            default_encoding = e->encoding;
    }

    if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
    printf("font table contains %d fonts total",total_fonts);
    if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");

    if (debug_mode) {
        int i;

        if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
        printf("font table dump: \n");
        for (i=0; i<total_fonts; i++) {
            printf(" font %d = %s encoding = %s\n", font_table[i].num, 
                   font_table[i].name, font_table[i].encoding);
        }
		
        if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
    }
}


/*========================================================================
 * Name:	process_index_entry
 * Purpose:	Processes an index entry of an RTF file.
 * Args:	Tree of words.
 * Returns:	None.
 *=======================================================================*/

void
process_index_entry (Word *w)
{
	Word *w2;

	CHECK_PARAM_NOT_NULL(w);

	while(w) {
		if ((w2=w->child)) {
			const char *str = word_string (w2);

			if (debug_mode && str) {
				if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
				printf("index entry word: %s ", str);			
				if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
			}
		}
		w=w->next;
	}
}


/*========================================================================
 * Name:	process_toc_entry
 * Purpose:	Processes an index entry of an RTF file.
 * Args:	Tree of words, flag to say whether to include a page#.
 * Returns:	None.
 *=======================================================================*/

void
process_toc_entry (Word *w, int include_page_num)
{
	Word *w2;

	CHECK_PARAM_NOT_NULL(w);

	while(w) {
		if ((w2=w->child)) {
			const char *str = word_string (w2);

			if (debug_mode && str) {
				
				if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
				printf("toc %s entry word: %s ", 
					include_page_num ? "page#":"no page#",
					str);
				if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
			}
		}
		w=w->next;
	}
}


/*========================================================================
 * Name:	process_info_group
 * Purpose:	Processes the \info group of an RTF file.
 * Args:	Tree of words.
 * Returns:	None.
 *=======================================================================*/

void
process_info_group (Word *w)
{
	Word *child;

        maybeopeniconv();
	/* amaral - 0.19.2 */
	/* CHECK_PARAM_NOT_NULL(w); */
	if (!w) printf("AUTHOR'S COMMENT: \\info command is null!\n"); 

	while(w)
	{
		child = w->child;
		if (child) {
			Word *w2;
			const char *s;

			s = word_string(child);
			if(!s)
				return;

			if (!inline_mode) {
				if (!strcmp("\\title", s)) {
					
					if (safe_printf(0, op->document_title_begin)) fprintf(stderr, TOO_MANY_ARGS, "document_title_begin");
					w2=child->next;
					while (w2) {
						const char *s2 = word_string(w2);
						if (s2 && s2[0] != '\\') 
						{
							print_with_special_exprs (s2);
						}
						else if (s2 && s2[1] == '\'' && s2[2] && s2[3])
						{
							int ch = h2toi (&s2[2]);
							accumulate_iconv_input(ch);
						}

						w2 = w2->next;
					}
                                        flush_iconv_input();
					if (safe_printf(0, op->document_title_end)) fprintf(stderr, TOO_MANY_ARGS, "document_title_end");
				}
				else if (!strcmp("\\keywords", s)) {
					if (safe_printf(0, op->document_keywords_begin)) fprintf(stderr, TOO_MANY_ARGS, "document_keywords_begin");
					w2=child->next;
					while (w2) {
						const char *s2 = word_string(w2);
						if (s2 && s2[0] != '\\') 
							printf("%s,", s2);
						w2 = w2->next;
					}
					if (safe_printf(0, op->document_keywords_end)) fprintf(stderr, TOO_MANY_ARGS, "document_keywords_end");
				}
				else if (!strcmp("\\author", s)) {
					if (safe_printf(0, op->document_author_begin)) fprintf(stderr, TOO_MANY_ARGS, "document_author_begin");
					w2=child->next;
					while (w2) {
						const char *s2 = word_string(w2);
						if (s2 && s2[0] != '\\') 
							printf("%s", s2);
						w2 = w2->next;
					}
					if (safe_printf(0, op->document_author_end)) fprintf(stderr, TOO_MANY_ARGS, "document_author_end");
				}
				else if (!strcmp("\\comment", s)) {
					if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
					printf("comments: ");
					w2=child->next;
					while (w2) {
						const char *s2 = word_string(w2);
						if (s2 && s2[0] != '\\') 
							printf("%s", s2);
						w2 = w2->next;
					}
					if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
				}
				else if (!strncmp("\\nofpages", s, 9)) {
					if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
					printf("total pages: %s",&s[9]);
					if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
				}
				else if (!strncmp("\\nofwords", s, 9)) {
					if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
					printf("total words: %s",&s[9]);
					if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
				}
				else if (!strncmp("\\nofchars", s, 9) && isdigit(s[9])) {
					if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
					printf("total chars: %s",&s[9]);
					if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
				}
				else if (!strcmp("\\creatim", s)) {
					if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
					printf("creation date: ");
					if (child->next) word_dump_date (child->next);
					if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
				}
				else if (!strcmp("\\printim", s)) {
					if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
					printf("last printed: ");
					if (child->next) word_dump_date (child->next);
					if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
				}
				else if (!strcmp("\\buptim", s)) {
					if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
					printf("last backup: ");
					if (child->next) word_dump_date (child->next);
					if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
				}
				else if (!strcmp("\\revtim", s)) {
					if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
					printf("revision date: ");
					if (child->next) word_dump_date (child->next);
					if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
				}
			}

			/* Irregardless of whether we're in inline mode,
			 * we want to process the following.
			 */
			if (!strcmp("\\hlinkbase", s)) {
				const char *linkstr = NULL;

				if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
				printf("hyperlink base: ");
				if (child->next) {
					Word *nextword = child->next;

					if (nextword) 
						linkstr=word_string (nextword);
				}

				if (linkstr)
					printf("%s", linkstr);
				else
					printf("(none)");
				if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");

				/* Store the pointer, it will remain good. */
				hyperlink_base = linkstr; 
			}
		} 
		w = w->next;
	}
}

/*-------------------------------------------------------------------*/

/* RTF color table colors are RGB */

typedef struct {
	unsigned char r,g,b;
} Color;

#define MAX_COLORS (1024)
static Color color_table[MAX_COLORS];
static int total_colors=0;


/*========================================================================
 * Name:	process_color_table
 * Purpose:	Processes the color table of an RTF file.
 * Args:	Tree of words.
 * Returns:	None.
 *=======================================================================*/

void
process_color_table (Word *w)
{
	int r,g,b;

	CHECK_PARAM_NOT_NULL(w);

	/* Sometimes, RTF color tables begin with a semicolon,
	 * i.e. an empty color entry. This seems to indicate that color 0 
	 * will not be used, so here I set it to black.
	 */
	r=g=b=0;

	while(w)
	{
		const char *s = word_string (w);
		if(s == 0 || total_colors >= MAX_COLORS)
			break;

		if (!strncmp("\\red",s,4)) {
			r = atoi(&s[4]);
			while(r>255) r>>=8;
		}
		else if (!strncmp("\\green",s,6)) {
			g = atoi(&s[6]);
			while(g>255) g>>=8;
		}
		else if (!strncmp("\\blue",s,5)) {
			b = atoi(&s[5]);
			while(b>255) b>>=8;
		}
		else
		/* If we find the semicolon which denotes the end of
		 * a color entry then store the color, even if we don't
		 * have all of it.
		 */
		if (!strcmp (";", s)) {
			color_table[total_colors].r = r;
			color_table[total_colors].g = g;
			color_table[total_colors++].b = b;
			if (debug_mode) {
				if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
				printf("storing color entry %d: %02x%02x%02x",
					total_colors-1, r,g,b);
				if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
			}
			r=g=b=0;
		}

		w=w->next;
	}

	if (debug_mode) {
		if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
	  	printf("color table had %d entries", total_colors);
		if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
	}
}

/*========================================================================
 * Name:	cmd_cf
 * Purpose:	Executes the \cf command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_cf (Word *w, int align, char has_param, int num)
{
	char str[40];

	if (!has_param || num<0 || num>=total_colors)
	{
		warning_handler ("font color change attempted is invalid");
	}
	else
	{
		sprintf(str,"#%02x%02x%02x",
			color_table[num].r,
			color_table[num].g,
			color_table[num].b);
		attr_push(ATTR_FOREGROUND,str);
	}
	return FALSE;
}



/*========================================================================
 * Name:	cmd_cb
 * Purpose:	Executes the \cb command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_cb (Word *w, int align, char has_param, int num)
{
	char str[40];

	if (!has_param || num<0 || num>=total_colors)
	{
		warning_handler ("font color change attempted is invalid");
	}
	else
	{
		sprintf(str,"#%02x%02x%02x",
			color_table[num].r,
			color_table[num].g,
			color_table[num].b);
		attr_push(ATTR_BACKGROUND,str);
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_fs
 * Purpose:	Executes the \fs command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_fs (Word *w, int align, char has_param, int points) {
	char str[20];

	if (!has_param) return FALSE;

	/* Note, fs20 means 10pt */
	points /= 2;

	sprintf(str,"%d",points);
	attr_push(ATTR_FONTSIZE,str);

	return FALSE;
}


/*========================================================================
 * Name:	cmd_field
 * Purpose:	Interprets fields looking for hyperlinks.
 * Comment:	Because hyperlinks are put in \field groups,
 *		we must interpret all \field groups, which is
 *		slow and laborious.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_field (Word *w, int align, char has_param, int num) {
	Word *child;

	CHECK_PARAM_NOT_NULL(w);
        maybeopeniconv();
	while(w) {
		child = w->child;
		if (child) {
			Word *w2;
			const char *s;

			s = word_string(child);
			if (!s)
				return TRUE;

#if 1 /* daved experimenting with fldrslt */
			if(!strcmp("\\fldrslt", s))
				return FALSE;
#endif


			if (!strcmp("\\*", s))
			{
			    w2=child->next;
			    while (w2)
			    {
				const char *s2 = word_string(w2);
				if (s2 && !strcmp("\\fldinst", s2))
				{
				    Word *w3;
				    const char *s;
				    const char *s4;
				    Word *w4;
				    w3=w2->next;
				    s = word_string(w3);
				    if (s && !strcmp(s, "SYMBOL") )
				    {
					w4=w3->next;
					while(w4 && word_string(w4) && !strcmp(word_string(w4), " "))
						w4 = w4->next;
					s4 = word_string(w4);
					if (s4)
					{
					   int char_num;
					   const char *string;
					   char_num = atoi(s4);

                                           string = op_translate_char(op, 
                                                     FONT_SYMBOL, char_num);
                                           if (string != NULL)
                                               printf("%s", string);
					    else
						fprintf(stderr, "unrtf: Error in translation SYMBOL character %d\n", char_num);
					}
				    }
				    while (w3 && !w3->child) {
					    w3=w3->next;
				    }
				    if (w3) w3=w3->child;
				    while (w3)
				    {
					    const char *s3=word_string(w3);
					    if (s3 && !strcmp("EN.CITE", s3))
					    {
						EndNoteCitations = TRUE;
					    }
					    /*
					    ** If we have a file with EndNote
					    ** citations, we don't want to
					    ** insert "hyperlink", we want the
					    ** citation text, so we return
					    ** FALSE.  (We could extract "<url>"
					    ** for a link, but probably not
					    ** wanted.)
					    */

					    if (s3 && !strcmp("HYPERLINK",s3) && !EndNoteCitations) {
						    Word *w4;
						    const char *s4;
						    w4=w3->next;
						    while (w4 && word_string(w4) && !strcmp(" ", word_string(w4)))
							    w4=w4->next;
						    if (w4) {
							    s4=word_string(w4);
							    if (safe_printf(0, op->hyperlink_begin)) fprintf(stderr, TOO_MANY_ARGS, "hyperlink_begin");
							    printf("%s", s4);
							    if (safe_printf(0, op->hyperlink_end)) fprintf(stderr, TOO_MANY_ARGS, "hyperlink_end");
							    	    return TRUE;
						    }
					    	
					    }
					    else
					    	return FALSE;
					    w3=w3->next;
				    }
				}
					w2 = w2->next;
			    }
				
			}
		}
		w=w->next;
	}
	return TRUE;
}

/*========================================================================
 * Name:	cmd_f
 * Purpose:	Executes the \f command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_f (Word *w, int align, char has_param, int num) 
{
    char *name;

    /* no param exit early XX */
    if (!has_param) 
        return FALSE;
    FontEntry *e = lookup_font(num);
    name = e ? e->name : NULL;
    if (!e || !name) {
        if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
        printf("invalid font number %d",num);
        if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
    } else {
        if (op->fonttable_begin != NULL)
        {
            // TOBEDONE: WHAT'S THIS ???
            name = my_malloc(12);
            sprintf(name, "%d", num);
        }

        /* we are going to output entities, so should not output font */
        if(strstr(name,"Symbol") == NULL)
            attr_push(ATTR_FONTFACE,name);

        desc = my_iconv_close(desc);
        char *encoding = default_encoding;
        if (e->encoding && *e->encoding) {
            encoding = e->encoding;
            attr_push(ATTR_ENCODING, encoding);
        }
        desc = my_iconv_open(output_encoding, encoding);
        set_current_encoding(encoding);
    }

    return FALSE;
}

/*========================================================================
 * Name:	cmd_deff
 * Purpose:	Executes the \deff command, set default font
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_deff (Word *w, int align, char has_param, int num) 
{
    if (has_param) 
        default_font_number = num;
    return FALSE;
}

/*========================================================================
 * Name:	cmd_highlight
 * Purpose:	Executes the \cf command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_highlight (Word *w, int align, char has_param, int num) 
{
	char str[40];

	if (!has_param || num<0 || num>=total_colors)
	{
		warning_handler ("font background color change attempted is invalid");
	}
	else
	{
		sprintf(str,"#%02x%02x%02x",
			color_table[num].r,
			color_table[num].g,
			color_table[num].b);
		attr_push(ATTR_BACKGROUND,str);
	}
	return FALSE;
}



/*========================================================================
 * Name:	cmd_tab
 * Purpose:	Executes the \tab command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_tab (Word *w, int align, char has_param, int param) 
{
	/* Tab presents a genuine problem
	 * since some output formats don't have 
	 * an equivalent. As a kludge fix, I shall 
	 * assume the font is fixed width and that
	 * the tabstops are 8 characters apart.
	 */
	int need= 8-(total_chars_this_line%8);
	total_chars_this_line += need;
	while(need>0) {
		if (safe_printf(0, op->forced_space)) fprintf(stderr, TOO_MANY_ARGS, "forced_space");
		need--;
	}
	printf("\n");
	return FALSE;
}


/*========================================================================
 * Name:	cmd_plain
 * Purpose:	Executes the \plain command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_plain (Word *w, int align, char has_param, int param) {
	attr_pop_all();
	return FALSE;
}


/*========================================================================
 * Name:	cmd_fnil
 * Purpose:	Executes the \fnil command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_fnil (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_FONTFACE,FONTNIL_STR);
	return FALSE;
}



/*========================================================================
 * Name:	cmd_froman
 * Purpose:	Executes the \froman command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_froman (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_FONTFACE,FONTROMAN_STR);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_fswiss
 * Purpose:	Executes the \fswiss command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_fswiss (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_FONTFACE,FONTSWISS_STR);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_fmodern
 * Purpose:	Executes the \fmodern command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_fmodern (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_FONTFACE,FONTMODERN_STR);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_fscript
 * Purpose:	Executes the \fscript command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_fscript (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_FONTFACE,FONTSCRIPT_STR);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_fdecor
 * Purpose:	Executes the \fdecor command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_fdecor (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_FONTFACE,FONTDECOR_STR);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_ftech
 * Purpose:	Executes the \ftech command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_ftech (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_FONTFACE,FONTTECH_STR);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_expand
 * Purpose:	Executes the \expand command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_expand (Word *w, int align, char has_param, int param) {
	char str[10];
	if (has_param) {
		sprintf(str, "%d", param/4);
		if (!param) 
			attr_pop(ATTR_EXPAND);
		else 
			attr_push(ATTR_EXPAND, str);
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_emboss
 * Purpose:	Executes the \embo command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_emboss (Word *w, int align, char has_param, int param) {
	char str[10];
	if (has_param && !param)
#ifdef SUPPORT_UNNESTED
		attr_find_pop(ATTR_EMBOSS);
#else
		attr_pop(ATTR_EMBOSS);
#endif
	else
	{
		sprintf(str, "%d", param);
		attr_push(ATTR_EMBOSS, str);
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_engrave
 * Purpose:	Executes the \impr command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_engrave (Word *w, int align, char has_param, int param) {
	char str[10];
	if (has_param && !param) 
		attr_pop(ATTR_ENGRAVE);
	else
	{
		sprintf(str, "%d", param);
		attr_push(ATTR_ENGRAVE, str);
	}
	return FALSE;
}

/*========================================================================
 * Name:	cmd_caps
 * Purpose:	Executes the \caps command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_caps (Word *w, int align, char has_param, int param) {
	if (has_param && !param)
		attr_pop(ATTR_CAPS);
	else 
		attr_push(ATTR_CAPS,NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_scaps
 * Purpose:	Executes the \scaps command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_scaps (Word *w, int align, char has_param, int param) {
	if (has_param && !param)
		attr_pop(ATTR_SMALLCAPS);
	else 
		attr_push(ATTR_SMALLCAPS,NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_bullet
 * Purpose:	Executes the \bullet command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_bullet (Word *w, int align, char has_param, int param) {
	if (op->chars.bullet) {
		if (safe_printf(0, op->chars.bullet)) fprintf(stderr, TOO_MANY_ARGS, "chars.bullet");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}

/*========================================================================
 * Name:	cmd_ldblquote
 * Purpose:	Executes the \ldblquote command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_ldblquote (Word *w, int align, char has_param, int param) {
	if (op->chars.left_dbl_quote) {
		if (safe_printf(0, op->chars.left_dbl_quote)) fprintf(stderr, TOO_MANY_ARGS, "chars.left_dbl_quote");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_rdblquote
 * Purpose:	Executes the \rdblquote command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_rdblquote (Word *w, int align, char has_param, int param) {
	if (op->chars.right_dbl_quote) {
		if (safe_printf(0, op->chars.right_dbl_quote)) fprintf(stderr, TOO_MANY_ARGS, "chars.right_dbl_quote");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_lquote
 * Purpose:	Executes the \lquote command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_lquote (Word *w, int align, char has_param, int param) {
	if (op->chars.left_quote) {
		if (safe_printf(0, op->chars.left_quote)) fprintf(stderr, TOO_MANY_ARGS, "chars.left_quote");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_nonbreaking_space
 * Purpose:	Executes the nonbreaking space command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_nonbreaking_space (Word *w, int align, char has_param, int param) {
	if (op->chars.nonbreaking_space) {
		if (safe_printf(0, op->chars.nonbreaking_space)) fprintf(stderr, TOO_MANY_ARGS, "chars.nonbreaking_space");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_nonbreaking_hyphen
 * Purpose:	Executes the nonbreaking hyphen command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_nonbreaking_hyphen (Word *w, int align, char has_param, int param) {
	if (op->chars.nonbreaking_hyphen) {
		if (safe_printf(0, op->chars.nonbreaking_hyphen)) fprintf(stderr, TOO_MANY_ARGS, "chars.nonbreaking_hyphen");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_optional_hyphen
 * Purpose:	Executes the optional hyphen command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_optional_hyphen (Word *w, int align, char has_param, int param) {
	if (op->chars.optional_hyphen) {
		if (safe_printf(0, op->chars.optional_hyphen)) fprintf(stderr, TOO_MANY_ARGS, "chars.optional_hyphen");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_emdash
 * Purpose:	Executes the \emdash command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_emdash (Word *w, int align, char has_param, int param) {
	if (op->chars.emdash) {
		if (safe_printf(0, op->chars.emdash)) fprintf(stderr, TOO_MANY_ARGS, "chars.emdash");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_endash
 * Purpose:	Executes the \endash command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_endash (Word *w, int align, char has_param, int param) {
	if (op->chars.endash) {
		if (safe_printf(0, op->chars.endash)) fprintf(stderr, TOO_MANY_ARGS, "chars.endash");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_rquote
 * Purpose:	Executes the \rquote command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_rquote (Word *w, int align, char has_param, int param) {
	if (op->chars.right_quote) {
		if (safe_printf(0, op->chars.right_quote)) fprintf(stderr, TOO_MANY_ARGS, "chars.right_quote");
		++total_chars_this_line; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_par
 * Purpose:	Executes the \par command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int 
cmd_par (Word *w, int align, char has_param, int param) {
	if (op->line_break) {
		if (safe_printf(0, op->line_break)) fprintf(stderr, TOO_MANY_ARGS, "line_break");
		total_chars_this_line = 0; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_line
 * Purpose:	Executes the \line command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int 
cmd_line (Word *w, int align, char has_param, int param) {
	if (op->line_break) {
		if (safe_printf(0, op->line_break)) fprintf(stderr, TOO_MANY_ARGS, "line_break");
		total_chars_this_line = 0; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_page
 * Purpose:	Executes the \page command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_page (Word *w, int align, char has_param, int param) {
	if (op->page_break) {
		if (safe_printf(0, op->page_break)) fprintf(stderr, TOO_MANY_ARGS, "page_break");
		total_chars_this_line = 0; /* \tab */
	}
	return FALSE;
}


/*========================================================================
 * Name:	cmd_intbl
 * Purpose:	Executes the \intbl command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_intbl (Word *w, int align, char has_param, int param) {
	++coming_pars_that_are_tabular;

	check_for_table();

	return FALSE;
}


/*========================================================================
 * Name:	cmd_ulnone
 * Purpose:	Executes the \ulnone command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ulnone (Word *w, int align, char has_param, int param) 
{

#ifdef BINARY_ATTRS
	attr_remove_underlining()) fprintf(stderr, TOO_MANY_ARGS, );
#else
	do {
		int attr = attr_read();
		if (attr==ATTR_UNDERLINE ||
		    attr==ATTR_DOT_UL ||
		    attr==ATTR_DASH_UL ||
		    attr==ATTR_DOT_DASH_UL ||
		    attr==ATTR_2DOT_DASH_UL ||
		    attr==ATTR_WORD_UL ||
		    attr==ATTR_WAVE_UL ||
		    attr==ATTR_THICK_UL ||
		    attr==ATTR_DOUBLE_UL)
		{
			/* Top attr is some kind of underline. Pop it */
			if (!attr_pop(attr)) {
				/* ?? Have to break, else we're looping */
				break;
			}
		} else {
			/* Top attr not underline, done */
			break;
		}
	} while (1);
#endif
	return FALSE;
}

/*========================================================================
 * Name:	cmd_ul
 * Purpose:	Executes the \ul command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ul (Word *w, int align, char has_param, int param) {
	if (has_param && param == 0) {
		cmd_ulnone(w, align, has_param, param);
	} else {
		attr_push(ATTR_UNDERLINE, NULL);
	}
	return FALSE;
}

/*========================================================================
 * Name:	cmd_uld
 * Purpose:	Executes the \uld command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_uld (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_DOUBLE_UL, NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_uldb
 * Purpose:	Executes the \uldb command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_uldb (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_DOT_UL, NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_uldash
 * Purpose:	Executes the \uldash command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_uldash (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_DASH_UL, NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_uldashd
 * Purpose:	Executes the \cmd_uldashd command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_uldashd (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_DOT_DASH_UL,NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_uldashdd
 * Purpose:	Executes the \uldashdd command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_uldashdd (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_2DOT_DASH_UL,NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_ulw
 * Purpose:	Executes the \ulw command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ulw (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_WORD_UL,NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_ulth
 * Purpose:	Executes the \ulth command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ulth (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_THICK_UL,NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_ulthd
 * Purpose:	Executes the \ulthd command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ulthd (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_THICK_UL, NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_ulthdash
 * Purpose:	Executes the \ulthdash command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ulthdash (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_THICK_UL, NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_ulwave
 * Purpose:	Executes the \ulwave command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ulwave (Word *w, int align, char has_param, int param) {
	attr_push(ATTR_WAVE_UL,NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_strike
 * Purpose:	Executes the \strike command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_strike (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
		attr_pop(ATTR_STRIKE);
	else
		attr_push(ATTR_STRIKE,NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_strikedl
 * Purpose:	Executes the \strikedl command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_strikedl (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
		attr_pop(ATTR_DBL_STRIKE);
	else
		attr_push(ATTR_DBL_STRIKE,NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_striked
 * Purpose:	Executes the \striked command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_striked (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
		attr_pop(ATTR_DBL_STRIKE);
	else
		attr_push(ATTR_DBL_STRIKE,NULL);
	return FALSE;
}


/*========================================================================
 * Name:	cmd_rtf
 * Purpose:	Executes the \rtf command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_rtf (Word *w, int align, char has_param, int param) {
	return FALSE;
}

/*========================================================================
 * Name:	cmd_shppict
 * Purpose:	Executes the \shppict command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_shppict (Word *w, int align, char has_param, int param)
{
	return FALSE;
}

/*========================================================================
 * Name:	cmd_up
 * Purpose:	Executes the \up command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_up (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
		attr_pop(ATTR_SUPER);
	else
		attr_push(ATTR_SUPER,NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_u
 * Purpose:	Processes a Unicode character
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, always false
 *=======================================================================*/

static int cmd_u (Word *w, int align, char has_param, int param) {
/* TODO: Unicode characters won't be correctly preprocessed if sizeof(int) < 4
 *      and document have unicode character which value is greater than 65536
 */

	short	done=0;
	long unicode_number = (long) param; /* On 16bit architectures int is too small to store unicode characters. - AF */
	char tmp[12]; /* Number of characters that can be in int type (including '\0'). If int size is greater than 4 bytes change this value. - AF */
	const char *alias;
#define DEBUG 0
#if DEBUG
	char	*str;
	if (has_param == TRUE)
	{
		fprintf(stderr,"param is %d (x%x) (0%o)\n", param,
			param, param);
	}
	if (w->hash_index)
	{
		str=hash_get_string (w->hash_index);
		fprintf(stderr,"string is %s\n", str);
	}
#endif
	/* 0.20.3 - daved added missing function call for unprocessed chars */
	if ((alias = get_alias(op, param)) != NULL)
	{
		printf("%s", alias);
		done++;
	}
	else
		if(!done && op->unisymbol_print)
		{
			if (unicode_number < 0)
			{
                            /* RTF spec: Unicode values beyond 32767 are represented by negative numbers */
				unicode_number += 65536;
			}
			sprintf(tmp, "%ld", unicode_number);

			if (safe_printf(1, op->unisymbol_print, tmp)) fprintf(stderr, TOO_MANY_ARGS, "unisymbol_print");
			done++;
		}
	
	/*
	** if we know how to represent the unicode character in the
	** output language, we need to skip the next word, otherwise
	** we will output that alternative.
	*/
	if (done)
		return(SKIP_ONE_WORD);
	return(FALSE);
}

/*========================================================================
 * Name:	cmd_dn
 * Purpose:	Executes the \dn command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_dn (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
		attr_pop(ATTR_SUB);
	else
		attr_push(ATTR_SUB,NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_nosupersub
 * Purpose:	Executes the \nosupersub command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_nosupersub (Word *w, int align, char has_param, int param) {
	attr_pop(ATTR_SUPER);
	attr_pop(ATTR_SUB);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_super
 * Purpose:	Executes the \super command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_super (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
		attr_pop(ATTR_SUPER);
	else
		attr_push(ATTR_SUPER,NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_sub
 * Purpose:	Executes the \sub command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_sub (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
		attr_pop(ATTR_SUB);
	else
		attr_push(ATTR_SUB,NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_shad
 * Purpose:	Executes the \shad command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_shad (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
		attr_pop(ATTR_SHADOW);
	else
		attr_push(ATTR_SHADOW,NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_b
 * Purpose:	Executes the \b command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
#define SUPPORT_UNNESTED

static int 
cmd_b (Word *w, int align, char has_param, int param) {
	if (has_param && param==0) {
#ifdef SUPPORT_UNNESTED
		attr_find_pop(ATTR_BOLD);
#else
		attr_pop(ATTR_BOLD);
#endif
	}
	else
		attr_push(ATTR_BOLD,NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_i
 * Purpose:	Executes the \i command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_i (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
#ifdef SUPPORT_UNNESTED
		attr_find_pop(ATTR_ITALIC);
#else
		attr_pop(ATTR_ITALIC);
#endif
	else
		attr_push(ATTR_ITALIC,NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_s
 * Purpose:	Executes the \s command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/
static int cmd_s (Word *w, int align, char has_param, int param) {
	return FALSE;
}

/*========================================================================
 * Name:	cmd_sect
 * Purpose:	Executes the \sect command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_sect (Word *w, int align, char has_param, int param) {
	/* XX kludge */
	if (op->paragraph_begin) {
		if (safe_printf(0, op->paragraph_begin)) fprintf(stderr, TOO_MANY_ARGS, "paragraph_begin");
	}
	return FALSE;
}

/*========================================================================
 * Name:	cmd_shp
 * Purpose:	Executes the \shp command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_shp (Word *w, int align, char has_param, int param) {
	if (op->comment_begin) {
		if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
		printf("Drawn Shape (ignored-not implemented yet)");
		if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
	}

	return FALSE;
}

/*========================================================================
 * Name:	cmd_outl
 * Purpose:	Executes the \outl command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_outl (Word *w, int align, char has_param, int param) {
	if (has_param && param==0)
		attr_pop(ATTR_OUTLINE);
	else
		attr_push(ATTR_OUTLINE,NULL);
	return FALSE;
}

/*========================================================================
 * Name:	cmd_ansi
 * Purpose:	Executes the \ansi command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ansi (Word *w, int align, char has_param, int param) {
    default_encoding = "CP1252";
    return FALSE;
}

/*========================================================================
 * Name:	cmd_ansicpg
 * Purpose:	Executes the \ansicpg command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ansicpg (Word *w, int align, char has_param, int param)
{
        default_encoding = cptoencoding(param);
        had_ansicpg = 1;
	return FALSE;
}

/*========================================================================
 * Name:	cmd_pc
 * Purpose:	Executes the \pc command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_pc (Word *w, int align, char has_param, int param) 
{
    default_encoding = "CP437";
    return FALSE;
}

/*========================================================================
 * Name:	cmd_pca
 * Purpose:	Executes the \pca command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_pca (Word *w, int align, char has_param, int param) 
{
    default_encoding = "CP850";
    return FALSE;
}

/*========================================================================
 * Name:	cmd_mac
 * Purpose:	Executes the \mac command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_mac (Word *w, int align, char has_param, int param) 
{
    default_encoding = "MAC";
    return FALSE;
}

/*========================================================================
 * Name:	cmd_colortbl
 * Purpose:	Executes the \colortbl command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_colortbl (Word *w, int align, char has_param, int param) {
	if (w->next) {
		process_color_table(w->next);
	}
	return TRUE;
}

/*========================================================================
 * Name:	cmd_fonttbl
 * Purpose:	Executes the \fonttbl command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_fonttbl (Word *w, int align, char has_param, int param) {
	if (w->next) {
		process_font_table(w->next);
	}
	return TRUE;
}

/*========================================================================
 * Name:	cmd_header
 * Purpose:	Executes the \header command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_header (Word *w, int align, char has_param, int param) {
	return TRUE;
}

/*========================================================================
 * Name:	cmd_headerl
 * Purpose:	Executes the \headerl command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_headerl (Word *w, int align, char has_param, int param) {
	return TRUE;
}

/*========================================================================
 * Name:	cmd_headerr
 * Purpose:	Executes the \headerr command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_headerr (Word *w, int align, char has_param, int param) {
	return TRUE;
}

/*========================================================================
 * Name:	cmd_headerf
 * Purpose:	Executes the \headerf command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_headerf (Word *w, int align, char has_param, int param) {
	return TRUE;
}

/*========================================================================
 * Name:	cmd_footer
 * Purpose:	Executes the \footer command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_footer (Word *w, int align, char has_param, int param) {
	return TRUE;
}

/*========================================================================
 * Name:	cmd_footerl
 * Purpose:	Executes the \footerl command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_footerl (Word *w, int align, char has_param, int param) {
	return TRUE;
}

/*========================================================================
 * Name:	cmd_footerr
 * Purpose:	Executes the \footerr command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_footerr (Word *w, int align, char has_param, int param) {
	return TRUE;
}

/*========================================================================
 * Name:	cmd_footerf
 * Purpose:	Executes the \footerf command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_footerf (Word *w, int align, char has_param, int param) {
	return TRUE;
}

/*========================================================================
 * Name:	cmd_ignore
 * Purpose:	Dummy function to get rid of subgroups 
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_ignore (Word *w, int align, char has_param, int param)
{
	return TRUE;
}

 /*========================================================================
  * Name:     cmd_maybe_ignore
  * Purpose:  Called when encountering {\* which specifies that the whole
  *           group should be discarded if the immediately following command is
  *           not known.
  * Args:     Word, paragraph align info, and numeric param if any.
  * Returns:  Flag, true only if rest of Words on line should be ignored.
  *=======================================================================*/

static int cmd_maybe_ignore (Word *w, int align, char has_param, int param)
{
	/* If the next command is known, we let it decide what to do 
	 * (which may still be to discard the group. If it is not found
	 * we return TRUE to discard the next elements.
	 */
	if(w && w->next)
	{
		int hasparam, param;
		const char *s = word_string(w->next);
		if (s && s[0] == '\\' && find_command(s + 1, &hasparam, &param))
			return FALSE;
	}
	
	return TRUE;
}

/*========================================================================
 * Name:	cmd_blipuid
 * Purpose:	Dummy function to get rid of uid 
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_blipuid (Word *w, int align, char has_param, int param)
{
	CHECK_PARAM_NOT_NULL (w);
	return TRUE;
}

/*========================================================================
 * Name:	cmd_info
 * Purpose:	Executes the \info command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_info (Word *w, int align, char has_param, int param) {
	process_info_group (w->next);
	return TRUE;
}

/*========================================================================
 * Name:	cmd_pict
 * Purpose:	Executes the \pict command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_pict (Word *w, int align, char has_param, int param)
{
	within_picture=TRUE;
	picture_width = picture_height = 0;
	picture_type = PICT_WB;
	return FALSE;		
}
/*========================================================================
 * Name:	cmd_picprop
 * Purpose:	Executes the \picprop
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_picprop (Word *w, int align, char has_param, int param)
{
	return TRUE;		
}


/*========================================================================
 * Name:	cmd_bin
 * Purpose:	Executes the \bin command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_bin (Word *w, int align, char has_param, int param) {
	return FALSE;		
}


/*========================================================================
 * Name:	cmd_macpict
 * Purpose:	Executes the \macpict command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_macpict (Word *w, int align, char has_param, int param) {
	picture_type = PICT_MAC;
	return FALSE;		
}

/*========================================================================
 * Name:	cmd_jpegblip
 * Purpose:	Executes the \jpegblip command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_jpegblip (Word *w, int align, char has_param, int param) {
	picture_type = PICT_JPEG;
	return FALSE;		
}

/*========================================================================
 * Name:	cmd_pngblip
 * Purpose:	Executes the \pngblip command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_pngblip (Word *w, int align, char has_param, int param) {
	picture_type = PICT_PNG;
	return FALSE;		
}

/*========================================================================
 * Name:	cmd_emfblip
 * Purpose:	Executes the \emfblip command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_emfblip (Word *w, int align, char has_param, int param) {
	picture_type = PICT_EMF;
	return FALSE;		
}

/*========================================================================
 * Name:	cmd_pnmetafile
 * Purpose:	Executes the \pnmetafile command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_pnmetafile (Word *w, int align, char has_param, int param) {
	picture_type = PICT_PM;
	return FALSE;		
}

/*========================================================================
 * Name:	cmd_wmetafile
 * Purpose:	Executes the \wmetafile command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_wmetafile (Word *w, int align, char has_param, int param) {
	picture_type = PICT_WM;
	if (within_picture && has_param) {
		picture_wmetafile_type=param;
		switch(param) {
		case 1: picture_wmetafile_type_str="MM_TEXT"; break;
		case 2: picture_wmetafile_type_str="MM_LOMETRIC"; break;
		case 3: picture_wmetafile_type_str="MM_HIMETRIC"; break;
		case 4: picture_wmetafile_type_str="MM_LOENGLISH"; break;
		case 5: picture_wmetafile_type_str="MM_HIENGLISH"; break;
		case 6: picture_wmetafile_type_str="MM_TWIPS"; break;
		case 7: picture_wmetafile_type_str="MM_ISOTROPIC"; break;
		case 8: picture_wmetafile_type_str="MM_ANISOTROPIC"; break;
		default: picture_wmetafile_type_str="default:MM_TEXT"; break;
		}
	}
	return FALSE;		
}

/*========================================================================
 * Name:	cmd_wbmbitspixel
 * Purpose:	Executes the \wbmbitspixel command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_wbmbitspixel (Word *w, int align, char has_param, int param) {
	if (within_picture && has_param) 
		picture_bits_per_pixel = param;
	return FALSE;		
}

/*========================================================================
 * Name:	cmd_picw
 * Purpose:	Executes the \picw command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_picw (Word *w, int align, char has_param, int param) {
	if (within_picture && has_param) 
		picture_width = param;
	return FALSE;		
}

/*========================================================================
 * Name:	cmd_pich
 * Purpose:	Executes the \pich command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_pich (Word *w, int align, char has_param, int param) {
	if (within_picture && has_param) 
		picture_height = param;
	return FALSE;		
}


/*========================================================================
 * Name:	cmd_xe
 * Purpose:	Executes the \xe (index entry) command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_xe (Word *w, int align, char has_param, int param) {
	process_index_entry (w);
	return TRUE;		
}

/*========================================================================
 * Name:	cmd_tc
 * Purpose:	Executes the \tc (TOC entry) command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_tc (Word *w, int align, char has_param, int param) {
	process_toc_entry (w, TRUE);
	return TRUE;		
}

/*========================================================================
 * Name:	cmd_tcn
 * Purpose:	Executes the \tcn (TOC entry, no page #) command.
 * Args:	Word, paragraph align info, and numeric param if any.
 * Returns:	Flag, true only if rest of Words on line should be ignored.
 *=======================================================================*/

static int cmd_tcn (Word *w, int align, char has_param, int param) {
	process_toc_entry (w, FALSE);
	return TRUE;		
}


/* All of the possible commands that RTF might recognize. */
static HashItem hashArray_other [] = {
/* 0.21.7
 *	the "*" command was ignored in earlier versions, but included pictures
 *	seem to often start with \*\shppict{\pict so if "*" is ignored, so is
 *	the picture, so I have defined a new function "cmd_maybe_ignore" which
 *	tests whether the next word is shppict and if so does not ignore what
 *	follows.  On simple tests this seems to work.  - daved
 */
#if 0
	{ "*", cmd_ignore, NULL },
#else
	{ "*", cmd_maybe_ignore, NULL },
#endif
	{ "-", cmd_optional_hyphen, "optional hyphen" },
	{ "_", cmd_nonbreaking_hyphen, "nonbreaking hyphen" },
	{ "~", cmd_nonbreaking_space, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_a [] = {
	{ "ansi", &cmd_ansi , NULL },
	{ "ansicpg", &cmd_ansicpg , NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_b [] = {
	{ "b", &cmd_b, NULL },
	{ "bullet", &cmd_bullet, NULL },
	{ "bin", &cmd_bin, "picture is binary" },
	{ "blipuid", &cmd_blipuid, NULL },
#if 0
	{ "bgbdiag", NULL, NULL },
	{ "bgcross", NULL, NULL },
	{ "bgdcross", NULL, NULL },
	{ "bgfdiag", NULL, NULL },
	{ "bghoriz", NULL, NULL },
	{ "bgkbdiag", NULL, NULL },
	{ "bgkcross", NULL, NULL },
	{ "bgkdcross", NULL, NULL },
	{ "bgkfdiag", NULL, NULL },
	{ "bgkhoriz", NULL, NULL },
	{ "bgkvert", NULL, NULL },
	{ "bgvert", NULL, NULL },
	{ "brdrcf", NULL, NULL },
	{ "brdrdb", NULL, NULL },
	{ "brdrdot", NULL, NULL },
	{ "brdrhair", NULL, NULL },
	{ "brdrs", NULL, NULL },
	{ "brdrsh", NULL, NULL },
	{ "brdrth", NULL, NULL },
	{ "brdrw", NULL, NULL },
#endif
	{ NULL, NULL, NULL}
};
static HashItem hashArray_c [] = {
	{ "caps", &cmd_caps, NULL },
	{ "cb", cmd_cb, NULL },
	{ "cf", cmd_cf, NULL },
	{ "colortbl", &cmd_colortbl, "color table" },
	{ "cols", NULL, "columns (not implemented)" },
	{ "column", NULL, "column break (not implemented)" },
	{ "cbpat", NULL, "Paragraph Shading" },
	{ "cellx", NULL, "Table Definitions" },
	{ "cfpat", NULL, NULL },
	{ "cgrid", NULL, NULL },
	{ "charrsid", NULL, "Revision Mark (ignore)" },
	{ "clbgbcross", NULL, NULL },
	{ "clbgbdiag", NULL, NULL },
	{ "clbgbkbdiag", NULL, NULL },
	{ "clbgbkcross", NULL, NULL },
	{ "clbgbkdcross", NULL, NULL },
	{ "clbgbkfdiag", NULL, NULL },
	{ "clbgbkhor", NULL, NULL },
	{ "clbgbkvert", NULL, NULL },
	{ "clbgdcross", NULL, NULL },
	{ "clbgfdiag", NULL, NULL },
	{ "clbghoriz", NULL, NULL },
	{ "clbgvert", NULL, NULL },
	{ "clbrdrb", NULL, NULL },
	{ "clbrdrl", NULL, NULL },
	{ "clbrdrr", NULL, NULL },
	{ "clbrdrt", NULL, NULL },
	{ "clcbpat", NULL, NULL },
	{ "clcfpat", NULL, NULL },
	{ "clmgf", NULL, NULL },
	{ "clmrg", NULL, NULL },
	{ "clshdng", NULL, NULL },
	{ "cs", NULL, "character style (not implemented)"},
	{ NULL, NULL, NULL}
};
static HashItem hashArray_d [] = {
	{ "deff", cmd_deff, "Default Font" },
	{ "dn", &cmd_dn, NULL },
#if 0
	{ "dibitmap", NULL, NULL },
#endif
	{ NULL, NULL, NULL}
};
static HashItem hashArray_e [] = {
	{ "emdash", cmd_emdash, NULL },
	{ "endash", cmd_endash, NULL },
	{ "embo", &cmd_emboss, NULL },
	{ "expand", &cmd_expand, NULL },
	{ "expnd", &cmd_expand, NULL },
	{ "emfblip", &cmd_emfblip, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_f [] = {
	{ "f", cmd_f, NULL },
	{ "fdecor", cmd_fdecor, NULL },
	{ "fmodern", cmd_fmodern, NULL },
	{ "fnil", cmd_fnil, NULL },
	{ "fonttbl", cmd_fonttbl, "font table" },
	{ "froman", cmd_froman, NULL },
	{ "fs", cmd_fs, NULL },
	{ "fscript", cmd_fscript, NULL },
	{ "fswiss", cmd_fswiss, NULL },
	{ "ftech", cmd_ftech, NULL },
	{ "field", cmd_field, NULL },
	{ "footer", cmd_footer, NULL },
	{ "footerf", cmd_footerf, NULL },
	{ "footerl", cmd_footerl, NULL },
	{ "footerr", cmd_footerr, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_h [] = {
	{ "highlight", &cmd_highlight, NULL },
	{ "header", cmd_header, NULL },
	{ "headerf", cmd_headerf, NULL },
	{ "headerl", cmd_headerl, NULL },
	{ "headerr", cmd_headerr, NULL },
	{ "hl", cmd_ignore, "hyperlink within object" },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_i [] = {
	{ "i", &cmd_i, NULL },
	{ "info", &cmd_info, NULL },
	{ "insrsid", NULL, "Revision Mark (ignore)" },
	{ "intbl", &cmd_intbl, NULL },
	{ "impr", &cmd_engrave, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_j [] = {
	{ "jpegblip", &cmd_jpegblip, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_l [] = {
	{ "ldblquote", &cmd_ldblquote, NULL },
	{ "line", &cmd_line, NULL },
	{ "lquote", &cmd_lquote, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_m [] = {
	{ "mac", &cmd_mac , NULL },
	{ "macpict", &cmd_macpict, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_n [] = {
	{ "nosupersub", &cmd_nosupersub, NULL },
	{ "nonshppict", &cmd_ignore, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_o [] = {
	{ "outl", &cmd_outl, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_p [] = {
	{ "page", &cmd_page, NULL },
	{ "par", &cmd_par, NULL },
	{ "pc", &cmd_pc , NULL },
	{ "pca", &cmd_pca , NULL },
	{ "pich", &cmd_pich, NULL },
	{ "pict", &cmd_pict, "picture" },
	{ "picprop", &cmd_picprop, "picture properties" },
	{ "picw", &cmd_picw, NULL },
	{ "plain", &cmd_plain, NULL },
	{ "pngblip", &cmd_pngblip, NULL },
	{ "pnmetafile", &cmd_pnmetafile, NULL },
	{ "emfblip", &cmd_emfblip, NULL },
#if 0
	{ "piccropb", NULL, NULL },
	{ "piccropl", NULL, NULL },
	{ "piccropr", NULL, NULL },
	{ "piccropt", NULL, NULL },
	{ "pichgoal", NULL, NULL },
	{ "pichgoal", NULL, NULL },
	{ "picscaled", NULL, NULL },
	{ "picscalex", NULL, NULL },
	{ "picwgoal", NULL, NULL },
#endif
	{ NULL, NULL, NULL}
};
static HashItem hashArray_r [] = {
	{ "rdblquote", &cmd_rdblquote, NULL },
	{ "rquote", &cmd_rquote, NULL },
	{ "rtf", &cmd_rtf, NULL },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_s [] = {
	{ "s", cmd_s, "style" },
	{ "sect", &cmd_sect, "section break"},
	{ "scaps", &cmd_scaps, NULL },
	{ "super", &cmd_super, NULL },
	{ "sub", &cmd_sub, NULL },
	{ "shad", &cmd_shad, NULL },
	{ "strike", &cmd_strike, NULL },
	{ "striked", &cmd_striked, NULL },
	{ "strikedl", &cmd_strikedl, NULL },
	{ "stylesheet", &cmd_ignore, "style sheet" },
	{ "shp", cmd_shp, "drawn shape" },
	{ "shppict", &cmd_shppict, "shppict wrapper" },
#if 0
	{ "shading", NULL, NULL },
#endif
	{ NULL, NULL, NULL}
};
static HashItem hashArray_t [] = {
	{ "tab", &cmd_tab, NULL },
	{ "tc", cmd_tc, "TOC entry" },
	{ "tcn", cmd_tcn, "TOC entry" },
	{ "trowd", NULL, "start new row in table" },
	{ NULL, NULL, NULL}
};
static HashItem hashArray_u [] = {
	{ "u", &cmd_u, NULL },
	{ "ul", &cmd_ul, NULL },
	{ "up", &cmd_up, NULL },
	{ "uld", &cmd_uld, NULL },
	{ "uldash", &cmd_uldash, NULL },
	{ "uldashd", &cmd_uldashd, NULL },
	{ "uldashdd", &cmd_uldashdd, NULL },
	{ "uldb", &cmd_uldb, NULL },
	{ "ulnone", &cmd_ulnone, NULL },
	{ "ulth", &cmd_ulth, NULL },
	{ "ulthd", &cmd_ulthd, NULL },
	{ "ulthdash", &cmd_ulthdash, NULL },
	{ "ulw", &cmd_ulw, NULL },
	{ "ulwave", &cmd_ulwave, NULL },
	{ NULL, NULL, NULL}
};

static HashItem hashArray_v [] = {
	{ "v", NULL, "Hidden Text" },
	{ NULL, NULL, NULL }
};

static HashItem hashArray_w [] = {
	{ "wbmbitspixel", &cmd_wbmbitspixel, NULL },
	{ "wmetafile", &cmd_wmetafile, NULL },
	{ NULL, NULL, NULL}
};

static HashItem hashArray_x [] = {
	{ "xe", cmd_xe, "index entry" },
	{ NULL, NULL, NULL}
};

static HashItem *hash [26] = {
	hashArray_a,
	hashArray_b,
	hashArray_c,
	hashArray_d,
	hashArray_e,
	hashArray_f,
	NULL,
	hashArray_h,
	hashArray_i,
	hashArray_j,
	NULL,
	hashArray_l,
	hashArray_m,
	hashArray_n,
	hashArray_o,
	hashArray_p,
	NULL,
	hashArray_r,
	hashArray_s,
	hashArray_t,
	hashArray_u,
	hashArray_v,
	hashArray_w,
	hashArray_x, 
	NULL, NULL
};


/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/

/*========================================================================
 * Name:    find_command
 * Purpose: Search command lists for input string and return handler and possible parameter
 * Args:    cmdpp pointer to string with command and optional parameter.
 *            ex: "\cmd ..." "\cmd123A..." "\cmd-2ABC..."
 *          hasparamp: parameter existence flag (output)
 *          paramp: parameter value if hasparamp is set
 * Returns: Pointer to command structure, or NULL
 *=======================================================================*/

static HashItem *find_command(const char *cmdpp, int *hasparamp, int *paramp)
{
	HashItem *hip;
	int ch;
	const char *p; /* Start of parameter */
	int len;

	/* Look for a parameter */
	*hasparamp = FALSE;
	p = cmdpp;
	while (*p && (!isdigit(*p) && *p != '-')) 
		p++;
	if (*p && (isdigit(*p) || *p == '-')) { 
		*hasparamp = TRUE; 
		*paramp = atoi(p);
	}
	len = p - cmdpp;

	/* Generate a hash index */
	ch = tolower(*cmdpp);
	if (ch >= 'a' && ch <= 'z') 
		hip = hash[ch - 'a'];
	else
		hip = hashArray_other;

	if (!hip) {
		if (debug_mode) {
			if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
			printf("Unfamiliar RTF command: %s (HashIndex not found)", cmdpp);
			if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
		}
		return NULL;
	}

	while (hip->name) {
		/* Don't change the order of tests ! */
		if (!strncmp(cmdpp, hip->name, len) && hip->name[len] == 0)
			return hip;
		hip++;
	}

	if (debug_mode) {
		if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
		printf("Unfamiliar RTF command: %s", cmdpp);
		if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
	}
	return NULL;
}


/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/


/*========================================================================
 * Name:	print_with_special_exprs
 * Purpose: print "normal" content string, obtained as "as-is" text,
 *          not through an \' or \u or other command. The input is
            guaranteed to be a string of 7-8 bit bytes representing
            single-byte characters in the current encoding. The things
            we do in there (with the caps conversion) would be seriously
            wrong if the input was an encoding not
            apparented to ascii, which hopefully is never the case.
            Also we just drop all chars outside [0x20-0x80[
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
print_with_special_exprs (const char *s) {
	int ch;
	int state;

enum { SMALL=0, BIG=1 };

	CHECK_PARAM_NOT_NULL(s);

        // Not sure that there are cases where the flush is needed, 
        // but this is conceivable, and can't hurt in any case.
        flush_iconv_input();

        maybeopeniconv();

	state=SMALL;  /* Pacify gcc,  st001906 - 0.19.6 */
	if (simulate_smallcaps) {
		if (*s >= 'a' && *s <= 'z') {
			state=SMALL;
			if (safe_printf(0, op->smaller_begin)) fprintf(stderr, TOO_MANY_ARGS, "smaller_begin");
		}
		else
			state=BIG;
	}

	while ((ch=*s)) {
		const char *post_trans = NULL;

		if (simulate_allcaps || simulate_smallcaps)
			ch = toupper (ch);

		if (ch >= 0x20 && ch < 0x80) {
			post_trans= op_translate_char(op, current_encoding, ch);
			if(post_trans)
				printf("%s",post_trans);
		}

		s++;

		if (simulate_smallcaps) {
			ch = *s;
			if (ch >= 'a' && ch <= 'z') {
				if (state==BIG)
					if (safe_printf(0, op->smaller_begin)) fprintf(stderr, TOO_MANY_ARGS, "smaller_begin");
				state=SMALL;
			}
			else
			{
				if (state==SMALL)
					if (safe_printf(0, op->smaller_end)) fprintf(stderr, TOO_MANY_ARGS, "smaller_end");
				state=BIG;
			}
		}
	}
}



/*========================================================================
 * Name:	
 * Purpose:	
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

static void 
begin_table()
{
	within_table=TRUE;
	have_printed_row_begin = FALSE;
	have_printed_cell_begin = FALSE;
	have_printed_row_end = FALSE;
	have_printed_cell_end = FALSE;
	attrstack_push();
	starting_body();
	if (safe_printf(0, op->table_begin)) fprintf(stderr, TOO_MANY_ARGS, "table_begin");
}


/*========================================================================
 * Name:	end_table
 * Purpose:	finish off table
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
end_table ()
{
	if (within_table) {
		if (!have_printed_cell_end) {
			attr_pop_dump();
			if (safe_printf(0, op->table_cell_end)) fprintf(stderr, TOO_MANY_ARGS, "table_cell_end");
		}
		if (!have_printed_row_end) {
			if (safe_printf(0, op->table_row_end)) fprintf(stderr, TOO_MANY_ARGS, "table_row_end");
		}
		if (safe_printf(0, op->table_end)) fprintf(stderr, TOO_MANY_ARGS, "table_end");
		within_table=FALSE;
		have_printed_row_begin = FALSE;
		have_printed_cell_begin = FALSE;
		have_printed_row_end = FALSE;
		have_printed_cell_end = FALSE;
	}
}

/*=======================================================================
 * Name:	check_for_table
 * Purpose:	make certain table has been started
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

static void check_for_table()
{
	//printf("EH: %d %d", coming_pars_that_are_tabular, within_table);

	if (!coming_pars_that_are_tabular && within_table)
	{
		//printf("END TABLE\n");
		end_table();
	}
	else if (coming_pars_that_are_tabular && !within_table)
	{
		//printf("BEGIN TABLE");
		begin_table();
	}
}

/*========================================================================
 * Name:	
 * Purpose:	
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
starting_text() {
	if (within_table) {
		if (!have_printed_row_begin) {
			if (safe_printf(0, op->table_row_begin)) fprintf(stderr, TOO_MANY_ARGS, "table_row_begin");
			have_printed_row_begin=TRUE;
			have_printed_row_end=FALSE;
			have_printed_cell_begin=FALSE;
		}
		if (!have_printed_cell_begin) {
			if (safe_printf(0, op->table_cell_begin)) fprintf(stderr, TOO_MANY_ARGS, "table_cell_begin");
			attrstack_express_all();
			have_printed_cell_begin=TRUE;
			have_printed_cell_end=FALSE;
		}
	}
}




/*========================================================================
 * Name:	
 * Purpose:	
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

static void
starting_paragraph_align (int align)
{
	if (within_header && align != ALIGN_LEFT)
		starting_body();

	switch (align) 
	{
	case ALIGN_CENTER:
		if (safe_printf(0, op->center_begin)) fprintf(stderr, TOO_MANY_ARGS, "center_begin");
		break;
	case ALIGN_LEFT:
		break;
	case ALIGN_RIGHT:
		if (safe_printf(0, op->align_right_begin)) fprintf(stderr, TOO_MANY_ARGS, "align_right_begin");
		break;
	case ALIGN_JUSTIFY:
		if (safe_printf(0, op->justify_begin)) fprintf(stderr, TOO_MANY_ARGS, "justify_begin"); /* But this is correct */
		break;
	}
}



/*========================================================================
 * Name:	
 * Purpose:	
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

static void
ending_paragraph_align (int align)
{
	switch (align) {
	case ALIGN_CENTER:
		if (safe_printf(0, op->center_end)) fprintf(stderr, TOO_MANY_ARGS, "center_end");
		break;
	case ALIGN_LEFT:
		break;
	case ALIGN_RIGHT:
		if (safe_printf(0, op->align_right_end)) fprintf(stderr, TOO_MANY_ARGS, "align_right_end");
		break;
	case ALIGN_JUSTIFY:
		if (safe_printf(0, op->justify_end)) fprintf(stderr, TOO_MANY_ARGS, "justify_end");
		break;
	}
}


#define IIBS 10240
static char iconv_buffer[IIBS];
static int iconv_cur = 0;
static void
flush_iconv_input()
{
/*  fprintf(stderr, "flush_iconv_input: iconv_cur %d\n", iconv_cur);*/
    if (iconv_cur <= 0) {
        iconv_cur = 0;
        return;
    }
    maybeopeniconv();
    if (!my_iconv_is_valid(desc)) {
        fprintf(stderr, "unrtf: flush: iconv not ready!\n");
        return;
    }

    char obuf[IIBS];
    size_t isiz = iconv_cur;
    char *ip = iconv_buffer;
    while (isiz > 0) {
        size_t osiz;
        char *ocp;

        osiz = IIBS;
        ocp = obuf;
        errno = 0;
        if(my_iconv(desc, &ip, &isiz, &ocp, &osiz) == (size_t)-1 && errno != E2BIG) {
            if (errno == EINVAL) {
                // Incomplete input sequence. Copy it to the
                // beginning of the buffer and leave it around
                // (more data probably coming)
                // iconv leaves ip pointing at the
                // beginning of the sequence.
                int cnt = iconv_cur - (ip - iconv_buffer);
/*            fprintf(stderr, "flush_iconv: incomp. input remain %d\n", cnt);*/
                memcpy(obuf, ip, cnt);
                memcpy(iconv_buffer, obuf, cnt);
                iconv_cur = cnt;
                return;
            } else {
/*           fprintf(stderr, "flush_iconv: rem. %d errno %d\n", isiz, errno);*/
                iconv_cur = 0;
                return;
            }
        }
/*        fprintf(stderr, "flush_iconv: ok: isiz %d out %d errno %d\n", 
          isiz, IIBS - osiz, errno); */

        
/*        fwrite(obuf, 1, IIBS - osiz, stdout);*/
        char *out = op_translate_buffer(op, obuf, IIBS - osiz);
        if (out == 0) {
            iconv_cur = 0;
            return;
        }
        fprintf(stdout, "%s", out);
        my_free(out);
    }
    iconv_cur = 0;
}

static void
accumulate_iconv_input(int ch)
{
/*    fprintf(stderr, "accumulate_iconv_input: 0x%x\n", ch);*/
    if (iconv_cur >= IIBS-1)
        flush_iconv_input();
    iconv_buffer[iconv_cur++] = ch;
}

/*========================================================================
 * Name:	
 * Purpose:	Recursive routine to produce the output in the target
 *		format given on a tree of words.
 * Args:	Word* (the tree).
 * Returns:	None.
 *=======================================================================*/

static void
word_print_core (Word *w, int groupdepth)
{
	const char *s;
	const char *alias;
	FILE *pictfile=NULL;
	int is_cell_group=FALSE;
	int paragraph_begined=FALSE;
	int paragraph_align=ALIGN_LEFT;

        if (groupdepth > MAX_GROUP_DEPTH) {
		warning_handler ("Max group depth reached");
		return;
        }
	CHECK_PARAM_NOT_NULL(w);

	//if (!coming_pars_that_are_tabular && within_table) {
		//end_table();
	//}
	//else if (coming_pars_that_are_tabular && !within_table) {
		//begin_table();
	//}
	check_for_table();

	/* Mark our place in the stack */
	attrstack_push();

	while (w) {

                s = word_string (w);
                // If we have hex data and we're getting out of the hex area
                // flush it.
                if (iconv_cur > 0 && s && strncmp(s, "\\'", 2))
                    flush_iconv_input();

		if (s) {

			/*--Ignore whitespace in header--------------------*/
			if (*s==' ' && within_header) {
				/* no op */
			} 
			else
			/*--Handle word -----------------------------------*/
			if (s[0] != '\\') 
			{
				starting_body();
				starting_text();

				if (!paragraph_begined) {
					starting_paragraph_align (paragraph_align);
					paragraph_begined=TRUE;
				}

				/*----------------------------------------*/
				if (within_picture)
				{
					if (within_picture_depth == 0)
					    within_picture_depth = groupdepth;
					starting_body();
					if (!pictfile && !nopict_mode) {
						char *ext=NULL;
						switch (picture_type) {
						case PICT_WB: ext="bmp"; break;
						case PICT_WM: ext="wmf"; break;
						case PICT_MAC: ext="pict"; break;
						case PICT_JPEG: ext="jpg"; break;
						case PICT_PNG: ext="png"; break;
						case PICT_DI: ext="dib"; break; /* Device independent bitmap=??? */
						case PICT_PM: ext="pmm"; break; /* OS/2 metafile=??? */
						case PICT_EMF: ext="emf"; break;  /* Enhanced MetaFile */
						}
						sprintf(picture_path, "pict%03d.%s", 
							picture_file_number++,ext);
						pictfile=fopen(picture_path,"wb");
					}

					if (s[0]!=' ') {
						const char *s2;
						if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
						printf("picture data found, ");
						if (picture_wmetafile_type_str) {
							printf("WMF type is %s, ",
								picture_wmetafile_type_str);
						}
						printf("picture dimensions are %d by %d, depth %d", 
							picture_width, picture_height, picture_bits_per_pixel);
						if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
						if (picture_width && picture_height && picture_bits_per_pixel) {
							s2=s;
							/* Convert hex char pairs. Guard against odd byte count from garbled file */
							while (*s2 && *(s2+1)) {
								unsigned int tmp,value;
								tmp=tolower(*s2++);
								if (tmp>'9') tmp-=('a'-10); 
								else tmp-='0';
								value=16*tmp;
								tmp=tolower(*s2++);
								if (tmp>'9') tmp-=('a'-10); 
								else tmp-='0';
								value+=tmp;
								if (pictfile) {
									fprintf(pictfile,"%c", value);
								}
							}
						}
					}
				}
				/*----------------------------------------*/
				else {
					total_chars_this_line += strlen(s);

					if (op->word_begin)
						if (safe_printf(0, op->word_begin)) fprintf(stderr, TOO_MANY_ARGS, "word_begin");

					print_with_special_exprs (s);

					if (op->word_end)
						if (safe_printf(0, op->word_end)) fprintf(stderr, TOO_MANY_ARGS, "word_end");
				}


			}
			/* output an escaped backslash */
			/* do we need special handling for latex? */
			/* we do for troff where we want the string for 92 */
			else if (*(s+1) == '\\')
			{
				s++;
				if ((alias = get_alias(op, 92)) != NULL)
				{
					printf("%s", alias);
				}
				else
				{
					putchar('\\');
				}
			}
			else if (*(s+1) == '{')
			{
				s++;
				putchar('{');
			}
			else if (*(s+1) == '}')
			{
				s++;
				putchar('}');
			}
			/*---Handle RTF keywords---------------------------*/
			else {
				s++;
/*----Paragraph alignment----------------------------------------------------*/
				if (!strcmp ("ql", s))
					paragraph_align = ALIGN_LEFT;
				else if (!strcmp ("qr", s))
					paragraph_align = ALIGN_RIGHT;
				else if (!strcmp ("qj", s))
					paragraph_align = ALIGN_JUSTIFY;
				else if (!strcmp ("qc", s))
					paragraph_align = ALIGN_CENTER;
				else if (!strcmp ("pard", s)) 
				{
					/* Clear out all font attributes.
					 */
					attr_pop_all();

					if (coming_pars_that_are_tabular) {
						--coming_pars_that_are_tabular;
					}

					/* Clear out all paragraph attributes.
					 */
					ending_paragraph_align(paragraph_align);
					paragraph_align = ALIGN_LEFT;
					paragraph_begined = FALSE;
				}
/*----Table keywords---------------------------------------------------------*/
				else
				if (!strcmp (s, "cell")) {

					is_cell_group=TRUE;
					if (!have_printed_cell_begin) {
						/* Need this with empty cells */
						if (safe_printf(0, op->table_cell_begin)) fprintf(stderr, TOO_MANY_ARGS, "table_cell_begin");
						attrstack_express_all();
					}
					attr_pop_dump();
					if (safe_printf(0, op->table_cell_end)) fprintf(stderr, TOO_MANY_ARGS, "table_cell_end");
					have_printed_cell_begin = FALSE;
					have_printed_cell_end=TRUE;
				}
				else if (!strcmp (s, "row")) {

					if (within_table) {
						if (safe_printf(0, op->table_row_end)) fprintf(stderr, TOO_MANY_ARGS, "table_row_end");
						have_printed_row_begin = FALSE;
						have_printed_row_end=TRUE;
					} else {
						if (debug_mode) {
							if (safe_printf(0, op->comment_begin)) fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
							printf("end of table row");
							if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
						}
					}
				}

/*----Special chars---------------------------------------------------------*/
				else if (*s == '\'' && s[1] && s[2]) {
					/* \'XX is a hex char code expression */

					int ch = h2toi (&s[1]);
                                        accumulate_iconv_input(ch);
				}
				else
/*----Search the RTF command hash-------------------------------------------*/
				{
					int have_param = FALSE, param = 0;
					HashItem *hip = find_command(s, &have_param, &param);
					if (hip) {
						int terminate_group;

						if (hip->func) {
							terminate_group = hip->func (w,paragraph_align, have_param, param);
							/* daved - 0.19.4 - unicode support may need to skip only one word */
							if (terminate_group == SKIP_ONE_WORD)
								w=w->next;
							else
								if (terminate_group)
									while(w) w=w->next;
						}

						if (hip->debug_print && debug_mode) {
							if (safe_printf(0, op->comment_begin))
								fprintf(stderr, TOO_MANY_ARGS, "comment_begin");
							printf("%s", hip->debug_print);
							if (safe_printf(0, op->comment_end)) fprintf(stderr, TOO_MANY_ARGS, "comment_end");
						}

					}
				}
			}
/*-------------------------------------------------------------------------*/
		} else {

			Word *child;

			child = w->child;

			if (!paragraph_begined) {
				starting_paragraph_align (paragraph_align);
				paragraph_begined=TRUE;
			}

			if (child) 
				word_print_core (child, groupdepth+1);
		}

		if (w) 
			w = w->next;
	}

	if (within_picture && within_picture_depth == groupdepth)
	{
		within_picture_depth = 0;
		if(pictfile)
		{ 
			fclose(pictfile);
			if (safe_printf(0, op->imagelink_begin)) fprintf(stderr, TOO_MANY_ARGS, "imagelink_begin");
			printf("%s", picture_path);
			if (safe_printf(0, op->imagelink_end)) fprintf(stderr, TOO_MANY_ARGS, "imagelink_end");
		}
		within_picture=FALSE;
	}

	/* Undo font attributes UNLESS we're doing table cells
	 * since they would appear between </td> and </tr>.
	 */

	if (!is_cell_group)
		attr_pop_all();
	else
		attr_drop_all();

	/* Undo paragraph alignment
	 */
	if (paragraph_begined)
		ending_paragraph_align (paragraph_align);

	attrstack_drop();

        // Flush iconv input
        flush_iconv_input();
        iconv_cur = 0;

        // If there is an encoding in the stacks, restore it, else
        // restore default.
        desc = my_iconv_close(desc);
        char *encoding = attr_get_param(ATTR_ENCODING);
        if (!encoding || !*encoding) 
            encoding = default_encoding;
        desc = my_iconv_open(output_encoding, encoding);
        set_current_encoding(encoding);
}


/*========================================================================
 * Name:	
 * Purpose:	
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
word_print (Word *w) 
{
	CHECK_PARAM_NOT_NULL (w);

	if (!inline_mode) {
		if (safe_printf(0, op->document_begin)) fprintf(stderr, TOO_MANY_ARGS, "document_begin");
		if (safe_printf(0, op->header_begin)) fprintf(stderr, TOO_MANY_ARGS, "header_begin");
		if (safe_printf(0, op->utf8_encoding)) fprintf(stderr, TOO_MANY_ARGS, "utf8_encoding");
	}

	print_banner ();

	within_header=TRUE;
	have_printed_body=FALSE;
	within_table=FALSE;
	simulate_allcaps=FALSE;
	word_print_core (w, 1);
	end_table();

	if (!inline_mode) {
		if (safe_printf(0, op->body_end)) fprintf(stderr, TOO_MANY_ARGS, "body_end");
		if (safe_printf(0, op->document_end)) fprintf(stderr, TOO_MANY_ARGS, "document_end");
	}
}
