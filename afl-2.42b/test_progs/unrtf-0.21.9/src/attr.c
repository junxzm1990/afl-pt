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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

   The maintainer is reachable by electronic mail at daved@physiol.usyd.edu.au
=============================================================================*/


/*----------------------------------------------------------------------
 * Module name:    attr
 * Author name:    Zachary Smith
 * Create date:    01 Aug 01
 * Purpose:        Character attribute stack.
 *----------------------------------------------------------------------
 * Changes:
 * 01 Aug 01, tuorfa@yahoo.com: moved code over from convert.c
 * 06 Aug 01, tuorfa@yahoo.com: added several font attributes.
 * 18 Sep 01, tuorfa@yahoo.com: added AttrStack (stack of stacks) paradigm
 * 22 Sep 01, tuorfa@yahoo.com: added comment blocks
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 16 Dec 07, daved@physiol.usyd.edu.au: fixed fore/background_begin error
 *                       and updated to GPL v3
 * 09 Nov 08, arkadiusz.firus@gmail.com: adopt safe_printf
 * 07 Oct 11, jf@dockes.org, fix to add_to_collection()
 * 09 Jun 13, peterli@salk.edu, added code to cope with bold/italic
 			requests that are not properly nested
 * 27 Jun 13, peterli@salk.edu, improved efficiency of the unnested code
 * 09 Dec 14, nicosr@free.frl, fix to return in attr_get_param for clang
 *--------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifndef HAVE_ATDARG_H
#include <stdarg.h>
#endif

#include "malloc.h"
#include "defs.h"
#include "error.h"
#include "attr.h"
#include "main.h"

extern void starting_body();
extern void starting_text();

extern int simulate_allcaps;
extern int simulate_smallcaps;


#define MAX_ATTRS (10000)


/* For each RTF text block (the text within braces) we must keep
 * an AttrStack which is a stack of attributes and their optional
 * parameter. Since RTF text blocks are nested, these make up a
 * stack of stacks. And, since RTF text blocks inherit attributes
 * from parent blocks, all new AttrStacks do the same from
 * their parent AttrStack.
 */
typedef struct _stack {
	unsigned char attr_stack[MAX_ATTRS];
	char *attr_stack_params[MAX_ATTRS]; 
	int tos;
	struct _stack *next;
} AttrStack;

/*@null@*/ static AttrStack *stack_of_stacks = NULL;
/*@null@*/ static AttrStack *stack_of_stacks_top = NULL;




/*========================================================================
 * Name:	attr_express_begin
 * Purpose:	Print the HTML for beginning an attribute.
 * Args:	Attribute number, optional string parameter.
 * Returns:	None.
 *=======================================================================*/

