/*	customprintf.c - Controlled environment printf-style output parser	*/

/*	Created:	03/09/97 - C. Jones										                  	*/

// Custom printf to handle printf style output
// Includes %a for able string printout
// Currently lacks long long output

// Handle kernel or user space applications
#ifdef COMPILE_OSX_KERNEL
	#include <IOKit/IOLib.h>
#else
	#include <ctype.h>
	#include <stdarg.h>
	#include <string.h>
#endif

#include "Standard.h"
#include "XPL.h"
#include "CustomPrintf.h"

#define conversion_buff_size	512
#define conversion_max			100
#define bad_conversion			0xFF

enum justification_options {
	left_justification,
	right_justification,
	zero_fill
};

enum sign_options {
	only_minus,
	sign_always,
	space_holder
};

enum argument_options {
	int_argument,
	short_argument,
	long_argument,
	pointer_argument
};

typedef struct {
	unsigned char	justification_options	: 2;
	unsigned char	sign_options			: 2;
	unsigned char	precision_specified		: 1;
	unsigned char	alternate_form			: 1;
	unsigned char	argument_options		: 2;
	unsigned char	conversion_char;
	char			field_width;
	char			precision;
} print_format;

static int isadigit(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	else
		return 0;
}

static char * parse_format(const char * format_string, va_list arg, print_format * format)
{
	print_format	f;
	const char *	s = format_string;
	int				c;
	int				flag_found;
		
	f.justification_options	= right_justification;
	f.sign_options					= only_minus;
	f.precision_specified			= 0;
	f.alternate_form				= 0;
	f.argument_options				= int_argument;
	f.field_width					= 0;
	f.precision						= 0;
	
	if ((c = *++s) == '%')
	{
		f.conversion_char = c;
		*format = f;
		return((char *) s + 1);
	}
	
	for (;;)
	{
		flag_found = 1;
		
		switch (c)
		{
			case '-':
				
				f.justification_options = left_justification;
				
				break;
			
			case '+':
				
				f.sign_options = sign_always;
				
				break;
			
			case ' ':
				
				if (f.sign_options != sign_always)
					f.sign_options = space_holder;
					
				break;
			
			case '#':
				
				f.alternate_form = 1;
				
				break;
			
			case '0':
				
				if (f.justification_options != left_justification)
					f.justification_options = zero_fill;
				
				break;
			
			default:
				
				flag_found = 0;
		}
		
		if (flag_found)
			c = *++s;
		else
			break;
	}
	
	if (c == '*')
	{
		if ((f.field_width = va_arg(arg, int)) < 0)
		{
			f.justification_options = left_justification;
			f.field_width           = -f.field_width;
		}
		
		c = *++s;
	}
	else
		while (isadigit(c))
		{
			f.field_width = (f.field_width * 10) + (c - '0');
			c = *++s;
		}
	
	if (f.field_width > conversion_max)
	{
		f.conversion_char = bad_conversion;
		*format = f;
		return((char *) s + 1);
	}
	
	if (c == '.')
	{
		f.precision_specified = 1;
		
		if ((c = *++s) == '*')
		{
			if ((f.precision = va_arg(arg, int)) < 0)
				f.precision_specified = 0;
			
			c = *++s;
		}
		else
			while (isadigit(c))
			{
				f.precision = (f.precision * 10) + (c - '0');
				c = *++s;
			}
	}
	
	flag_found = 1;
	
	switch (c)
	{
		case 'h':
			
			f.argument_options = short_argument;
			
			break;
		
		case 'l':
			
			f.argument_options = long_argument;
			
			break;
		
		default:
			
			flag_found = 0;
	}
	
	if (flag_found)
		c = *++s;
	
	f.conversion_char = c;
	
	switch (c)
	{
		case 'd':
		case 'i':
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			
			if (!f.precision_specified)
				f.precision = 1;
			else if (f.justification_options == zero_fill)
				f.justification_options = right_justification;
			
			break;
		
		case 'g':
		case 'G':
			
			if (!f.precision)
				f.precision = 1;
		
		case 'f':
		case 'e':
		case 'E':
			
			if (f.argument_options == short_argument || f.argument_options == long_argument)
			{
				f.conversion_char = bad_conversion;
				break;
			}
			
			if (!f.precision_specified)
				f.precision = 6;
			
			break;
		
		case 'p':
			
			f.argument_options = pointer_argument;
			f.alternate_form   = 1;
			f.conversion_char  = 'x';
			f.precision        = 16;
			
			break;
			
		case 'c':
			
			if (f.precision_specified || f.argument_options != int_argument)
				f.conversion_char = bad_conversion;
			
			break;
			
		case 's':
		case 'a':
			
			if (f.argument_options != int_argument)
				f.conversion_char = bad_conversion;
			
			break;
			
		case 'n':
			
			break;
			
		default:
			
			f.conversion_char = bad_conversion;
			
			break;
	}
	
	*format = f;
	
	return((char *) s + 1);
}

