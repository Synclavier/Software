/*	CustomPrintf.h																		*/

#ifndef __CUSTOM_PRINTF_H
#define __CUSTOM_PRINTF_H

// Header file for custom printf parser
// C. Jones 3/24/2006

#include <stdarg.h>

int parse_printf(void (*out_routine)(char it), const char * format_str, va_list arg);

#endif