void 
attr_express_begin (int attr, char* param) {
	switch(attr) 
	{
	case ATTR_BOLD: 
		if (safe_printf(0, op->bold_begin)) fprintf(stderr, TOO_MANY_ARGS, "bold_begin");;
		break;
	case ATTR_ITALIC: 
		if (safe_printf(0, op->italic_begin)) fprintf(stderr, TOO_MANY_ARGS, "italic_begin");;
		break;

	/* Various underlines, they all resolve to HTML's <u> */
	case ATTR_THICK_UL:
	case ATTR_WAVE_UL:
	case ATTR_DASH_UL:
	case ATTR_DOT_UL: 
	case ATTR_DOT_DASH_UL:
	case ATTR_2DOT_DASH_UL:
	case ATTR_WORD_UL: 
	case ATTR_UNDERLINE: 
		if (safe_printf(0, op->underline_begin)) fprintf(stderr, TOO_MANY_ARGS, "underline_begin");;
		break;

	case ATTR_DOUBLE_UL: 
		if (safe_printf(0, op->dbl_underline_begin)) fprintf(stderr, TOO_MANY_ARGS, "dbl_underline_begin");;
		break;

	case ATTR_FONTSIZE: 
		op_begin_std_fontsize (op, atoi (param));
		break;

	case ATTR_FONTFACE: 
		if (safe_printf(1, op->font_begin,param)) fprintf(stderr, TOO_MANY_ARGS, "font_begin");;
		break;

	case ATTR_FOREGROUND: 
		if (safe_printf(1, op->foreground_begin, param)) fprintf(stderr, TOO_MANY_ARGS, "foreground_begin");;
		break;

	case ATTR_BACKGROUND: 
		if (!simple_mode)
		  if (safe_printf(1, op->background_begin,param)) fprintf(stderr, TOO_MANY_ARGS, "background_begin");;
		break;

	case ATTR_SUPER: 
		if (safe_printf(0, op->superscript_begin)) fprintf(stderr, TOO_MANY_ARGS, "superscript_begin");;
		break;
	case ATTR_SUB: 
		if (safe_printf(0, op->subscript_begin)) fprintf(stderr, TOO_MANY_ARGS, "subscript_begin");;
		break;

	case ATTR_STRIKE: 
		if (safe_printf(0, op->strikethru_begin)) fprintf(stderr, TOO_MANY_ARGS, "strikethru_begin");;
		break;

	case ATTR_DBL_STRIKE: 
		if (safe_printf(0, op->dbl_strikethru_begin)) fprintf(stderr, TOO_MANY_ARGS, "dbl_strikethru_begin");;
		break;

	case ATTR_EXPAND: 
		if (safe_printf(1, op->expand_begin, param)) fprintf(stderr, TOO_MANY_ARGS, "expand_begin");;
		break;

	case ATTR_OUTLINE: 
		if (safe_printf(0, op->outline_begin)) fprintf(stderr, TOO_MANY_ARGS, "outline_begin");;
		break;
	case ATTR_SHADOW: 
		if (safe_printf(0, op->shadow_begin)) fprintf(stderr, TOO_MANY_ARGS, "shadow_begin");;
		break;
	case ATTR_EMBOSS: 
		if (safe_printf(0, op->emboss_begin)) fprintf(stderr, TOO_MANY_ARGS, "emboss_begin");;
		break;
	case ATTR_ENGRAVE: 
		if (safe_printf(0, op->engrave_begin)) fprintf(stderr, TOO_MANY_ARGS, "engrave_begin");;
		break;

	case ATTR_CAPS:
		if (op->simulate_all_caps)
			simulate_allcaps = TRUE;
		break;

	case ATTR_SMALLCAPS: 
		if (op->simulate_small_caps)
			simulate_smallcaps = TRUE;
		else {
			if (op->small_caps_begin)
				if (safe_printf(0, op->small_caps_begin)) fprintf(stderr, TOO_MANY_ARGS, "small_caps_begin");;
		}
		break;
	}
}


/*========================================================================
 * Name:	attr_express_end
 * Purpose:	Print HTML to complete an attribute.
 * Args:	Attribute number.
 * Returns:	None.
 *=======================================================================*/