static char * ll2str(long long num, char * buff, print_format format)
{
	unsigned long long  unsigned_num = num, base = 10;
	char *				p;
	int					n, digits;
	int					minus = 0;
	
	p = buff;
	
	*--p = 0;
	
	digits = 0;
	
	if (!num && !format.precision && !(format.alternate_form && format.conversion_char == 'o'))
		return(p);
	
	switch (format.conversion_char)
	{
		case 'd':
		case 'i':
			
			base =  10;
	
			if (num < 0)
			{
				unsigned_num = -unsigned_num;
				minus        = 1;
			}
			
			break;
			
		case 'o':
			
			base =   8;
			
			format.sign_options = only_minus;
						
			break;
			
		case 'u':
			
			base =  10;
			
			format.sign_options = only_minus;
			
			break;
			
		case 'x':
		case 'X':
			
			base =  16;
			
			format.sign_options = only_minus;
			
			break;
	}
	
	do
	{
		n = (int) (unsigned_num % base);
		
		unsigned_num /= base;
		
		if (n < 10)
			n += '0';
		else
		{
			n -= 10;
			
			if (format.conversion_char == 'x')
				n += 'a';
			else
				n += 'A';
		}
		
		*--p = n;
		
		++digits;
	}
	while (unsigned_num != 0);
	
	if (base == 8 && format.alternate_form && *p != '0')
	{
		*--p = '0';
		++digits;
	}
	
	if (format.justification_options == zero_fill)
	{
		format.precision = format.field_width;
		
		if (minus || format.sign_options != only_minus)
			--format.precision;
		
		if (base == 16 && format.alternate_form)
			format.precision -= 2;
	}
	
	if (buff - p + format.precision > conversion_max)
		return(NULL);
	
	while (digits < format.precision)
	{
		*--p = '0';
		++digits;
	}
	
	if (base == 16 && format.alternate_form)
	{
		*--p = format.conversion_char;
		*--p = '0';
	}
	
	if (minus)
		*--p = '-';
	else if (format.sign_options == sign_always)
		*--p = '+';
	else if (format.sign_options == space_holder)
		*--p = ' ';
	
	return(p);
}

static	int out_string(void (*out_routine)(char it), const char *the_string, int num_chars)
{
   int written = num_chars;
   
   while (num_chars--)
   	 out_routine(*the_string++);
   	 
   return (written);
}

static	int out_able(void (*out_routine)(char it), const fixed *the_string, int num_chars)
{
   int written = num_chars;
   
   while (1)
   {
   	 if (num_chars == 0) return (written);
   	
   	 out_routine((char) (*the_string & 0xFF));		  /* able: first byte is in lower half */
   	 num_chars--;
   	
   	 if (num_chars == 0) return (written);
   	
   	 out_routine((char) (*the_string++ >> 8));		  /* able: second byte is in upper half */
   	 num_chars--;
   } 
}

static unsigned int	 itsLen(const char* it) {
    return (unsigned int) strlen(it);
}

