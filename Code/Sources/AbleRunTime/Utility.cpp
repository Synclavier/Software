/*  utility.c                                                               */

/*  Created:                                                                */
/*      06/18/96        C. Jones                                            */

/*  Contents:                                                               */
/*      Utility routines                                                    */

#include    "Standard.h"
#include    "Utility.h"


/*------------------------------------------------------------------------- */
/* Global Variables                                                         */
/*------------------------------------------------------------------------- */

uint32  g_bitlist[32]  =            /* list of bits                         */
{0x00000001, 0x00000002, 0x00000004, 0x00000008,
 0x00000010, 0x00000020, 0x00000040, 0x00000080,
 0x00000100, 0x00000200, 0x00000400, 0x00000800,
 0x00001000, 0x00002000, 0x00004000, 0x00008000,
 0x00010000, 0x00020000, 0x00040000, 0x00080000,
 0x00100000, 0x00200000, 0x00400000, 0x00800000,
 0x01000000, 0x02000000, 0x04000000, 0x08000000,
 0x10000000, 0x20000000, 0x40000000, 0x80000000};


/**------------------------------------------------------------------------ */
/* Misc functions: zero_mem                                                 */
/*------------------------------------------------------------------------- */

void zero_mem(byte *the_data, uint32 the_length)
{
	uint16 *data_p;
	
	if (the_length & 1)
	{
		*the_data++ = 0;
		the_length--;
	}
	
	data_p = (uint16 *)the_data;
	the_length >>= 1;

	while (the_length >= 16)
	{
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;

		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;
		*data_p++ = 0;

		the_length -= 16;
	}
	
	while (the_length--)
		*data_p++ = 0;
}