void 
attr_express_end (int attr, char *param)
{
	switch(attr) 
	{
	case ATTR_BOLD: 
		if (safe_printf(0, op->bold_end)) fprintf(stderr, TOO_MANY_ARGS, "bold_end");;
		break;
	case ATTR_ITALIC: 
		if (safe_printf(0, op->italic_end)) fprintf(stderr, TOO_MANY_ARGS, "italic_end");;
		break;

	/* Various underlines, they all resolve to HTML's </u> */
	case ATTR_THICK_UL:
	case ATTR_WAVE_UL:
	case ATTR_DASH_UL:
	case ATTR_DOT_UL: 
	case ATTR_DOT_DASH_UL:
	case ATTR_2DOT_DASH_UL: 
	case ATTR_WORD_UL: 
	case ATTR_UNDERLINE: 
		if (safe_printf(0, op->underline_end)) fprintf(stderr, TOO_MANY_ARGS, "underline_end");;
		break;

	case ATTR_DOUBLE_UL: 
		if (safe_printf(0, op->dbl_underline_end)) fprintf(stderr, TOO_MANY_ARGS, "dbl_underline_end");;
		break;

	case ATTR_FONTSIZE: 
		op_end_std_fontsize (op, atoi (param));
		break;

	case ATTR_FONTFACE: 
		if (safe_printf(0, op->font_end)) fprintf(stderr, TOO_MANY_ARGS, "font_end");;
		break;

	case ATTR_FOREGROUND: 
		if (safe_printf(0, op->foreground_end)) fprintf(stderr, TOO_MANY_ARGS, "foreground_end");;
		break;
	case ATTR_BACKGROUND: 
		if (!simple_mode)
		  if (safe_printf(0, op->background_end)) fprintf(stderr, TOO_MANY_ARGS, "background_end");;
		break;

	case ATTR_SUPER: 
		if (safe_printf(0, op->superscript_end)) fprintf(stderr, TOO_MANY_ARGS, "superscript_end");;
		break;
	case ATTR_SUB: 
		if (safe_printf(0, op->subscript_end)) fprintf(stderr, TOO_MANY_ARGS, "subscript_end");;
		break;

	case ATTR_STRIKE: 
		if (safe_printf(0, op->strikethru_end)) fprintf(stderr, TOO_MANY_ARGS, "strikethru_end");;
		break;

	case ATTR_DBL_STRIKE: 
		if (safe_printf(0, op->dbl_strikethru_end)) fprintf(stderr, TOO_MANY_ARGS, "dbl_strikethru_end");;
		break;

	case ATTR_OUTLINE: 
		if (safe_printf(0, op->outline_end)) fprintf(stderr, TOO_MANY_ARGS, "outline_end");;
		break;
	case ATTR_SHADOW: 
		if (safe_printf(0, op->shadow_end)) fprintf(stderr, TOO_MANY_ARGS, "shadow_end");;
		break;
	case ATTR_EMBOSS: 
		if (safe_printf(0, op->emboss_end)) fprintf(stderr, TOO_MANY_ARGS, "emboss_end");;
		break;
	case ATTR_ENGRAVE: 
		if (safe_printf(0, op->engrave_end)) fprintf(stderr, TOO_MANY_ARGS, "engrave_end");;
		break;

	case ATTR_EXPAND: 
		if (safe_printf(0, op->expand_end)) fprintf(stderr, TOO_MANY_ARGS, "expand_end");;
		break;

	case ATTR_CAPS:
		if (op->simulate_all_caps)
			simulate_allcaps = FALSE;
		break;

	case ATTR_SMALLCAPS: 
		if (op->simulate_small_caps)
			simulate_smallcaps = FALSE;
		else {
			if (op->small_caps_end)
				if (safe_printf(0, op->small_caps_end)) fprintf(stderr, TOO_MANY_ARGS, "small_caps_end");;
		}
		break;
	}
}



/*========================================================================
 * Name:	attr_push
 * Purpose:	Pushes an attribute onto the current attribute stack.
 * Args:	Attribute number, optional string parameter.
 * Returns:	None.
 *=======================================================================*/

void 
attr_push(int attr, char* param) 
{
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		warning_handler("No stack to push attribute onto");
		return;
	}

	if (stack->tos >= MAX_ATTRS -1) {
		fprintf(stderr, "Too many attributes!\n");
		return;
	}

	/* Make sure it's understood we're in the <body> section. */
	/* KLUDGE */
	starting_body();
	starting_text();

	++stack->tos;
	stack->attr_stack[stack->tos] = attr;
	if (param) 
		stack->attr_stack_params[stack->tos] = my_strdup(param);
	else
		stack->attr_stack_params[stack->tos] = NULL;

	attr_express_begin(attr, param);
}


/*========================================================================
 * Name:	attr_get_param
 * Purpose:	Reads an attribute from the current attribute stack.
 * Args:	Attribute number
 * Returns:	string.
 *=======================================================================*/

char * 
attr_get_param(int attr) 
{
	int i;
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		if (attr != ATTR_ENCODING) {
			/*
			 * attr_get_param(ATTR_ENCODING) is always called
			 * called once without a stack being available.
			 */
			warning_handler("No stack to get attribute from");
		}
		return NULL;
	}

	i=stack->tos;
	while (i>=0)
	{
		if(stack->attr_stack [i] == attr)
		{
			if(stack->attr_stack_params [i] != NULL)
				return stack->attr_stack_params [i];
			else	
				return NULL;
		}
		i--;
	}
	return NULL;
}


/*========================================================================
 * Name:	attrstack_copy_all
 * Purpose:	Routine to copy all attributes from one stack to another.
 * Args:	Two stacks.
 * Returns:	None.
 *=======================================================================*/

void 
attrstack_copy_all (AttrStack *src, AttrStack *dest) 
{
	int i;
	int total;

	CHECK_PARAM_NOT_NULL(src);
	CHECK_PARAM_NOT_NULL(dest);

	total = src->tos + 1;

	for (i=0; i<total; i++)
	{
		int attr=src->attr_stack [i];
		char *param=src->attr_stack_params [i];

		dest->attr_stack[i] = attr;
		if (param)
			dest->attr_stack_params[i] = my_strdup (param);
		else
			dest->attr_stack_params[i] = NULL;
	}

	dest->tos = src->tos;
}

