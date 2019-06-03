/*----------------------------------------------------------------------
 * Module name:  my_iconv
 * Author name:  Arkadiusz Firus
 * Create date:  07 Sep 08
 * Purpose:    iconv handles
 *----------------------------------------------------------------------
 Changes:
 * 04 Jan 10, daved@physiol.usyd.edu.au: use path specfied with -P to
 *	load charmap file(s)
 * 07 Oct 11, jf@dockes.org, major changes to unicode translations
 *--------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "malloc.h"
#include "my_iconv.h"
#include "util.h"
#include "unicode.h"
#include "path.h"

extern int verbose_mode;

/* Convert from charmap file entry to charmap table one. 
   1st byte in table entry is code length 
*/
static char *
get_code_str(FILE *f, iconv_t desc)
{
	char *icp, *ocp0, *ocp;
	size_t ibytes, obytes;
	char *obuf;

	char *utf8 = get_unicode_utf8(f);
	if (utf8 == NULL || *utf8 == 0) {
		/* fprintf(stderr, "get_code_str: NULL entry\n");*/
		return NULL;
	}
	#if 0 /* debug */
	fprintf(stderr, "get_code_str: utf8: ");
	for (ocp = utf8; *ocp; ocp++)
	fprintf(stderr, "%x", (unsigned)*ocp);
	fprintf(stderr, "\n");
	#endif

	obytes = 10;
	ibytes = strlen(utf8);
	obuf = malloc(obytes);
	if (obuf == NULL)
		return NULL;
	icp = utf8;
	ocp0 = ocp = obuf + 1;
	if (iconv(desc, &icp, &ibytes, &ocp, &obytes) == -1) {
		/*        fprintf(stderr, "unrtf: my_iconv: iconv error\n");*/
		return NULL;
	}

	// Set 1st byte to length
	obuf[0] = ocp - ocp0;
	my_free(utf8);
	return obuf;
}

my_iconv_t
my_iconv_open(const char *tocode, const char *fromcode)
{
	FILE *f;
	my_iconv_t cd = MY_ICONV_T_CLEAR;
	int c, i;
/*      fprintf(stderr, "my_iconv_open: from %s to %s\n", fromcode, tocode);*/
	if ((cd.desc = iconv_open(tocode, fromcode)) == (iconv_t) -1) {
	    char *path = search_in_path(fromcode, "charmap");
            if (path == NULL) {
                return cd;
            }
            if((f = fopen(path, "r")) == NULL && verbose_mode)
                fprintf(stderr, "failed to open charmap file %s\n", path);

            if (f != NULL) {
                /* Open iconv utf8->tocode conversion */
                iconv_t desc;
                if ((desc = iconv_open(tocode, "UTF-8")) == (iconv_t) -1) {
                    fclose(f);
                    return cd;
                }
                cd.char_table = (char **)my_malloc(char_table_size * 
                                                   sizeof(char *));
                c = fgetc(f);

                for (i = 0; i < char_table_size && c != EOF; i++)
                {
                    if (c == '<')
                        cd.char_table[i] = get_code_str(f, desc);
                    leave_line(f);//read up to including \n or eof
                    c = fgetc(f);
                }
                iconv_close(desc);
                fclose(f);
            }

            my_free(path);
	}

	return cd;
}

size_t
my_iconv(my_iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
	int c, i;
	size_t result = 0;
	**outbuf = 0;
	if (cd.desc == (iconv_t) -1) {
	if (cd.char_table != NULL)
	{
		while (*inbytesleft > 0 && *outbytesleft > 0)
		{
			c = **(unsigned char**)inbuf;
			if (cd.char_table[c] != NULL)
			{
				for (i = 0; i < cd.char_table[c][0] && *outbytesleft > 0; i++)
				{
				**outbuf = cd.char_table[c][i+1];
				(*outbytesleft)--;
				(*outbuf)++;
				}
			} else {
		/*                    fprintf(stderr, "my_iconv: no conversion for 0x%x\n", 
			      (unsigned)c);*/
				errno = EILSEQ;
				return (size_t)-1;
			}

			(*inbuf)++;
			(*inbytesleft)--;
			result++;
		}

	    if (*outbytesleft == 0 && *inbytesleft > 0) {
		errno = E2BIG;
		result = (size_t)-1;
	    }
	}
	}
	else
		result = iconv(cd.desc, inbuf, inbytesleft, outbuf, outbytesleft);

	return result;
}

my_iconv_t
my_iconv_close(my_iconv_t cd)
{
	int i;

	if (cd.char_table != NULL)
	{
		for (i = 0; i < char_table_size; i++)
		{
		    if (cd.char_table[i] != NULL)
			my_free(cd.char_table[i]);
		}

		my_free((void *)cd.char_table);
		cd.char_table = NULL;
	}

	if (cd.desc != (iconv_t) -1)
	{
		iconv_close(cd.desc);
		cd.desc = (iconv_t) -1;
	}

	return cd;
}

int 
my_iconv_is_valid (my_iconv_t cd)
{
	if (cd.desc != (iconv_t) -1 || cd.char_table != NULL)
		return 1;

	return 0;
}

void
my_iconv_t_make_invalid(my_iconv_t *cd)
{
	cd->desc = (iconv_t) -1;
	cd->char_table = NULL;
}