int parse_printf(void (*out_routine)(char it), const char * format_str, va_list arg)
{
	unsigned int	num_chars, chars_written, field_width;
	const char *	format_ptr;
	const char *	curr_format;
	print_format	format;
	long long		ll_num;
	char			buff[conversion_buff_size];
	char *			buff_ptr   = NULL;
	char			fill_char  = ' ';
	fixed *			able_ptr   = NULL;
	
	if (!out_routine)
		return (0);
	
	format_ptr    = format_str;
	chars_written = 0;
	
	while (*format_ptr)
	{
		// Look for '%'
		if ((curr_format = strchr(format_ptr, '%')) == NULL)
		{
			num_chars      = itsLen(format_ptr);
			chars_written += num_chars;
			
			if (num_chars && !out_string(out_routine, format_ptr, num_chars))
				return(-1);
				
			break;
		}
		
		num_chars      = (unsigned int) (curr_format - format_ptr);
		chars_written += num_chars;
		
		if (num_chars && !out_string(out_routine, format_ptr, num_chars))
			return(-1);
		
		format_ptr = curr_format;
		
		format_ptr = parse_format(format_ptr, arg, &format);
		
		switch (format.conversion_char)
		{
			case 'd':
			case 'i':
				
				if (format.argument_options == pointer_argument)
					ll_num = (long long) (va_arg(arg, void*));
				else if (format.argument_options == long_argument)
					ll_num = (long long) (va_arg(arg, long));
				else
					ll_num = (long long) (va_arg(arg, int));
				
				if (format.argument_options == short_argument)
					ll_num = (long long) (short) ll_num;
					
				if ((buff_ptr = ll2str(ll_num, buff + conversion_buff_size, format)) == NULL)
					goto conversion_error;
				
				num_chars = (unsigned int) (buff + conversion_buff_size - 1 - buff_ptr);
				
				break;
				
			case 'o':
			case 'u':
			case 'x':
			case 'X':
				
				if (format.argument_options == pointer_argument)
					ll_num = (long long) (va_arg(arg, void*));
				else if (format.argument_options == long_argument)
					ll_num = (long long) (va_arg(arg, unsigned long));
				else
					ll_num = (long long) (va_arg(arg, unsigned int));
				
				if (format.argument_options == short_argument)
					ll_num = (long long) (unsigned short) ll_num;
					
				if ((buff_ptr = ll2str(ll_num, buff + conversion_buff_size, format)) == NULL)
					goto conversion_error;
				
				num_chars = (unsigned int) (buff + conversion_buff_size - 1 - buff_ptr);
				
				break;

            case 'a':																/* able string */
            case 'A':
            case 'p':																/* able string */
            case 'P':
				
				able_ptr = va_arg(arg, fixed*);				/* get pointer 	*/
				
				num_chars = (int) *able_ptr++;				/* get length		*/
				
				if (format.precision_specified)
				{
				  if (format.precision < (int) num_chars)
				  	num_chars = format.precision;
				}
				
				break;
			
			case 's':
				
				buff_ptr = va_arg(arg, char*);
				
				if (format.alternate_form)
				{
					num_chars = (unsigned char) *buff_ptr++;
					
					if (format.precision_specified && (int) num_chars > format.precision)
						num_chars = format.precision;
				}
				else if (format.precision_specified)
				{
					num_chars = format.precision;
					
					if (itsLen(buff_ptr) < num_chars)
						num_chars = itsLen(buff_ptr);
				}
				else
					num_chars = itsLen(buff_ptr);
				
				break;
			
			case 'n':
				
				buff_ptr = va_arg(arg, char*);
				
				switch (format.argument_options)
				{
					case int_argument:     * (int *)   buff_ptr = chars_written; break;
					case short_argument:   * (short *) buff_ptr = chars_written; break;
					case long_argument:    * (long *)  buff_ptr = chars_written; break;
					case pointer_argument: * (void **) buff_ptr = (void *) (unsigned long long) chars_written; break;
				}
				
				continue;
				
			case 'c':
				
				buff_ptr = buff;
				
				*buff_ptr = va_arg(arg, int);
				
				num_chars = 1;
				
				break;
			
			case '%':
				
				buff_ptr = buff;
				
				*buff_ptr = '%';
				
				num_chars = 1;
				
				break;
				
			case bad_conversion:
			conversion_error:
			default:
				
				num_chars      = itsLen(curr_format);
				chars_written += num_chars;
				
				if (num_chars && !out_string(out_routine, curr_format, num_chars))
					return(-1);
				
				return(chars_written);
		}
		
		field_width = num_chars;
		
		if (format.justification_options != left_justification)
		{
			fill_char = (format.justification_options == zero_fill) ? '0' : ' ';
			while ((int) field_width < format.field_width)
			{
				if (out_string(out_routine, &fill_char, 1) == (-1))
					return(-1);
					
				++field_width;
			}
		}                                                                       /*mm-960722*/
		
		if (buff_ptr)
		{
			if (num_chars && !out_string(out_routine, buff_ptr, num_chars))
				return(-1);
			buff_ptr = NULL;
		}
		
		if (able_ptr)
		{
			if (num_chars && !out_able(out_routine, able_ptr, num_chars))
				return(-1);
			able_ptr = NULL;
		}
		
		if (format.justification_options == left_justification)
			while ((int) field_width < format.field_width)
			{
				if (out_string(out_routine, " ", 1) == (-1))
					return(-1);
					
				++field_width;
			}
		
		chars_written += field_width;
	}
	
	return(chars_written);
}