/*========================================================================
 * Name:	attrstack_unexpress_all
 * Purpose:	Routine to un-express all attributes heretofore applied,
 * 		without removing any from the stack.
 * Args:	Stack whose contents should be unexpressed.
 * Returns:	None.
 * Notes:	This is needed by attrstack_push, but also for \cell, which
 * 		often occurs within a brace group, yet HTML uses <td></td> 
 *		which clear attribute info within that block.
 *=======================================================================*/

void 
attrstack_unexpress_all (AttrStack *stack)
{
	int i;

	CHECK_PARAM_NOT_NULL(stack);

	i=stack->tos;
	while (i>=0)
	{
		int attr=stack->attr_stack [i];
		char *param=stack->attr_stack_params [i];

		attr_express_end (attr, param);
		i--;
	}
}


/*========================================================================
 * Name:	attrstack_push
 * Purpose:	Creates a new attribute stack, pushes it onto the stack
 *		of stacks, performs inheritance from previous stack.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/
void 
attrstack_push () 
{
	AttrStack *new_stack;

	new_stack = (AttrStack*) my_malloc (sizeof (AttrStack));
	memset ((void*) new_stack, 0, sizeof (AttrStack));

	if (!stack_of_stacks) {
		stack_of_stacks = new_stack;
	} else {
		stack_of_stacks_top->next = new_stack;
	}
	stack_of_stacks_top = new_stack;
	new_stack->tos = -1;
}



/*========================================================================
 * Name:	attr_pop 
 * Purpose:	Removes and undoes the effect of the top attribute of
 *		the current AttrStack.
 * Args:	The top attribute's number, for verification.
 * Returns:	Success/fail flag.
 *=======================================================================*/

int 
attr_pop (int attr) 
{
	AttrStack *stack = stack_of_stacks_top;

	if (!stack) {
		warning_handler ("no stack to pop attribute from");
		return FALSE;
	}

	if(stack->tos>=0 && stack->attr_stack[stack->tos]==attr)
	{
		char *param = stack->attr_stack_params [stack->tos];

		attr_express_end (attr, param);

		if (param) my_free(param);

		stack->tos--;

		return TRUE;
	}
	else
		return FALSE;
}



/*========================================================================
 * Name:	attr_read
 * Purpose:	Reads but leaves in place the top attribute of the top
 * 		attribute stack.
 * Args:	None.
 * Returns:	Attribute number.
 *=======================================================================*/

int 
attr_read() {
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		warning_handler ("no stack to read attribute from");
		return FALSE;
	}

	if(stack->tos>=0)
	{
		int attr = stack->attr_stack [stack->tos];
		return attr;
	}
	else
		return ATTR_NONE;
}


/*========================================================================
 * Name:	attr_drop_all 
 * Purpose:	Undoes all attributes that an AttrStack contains.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
attr_drop_all () 
{
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		warning_handler ("no stack to drop all attributes from");
		return;
	}

	while (stack->tos>=0) 
	{
		char *param=stack->attr_stack_params [stack->tos];
		if (param) my_free(param);
		stack->tos--;
	}
}


/*========================================================================
 * Name:	attrstack_drop
 * Purpose:	Removes the top AttrStack from the stack of stacks, undoing
 *		all attributes that it had in it.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
attrstack_drop () 
{
	AttrStack *stack = stack_of_stacks_top;
	AttrStack *prev_stack;
	if (!stack) {
		warning_handler ("no attr-stack to drop");
		return;
	}

	attr_pop_all ();
	prev_stack = stack_of_stacks;

	while(prev_stack && prev_stack->next && prev_stack->next != stack)
		prev_stack = prev_stack->next;

        /* prev_stack == stack_of_stacks_top if there was only one
           element. We'll be empty. Note that prev_stack can't be
           actually NULL here, this would happen with an empty stack,
           which would have been detected by the !stack test above.
        */
	if (prev_stack && (prev_stack != stack_of_stacks_top)) {
		stack_of_stacks_top = prev_stack;
		prev_stack->next = NULL;
	} else {
		stack_of_stacks_top = NULL;
		stack_of_stacks = NULL;
	}

	my_free ((void*) stack);
}

