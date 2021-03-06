// PPCPortIO.h

// Public Interface for memory-to-memory interface
// to Termulator

#ifndef ppc_port_io_h
#define ppc_port_io_h

#include "PPCLibraryX.h"

// Constants
#define	PPC_PORTIO_IN_OUT_BUF_SIZE	8192

// Memory-to-memory serial port interface
extern	void	serial_interface_reset()                                ; // call to reset buffer contents
extern	void	serial_interface_post (char it)                         ; // post an input character
extern	int     serial_interface_avail()                                ; // nummber available for fetching available
extern	void	serial_interface_fetch(unsigned char *where, int count) ; // fetch input characters
extern	void	serial_interface_post_msg(const char *msg)              ; // post message for internal echo

extern	char *	to_termulator_buf;
extern	char *	to_termulator_buf_allocated;
extern	int     to_termulator_buf_size;
extern	int     to_termulator_write_ptr;
extern	int     to_termulator_read_ptr;

extern	char *	from_termulator_buf;
extern	char *	from_termulator_buf_allocated;
extern	int     from_termulator_buf_size;
extern	int     from_termulator_write_ptr;
extern	int     from_termulator_read_ptr;

extern	void	(*PPCPortIO_sync_io)();										// must provide a sync io routine for multi processor applications

extern	PPC_SessionPtr	serial_session;										// session in use
extern  int             serial_interface_is_posting;						// set when task level routines are posting a character for the interpreter

#endif
