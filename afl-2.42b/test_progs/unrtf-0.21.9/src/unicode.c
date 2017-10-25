/*----------------------------------------------------------------------
 * Module name:    unicode
 * Author name:    Arkadiusz Firus
 * Create date:    09 Nov 08
 * Purpose:        unicode translations
 *----------------------------------------------------------------------
 * Changes:
 * 04 Jan 10, daved@physiol.usyd.edu.au: null terminate strings in
 *		unicode_to_string
 * 21 Aug 10, daved@physiol.usyd.edu.au: test feof() rather than EOF for
 *              AIX support
 * 07 Oct 11, jf@dockes.org: major changes including change of
 *	get_unicode_char to get_unicode_utf8
 *--------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "malloc.h"

/*========================================================================
 * Name		get_unicode
 * Purpose:	Translates string like U221E or 221E to int value
 * Args:	Unicode character.
 * Returns:	Unicode number.
 *=======================================================================*/
int
get_unicode(char *string)
{
    unsigned long uc;
    if (string[0] == 'U' || string[0] == 'u')
        string++;
    uc = strtoul(string, 0, 16);
    return uc;
}

/*========================================================================
 * Name		unicode_to_utf8
 * Purpose:	Translates unicode number to UTF-8 string
 * Args:	Unicode number.
 * Returns:	malloced UTF-8 string
 *=======================================================================*/
char *
unicode_to_utf8(unsigned int uc)
{
        unsigned char *string = NULL;
	if (uc < 0x7f)
	{
		string = (unsigned char *)my_malloc(2 * sizeof(char));
		string[0] = (unsigned char) uc;
		string[1] = '\0';
	}
	else if (uc < 0x7ff)
	{
		string =  (unsigned char *)my_malloc(3 * sizeof(char));
		string[0] = (unsigned char) 192 + (uc / 64);
		string[1] = (unsigned char) 128 + (uc % 64);
		string[2] = '\0';
	}
	else if (uc < 0xffff)
	{
		string =  (unsigned char *)my_malloc(4 * sizeof(char));
		string[0] = (unsigned char) 224 + (uc / (64 * 64));
		string[1] = (unsigned char) 128 + ((uc / 64) % 64);
		string[2] = (unsigned char) 128 + (uc % 64);
		string[3] = '\0';
	}
	else if (uc < 0x1FFFFF)
	{
		string =  (unsigned char *)my_malloc(5 * sizeof(char));
		string[0] = (unsigned char) 240 + (uc / (64 * 64 * 64));
		string[1] = (unsigned char) 128 + ((uc / (64 * 64)) % 64);
		string[2] = (unsigned char) 128 + ((uc / 64) % 64);
		string[3] = (unsigned char) 128 + (uc % 64);
		string[4] = '\0';
	}
	else if (uc < 0x3FFFFFF)
	{
		string =  (unsigned char *)my_malloc(6 * sizeof(char));
		string[0] = (unsigned char) 248 + (uc / (64 * 64 * 64 * 64));
		string[1] = (unsigned char) 128 + ((uc / (64 * 64 * 64)) % 64);
		string[2] = (unsigned char) 128 + ((uc / (64 * 64)) % 64);
		string[3] = (unsigned char) 128 + ((uc / 64) % 64);
		string[4] = (unsigned char) 128 + (uc % 64);
		string[5] = '\0';
	}
	else if (uc < 0x7FFFFFFF)
	{
		string =  (unsigned char *)my_malloc(7 * sizeof(char));
		string[0] = (unsigned char) 252 + (uc / (64 * 64 * 64 * 64 * 64));
		string[1] = (unsigned char) 128 + ((uc / (64 * 64 * 64 * 64)) % 64);
		string[2] = (unsigned char) 128 + ((uc / (64 * 64 * 64)) % 64);
		string[3] = (unsigned char) 128 + ((uc / (64 * 64)) % 64);
		string[4] = (unsigned char) 128 + ((uc / 64) % 64);
		string[5] = (unsigned char) 128 + (uc % 64);
		string[6] = '\0';
	}

	return (char *)string;
}

/*========================================================================
 * Name		get_unicode_int
 * Purpose:	Reads unicode character (in format <UN...N> and translates
		it to printable unicode character.
 * Caution:	This function should be executed after char '<'  was read.
		It reads until char '>' was found or EOL or EOF.
 * Args:	File to read from.
 * Returns:	Unicode character encoded in UTF-8
 *=======================================================================*/

int
get_unicode_int(FILE *file)
{
	int allocated = 5, len = 0;
	char c, *unicode_number = my_malloc(allocated * sizeof(char));

	c = fgetc(file);

	while (c != '>' && c != '\n' && !feof(file) && !ferror(file))
	{
		unicode_number[len] = c;
		c = fgetc(file);
		len++;

		if (len == allocated)
		{
			allocated *= 2;
			unicode_number = my_realloc(unicode_number, allocated / 2, allocated);
		}
	}

	if (c != '>')
		ungetc(c, file);

	unicode_number[len] = '\0';
	return get_unicode(unicode_number);
}

char *
get_unicode_utf8(FILE *file)
{
    int uc = get_unicode_int(file);
    return unicode_to_utf8(uc);
}