/*========================================================================
 * Name:	attr_pop_all
 * Purpose:	Routine to undo all attributes heretofore applied, 
 *		also reversing the order in which they were applied.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
attr_pop_all() 
{
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		warning_handler ("no stack to pop from");
		return;
	}

	while (stack->tos>=0) {
		int attr=stack->attr_stack [stack->tos];
		char *param=stack->attr_stack_params [stack->tos];
		attr_express_end (attr,param);
		if (param) my_free(param);
		stack->tos--;
	}
}


/*========================================================================
 * Name:	attrstack_express_all
 * Purpose:	Routine to re-express all attributes heretofore applied.
 * Args:	None.
 * Returns:	None.
 * Notes:	This is needed by attrstack_push, but also for \cell, which
 * 		often occurs within a brace group, yet HTML uses <td></td> 
 *		which clear attribute info within that block.
 *=======================================================================*/

void 
attrstack_express_all() {
	AttrStack *stack = stack_of_stacks_top;
	int i;

	if (!stack) {
		warning_handler ("no stack to pop from");
		return;
	}

	i=0;
	while (i<=stack->tos) 
	{
		int attr=stack->attr_stack [i];
		char *param=stack->attr_stack_params [i];
		attr_express_begin (attr, param);
		i++;
	}
}


/*========================================================================
 * Name:	attr_pop_dump
 * Purpose:	Routine to un-express all attributes heretofore applied.
 * Args:	None.
 * Returns:	None.
 * Notes:	This is needed for \cell, which often occurs within a 
 *		brace group, yet HTML uses <td></td> which clear attribute 
 *		info within that block.
 *=======================================================================*/

void 
attr_pop_dump() {
	AttrStack *stack = stack_of_stacks_top;
	int i;

	if (!stack) return;

	i=stack->tos;
	while (i>=0) 
	{
		int attr=stack->attr_stack [i];
		attr_pop (attr);
		i--;
	}
}

/*========================================================================
 * Name:	safe_printf
 * Purpose:	Prevents format string attack and writes empty string
		instead of NULL.
 * Args:	Number of parameters (without a string), string to write,
		additional parameters to print (have to be strings).
 * Returns:	Returns 0 if number of not escaped '%' in string
		 is not greater than nr, else returns -1
 *=======================================================================*/


int
safe_printf(int nr, char *string, ...)
{

	char *s;
	int i = 0, ret_code = 0;
	va_list arguments;

	if (string == NULL)
		;
	else
	{
		va_start(arguments, string);

		for (; nr > 0; nr--)
		{
			while (string[i] != '\0' && (string[i] != '%' || (string[i] == '%' && (i != 0 && string[i-1] == '\\'))))
			{
				if (string[i] != '\\' || string[i+1] != '%')
					printf("%c", string[i]);
				i++;
			}

			if (string[i] != '\0')
			{
				s = va_arg(arguments, char *);
				printf("%s", s);
				i++;
			}
		}
		va_end(arguments);

		while (string[i] != '\0')
		{
			if (string[i] != '\\' || (string[i] == '\\' && string[i+1] != '%'))
			{
				if (string[i] != '%' || (string[i] == '%' && (i != 0 && string[i-1] == '\\')))
					printf("%c", string[i]);
				else
					ret_code = -1;
			}
			i++;
		}
	}

	return ret_code;
}

/*========================================================================
 * Name:	assemble_string
 * Purpose:	See Returns
 * Args:	String to return and int to put into first parameter.
 * Returns:	Returns first parameter where first not escaped
		character % is substituted with second parameter.
 *=======================================================================*/

char *
assemble_string(char *string, int nr)
{

	char *s, tmp[12];/* Number of characters that can be in int type (including '\0') - AF */
	int i = 0, j = 0;

	if (string == NULL)
		return NULL;
	else {
		s = my_malloc(strlen(string) + 1 + 12/* Number of characters that can be in int type (including '\0') - AF */);
		while(string[i] != '\0' && (string[i] != '%' || (string[i] == '%' && (i != 0 && string[i-1] == '\\')))) {
			if (string[i] != '\\' || string[i+1] != '%') {
				 s[j] = string[i];
				j++;
			}
			i++;
		}

		if (string[i] != '\0') {
			sprintf(tmp, "%d", nr);
			strcpy(&s[j], tmp);
			j = j + strlen(tmp);
		}

		while (string[i] != '\0') {
			if (string[i] != '\\' || (string[i] == '\\' && string[i+1] != '%')) {
				if (string[i] != '%' || (string[i] == '%' && (i != 0 && string[i-1] == '\\'))) {
					 s[j] = string[i];
					j++;
				}
				else {
					/* More than one char % occured */			
				}
			}
			i++;
		}
	}

	s[j] = '\0';

	return s;
}


