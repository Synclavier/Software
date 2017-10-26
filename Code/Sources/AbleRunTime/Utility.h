/*  file:   Utility.h   */
/*  author: C. Jones    */
/*  date:   06/18/96    */


/*  Modification History:               */
/*  06/18/96 - C. Jones - Created       */

#ifndef utility_h
#define utility_h

#include    "Standard.h"


/* Basic global variables: */

extern  uint32       g_bitlist[32];                 /* list of bits             */


/* Memory: */

void    zero_mem    (byte *the_data, uint32 the_length);


#endif
