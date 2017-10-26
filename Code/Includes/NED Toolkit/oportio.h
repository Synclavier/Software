/******************************************************************************
* FILE: portio.h
*
* Routines for handling serial port I/O.
*
* The function initport() opens and sets up one of the serial ports for the
*   baud rate system constant passed in, 1 start bit, 8 data bits, 1 stop bit,
*   no parity, and XON/XOFF input flow control.
* Please note: you should not use charspend[], charsavail, cpnext, or 
*   cpend directly.  They may go away in future versions of this file.
*   Use instead the functions get_serial_char() and chars_in_buffer() to check
*   for and receive incoming characters.  These perform all the necesary
*   housekeeping for you.  charsavail has been redefined to call 
*   chars_in_buffer().  You should use that function directly rather than charsavail.
*
* Original version written by Devon L. Petty
* New England Digital Corp. 1/28/88
*******************************************************************************/
 
#ifndef	NED__PORTIO
#define	NED__PORTIO

#define	IN_BUF_EMPTY	(-1)						// indicates no chars in input buffers
#define	charsavail	chars_in_buffer()				// chars to process?

#define	sPortPPCSession		99						// identifier for serial port linked to local PPC session. Any nonzero value less than this is a real hardware serial port.
#define	sPortInterpreter	100						// identifier for serial port hard wired to interpreter
#define	sPortLocalEcho		101						// identifier for serial port not available - local echo

extern unsigned char	*cpnext;					// next char to read
extern unsigned char	*cpend;						// last char to read + 1

extern int16			serial_port;				// indicates modem or printer (e.g., sPortA)
extern bool				serial_port_got_trashed;	// set true when serial driver gets re-opened
extern bool				global_use_async_io;		// set true to use faster aync serial IO
extern int				in_baud_rate;				// what baud rate we're running at
extern int				out_baud_rate;

bool initport(unsigned int whichPort, unsigned int inbaud, unsigned int outbaud);	// set up serial port
void closeport(void);							// close down serial port
void sendBRK(void);								// send a break
void sendXON(void);								// send an XOn char
void sendXOFF(void);							// send an XOff char
void sendchar(char c);							// send one char
void sendstr(char *s);							// send sequence of chars
void serialpoll(void);							// try to read host chars
bool chars_in_buffer(void);						// tests for presence of chars in input buffer
int	 get_serial_char(void);						// removes/returns one char from input buffer
void reset_port(uint16 port_configuration);			/* Set bits to indicate configuration	*/
void setbaudrate( int in_baud_rate, int out_baud_rate );
void set_port_handshake_mode(bool input_handshake, bool output_handshake);

// Class for managing OS X serial ports
#ifdef __cplusplus
	class CSerialPort
	{
		public:
		
		CSerialPort() {fBSDName = NULL; fUserName = NULL;};
		CSerialPort(CFStringRef bsdName, CFStringRef userName) {fBSDName = bsdName; fUserName = userName; fMenuIndex = 0;};
		
		CFStringRef		fBSDName;					// It's BSD name
		CFStringRef		fUserName;					// It's name published to user
		int				fMenuIndex;					// Where in menu it is (or its tag)
		
		static	void			PollFor ();
		static	int				GetNumOf();
		static	CSerialPort&	GetNth  (int which);
		static	CSerialPort*	GetWhich(int which);
	};
#endif
#endif