/*========================================================================
 * Name:	add_to_collection
 * Purpose:	Adds (substitutes) element under index nr.
 * Args:	Collection, index, text to add.
 * Returns:	Collection.
 *=======================================================================*/

Collection *
add_to_collection(Collection *col, int nr, const char *text)
{
	Collection *c = col;

	if (col == NULL)
	{
		col = (Collection *)my_malloc(sizeof(Collection));
		col->nr = nr;
		col->text = text;
		col->next = NULL;
	}
	else
	{
            do	{
			if (c->nr == nr)
			{
/* Here is a memory leak but not heavy. Do we need to care about this?
				my_free(a->alias.text);
*/
				c->text = text;

				return col;
			}

                        if (c->next == NULL)
                            break;
			c = c->next;
            } while (1);

		c->next = (Collection *)my_malloc(sizeof(Collection));
		c->next->nr = nr;
		c->next->text = text;
		c->next->next = NULL;
	}

	return col;
}

/*========================================================================
 * Name:	get_from_collection
 * Purpose:	Search for element under index nr.
 * Args:	Collection, index.
 * Returns:	NULL or found elemet.
 *=======================================================================*/

const char *
get_from_collection(Collection *c, int nr)
{
	while (c != NULL)
	{
		if (c->nr == nr)
			return c->text;

		c = c->next;
	}

	return NULL;
}

/*========================================================================
 * Name:	free_collection
 * Purpose:	Release memory used by collection.
 * Args:	Needless collection.
 * Returns:	
 *=======================================================================*/

void
free_collection(Collection *c)
{
	Collection *c2;

	while (c != NULL)
	{
		c2 = c->next;
		my_free((void *)c);
		c = c2;
	}
}

#define SUPPORT_UNNESTED
#ifdef SUPPORT_UNNESTED

// Push onto AttrStack generally, without extra bells and whistles
void attr_push_gen(AttrStack *stack, int attr, char *param) {
  stack->tos++;
  stack->attr_stack[stack->tos] = attr;
  if (param)
    stack->attr_stack_params[stack->tos] = param;
  else
    stack->attr_stack_params[stack->tos] = NULL;
}

// Pop from AttrStack generally, without extra bells and whistles
void attr_pop_gen(AttrStack *stack, int *attr, char **param) {
  *attr = stack->attr_stack[stack->tos];
  *param = stack->attr_stack_params[stack->tos];
  stack->tos--;
}

// Iterate through stack looking for findattr 
int attr_find(AttrStack *stack, int findattr) {
  int i;
  for (i = 0; i <= stack->tos; ++i)
    if (stack->attr_stack[i] == findattr) return i;
  return -1;
}

// Pop attrs until we find the match, then push the non-matches back
static AttrStack temp_stack;
int attr_find_pop(int findattr) {
  temp_stack.tos = -1; // initialize temp_stack if necessary

  AttrStack *stack = stack_of_stacks_top;
  if (!stack) {
    warning_handler ("no stack to pop attributes from");
    return FALSE;
  }

  // Check whether findattr is on stack at all; if not, just give up
  if (attr_find(stack, findattr) < 0) return FALSE;

  // Get findattr from stack, expressing endings.
  // Store any non-matches to temp_stack.
  int attr = ATTR_NONE;
  char *param = NULL;
  while (stack->tos >= 0) {
    attr_pop_gen(stack, &attr, &param);
    attr_express_end(attr, param);
    if (attr == findattr) break;
    attr_push_gen(&temp_stack, attr, param);
  }

  // Put the non-matches back, reexpressing beginnings
  while (temp_stack.tos >= 0) {
    attr_pop_gen(&temp_stack, &attr, &param);
    attr_push(attr, param);
  }

  return TRUE;
}
#endif /* SUPPORT_UNNESTED */
