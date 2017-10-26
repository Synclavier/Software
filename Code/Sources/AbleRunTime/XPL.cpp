/*	xpl_run_time.c																*/

/*	Contents: Executable code that supports XPL translations					*/

/*	Created:	11/09/96 - C. Jones												*/

// Local includes
#include "XPL.h"

#include <string.h>

/*------------------------------------------------------------------------------*/
/* Able-to-C string routines													*/
/*------------------------------------------------------------------------------*/

// Converts a C string to an able string. Cannot convert in place.
// C string limited to 254 bytes; so the resulting string fits in a
// 256 byte buffer including the length word.
void	to_able_string(const char *c_string, fixed *a_string)
{
	fixed	i = strlen(c_string);
	fixed	j;
	
	if (i > 254) i = 254;
	
	a_string[0] = i;
	
	for (j=0; j < i; j++)
		pbyte(a_string, j, c_string[j]);
	
    // Zero fill upper half of last word
	if (i&1)
		pbyte(a_string, i, 0);
}

void	to_c_string(const fixed *a_string, char *c_string)
{
	fixed	i = a_string[0];
	fixed	j;
	
	if (i > 254) i = 254;
	
	for (j=0; j < i; j++)
		c_string[j] = byte(a_string, j);
	
	c_string[i] = 0;
}
